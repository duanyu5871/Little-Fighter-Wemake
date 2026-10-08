#include "lfw/ui/component/difficulty_text.h"

#include "lfw/defines/difficulty.h"
#include "lfw/ui/uinode.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

const ClazzTag* DifficultyText::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& DifficultyText::TAGS() {
  static const std::vector<std::u16string> tags{u"DifficultyText", u"difficulty_text"};
  return tags;
}

void DifficultyText::on_resume() {
  if (!_remover) {
    _remover = world().callbacks.on(
        u"on_dataset_change", [this](const WorldCallbacks::Payloads& p) {
          for (const WorldCallbackArgs& a : p) on_dataset_change(a.key);
        });
  }
  on_dataset_change(u"difficulty");
}

void DifficultyText::on_pause() {
  if (_remover) {
    _remover();
    _remover = nullptr;
  }
}

void DifficultyText::on_dataset_change(const std::u16string& key) {
  if (key != u"difficulty") return;
  const double d = to_number(world().dataset.get(u"difficulty"));
  node.set_text(difficulty_label(d));
}

}
}
