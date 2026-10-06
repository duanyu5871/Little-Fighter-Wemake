#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {

// Mirrors `src/LFW/ditto/zip/IZipObject.ts`。TS 的读取方法全是 `async`（可能 reject）⇒ 端口给
// `bool` + 失败出参（`error` 非空即当抛），调用方（`Resources`）都不 catch ⇒ 直接往外传。
//
// 值的形状与 TS 的差异（记在 README 偏差表）：
//   * `array_buffer` 给的是**字节数组**（`Value` 里的 `Array`），TS 是 `ArrayBuffer`；
//   * `image_bitmap` 的结果（`ImageBitmap`）无法移植 ⇒ 只给一个 `Value` 标记，由调用方/台面自己认。
// `uint8_array` 本刀没有调用点，随用到的刀再补。
class IZipObject {
 public:
  virtual ~IZipObject() = default;
  virtual const std::u16string& name() const = 0;
  // `await obj.json<T>()`。
  virtual bool json(Value& out, std::u16string& error) = 0;
  // `await obj.text()`。
  virtual bool text(Value& out, std::u16string& error) = 0;
  // `await obj.blob_url()`（TS 的 `BlobUrl` 就是 `string`）。
  virtual bool blob_url(Value& out, std::u16string& error) = 0;
  // `await obj.array_buffer()`：字节数组，见上面的说明。
  virtual bool array_buffer(Value& out, std::u16string& error) = 0;
  // `await obj.image_bitmap()`。
  virtual bool image_bitmap(Value& out, std::u16string& error) = 0;
};

}
