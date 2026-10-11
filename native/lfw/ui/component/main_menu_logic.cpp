#include "lfw/ui/component/main_menu_logic.h"

#include <string>
#include <vector>

#include "lfw/lfw.h"
#include "lfw/ui/component/reachable_group.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/is_class.h"

namespace lfw {
namespace ui {

const ClazzTag* MainMenuLogic::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& MainMenuLogic::TAGS() {
  static const std::vector<std::u16string> tags{u"MainMenuLogic"};
  return tags;
}

std::vector<UINode*>& MainMenuLogic::survival_btns() {
  if (!_survival_btns.has_value()) {
    const std::u16string ids[] = {u"btn_survival", u"btn_survival_2p"};
    std::vector<UINode*> found;
    for (const std::u16string& id : ids) {
      UINode* const n = node.search_node(id);
      if (n != nullptr) found.push_back(n);
    }
    _survival_btns = std::move(found);
  }
  return *_survival_btns;
}

std::vector<UINode*>& MainMenuLogic::danmu_btns() {
  if (!_danmu_btns.has_value()) {
    UINode* const btn = node.search_node(u"btn_danmu_mode");
    std::vector<UINode*> found;
    if (btn != nullptr) found.push_back(btn);
    _danmu_btns = std::move(found);
  }
  return *_danmu_btns;
}

ReachableGroup* MainMenuLogic::reachable_group() {
  UIComponent* const c = node.search_component(ReachableGroup::class_tag());
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), ReachableGroup::class_tag()) ? static_cast<ReachableGroup*>(c)
                                                           : nullptr;
}

void MainMenuLogic::apply() {
  const bool available = lfw().survival_rank_available();
  for (UINode* const btn : survival_btns()) {
    btn->set_visible(available);
    btn->set_disabled(!available);
  }
  const bool danmu_available = lfw().danmu_available;
  for (UINode* const btn : danmu_btns()) {
    btn->set_visible(danmu_available);
    btn->set_disabled(!danmu_available);
  }
  // 生存入口不可用时，初始焦点交给菜单里第一个可用项（vs mode），
  // 否则 auto_focus 落在被隐藏的入口上，整个菜单都没有焦点。
  if (!available && node.focused_node() == nullptr) {
    ReachableGroup* const g = reachable_group();
    if (g != nullptr) g->focus_next();
  }
}

void MainMenuLogic::on_start() {
  UIComponent::on_start();
  apply();
}

void MainMenuLogic::on_resume() {
  UIComponent::on_resume();
  apply();
}

}
}
