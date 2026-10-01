#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace lfw {

class StateHash {
 public:
  void u8(uint8_t v);
  void u32(uint32_t v);
  void i32(int32_t v);
  void u64(uint64_t v);
  void f64(double v);
  void str(std::string_view v);
  void raw(const void* data, size_t size);

  uint64_t value() const { return _h; }
  std::string hex() const;

 private:
  uint64_t _h = 0xcbf29ce484222325ull;
};

}
