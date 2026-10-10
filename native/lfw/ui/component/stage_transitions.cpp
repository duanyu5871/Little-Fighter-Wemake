#include "lfw/ui/component/stage_transitions.h"

namespace lfw {
namespace ui {

const ClazzTag* StageTransitions::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& StageTransitions::TAGS() {
  static const std::vector<std::u16string> tags{u"StageTransitions"};
  return tags;
}

void StageTransitions::on_resume() {
  if (!_remover) {
    _remover = lfw().callbacks.on(
        u"on_enter_next_stage",
        [this](const LfwCallbacks::Payloads&) { on_enter_next_stage(); });
  }
}

void StageTransitions::on_pause() {
  if (_remover) {
    _remover();
    _remover = nullptr;
  }
}

}
}
