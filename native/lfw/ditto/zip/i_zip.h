#pragma once

#include <string>

#include "lfw/ditto/zip/i_zip_object.h"

namespace lfw {

// Mirrors `src/LFW/ditto/zip/IZip.ts` 里 `ZipMgr` 会用到的部分（`name` 与 `file(path: string)`）。
// `files` / `md5` / `set` / `blob` / `file(RegExp)` 这些重载随用到的刀再补。
//
// 注意：真实的 `IZip.file` 是纯查表、不会抛；端口不给抛错面（记在 README 偏差表里）。
class IZip {
 public:
  virtual ~IZip() = default;
  virtual const std::u16string& name() const = 0;
  // `zip.file(path)`：未命中 ⇒ `nullptr`（TS `null`）。
  virtual IZipObject* file(const std::u16string& path) = 0;
};

}
