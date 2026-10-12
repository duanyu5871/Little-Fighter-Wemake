#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {

class LFW;

namespace ui {

// TS `ui/ui_load_img.ts`：校验（错 ⇒ `errors` 按 `\n` 拼接后返回 false）→ 拼 `img_key`
// （`${path}?x=${MD5("x,y,w,h,dw,dh,flip_x,flip_y" 的 join)}`，宿主 MD5）→
// `dw || dh` 时 push crop（`{type:'crop', ...img}`）、`flip_x || flip_y` 时 push flip
// （`x/y` 是**默认 0 之后的**值）→ 宿主 `images.load_img` + `pin`。
//
// 失败通道：`error` 一律是**裸消息**（TS 里 `e.message`；宿主拒绝时是宿主给的原样文本）。
// `host_error != nullptr` 时会写入**错误来源**：宿主拒绝 ⇒ true（TS 里宿主抛出去的任意
// 值，可能是字符串——`'' + e` 无前缀）；校验失败等端口内 `throw new Error(...)` ⇒ false
// （`'' + e` 要带 `Error: ` 前缀）。调用方按自己 TS 对应物的 stringify 口径组前缀。
bool ui_load_img(LFW& lfw, const Value& img, Value& out, std::u16string& error,
                 bool* host_error = nullptr);

}
}
