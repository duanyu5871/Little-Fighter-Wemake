#include "host/godot/lfw_godot_host.h"

#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/utf8.h"
#include "lfw/world.h"

namespace lfw::host {

namespace {

std::string str_or(const Value& v, const std::string& fallback) {
  const std::u16string* const s = std::get_if<std::u16string>(&v);
  if (s == nullptr) return fallback;
  const std::vector<uint8_t> buf = lfw::encode_utf8(*s);
  return std::string(buf.begin(), buf.end());
}

double num_or(const Value& holder, const char16_t* key, double fallback) {
  const Value v = lfw::field_or(holder, key);
  const double* const d = std::get_if<double>(&v);
  return d != nullptr ? *d : fallback;
}

}

LfwGodotHost::LfwGodotHost(std::vector<std::filesystem::path> roots, bool dev)
    : HeadlessHost(std::move(roots), dev) {}

void LfwGodotHost::push_frame(World& world) {
  _accumulator_ms -= _step_ms;
  clock().advance(_step_ms);
  world.update_once(_step_ms);
  ++_steps;
}

// 文字度量：装了引擎那一半（`lfw_godot_node.cpp` 用真字体）就走它，否则退回 headless 的零尺寸桩。
Value LfwGodotHost::measure_text(const Value& text, const Value& style) {
  if (_measure_text) return _measure_text(text, style);
  return HeadlessHost::measure_text(text, style);
}

// 图片装载：基类那份给的是"零尺寸占位"（只知道图存在），这里把 TS `ImageInfo` 里的
// `w`/`h`/`scale` 补成真值 —— 只有这三个数会被核心侧读走（`UIImgLoader::load` 的
// `resize(w / scale, h / scale)`；`cook_ui_info` 按 `img_info` 给零轴补算）。
//   * `w`/`h` 是**结果位图**的自然像素：`ui_load_img` 在 `dw || dh` 时加一条 `crop` 操作，
//     它的产物是 `dw × dh` 的一倍图逻辑像素 ⇒ 自然像素 = `dw * scale`（`handle_image_operation_crop`）。
//   * `scale` 是命中条目文件名里的 `@Nx`（`probe_path` 与画的那一遍读同一张表）。
bool LfwGodotHost::ui_image_load(const std::u16string& img_key, const Value& path, const Value& ops,
                                 Value& out, std::u16string& error) {
  if (!HeadlessHost::ui_image_load(img_key, path, ops, out, error)) return false;
  Object* const info = as_object(out);
  if (info == nullptr) return true;

  double crop_w = 0, crop_h = 0;
  const Array* const op_list = as_array(ops);
  for (size_t i = 0; op_list != nullptr && i < op_list->size(); ++i) {
    const Value op = op_list->at(i);
    if (str_or(lfw::field_or(op, u"type"), "") != "crop") continue;
    crop_w = num_or(op, u"dw", 0.0);
    crop_h = num_or(op, u"dh", 0.0);
  }

  double scale = 1.0, w = 0.0, h = 0.0;
  if (_renderer != nullptr) {
    const GodotWorldRenderer::Resolved& res = _renderer->probe_path(str_or(path, ""));
    scale = res.scale > 0.0 ? res.scale : 1.0;
    w = res.img_w * scale;
    h = res.img_h * scale;
    if (crop_w > 0.0) w = crop_w * scale;
    if (crop_h > 0.0) h = crop_h * scale;
  }
  info->set(u"w", Value(w));
  info->set(u"h", Value(h));
  info->set(u"scale", Value(scale));
  return true;
}

}  // namespace lfw::host
