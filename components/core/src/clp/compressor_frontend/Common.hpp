#ifndef COMPRESSOR_FRONTEND_COMMON_HPP
#define COMPRESSOR_FRONTEND_COMMON_HPP

#include <functional>
#include <string>


namespace compressor_frontend {
    uint32_t scan_token(std::function<char(int)> peek, uint32_t limit);
    uint32_t scan_white_space(std::function<char(int)> peek, uint32_t limit);

    bool is_readable(const std::string &s);

    std::string escape(std::string s);

    std::string to_int_repr(const std::string &s);
    std::string from_int_repr(const std::string &s);

    void write_int_vector(std::ofstream &ostream, std::vector<int> v);
    void read_int_vector(std::ifstream &istream, std::vector<int> &v);

    bool file_exists(const std::string &path);
}
#endif // COMPRESSOR_FRONTEND_COMMON_HPP
