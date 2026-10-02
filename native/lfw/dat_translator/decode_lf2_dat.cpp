#include "lfw/dat_translator/decode_lf2_dat.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace lfw {
namespace dat_translator {
namespace {

const char16_t kPwd[] = u"SiuHungIsAGoodBearBecauseHeIsVeryGood";

constexpr size_t kHeadPlaceholderLength = 123;

void decode(uint8_t* buf, size_t len) {
  const size_t pwd_len = sizeof(kPwd) / sizeof(kPwd[0]) - 1;
  for (size_t i = 0; i < len; ++i) {
    buf[i] = static_cast<uint8_t>(buf[i] - static_cast<uint8_t>(kPwd[i % pwd_len]));
  }
}

}

std::u16string decode_lf2_dat(const uint8_t* data, size_t size) {
  std::vector<uint8_t> buf(data, data + size);
  decode(buf.data(), buf.size());
  std::u16string out;
  if (buf.size() <= kHeadPlaceholderLength) return out;
  out.reserve(buf.size() - kHeadPlaceholderLength);
  for (size_t i = kHeadPlaceholderLength; i < buf.size(); ++i) {
    out.push_back(static_cast<char16_t>(buf[i]));
  }
  return out;
}

}
}
