#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

// TS `loader/validate_ui_img_operation_crop.ts`（`IImageOp_Crop` 的形状校验）。
//
// TS 的 `errors: string[] = []`（原地 push、返回值另说 `any is IImageOp_Crop`）⇒ 端口按
// `validate_ui_img_info` 的样式收一个可空向量指针。调用方是**平台层**的 `ImageMgr`
// （`src/ditto/**` 是宿主边界，端口由宿主实现）⇒ 端口侧暂无调用点。
//
// 偏差：TS 对 `null` 入参会走 `typeof null === 'object'` 这一支，然后在读 `v.type` 时
// TypeError；端口把「不是对象」一律算校验失败（连同 `null`），与其它 `validate_*` 一致。
bool validate_ui_img_operation_crop(const Value& any, std::vector<std::u16string>* errors);

}
