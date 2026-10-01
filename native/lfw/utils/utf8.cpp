#include "utf8.h"

namespace lfw {

std::vector<uint8_t> encode_utf8(const std::u16string& str) {
  const size_t len = str.size();

  size_t byte_len = 0;
  for (size_t i = 0; i < len; ++i) {
    const uint16_t c = str[i];
    if (c < 0x80) {
      byte_len += 1;
    } else if (c < 0x800) {
      byte_len += 2;
    } else if (c >= 0xd800 && c < 0xdc00) {
      byte_len += 4;
      ++i;
    } else {
      byte_len += 3;
    }
  }

  std::vector<uint8_t> bytes(byte_len);
  size_t pos = 0;
  for (size_t i = 0; i < len; ++i) {
    uint32_t c = str[i];
    if (c < 0x80) {
      bytes[pos++] = static_cast<uint8_t>(c);
    } else if (c < 0x800) {
      bytes[pos++] = static_cast<uint8_t>(0xc0 | (c >> 6));
      bytes[pos++] = static_cast<uint8_t>(0x80 | (c & 0x3f));
    } else if (c >= 0xd800 && c < 0xdc00) {
      const uint16_t hi = static_cast<uint16_t>(c);
      ++i;
      const uint16_t lo = i < len ? str[i] : 0;
      c = 0x10000u + static_cast<uint32_t>(((hi & 0x3ffu) << 10) | (lo & 0x3ffu));
      bytes[pos++] = static_cast<uint8_t>(0xf0 | (c >> 18));
      bytes[pos++] = static_cast<uint8_t>(0x80 | ((c >> 12) & 0x3f));
      bytes[pos++] = static_cast<uint8_t>(0x80 | ((c >> 6) & 0x3f));
      bytes[pos++] = static_cast<uint8_t>(0x80 | (c & 0x3f));
    } else {
      bytes[pos++] = static_cast<uint8_t>(0xe0 | (c >> 12));
      bytes[pos++] = static_cast<uint8_t>(0x80 | ((c >> 6) & 0x3f));
      bytes[pos++] = static_cast<uint8_t>(0x80 | (c & 0x3f));
    }
  }
  return bytes;
}

std::u16string decode_utf8(const std::vector<uint8_t>& buf) {
  const size_t len = buf.size();
  std::u16string out;
  size_t i = 0;

  while (i < len) {
    const uint8_t b0 = buf[i++];
    if (b0 < 0x80) {
      out.push_back(static_cast<char16_t>(b0));
    } else if ((b0 & 0xe0) == 0xc0) {
      const uint8_t b1 = i < len ? buf[i++] : 0;
      out.push_back(static_cast<char16_t>(((b0 & 0x1fu) << 6) | (b1 & 0x3fu)));
    } else if ((b0 & 0xf0) == 0xe0) {
      const uint8_t b1 = i < len ? buf[i++] : 0;
      const uint8_t b2 = i < len ? buf[i++] : 0;
      out.push_back(static_cast<char16_t>(((b0 & 0x0fu) << 12) | ((b1 & 0x3fu) << 6) | (b2 & 0x3fu)));
    } else if ((b0 & 0xf8) == 0xf0) {
      const uint8_t b1 = i < len ? buf[i++] : 0;
      const uint8_t b2 = i < len ? buf[i++] : 0;
      const uint8_t b3 = i < len ? buf[i++] : 0;
      const uint32_t cp = (static_cast<uint32_t>(b0 & 0x07u) << 18) |
                          (static_cast<uint32_t>(b1 & 0x3fu) << 12) |
                          (static_cast<uint32_t>(b2 & 0x3fu) << 6) |
                          static_cast<uint32_t>(b3 & 0x3fu);
      const uint32_t off = cp - 0x10000u;
      out.push_back(static_cast<char16_t>(0xd800u + (off >> 10)));
      out.push_back(static_cast<char16_t>(0xdc00u + (off & 0x3ffu)));
    }
  }
  return out;
}

}
