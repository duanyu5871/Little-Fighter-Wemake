#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lfw {

std::vector<uint8_t> encode_utf8(const std::u16string& str);

std::u16string decode_utf8(const std::vector<uint8_t>& buf);

}
