// C++ libraries
#include <memory.h>
#include <string>
#include <string_view>
#include <iostream>

// spdlog
#include <spdlog/spdlog.h>

// Project Headers
#include "LogInputBuffer.hpp"

using std::string;
using std::to_string;

namespace compressor_frontend {
    void LogInputBuffer::reset () {
        m_log_fully_consumed = false;
        finished_reading_input = false;
        m_consumed_pos = 0;
        m_commited_pos = 0;
        m_pos_last_read_char = 0;
        m_last_read_first_half = false;
        m_storage.reset();
        dummy_type_ids = new std::vector<int>{ 0 };
        m_total_scanned = 0;
        m_skip_callback = nullptr;
    }

    bool LogInputBuffer::read_is_safe () {
        if (finished_reading_input) {
            return false;
        }
        // If the next message starts at 0, the previous ended at size - 1
        if (m_consumed_pos == -1) {
            m_consumed_pos = m_storage.size() - 1;
        }
        // Check if the last log message ends in the buffer half last read.
        // This means the other half of the buffer has already been fully used.
        if ((!m_last_read_first_half && m_commited_pos > m_storage.size() / 2) ||
            (m_last_read_first_half && m_commited_pos < m_storage.size() / 2 &&
             m_commited_pos > 0))
        {
            return true;
        }
        return false;
    }

    char LogInputBuffer::get_next_character () {
        if (finished_reading_input && m_storage.pos() == m_pos_last_read_char) {
            m_log_fully_consumed = true;
            return utf8::cCharEOF;
        }
        unsigned char character = m_storage.get_curr_value();
        m_storage.increment_pos();
        if (m_storage.pos() == m_storage.size()) {
            m_storage.set_pos(0);
        }
        return character;
    }

    void LogInputBuffer::read(std::string const& message) {
        // We assume the length of message is shorter than 60000 (default size of m_storage),
        // so we don't change `m_last_read_first_half`.
        m_storage.read(message);
        m_pos_last_read_char += message.size();
    }

    void LogInputBuffer::read(std::string_view& message) {
        m_storage.read(message);
        m_pos_last_read_char += message.size();
    }

    RDToken LogInputBuffer::read_n(uint32_t n) {
        RDToken res;
        /* if (m_storage.pos() + n < m_storage.size()) {
            res = RDToken{m_storage.pos(), m_storage.pos() + n,
                          m_storage.get_active_buffer(), m_storage.size(),
                          0, dummy_type_ids};
        } else {
            res = RDToken{m_storage.pos(), m_storage.pos() + n - m_storage.size(),
                          m_storage.get_active_buffer(), m_storage.size(),
                          0, dummy_type_ids};
	    } */
        skip_offset_without_callback(n);
        return res;
    }

    RDToken LogInputBuffer::read_token() {
        const unsigned short SCANNING       = 0 << 8,
                             IN_PARENTHESES = 1 << 8,
                             IN_BRACKETS    = 2 << 8,
                             IN_BRACES      = 3 << 8;
        unsigned short state = SCANNING,
                       level = 0;

        char c;
        uint32_t i = 0;
        for ( ; ; i++) {
            c = peek_offset(i);
            switch (state | c) {
                case SCANNING | ' ':
                case SCANNING | '\n':
                    goto END_SCANNING;
                case SCANNING | '(':
                    state = IN_PARENTHESES;
                    level += 1;
                    break;
                case SCANNING | '[':
                    state = IN_BRACKETS;
                    level += 1;
                    break;
                case SCANNING | '{':
                    state = IN_BRACES;
                    level += 1;
                    break;
                case IN_PARENTHESES | '(':
                case IN_BRACKETS | '[':
                case IN_BRACES | '{':
                    level += 1;
                    break;
                case IN_PARENTHESES | ')':
                case IN_BRACKETS | ']':
                case IN_BRACES | '}':
                    level -= 1;
                    if (level == 0) {
                        state = SCANNING;
                    }
                    break;
            }
        }
END_SCANNING:
        return read_n(i);
    }

    RDToken LogInputBuffer::read_until_space() {
        auto n = m_storage.find_space();
        m_total_scanned += n;
        return read_n(n);
    }

    RDToken LogInputBuffer::read_until_newline() {
        return read_n(m_storage.find_newline());
    }

    void LogInputBuffer::skip_offset(uint32_t n) {
        if (m_skip_callback != nullptr) {
            m_skip_callback({m_storage.get_active_buffer() + m_storage.pos(),
			     m_storage.get_active_buffer() + m_storage.pos() + n});
        }

        m_storage.skip_offset(n);
        m_consumed_pos += n;
        if (m_consumed_pos > m_storage.size()) {
            m_consumed_pos -= m_storage.size();
        }
    }

    void LogInputBuffer::skip_offset_without_callback(uint32_t n) {
        m_storage.skip_offset(n);
        m_consumed_pos += n;
        if (m_consumed_pos > m_storage.size()) {
            m_consumed_pos -= m_storage.size();
        }
    }

    char LogInputBuffer::peek_offset(uint32_t n) {
        return m_storage.peek_value(n);
    }

    void LogInputBuffer::peek_n(uint32_t n) {
        for (int i = 0; i < n; ++i) {
            std::cout << m_storage.peek_value(i);
        }
        std::cout << std::endl;
    }

    void LogInputBuffer::rewind() {
        m_storage.rewind(m_commited_pos);
    }

    void LogInputBuffer::commit() {
        m_commited_pos = m_storage.get_pos();
    }
}
