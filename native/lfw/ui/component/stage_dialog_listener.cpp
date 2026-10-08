#include "lfw/ui/component/stage_dialog_listener.h"

#include <utility>

#include "lfw/core/same_ref.h"
#include "lfw/ui/component/ui_component.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

bool same_dialog(const Value& a, const Value& b) {
  const bool na = is_nullish(a);
  const bool nb = is_nullish(b);
  if (na || nb) return na && nb;
  return same_ref(a, b);
}

}

StageDialogListener::StageDialogListener(UIComponent& owner, Handler handler)
    : _owner(&owner), _handler(std::move(handler)) {}

void StageDialogListener::start() {
  set_stage(_owner->world().stage());
  if (!_world_remover) {
    _world_remover = _owner->world().callbacks.on(
        u"on_stage_change", [this](const WorldCallbacks::Payloads& p) {
          for (const WorldCallbackArgs& a : p) set_stage(a.stage);
        });
  }
}

void StageDialogListener::stop() {
  if (_world_remover) {
    _world_remover();
    _world_remover = nullptr;
  }
  if (_stage_remover) {
    _stage_remover();
    _stage_remover = nullptr;
  }
}

StageDialogListener& StageDialogListener::set_handler(Handler handler) {
  _handler = std::move(handler);
  return *this;
}

void StageDialogListener::set_stage(stage::Stage* stage) {
  if (_stage == stage) return;
  std::function<void()> old = std::move(_stage_remover);
  _stage_remover = nullptr;
  if (stage != nullptr) {
    _stage_remover = stage->callbacks().on(
        u"on_dialogs_changed", [this](const stage::StageCallbacks::Payloads& p) {
          for (const stage::StageCallbackArgs& a : p) set_dialog(a.stage->dialog());
        });
  }
  if (old) old();
  _stage = stage;
  set_dialog(stage != nullptr ? stage->dialog() : Value(NullTag{}));
}

void StageDialogListener::set_dialog(const Value& dialog) {
  if (same_dialog(_dialog, dialog)) return;
  const Value prev = _dialog;
  _dialog = dialog;
  if (_handler) _handler(_dialog, prev);
}

}
}
