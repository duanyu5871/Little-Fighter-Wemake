#pragma once

#include <string>

#include "lfw/core/value.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/team_enum.h"

namespace lfw {

// TS: `base/get_team_text_color.ts` 与 `base/get_team_shadow_color.ts`（后者的**文件名**与它导出的
// 名字不一致 —— 导出的是 `get_team_outline_color`）。两者都读 `Defines.TeamInfoMap`，在端口里就是
// `Defines` 运行时表里那张 JSON。
//
// `fallback` 用指针表达 TS 的**默认参数**：`nullptr` = 「没传」（这时现取
// `TeamInfoMap[TeamEnum.Independent].txt_color`）；传 `&u""` 与传别的值一样，是显式值
// （TS 的默认参数只在 `undefined` 时生效）。
//
// 两个函数对「队名不在表里」的处理**不一样**，别合并：
//   * text：`info?.txt_color || fallback` —— 没有「回落到 Independent」这一层，找不到就用 fallback；
//   * outline：`TeamInfoMap[team] || TeamInfoMap[Independent]` —— 先回落到 Independent，再取字段。
std::u16string get_team_text_color(const std::u16string& team, const std::u16string* fallback = nullptr);
std::u16string get_team_outline_color(const std::u16string& team);

namespace team_color_detail {

inline const Object* team_info(const std::u16string& team) {
  const Value* map = defines::find(u"Defines.TeamInfoMap");
  const Object* obj = map != nullptr ? as_object(*map) : nullptr;
  if (obj == nullptr) return nullptr;
  const Value* info = obj->get(team);
  return info != nullptr ? as_object(*info) : nullptr;
}

// 字段取值：数据里这两个颜色字段恒为非空字符串（见 DESIGN §60.4），所以只认字符串这一种。
inline const std::u16string* team_field(const Object* info, const char16_t* key) {
  if (info == nullptr) return nullptr;
  const Value* v = info->get(std::u16string(key));
  return v != nullptr ? std::get_if<std::u16string>(v) : nullptr;
}

}

inline std::u16string get_team_text_color(const std::u16string& team, const std::u16string* fallback) {
  const std::u16string* color = team_color_detail::team_field(
      team_color_detail::team_info(team), u"txt_color");
  if (color != nullptr && !color->empty()) return *color;
  if (fallback != nullptr) return *fallback;
  const std::u16string* ind = team_color_detail::team_field(
      team_color_detail::team_info(std::u16string(team_enum::kIndependent)), u"txt_color");
  return ind != nullptr ? *ind : std::u16string();
}

inline std::u16string get_team_outline_color(const std::u16string& team) {
  const Object* info = team_color_detail::team_info(team);
  if (info == nullptr) info = team_color_detail::team_info(std::u16string(team_enum::kIndependent));
  const std::u16string* color = team_color_detail::team_field(info, u"txt_outline_color");
  return color != nullptr ? *color : std::u16string();
}

}
