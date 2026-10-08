#include "lfw/ui/component/prefix_and_difficulty_text.h"

#include <memory>

#include "lfw/defines/difficulty.h"
#include "lfw/ui/uinode.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

namespace {

// TS 的 `_style` 字面量（插入序 = 键序）。
Value make_style() {
  Value out(std::make_shared<Object>());
  Object* const o = as_object(out);
  o->set(u"fill_style", Value(std::u16string(u"white")));
  o->set(u"font", Value(std::u16string(u"12px Arial")));
  o->set(u"line_width", Value(1.0));
  o->set(u"disposable", Value(true));
  return out;
}

}

const ClazzTag* PrefixAndDifficultyText::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& PrefixAndDifficultyText::TAGS() {
  static const std::vector<std::u16string> tags{u"PrefixAndDifficultyText"};
  return tags;
}

std::u16string PrefixAndDifficultyText::text() {
  std::u16string out = _prefix;
  out += u" (";
  out += difficulty_full_name(to_number(world().dataset.get(u"difficulty")));
  out += u")";
  return out;
}

void PrefixAndDifficultyText::on_start() {
  _prefix = str(0).value_or(std::u16string());
}

void PrefixAndDifficultyText::on_resume() {
  if (!_remover) {
    _remover = world().callbacks.on(
        u"on_dataset_change", [this](const WorldCallbacks::Payloads& p) {
          for (const WorldCallbackArgs& a : p) on_dataset_change(a.key);
        });
  }
  on_dataset_change(u"difficulty");
}

void PrefixAndDifficultyText::on_pause() {
  if (_remover) {
    _remover();
    _remover = nullptr;
  }
}

void PrefixAndDifficultyText::on_dataset_change(const std::u16string& key) {
  if (key != u"difficulty") return;
  node.set_text(text(), make_style());
}

}
}
