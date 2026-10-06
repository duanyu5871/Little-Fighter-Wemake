#pragma once

#include <string>

namespace lfw {

// Mirrors `src/LFW/ditto/zip/IZipObject.ts` 里 `ZipMgr` 会读到的部分。
// 读取类方法（`text` / `json` / `blob` / `blob_url` / `array_buffer` / `uint8_array` /
// `image_bitmap`）随 `Resources` 那刀再补。
class IZipObject {
 public:
  virtual ~IZipObject() = default;
  virtual const std::u16string& name() const = 0;
};

}
