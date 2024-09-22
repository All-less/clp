#ifndef COMPRESSOR_FRONTEND_RDPARSER_HPP
#define COMPRESSOR_FRONTEND_RDPARSER_HPP

#include <string_view>
#include <list>
#include <vector>
#include <optional>

#include "LogParser.hpp"
#include "LogInputBuffer.hpp"
#include "LogOutputBuffer.hpp"

namespace compressor_frontend {

    using Action8 = unsigned char;
    using Action16 = unsigned short;
    using Table16 = std::vector<Action16>;

    class LookupTable {
    public:
        Action16 a;
        Table16 t;
    };

    enum class ParsingAction {
        None,
        Compress,
        CompressAndFinish
    };

    class SpecParsingError : public std::exception {
    public:
        const char *what () const noexcept {
            return "Error in speculative parsing.";
        }
    };

    class RDParser {
    public:
        static RDParser *rd_parser_from_states(char const *state_file);

        RDParser(char const *states_file);

        bool init(LogInputBuffer &ib);
        void parse_new(LogInputBuffer &ib);

        void read_states(char const *states_dir);
        void read_parser_states(char const *states_file);
        void write_states(char const *states_dir);


    private:
        bool spec_mode;

        std::list<Action8> metadata_parsing;
        std::vector<LookupTable> type_parsing;
        std::vector<std::list<Action16>> var_parsing;

        void scan_parse(LogInputBuffer &ib);
        void spec_parse(LogInputBuffer &ib);

        void parse_metadata(LogInputBuffer &ib);
        void detect_type(LogInputBuffer &ib);
        void parse_part(LogInputBuffer &ib, LookupTable &table);
        void parse_variables(LogInputBuffer &ib);
        void validate(LogInputBuffer &ib);

        void read_n(LogInputBuffer &ib, size_t n);
        void read_token(LogInputBuffer &ib);
        void read_until_space(LogInputBuffer &ib);
        void read_until_newline(LogInputBuffer &ib);
    };
}
#endif //COMPRESSOR_FRONTEND_RDPARSER_HPP
