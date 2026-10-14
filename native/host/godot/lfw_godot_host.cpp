#include "host/godot/lfw_godot_host.h"

#include <utility>

#include "lfw/world.h"

namespace lfw::host {

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

}  // namespace lfw::host
