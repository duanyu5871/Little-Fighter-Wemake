#include "state_hash.h"

#include "js_num.h"

namespace lfw {
namespace {

constexpr uint64_t kPrime = 0x00000100000001b3ull;

}

void StateHash::u8(uint8_t v) {
  _h ^= static_cast<uint64_t>(v);
  _h *= kPrime;
}

void StateHash::raw(const void* data, size_t size) {
  const auto* bytes = static_cast<const uint8_t*>(data);
  for (size_t i = 0; i < size; ++i) u8(bytes[i]);
}

void StateHash::u32(uint32_t v) {
  const uint8_t b[4] = {
      static_cast<uint8_t>(v),
      static_cast<uint8_t>(v >> 8),
      static_cast<uint8_t>(v >> 16),
      static_cast<uint8_t>(v >> 24),
  };
  raw(b, sizeof b);
}

void StateHash::i32(int32_t v) { u32(static_cast<uint32_t>(v)); }

void StateHash::u64(uint64_t v) {
  uint8_t b[8];
  for (int i = 0; i < 8; ++i) b[i] = static_cast<uint8_t>(v >> (8 * i));
  raw(b, sizeof b);
}

void StateHash::f64(double v) { u64(f64_bits(v)); }

void StateHash::str(std::string_view v) {
  u32(static_cast<uint32_t>(v.size()));
  raw(v.data(), v.size());
}

std::string StateHash::hex() const {
  static constexpr char kDigits[] = "0123456789abcdef";
  std::string s(16, '0');
  for (int i = 0; i < 16; ++i) {
    s[static_cast<size_t>(15 - i)] = kDigits[(_h >> (4 * i)) & 0xf];
  }
  return s;
}

}
