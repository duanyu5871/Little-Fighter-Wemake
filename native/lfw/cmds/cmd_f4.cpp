#include "lfw/cmds/cmd_f4.h"

#include <cstddef>
#include <vector>

#include "lfw/lfw.h"
#include "lfw/ui/uilayer.h"
#include "lfw/ui/uinode.h"
#include "lfw/world.h"

namespace lfw {
namespace cmds {

const char16_t* cmd_f4_help() {
  return uR"(Usage: F4

Popup the current UI / 弹出当前 UI)";
}

void cmd_f4(CMDS& ctx) {
  ui::UILayers* const layers = ctx.world().lfw().ui_layers();
  // TS 那边 `lfw.layers` 一定存在（构造里就建了）；端口对假件留一条空路。
  if (layers == nullptr) return;
  const std::vector<ui::UILayer*> all = layers->all();
  for (int i = static_cast<int>(all.size()) - 1; i >= 0; --i) {
    // `Array.from(this._all)` 保留洞 ⇒ 空的层位是 nullptr（TS 的 `undefined`）。
    ui::UILayer* const layer = all[static_cast<size_t>(i)];
    if (layer == nullptr) continue;
    const double length = static_cast<double>(layer->pages().size());
    if (length <= 1.0 && i == 0) continue;
    if (length <= 0.0) continue;
    layer->pop(ui::UIPopPageOpts{});
    break;
  }
}

}
}
