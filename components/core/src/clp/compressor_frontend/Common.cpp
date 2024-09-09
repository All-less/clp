#include <sys/stat.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cctype>

#include <boost/algorithm/string.hpp>

#include "Common.hpp"


namespace compressor_frontend {

    uint32_t scan_token(std::function<char(int)> peek, uint32_t limit) {
        const unsigned short SCANNING       = 0 << 8,
                             IN_PARENTHESES = 1 << 8,
                             IN_BRACKETS    = 2 << 8,
                             IN_BRACES      = 3 << 8;
        unsigned short state = SCANNING,
                       level = 0;

        char c;
        uint32_t i = 0;
        for ( ; i < limit; i++) {
            c = peek(i);
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
        return i;
    }

    uint32_t scan_white_space(std::function<char(int)> peek, uint32_t limit) {
        char c;
        for (uint32_t i = 0; i < limit; i++) {
            c = peek(i);
            if (c != ' ' and c != '\n') {
                return i;
            }
        }
        return limit;
    }

    bool is_readable(const std::string &s) {
        for (int i = 0; i < s.length(); ++i) {
            if (!isalpha(s[i])) {
                return false;
            }
        }
        return true;
    }

    std::string escape(std::string s) {
        boost::algorithm::replace_all(s, "\n", "\\n");
        return s;
    }

    std::string to_int_repr(const std::string &s) {
        std::stringstream ss;

        for (auto &c : s) {
            ss << +c << " ";
        }

        return ss.str();
    }

    std::string from_int_repr(const std::string &s) {
        std::stringstream ss(s);
        std::string res;

        int i;
        while (ss >> i) {
            res += static_cast<char>(i);
        }

        return res;
    }

    void write_int_vector(std::ofstream &ostream, std::vector<int> v) {
        ostream << v.size() << std::endl;
        for (auto i : v) {
            ostream << i << std::endl;
        }
    }

    void read_int_vector(std::ifstream &istream, std::vector<int> &v) {
        int size;
        istream >> size;

        int ele;
        for (auto i = 0; i < size; i++) {
            istream >> ele;
            v.push_back(ele);
        }
    }

    bool file_exists(const std::string &path) {
        struct stat buffer;
        return stat(path.c_str(), &buffer) == 0;
    }
}
