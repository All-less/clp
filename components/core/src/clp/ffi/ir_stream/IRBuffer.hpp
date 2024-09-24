#ifndef CLP_FFI_IR_STREAM_IRBUFFER_HPP
#define CLP_FFI_IR_STREAM_IRBUFFER_HPP

#include <cstdint>

namespace clp::ffi::ir_stream {

class IRBuffer {
public:
    IRBuffer() : m_size(0), m_pos(0) {}

    int8_t* data() {
        return m_buf;
    }

    size_t size() {
        return m_size;
    }

    void push_back(int8_t e) {
        m_buf[m_pos] = e;
        m_pos += 1;
        m_size += 1;
    }

    void insert(int8_t* begin, int8_t* end) {
        auto len = end - begin;
        for (auto i = 0; i < len; i++) {
            m_buf[m_pos+i] = *(begin+i);
        }
        m_pos += len;
        m_size += len;
    }

    void clear() {
        m_size = 0;
        m_pos = 0;
    }

private:
    int8_t m_buf[130'000'000];
    size_t m_size;
    size_t m_pos;
};
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_IRBUFFER_HPP
