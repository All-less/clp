#include "LogParser.hpp"

// C++ standard libraries
#include <filesystem>
#include <iostream>

// Project headers
#include "../clp/utils.hpp"
#include "../streaming_archive/writer/utils.hpp"
#include "../spdlog_with_specializations.hpp"
#include "Constants.hpp"
#include "SchemaParser.hpp"

using compressor_frontend::finite_automata::RegexAST;
using compressor_frontend::finite_automata::RegexASTCat;
using compressor_frontend::finite_automata::RegexASTGroup;
using compressor_frontend::finite_automata::RegexASTInteger;
using compressor_frontend::finite_automata::RegexASTLiteral;
using compressor_frontend::finite_automata::RegexASTMultiplication;
using compressor_frontend::finite_automata::RegexASTOr;
using std::make_unique;
using std::runtime_error;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
using clp::ErrorCode;
using clp::ErrorCode_Success;

namespace compressor_frontend {
LogParser::LogParser(string const& schema_file_path) {
    m_active_uncompressed_msg = nullptr;
    m_uncompressed_msg_size = 0;

    std::unique_ptr<compressor_frontend::SchemaFileAST> schema_ast
            = compressor_frontend::SchemaParser::try_schema_file(schema_file_path);
    add_delimiters(schema_ast->m_delimiters);
    add_rules(schema_ast);
    m_lexer.generate();
}

void LogParser::add_delimiters(unique_ptr<ParserAST> const& delimiters) {
    auto delimiters_ptr = dynamic_cast<DelimiterStringAST*>(delimiters.get());
    if (delimiters_ptr != nullptr) {
        m_lexer.add_delimiters(delimiters_ptr->m_delimiters);
    }
}

void LogParser::add_rules(unique_ptr<SchemaFileAST> const& schema_ast) {
}

void LogParser::increment_uncompressed_msg_pos(ReaderInterface& reader) {
}

void LogParser::parse(ReaderInterface& reader) {
}

Token LogParser::get_next_symbol() {
    return m_lexer.scan();
}
}  // namespace compressor_frontend
