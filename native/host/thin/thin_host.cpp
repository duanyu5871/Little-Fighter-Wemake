#include "host/thin/thin_host.h"

namespace lfw::host {

bool ThinHost::step_frame(lfw::World& world, double dt) {
  const int before = _recorder->render_count();
  world.update_once(dt);
  if (_recorder->render_count() != before) return false;  // 世界自己在这一步里渲染了
  world.render_once(dt);
  return true;
}

}  // namespace lfw::host
