#ifndef CLP_FFI_IR_STREAM_IRBUFFER_HPP
#define CLP_FFI_IR_STREAM_IRBUFFER_HPP

#include <cstdint>
#include <span>
#include <immintrin.h>
#include <vector>

namespace clp::ffi::ir_stream {

class IRBuffer {
public:
    IRBuffer(size_t capacity) : m_size(0), m_pos(0) {
        m_buf = new int8_t[capacity];
    }

    const int8_t* data() {
        return m_buf;
    }

    const int8_t* begin() {
        return m_buf;
    }

    const int8_t* end() {
        return m_buf + m_size;
    }

    size_t size() {
        return m_size;
    }

    void push_back(int8_t e) {
        m_buf[m_pos] = e;
        m_pos += 1;
        m_size += 1;
    }

    void insert(const int8_t* begin, const int8_t* end) {
	/*
        auto len = end - begin;
        for (auto i = 0; i < len; i++) {
            m_buf[m_pos+i] = *(begin+i);
        }
        m_pos += len;
        m_size += len;
	*/

	auto len = end - begin;
        const size_t WIDTH = 32;

        size_t chunks = len / WIDTH;
        for (size_t i = 0; i < chunks; ++i) {
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(begin + i * WIDTH));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(m_buf + m_pos + i * WIDTH), data);
        }

        size_t bytes = len % WIDTH;
        const int8_t* src = begin + chunks * WIDTH;
        int8_t* dst = m_buf + m_pos + chunks * WIDTH;
        for (size_t i = 0; i < bytes; ++i) {
            dst[i] = src[i];
        }

        m_pos += len;
        m_size += len;
    }

    void insert(const char* begin, const char* end) {
	    insert(reinterpret_cast<const int8_t*>(begin), reinterpret_cast<const int8_t*>(end));
    }

    void insert(std::vector<signed char>::iterator begin, std::vector<signed char>::iterator end) {
	    insert(&(*begin), &(*end));
    }

    void clear() {
        m_size = 0;
        m_pos = 0;
    }

private:
    int8_t* m_buf;
    size_t m_size;
    size_t m_pos;
};
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_IRBUFFER_HPP
