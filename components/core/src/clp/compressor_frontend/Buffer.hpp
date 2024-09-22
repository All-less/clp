#ifndef COMPRESSOR_FRONTEND_BUFFER_HPP
#define COMPRESSOR_FRONTEND_BUFFER_HPP

// spdlog
#include <spdlog/spdlog.h>

// C++ libraries
#include <cstdint>
#include <vector>
#include <immintrin.h>
#include <string_view>

// Project Headers
#include "Constants.hpp"

namespace compressor_frontend {
    /**
     * A base class for the efficient implementation of a single growing buffer.
     * Under the hood it keeps track of one static buffer and multiple dynamic
     * buffers. The buffer object uses the underlying static buffer whenever
     * possible, as the static buffer is on the stack and results in faster
     * reads and writes. In outlier cases, where the static buffer is not large
     * enough to fit all the needed data, the buffer object switches to using
     * the underlying dynamic buffers. A new dynamic buffer is used each time
     * the size must be grown to preserve any pointers to the buffer. All
     * pointers to the buffer are valid until reset() is called and the
     * buffer returns to using the underlying static buffer. The base class does
     * not grow the buffer itself, the child class is responsible for doing
     * this.
     */
    template <typename Item>
    class Buffer {
    public:
        Buffer () : m_pos(0), m_active_storage(m_static_storage),
                    m_active_size(cStaticByteBuffSize), m_spaces(_mm256_set1_epi8(' ')),
                    m_newlines(_mm256_set1_epi8('\n')) { }

        ~Buffer () {
            reset();
        }

        void increment_pos () {
            m_pos++;
        }

        void set_value (uint32_t pos, Item& value) {
            m_active_storage[pos] = value;
        }

        [[nodiscard]] const Item& get_value (uint32_t pos) const {
            return m_active_storage[pos];
        }

        void set_curr_value (Item& value) {
            m_active_storage[m_pos] = value;
        }

        void set_pos (uint32_t curr_pos) {
            m_pos = curr_pos;
        }

        [[nodiscard]] uint32_t pos () const {
            return m_pos;
        }

        [[nodiscard]] const Item& get_curr_value () const {
            return m_active_storage[m_pos];
        }

        const Item &peek_value(uint32_t offset) const {
            auto pos = m_pos + offset;
            if (pos >= m_active_size) {
                pos -= m_active_size;
            }
            return m_active_storage[pos];
        }

        uint32_t find(__m256i target) const {
            __m256i chunk;
            int mask;
            uint32_t offset = 0;

            for (uint32_t i = m_pos; ; ) {
                if (i + 32 < m_active_size) {
                    chunk = _mm256_loadu_si256((const __m256i*)(m_active_storage + i));
                } else {
                    char bytes[32] = "0000000000000000000000000000000";
                    memcpy(bytes, m_active_storage + i, m_active_size - i);
                    chunk = _mm256_loadu_si256((const __m256i*)(bytes));

                }

                mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, target));
                if (mask != 0) {
                    return offset + __builtin_ctz(mask);
                }

                if (i + 32 < m_active_size) {
                    i += 32;
                    offset += 32;
                } else {
                    offset += m_active_size - i;
                    i = 0;
                }
            }
        }

        uint32_t find_space() const {
            return find(m_spaces);
        }

        uint32_t find_newline() const {
            return find(m_newlines);
        }

        void rewind(uint32_t pos) {
            m_pos = pos;
        }

        uint32_t get_pos() {
            return m_pos;
        }

        void skip_offset(uint32_t offset) {
            m_pos += offset;
            if (m_pos >= m_active_size) {
                m_pos -= m_active_size;
            }
        }

        void set_active_buffer (Item* storage, uint32_t size, uint32_t pos) {
            m_active_storage = storage;
            m_active_size = size;
            m_pos = pos;
        }

        [[nodiscard]] Item* get_active_buffer () const {
            return m_active_storage;
        }

        [[nodiscard]] Item* get_mutable_active_buffer () {
            return m_active_storage;
        }

        [[nodiscard]] uint32_t size () const {
            return m_active_size;
        }

        [[nodiscard]] uint32_t static_size () const {
            return cStaticByteBuffSize;
        }

        void reset () {
            m_pos = 0;
            for (auto dynamic_storage : m_dynamic_storages) {
                free(dynamic_storage);
            }
            m_dynamic_storages.clear();
            m_active_storage = m_static_storage;
            m_active_size = cStaticByteBuffSize;
        }

        const Item* double_size() {
            Item* new_dynamic_buffer = m_dynamic_storages.emplace_back(
                                            (Item*)malloc(2 * m_active_size * sizeof(Item)));
            if (new_dynamic_buffer == nullptr) {
                SPDLOG_ERROR("Failed to allocate output buffer of size {}.",
                             2 * m_active_size);
                /// TODO: update exception when an exception class is added
                /// (e.g., "failed_to_compress_log_continue_to_next")
                throw std::runtime_error(
                        "Lexer failed to find a match after checking entire buffer");
            }
            m_active_storage = new_dynamic_buffer;
            m_active_size *= 2;
            return new_dynamic_buffer;
        }

        void copy (const Item* storage_to_copy_first, const Item* storage_to_copy_last,
                   uint32_t offset) {
            std::copy(storage_to_copy_first, storage_to_copy_last, m_active_storage + offset);
        }

        void read(std::string const& message) {
            m_active_storage = (char*) message.data();
            // std::copy(message.data(), message.data() + message.size(), m_active_storage);
        }

        void read(std::string_view& message) {
            m_active_storage = (char*) message.data();
	    // std::copy(message.data(), message.data() + message.size(), m_active_storage);
        }


    protected:
        uint32_t m_pos;
        uint32_t m_active_size;
        Item* m_active_storage;
        std::vector<Item*> m_dynamic_storages;
        Item m_static_storage[4];

        __m256i m_spaces;
        __m256i m_newlines;
    };
}

#endif // COMPRESSOR_FRONTEND_BUFFER_HPP
