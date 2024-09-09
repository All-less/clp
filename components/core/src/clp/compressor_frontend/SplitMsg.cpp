#include <cctype>
#include <iostream>

#include "SplitMsg.hpp"
#include "Common.hpp"

namespace compressor_frontend {

    SplitMsg::SplitMsg(const std::string &orig) : msg(orig) {

        bool next_is_space = std::isspace(orig[0]);

        uint32_t offset = 0, len;
        int type;

        while (offset < orig.length()) {
            auto peek = [orig, offset](int i){ return orig[offset+i]; };
            uint32_t limit = orig.length() - offset;

            if (next_is_space) {
                len = scan_white_space(peek, limit);
                type = WHITE_SPACE;
            } else {
                len = scan_token(peek, limit);
                type = is_readable(orig.substr(offset, len)) ? READABLE : UNREADABLE;
            }

            parts.push_back(MsgPart{offset, len, type});
            next_is_space = !next_is_space;
            offset += len;
        }
    }

    void SplitMsg::mark_meta(int index) {
        this->parts[index].is_meta = true;
    }

    std::string SplitMsg::gen_tpl() const {
        std::string res;

        /* VERSION 1: unreadable parts replaced with <*>
         *
        for (auto p : this->parts) {
            res += p.type == UNREADABLE ? "<*>" : this->msg.substr(p.offset, p.len);
        }
         */

        /* VERSION 2: VERSION 1 + white spaces in metadata replaced with single space
         *
        for (auto p : this->parts) {
            if (p.type == UNREADABLE) {
                res += "<*>";
            } else if (p.type == WHITE_SPACE && p.is_meta) {
                res += " ";  // collapse white spaces in metadata to one single space
            } else {
                res += this->msg.substr(p.offset, p.len);
            }
        }
         */

        /* VERSION 3: VERSION 1 + stripping away metadata
         *
         */
        for (auto p : this->parts) {
            if (p.is_meta) {
                continue;
            }
            res += p.type == UNREADABLE ? "<*>" : this->msg.substr(p.offset, p.len);
        }

        return res;
    }

    std::string SplitMsg::read_part(int index) const {
        auto p = this->parts[index];
        return this->msg.substr(p.offset, p.len);
    }

    void SplitMsg::print_msg() {
        std::cout << "--- msg ---" << std::endl;
        std::cout << this->msg << std::endl;

        std::cout << "--- parts ---" << std::endl;
        for (auto p : this->parts) {
            std::cout << "type: " << p.type << "\t"
                      << "offset: " << p.offset << "\t"
                      << "len: " << p.len << "\t"
                      << "content: \"" << this->msg.substr(p.offset, p.len) << "\"" << std::endl;
            std::cout << std::endl;
        }
    }
}
