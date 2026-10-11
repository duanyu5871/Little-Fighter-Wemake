#pragma once

#include <optional>
#include <string>
#include <vector>

#include "lfw/entity/entity.h"
#include "lfw/ui/component/smooth_number.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

class Picture;
class UINode;

// TS `ui/component/FighterStatBar.ts`（TAGS = ["FighterStatBar"]）：角色小血条。
// 十一个 `SmoothNumber` 负责各项数值的缓动，各自挂 handler 回写节点。
class FighterStatBar : public UIComponent {
 public:
  static constexpr const char* TAG = "FighterStatBar";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();
  static const Value& PROPS();

  explicit FighterStatBar(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override { return PROPS(); }

  Entity* entity = nullptr;

  void set_entity(Entity* e);
  void on_show() override;
  void update(double dt) override;

  void update_defend_value(std::optional<double> val = {}, std::optional<double> max = {});
  void update_fall_value(std::optional<double> val = {}, std::optional<double> max = {});
  void update_toughness(std::optional<double> val = {}, std::optional<double> max = {});
  void update_hp(std::optional<double> val = {});
  void update_hp_r(std::optional<double> val = {});
  void update_mp(std::optional<double> val = {}, std::optional<double> max = {});
  void update_mp_max();
  void update_head();
  void update_name();
  void update_team();
  void update_bar(UINode* bar, double val = 0, double max = 0);
  void update_hp_tier_colors(const std::vector<UINode*>& tiers, bool flashing);

  UINode* prop_node(const std::u16string& prop_name);
  Picture* head_img();

  double dbg_tier_colors() const {
    return _tier_colors.has_value() ? static_cast<double>(_tier_colors->size()) : -1.0;
  }

  SmoothNumber defend_value_max;
  SmoothNumber defend_value;
  SmoothNumber fall_value_max;
  SmoothNumber fall_value;
  SmoothNumber toughness;
  SmoothNumber toughness_max;
  SmoothNumber hp_max;
  SmoothNumber hp_r;
  SmoothNumber hp;
  SmoothNumber mp_max;
  SmoothNumber mp;

  bool hp_flashing = false;
  bool mp_healing = false;
  bool dead = false;
  std::u16string direction;

 protected:
  std::optional<std::u16string> _eid;
  std::optional<std::u16string> _head;
  std::optional<std::u16string> _name;
  double _name_version = -1;
  std::optional<std::u16string> _name_outline;
  std::optional<std::vector<std::u16string>> _tier_colors;

  void update_tiers(UINode* bar, double val);
};

}
}
