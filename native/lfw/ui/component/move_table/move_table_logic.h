#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {

class Entity;

namespace ui {

class Picture;
class ScrollView;
class UINode;

// TS `ui/component/MoveTable/MoveTableLogic.ts`（TAGS = ["MoveTableLogic"]）：招式表演台
// （左侧角色/招式列表，中间舞台放一段按键序列的演出）。页面 `ui/move_table_page.ui.json5`
// 里挂的就是这个名字。
//
// 形状差（源头是端口没有 TS 的 `IMoveListData` / `IMoveInfo` / `IMoveStep` /
// `IMoveDemoEntity` 类型）：
//   * 招式表是 `DatMgr::moves()` 的**未定型** `Value`（`data/moves/*.moves.json5` 一线）⇒
//     字段一律 `field_of` / `field_or` 现读，字段名照 TS 的接口写。
//   * 候选表与步骤里的 `spec` 存 `Value`（与数据表共享同一份对象），不是结构体。
//   * `LGK` 在端口就是 `std::u16string`（`gk::kL` 那些）⇒ `parse_keys` 回 `std::vector`。
//   * `_steps` 的排序必须用 `std::stable_sort`：TS 的 `Array.prototype.sort` 是**稳定**的
//     （README 早把这条列为雷）。
//   * `lfw.load(...zips)` 在端口是**同步** `bool` + 回调总线 ⇒ `load_lists` 先挂
//     `on_loading_end` / `on_loading_failed`（同 `LoadingContentText::on_resume`），
//     没有 `.then` 的微任务延迟。
//   * `this.warn(...)` 在端口是空转（`UIComponent::warn`）⇒ 告警改走 `lfw().warn(...)`。
//   * 控制器是 `click/key_down/key_up(std::vector<std::u16string>)`，不是可变参数。
class MoveTableLogic : public UIComponent {
 public:
  static constexpr const char* TAG = "MoveTableLogic";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();
  static const Value& PROPS();

  MoveTableLogic(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  void on_start() override;
  void on_stop() override;
  void on_key_down(LFWKeyEvent& e) override;
  void update(double dt) override;

 private:
  // TS `enum StepKind`。
  enum class StepKind { Click, Down, Up, Spawn };

  // TS `interface IKeyStep`（`spec` 在端口是未定型的 `Value`）。
  struct KeyStep {
    double at = 0;
    StepKind kind = StepKind::Click;
    std::u16string keys;
    std::optional<size_t> entity;
    Value spec;
  };

  // TS 的两个 getter（`lists` / `list`）：未定型的表，按需过滤。
  std::vector<Value> lists();
  Value list();

  void bind_rows(UINode& content);
  void load_lists();
  void enter_list(int index);
  void start_move(int index);
  void rebuild_scene(const Value& list, const Value& move);
  void enter_move_frame(const std::u16string& id);
  void set_move_keys(UINode* node, const Value& move);
  void set_move_name(UINode* node, const Value& move, const std::u16string& prefix);
  void update_move_labels(const Value& list, const Value& move);
  void click_row(size_t index);
  void build_steps(const Value& move);
  void push_steps(const Value& seq, std::optional<size_t> entity);
  void exec_step(const KeyStep& step);
  std::vector<std::u16string> parse_keys(const std::u16string& str, const Entity* entity) const;
  Entity* spawn_actor(const Value& list, const Value& move);
  void spawn_entities(const Value& spec);
  double clamp_x(double x) const;
  double calc_entity_fit(const std::vector<Value>& spec) const;
  void spawn_entity(const Value& spec, size_t index);

  int _list_index = 0;
  Value _list_ref;
  int _move_index = 0;
  Entity* _actor = nullptr;
  std::vector<Entity*> _entities;
  std::vector<Value> _entity_specs;
  double _entity_fit = 1;
  std::vector<KeyStep> _steps;
  size_t _step_index = 0;
  double _move_time = 0;
  double _end_wait = 0;
  int _phase = 0;
  bool _loading = false;
  std::function<void()> _load_remover;
  UINode* _char_label = nullptr;
  UINode* _char_prev = nullptr;
  UINode* _char_next = nullptr;
  UINode* _name_label = nullptr;
  UINode* _keys_label = nullptr;
  UINode* _desc_label = nullptr;
  Picture* _face = nullptr;
  ScrollView* _scroll = nullptr;
  bool _rows_bound = false;
  bool _arrows_bound = false;
  std::vector<UINode*> _rows;
  std::vector<UINode*> _row_names;
  std::vector<UINode*> _row_keys;
};

}
}
