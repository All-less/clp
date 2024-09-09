#ifndef COMPRESSOR_FRONTEND_SPLITMSG_HPP
#define COMPRESSOR_FRONTEND_SPLITMSG_HPP

#include <string>
#include <vector>

namespace compressor_frontend {

    const int WHITE_SPACE = 0,
              READABLE = 1,
              UNREADABLE = 2;

    class MsgPart {
    public:
        MsgPart(uint32_t o, uint32_t l, int t) :
            offset(o), len(l), type(t), is_meta(false) {}
        int get_type() { return type; }
        uint32_t get_offset() { return offset; }
        uint32_t get_len() { return len; }

    friend class SplitMsg;

    private:
        uint32_t offset;
        uint32_t len;
        int type;  // white space | readable | unreadable
        bool is_meta;
    };

    class SplitMsg {
    public:
        SplitMsg(const std::string &orig);
        void mark_meta(int index);
        std::string gen_tpl() const;

        std::vector<MsgPart> &get_parts() { return parts; }
        const std::string &get_msg() { return msg; }

        std::string read_part(int index) const;
        void print_msg();

    private:
        const std::string &msg;
        std::vector<MsgPart> parts;
        std::vector<int> white_spaces;  // indices of white-space parts
        std::vector<int> metadata;  // indices of metadata parts
        std::vector<int> readable;  // indices of readable parts
        std::vector<int> unreadable;  // indices of unreadable parts
    };
}

#endif //COMPRESSOR_FRONTEND_SPLITMSG_HPP
