#include "RDToken.hpp"

using std::string;

namespace compressor_frontend {

    string RDToken::get_string () const {
        if (m_start_pos <= m_end_pos) {
            return {m_buffer + m_start_pos, m_buffer + m_end_pos};
        } else {
            return string(m_buffer + m_start_pos, m_buffer + m_buffer_size) +
                   string(m_buffer, m_buffer + m_end_pos);
        }
    }

    char RDToken::get_char (uint8_t i) const {
        return m_buffer[m_start_pos + i];
    }

    string RDToken::get_delimiter () const {
        return {m_buffer + m_start_pos, m_buffer + m_start_pos + 1};
    }

    uint32_t RDToken::get_length () const {
        if (m_start_pos <= m_end_pos) {
            return m_end_pos - m_start_pos;
        } else {
            return m_buffer_size - m_start_pos + m_end_pos;
        }
    }
}
