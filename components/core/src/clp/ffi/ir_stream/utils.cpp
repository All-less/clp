#include "utils.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>

#include "../../type_utils.hpp"
#include "protocol_constants.hpp"

namespace clp::ffi::ir_stream {
auto serialize_metadata(nlohmann::json& metadata, IRBuffer& output_buf) -> bool {
    output_buf.push_back(cProtocol::Metadata::EncodingJson);

    auto const metadata_serialized
            = metadata.dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore);
    auto const metadata_serialized_length = metadata_serialized.length();
    if (metadata_serialized_length <= UINT8_MAX) {
        output_buf.push_back(cProtocol::Metadata::LengthUByte);
        output_buf.push_back(bit_cast<int8_t>(static_cast<uint8_t>(metadata_serialized_length)));
    } else if (metadata_serialized_length <= UINT16_MAX) {
        output_buf.push_back(cProtocol::Metadata::LengthUShort);
        serialize_int(static_cast<uint16_t>(metadata_serialized_length), output_buf);
    } else {
        // Can't encode metadata longer than 64 KiB
        return false;
    }
    output_buf.insert(metadata_serialized.cbegin(), metadata_serialized.cend());

    return true;
}

auto serialize_string(std::string_view str, IRBuffer& output_buf) -> bool {
    auto const length{str.length()};
    if (length <= UINT8_MAX) {
        output_buf.push_back(cProtocol::Payload::StrLenUByte);
        output_buf.push_back(bit_cast<int8_t>(static_cast<uint8_t>(length)));
    } else if (length <= UINT16_MAX) {
        output_buf.push_back(cProtocol::Payload::StrLenUShort);
        serialize_int(static_cast<uint16_t>(length), output_buf);
    } else if (length <= UINT32_MAX) {
        output_buf.push_back(cProtocol::Payload::StrLenUInt);
        serialize_int(static_cast<uint32_t>(length), output_buf);
    } else {
        // Out of range
        return false;
    }
    output_buf.insert(str.cbegin(), str.cend());
    return true;
}
}  // namespace clp::ffi::ir_stream
