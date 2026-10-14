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

}  // namespace lfw::host
