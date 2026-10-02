#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace lfw {
namespace dat_translator {

std::u16string decode_lf2_dat(const uint8_t* data, size_t size);

}
}
