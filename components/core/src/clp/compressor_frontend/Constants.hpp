#ifndef COMPRESSOR_FRONTEND_CONSTANTS_HPP
#define COMPRESSOR_FRONTEND_CONSTANTS_HPP

#include <cstdint>

namespace compressor_frontend {

typedef std::pair<uint32_t, uint32_t> Interval;

constexpr uint32_t cUnicodeMax = 0x10'FFFF;
constexpr uint32_t cSizeOfByte = 256;
constexpr uint32_t cSizeOfAllChildren = 10'000;
constexpr uint32_t cNullSymbol = 10'000'000;

enum class SymbolID {
    TokenEndID,
    TokenUncaughtStringID,
    TokenIntId,
    TokenFloatId,
    TokenFirstTimestampId,
    TokenNewlineTimestampId,
    TokenNewlineId
};

constexpr char cTokenEnd[] = "$end";
constexpr char cTokenUncaughtString[] = "$UncaughtString";
constexpr char cTokenInt[] = "int";
constexpr char cTokenFloat[] = "float";
constexpr char cTokenFirstTimestamp[] = "firstTimestamp";
constexpr char cTokenNewlineTimestamp[] = "newLineTimestamp";
constexpr char cTokenNewline[] = "newLine";

constexpr uint32_t cStaticByteBuffSize = 60'000;

namespace utf8 {
    // 0xC0, 0xC1, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF are invalid
    // UTF-8 code units
    static uint32_t const cError = 0xFE;
    static unsigned char const cCharEOF = 0xFF;
};  // namespace utf8
}  // namespace compressor_frontend
#endif  // COMPRESSOR_FRONTEND_CONSTANTS_HPP
