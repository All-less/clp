#include "utils.hpp"

// C++ standard libraries
#include <memory>

// Project headers
#include "../FileReader.hpp"
#include "Constants.hpp"
#include "LALR1Parser.hpp"
#include "SchemaParser.hpp"

using std::unique_ptr;

namespace compressor_frontend {
void load_lexer_from_file(
        std::string const& schema_file_path,
        bool reverse,
        lexers::ByteLexer& lexer
) {
}
}  // namespace compressor_frontend
