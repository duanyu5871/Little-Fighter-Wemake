#pragma once

#include "lfw/base/expression.h"
#include "lfw/core/value.h"

namespace lfw {

class Entity;

namespace loader {

// TS: `preprocess_next_frame`（`src/LFW/loader/preprocess_next_frame.ts`）。
//
// 端口只搬**校验那一半**：TS 的返回新数组 / 原样返回，在端口对应「递归检查 + `bool` 结果」
// （调用方拿它决定加载是否失败）。
//
// 编译那一半照 TS 走同一个门：`expression` 是字符串的下帧对象要挂上
// `new Expression(nf.expression, get_val_getter_from_entity)`。端口不往 `Value` 里塞函数对象
// （§4.40），改为按**对象身份**登记进一张表（`Value` 的对象是 `shared_ptr` 共享的，身份跨拷贝
// 稳定），运行时由 `Entity::has_next_frame_judge` / `next_frame_judge` 查这张表 ——
// 「有没有判定器」就等于 TS 的 `!!nf.__judger`，作用域也因此与 TS 完全一致（只有 TS 调用
// `preprocess_next_frame` 的那些位置才会登记）。
bool preprocess_next_frame(Value& nf);

// `nf.__judger`：未登记（或登记后源串对不上）给 `nullptr`（对应 TS 的 `undefined`）。
// 非 const：`Expression::run` 会写自己的 `result`（与 TS 每次 `run` 同一个实例一致）。
Expression<Entity>* judger_of(const Value& nf);

}
}
