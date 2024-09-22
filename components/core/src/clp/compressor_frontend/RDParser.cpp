#include <optional>
#include <fstream>
#include <iostream>
#include <bitset>
#include <immintrin.h>

#include "Common.hpp"
#include "RDParser.hpp"

namespace compressor_frontend {

    RDParser *RDParser::rd_parser_from_states(char const *states_dir) {
        auto rp = new RDParser(states_dir);
        return rp;
    }

    RDParser::RDParser(char const *states_dir) {
        this->read_states(states_dir);
        this->spec_mode = true;
    }

    void RDParser::write_states(char const *states_dir) {
        auto path = std::string(states_dir) + "/learner_state.txt";
        std::cout << "Write states to directory " << states_dir << std::endl;
        // learner.write_states(path.c_str());
    }

    void RDParser::read_states(char const *states_dir) {
        auto path = std::string(states_dir) + "/parser_state.txt";
        if (file_exists(path)) {
            std::cout << "Read parser states from " << path << std::endl;
            this->read_parser_states(path.c_str());
        }

        path = std::string(states_dir) + "/learner_state.txt";
        if (file_exists(path)) {
            std::cout << "Read learner states from " << path << std::endl;
            // learner.read_states(path.c_str());
        }
    }

    void RDParser::read_parser_states(char const *states_file) {
        std::ifstream istream(states_file, std::ios::binary);

        std::string s;
        int count;
        istream >> s >> count;
        assert(".metadata" == s);
        for (int i = 0; i < count; ++i) {
            istream >> s;
            metadata_parsing.push_back(std::stoi(s, nullptr, 2));
        }

        istream >> s >> count;
        assert(".type" == s);
        for (int i = 0; i < count; ++i) {
            istream >> s;
            auto t = LookupTable{ static_cast<Action16>(std::stoi(s, nullptr, 2)), std::vector<Action16>() };
            int len;
            istream >> len;
            for (int j = 0; j < len; ++j) {
                istream >> s;
                t.t.push_back(std::stoi(s, nullptr, 2));
            }
            type_parsing.push_back(t);
        }

        istream >> s >> count;
        assert(".variables" == s);
        for (int i = 0; i < count; ++i) {
            var_parsing.push_back(std::list<Action16>());
            int len;
            istream >> len;
            for (int j = 0; j < len; ++j) {
                istream >> s;
                var_parsing[i].push_back(std::stoi(s, nullptr, 2));
            }
        }
    }

    /**
     * Parse a single log message from input_buffer.
     *
     * @param ib
     */
    void RDParser::parse_new(LogInputBuffer &ib) {
	    spec_parse(ib);
    }

    void RDParser::spec_parse(LogInputBuffer &ib) {
        parse_metadata(ib);
        detect_type(ib);
        parse_variables(ib);
        validate(ib);
    }

    void RDParser::scan_parse(LogInputBuffer &ib) {

        // parse_metadata(ib);

        // learner.ingest_parse(ib);

        // learner.gen_parsing_tables();
        // read_states("/mnt/clp/scripts/parsing_states.txt");
    }

    void RDParser::parse_metadata(LogInputBuffer &ib) {
        const Action8 BITMASK       = 1 << 7,
                      SCAN_WIDTH    = 1 << 7,
                      SCAN_TO_SPACE = 0 << 7;

        for (auto &&action : metadata_parsing) {
            // std::cout << "a: " << std::bitset<8>(action) << std::endl;
            switch (action & BITMASK) {
                case SCAN_WIDTH: {
                    auto width = (size_t) (action & ~BITMASK);
                    // std::cout << "width: " << width << std::endl;
                    read_n(ib, width);
                    ib.skip_offset(1);
                    break;
                }
                case SCAN_TO_SPACE:
                    read_token(ib);
                    ib.skip_offset(1);
                    break;
            }
        }
    }

    void RDParser::detect_type(LogInputBuffer &ib) {
        const Action16 ACTION_FLAG_MASK = 1 << 15,
                       ADD_OFFSET       = 0 << 15,
                       EXTRACT_BITS     = 1 << 15;

        const Action16 ACTION_POS_MASK   = 0b0000'0000'1111'1111,
                       ACTION_VALUE_MASK = 0b0111'1111'0000'0000;

        const Action16 ENTRY_FLAG_MASK = 0b11 << 14,
                       GOTO_TABLE      = 0b00 << 14,
                       FOUND_TYPE      = 0b01 << 14,
                       NEXT_TOKEN      = 0b10 << 14,
                       NO_MATCH        = 0b11 << 14;

        const Action16 ENTRY_VALUE_MASK = 0b0011'1111'1111'1111;

        auto table = type_parsing[0];
        for ( ; ; ) {
            auto pos = (size_t) table.a & ACTION_POS_MASK;
            auto val = (table.a & ACTION_VALUE_MASK) >> 8;
            int e = 0;

            // std::cout << "table.a: " << std::bitset<16>(table.a) << std::endl;
            // std::cout << "pos: " << pos << std::endl;
            // std::cout << "peekOffset(pos): " << ib.peek_offset(pos) << std::endl;
            // std::cout << "value: " << val << std::endl;

            switch (table.a & ACTION_FLAG_MASK) {
                case ADD_OFFSET:
                    e = ib.peek_offset(pos) - val;
                    break;
                case EXTRACT_BITS:
                    if (val == 0) {  // all-zero bitmask means "scan_token next variable"
                        parse_part(ib, table);
                        table = type_parsing[pos];
                        read_token(ib);
                        continue;
                    } else {
                        e = _pext_u32(ib.peek_offset(pos), val);
                    }
                    break;
                default:
                    throw std::runtime_error("Invalid control word in type table.");
            }

            // std::cout << "e: " << e << std::endl;
            val = table.t[e] & ENTRY_VALUE_MASK;
            switch (table.t[e] & ENTRY_FLAG_MASK) {
                case GOTO_TABLE:
                    // std::cout << "GOTO_TABLE: " << val << std::endl;
                    table = type_parsing[val];
                    break;
                case FOUND_TYPE:
                    // std::cout << "FOUND_TYPE: " << val << std::endl;
                    ib.set_type_id(val);
                    goto FOUND;
                case NEXT_TOKEN:
                    // std::cout << "NEXT_TOKEN: " << val << std::endl;
                    ib.skip_offset(val);
                    read_token(ib);
                    break;
                default:
                    throw std::runtime_error("Encountered invalid entry in type table.");
            }

        }
FOUND:
        return;
    }

    void RDParser::parse_part(LogInputBuffer &ib, LookupTable &table) {
        const Action16 FLAG_MASK = 0b1 << 15,
                       SKIP      = 0b1 << 15,
                       READ      = 0b0 << 15;
        const Action16 VALUE_MASK = 0b0111'1111'1111'1111;

        for (auto &&e : table.t) {
            auto val = e & VALUE_MASK;

            // std::cout << "parsePart action: " << std::bitset<16>(e) << std::endl;
            // std::cout << "val: " << val << std::endl;
            switch (e & FLAG_MASK) {
                case SKIP:
                    // std::cout << "SKIP: " << val << std::endl;
                    ib.skip_offset(val);
                    break;
                case READ:
                    // std::cout << "READ: " << val << std::endl;
                    read_n(ib, val);
                    break;
            }
        }
    }

    void RDParser::parse_variables(LogInputBuffer &ib) {
        const Action16 FLAG_MASK = 0b11 << 14,
                       SKIP      = 0b00 << 14,
                       READ_VAR  = 0b01 << 14,
                       SCAN_VAR  = 0b10 << 14;
        const Action16 VALUE_MASK = 0b0011'1111'1111'1111;
        const Action16 SCAN_TYPE_MASK       = 0b11 << 12,
                       SCAN_READABLE        = 0b00 << 12,
                       SCAN_UNTIL_SPACE     = 0b01 << 12,
                       SCAN_UNTIL_NEWLINE   = 0b10 << 12;


        // TODO: further reduce space by store instructions in bytes
        // Now SCAN_VAR has 14 wasted bits. We could reduce it by:
        //      SKIP     00XX_XXXX XXXX_XXXX (XX means offset)
        //      READ_VAR 01XX_XXXX XXXX_XXXX
        //      SCAN_VAR 1100_0000

        for (auto &&action : var_parsing[ib.get_type_id()]) {
            auto val = action & VALUE_MASK;

            // std::cout << "action: " << std::bitset<16>(action) << std::endl;
            // std::cout << "val: " << val << std::endl;
            switch (action & FLAG_MASK) {
                case SKIP:
                    // std::cout << "SKIP OFFSET: " << val << std::endl;
                    ib.skip_offset(val);
                    break;
                case READ_VAR:
                    // std::cout << "READ FIXED_LEN VAR: " << val << std::endl;
                    read_n(ib, val);
                    break;
                case SCAN_VAR:
                    // std::cout << "SACN VAR" << std::endl;
                    switch (action & SCAN_TYPE_MASK) {
                        case SCAN_READABLE:
                            read_token(ib);
                            break;
                        case SCAN_UNTIL_SPACE:
                            read_until_space(ib);
                            break;
                        case SCAN_UNTIL_NEWLINE:
                            read_until_newline(ib);
                            break;
                    }
                    break;
                default:
                    throw std::runtime_error("Invalid instruction in variable parsing table.");
            }
        }
    }

    void RDParser::validate(LogInputBuffer &ib) {

    }

    void RDParser::read_n(LogInputBuffer &ib, size_t n) {
        ib.read_n(n);
    }

    void RDParser::read_until_space(LogInputBuffer &ib) {
        ib.read_until_space();
    }

    void RDParser::read_until_newline(LogInputBuffer &ib) {
        ib.read_until_newline();
    }

    void RDParser::read_token(LogInputBuffer &ib) {
        ib.read_token();
    }

    bool RDParser::init(LogInputBuffer &ib) {
        return false;
    }
}
