#include "lfw/ui/component/move_table/move_table_logic.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/bg/background.h"
#include "lfw/camera.h"
#include "lfw/controller/base_controller.h"
#include "lfw/core/js_num.h"
#include "lfw/core/value.h"
#include "lfw/defines/cheat_type.h"
#include "lfw/defines/defines.h"
#include "lfw/defines/entity_group.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/game_key.h"
#include "lfw/defines/state_enum.h"
#include "lfw/defines/team_enum.h"
#include "lfw/entity/entity.h"
#include "lfw/lfw.h"
#include "lfw/loader/dat_mgr.h"
#include "lfw/ui/component/picture.h"
#include "lfw/ui/component/scroll_view.h"
#include "lfw/ui/ui_event.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/type_check.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

namespace {

// TS 文件头那一串常量（名字照抄，避免与 <windows.h> 的宏撞车所以加了 k 前缀）。
constexpr double kDefaultEntityDx = 150;
constexpr double kDefaultEntityHp = 9999;
constexpr double kDefaultStepWait = 100;
constexpr double kDefaultLoopGap = 150;
constexpr double kStartDelay = 1000;
constexpr double kDefaultEndDelay = 2000;
constexpr double kMinInputTime = 200;
constexpr double kMaxInputTime = 4000;
constexpr double kDemoMp = 1000000;
constexpr double kDemoRectLeft = 399;
constexpr double kDemoRectRight = 780;
constexpr double kDemoPad = 45;
constexpr double kActorX = (kDemoRectLeft + kDemoRectRight) / 2;
constexpr double kRowH = 24;
const char16_t* const kDefaultBg = u"bg_move_table";
const char16_t* const kRowCursor = u"▶ ";
const char16_t* const kRowIndent = u"   ";

// TS `x ?? fallback`：只有「缺」或「nullish」才取兜底（`0`、`""` 不算空）。
const Value* nullish_field(const Value& o, const char16_t* name) {
  const Value* const v = field_of(o, name);
  if (v == nullptr) return nullptr;
  if (std::holds_alternative<std::monostate>(*v) || std::holds_alternative<NullTag>(*v)) {
    return nullptr;
  }
  return v;
}

double nullish_num_or(const Value& o, const char16_t* name, double fallback) {
  const Value* const v = nullish_field(o, name);
  return v != nullptr && is_num(*v) ? to_number(*v) : fallback;
}

std::u16string str_field(const Value& o, const char16_t* name) {
  const Value* const v = field_of(o, name);
  if (v == nullptr) return std::u16string();
  const std::u16string* const s = std::get_if<std::u16string>(v);
  return s != nullptr ? *s : std::u16string();
}

std::u16string nullish_str_or(const Value& o, const char16_t* name, const char16_t* fallback) {
  const Value* const v = nullish_field(o, name);
  if (v == nullptr) return std::u16string(fallback);
  const std::u16string* const s = std::get_if<std::u16string>(v);
  return s != nullptr ? *s : std::u16string(fallback);
}

const Array* array_field(const Value& o, const char16_t* name) {
  const Value* const v = field_of(o, name);
  return v != nullptr ? as_array(*v) : nullptr;
}

// TS `groups?.some(g => g === EntityGroup.X)`。
bool groups_has(const Value* groups, const char16_t* group) {
  const Array* const arr = groups != nullptr ? as_array(*groups) : nullptr;
  if (arr == nullptr) return false;
  for (size_t i = 0; i < arr->size(); ++i) {
    if (strict_equals(arr->at(i), Value(std::u16string(group)))) return true;
  }
  return false;
}

// TS `Object.values(data.frames).find(f => f.state === StateEnum.Tired)?.id`。
// 端口的 `Object` 把「整数样」的键放 `_ints`（升序 `std::map`）⇒ 与 JS 的对象键序一致。
std::u16string find_tired_frame_id(const Value& data) {
  const Value* const frames_v = field_of(data, u"frames");
  const Object* const frames = frames_v != nullptr ? as_object(*frames_v) : nullptr;
  if (frames == nullptr) return std::u16string();
  const Value tired = Value(static_cast<double>(StateEnum::Tired));
  for (const std::u16string& key : frames->keys()) {
    const Value* const frame = frames->get(key);
    if (frame == nullptr) continue;
    if (!strict_equals(field_or(*frame, u"state"), tired)) continue;
    return str_field(*frame, u"id");
  }
  return std::u16string();
}

}

const ClazzTag* MoveTableLogic::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& MoveTableLogic::TAGS() {
  static const std::vector<std::u16string> tags{u"MoveTableLogic"};
  return tags;
}

const std::u16string& MoveTableLogic::props_tag() const {
  static const std::u16string tag = u"MoveTableLogic";
  return tag;
}

const Value& MoveTableLogic::PROPS() {
  // TS 没有 `static PROPS`（这个组件不吃 props）⇒ 空对象，`ensure_props()` 直接过。
  static const Value props = Value(std::make_shared<Object>());
  return props;
}

const Value& MoveTableLogic::props_meta() const { return PROPS(); }

MoveTableLogic::MoveTableLogic(UINode& layout, const std::u16string& f_name, const Value& info)
    : UIComponent(layout, f_name, info) {}

std::vector<Value> MoveTableLogic::lists() {
  const std::vector<Value>& moves = lfw().datas().moves();
  const bool cheat_0 = lfw().is_cheat(cheat_enum::kLF2_NET);
  const bool cheat_1 = lfw().is_cheat(cheat_enum::kGIM_INK);
  if (cheat_0 && cheat_1) return moves;
  std::vector<Value> ret;
  for (const Value& v : moves) {
    const Value* const data = lfw().datas().find(field_or(v, u"oid"));
    const Value* const base = data != nullptr ? field_of(*data, u"base") : nullptr;
    const Value* const groups = base != nullptr ? field_of(*base, u"group") : nullptr;
    if (!cheat_0 && groups_has(groups, entity_group::kHidden)) continue;
    if (!cheat_1 && groups_has(groups, entity_group::kDev)) continue;
    ret.push_back(v);
  }
  return ret;
}

Value MoveTableLogic::list() {
  const std::vector<Value> all = lists();
  if (_list_index < 0) return Value();
  const size_t i = static_cast<size_t>(_list_index);
  return i < all.size() ? all[i] : Value();
}

void MoveTableLogic::bind_rows(UINode& content) {
  const std::vector<UINode*>& children = content.children();
  for (size_t i = 0; i < children.size(); ++i) {
    UINode* const row = children[i];
    if (row == nullptr) continue;
    row->move_to(0.0, static_cast<double>(i) * kRowH, 0.0);
    if (!_rows_bound) row->callbacks.on_click = [this, i](LFWPointerEvent&) { click_row(i); };
    _rows.push_back(row);
    const std::vector<UINode*>& kids = row->children();
    _row_names.push_back(kids.size() > 0 ? kids[0] : nullptr);
    _row_keys.push_back(kids.size() > 1 ? kids[1] : nullptr);
  }
  _rows_bound = true;
}

void MoveTableLogic::on_start() {
  _char_label = node.search_node(u"move_table_char");
  _char_prev = node.search_node(u"move_table_char_prev");
  _char_next = node.search_node(u"move_table_char_next");
  _name_label = node.search_node(u"move_table_name");
  _keys_label = node.search_node(u"move_table_keys");
  _desc_label = node.search_node(u"move_table_desc");
  UINode* const face_node = node.search_node(u"move_table_face");
  _face = static_cast<Picture*>(
      face_node != nullptr ? face_node->find_component(Picture::class_tag()) : nullptr);
  UINode* const scroll_node = node.search_node(u"move_list_scroll");
  _scroll = static_cast<ScrollView*>(
      scroll_node != nullptr ? scroll_node->find_component(ScrollView::class_tag()) : nullptr);
  _rows.clear();
  _row_names.clear();
  _row_keys.clear();
  UINode* const content = node.search_node(u"move_list_content");
  if (content != nullptr) bind_rows(*content);
  if (!_arrows_bound) {
    _arrows_bound = true;
    if (_char_prev != nullptr) {
      _char_prev->callbacks.on_click = [this](LFWPointerEvent&) { enter_list(_list_index - 1); };
    }
    if (_char_next != nullptr) {
      _char_next->callbacks.on_click = [this](LFWPointerEvent&) { enter_list(_list_index + 1); };
    }
  }
  if (!lists().empty()) enter_list(0);
  else load_lists();
}

void MoveTableLogic::load_lists() {
  if (_loading) return;
  std::vector<LFW::ZipItem> zips;
  const std::vector<LFW::ZipItem>& all = LFW::ZIPS();
  for (size_t i = 1; i < all.size(); ++i) zips.push_back(all[i]);
  if (zips.empty()) return;
  _loading = true;
  if (_char_label != nullptr) {
    _char_label->set_text(to_string(lfw().string(Value(u"now_loading"))));
  }
  // TS 是 `lfw.load(...zips).then(ok, err)`。端口的 `load` 是**同步** `bool` + 回调总线 ⇒
  // 先挂总线再调（同 `LoadingContentText::on_resume`），成功/失败各收一次尾。
  if (_load_remover) {
    _load_remover();
    _load_remover = nullptr;
  }
  std::vector<std::function<void()>> removers;
  removers.push_back(lfw().callbacks.on(u"on_loading_end", [this](const LfwCallbacks::Payloads&) {
    _loading = false;
    enter_list(0);
  }));
  removers.push_back(
      lfw().callbacks.on(u"on_loading_failed", [this](const LfwCallbacks::Payloads& p) {
        _loading = false;
        lfw().warn(u"[MoveTableLogic] failed to load data zips: " +
                   (p.empty() ? std::u16string() : to_string(p[0].value)));
      }));
  _load_remover = [removers]() {
    for (const std::function<void()>& r : removers) r();
  };
  std::u16string error;
  lfw().load(zips, error);
}

void MoveTableLogic::on_stop() {
  world().clear();
  _actor = nullptr;
  _entities.clear();
  if (_load_remover) {
    _load_remover();
    _load_remover = nullptr;
  }
}

void MoveTableLogic::on_key_down(LFWKeyEvent& e) {
  if (lists().empty()) return;
  if (e.game_key == std::u16string(gk::kL)) enter_list(_list_index - 1);
  else if (e.game_key == std::u16string(gk::kR)) enter_list(_list_index + 1);
  else if (e.game_key == std::u16string(gk::kU)) start_move(_move_index - 1);
  else if (e.game_key == std::u16string(gk::kD)) start_move(_move_index + 1);
  else if (e.game_key == std::u16string(gk::ka)) start_move(_move_index);
  else if (e.game_key == std::u16string(gk::kj)) start_move(_move_index);
}

void MoveTableLogic::update(double dt) {
  if (world().paused()) return;
  const Value list_v = list();
  if (!truthy(list_v) || !strict_equals(list_v, _list_ref)) {
    if (!lists().empty()) enter_list(_list_index);
    return;
  }
  const Array* const moves = array_field(list_v, u"moves");
  if (moves == nullptr || moves->empty()) return;
  if (_actor == nullptr || _actor->hp() <= 0) {
    start_move(_move_index);
    return;
  }
  _actor->position.x = clamp_x(_actor->position.x);
  for (Entity* const e : _entities) {
    if (e != nullptr) e->position.x = clamp_x(e->position.x);
  }
  _move_time += dt;
  if (_phase == 0) {
    while (_step_index < _steps.size() && _steps[_step_index].at <= _move_time) {
      exec_step(_steps[_step_index]);
      _step_index += 1;
    }
    if (_step_index >= _steps.size() && _move_time >= kMinInputTime) _phase = 1;
  } else if (_phase == 1) {
    const Value state = _actor->state();
    const bool idle = strict_equals(state, Value(static_cast<double>(StateEnum::Standing))) ||
                      strict_equals(state, Value(static_cast<double>(StateEnum::Walking)));
    if (idle || _move_time >= kMaxInputTime) {
      _phase = 2;
      _end_wait = 0;
    }
  } else {
    _end_wait += dt;
    const size_t mi = static_cast<size_t>(_move_index);
    const Value move = mi < moves->size() ? moves->at(mi) : Value();
    const Value* const md = nullish_field(move, u"end_delay");
    const double end_delay =
        md != nullptr ? to_number(*md) : nullish_num_or(list_v, u"end_delay", kDefaultEndDelay);
    if (_end_wait >= end_delay) start_move(_move_index);
  }
}

void MoveTableLogic::enter_list(int index) {
  const std::vector<Value> all = lists();
  if (all.empty()) return;
  const int len = static_cast<int>(all.size());
  _list_index = ((index % len) + len) % len;
  const Value& list_v = all[static_cast<size_t>(_list_index)];
  const Value* const data = lfw().datas().find(field_or(list_v, u"oid"));
  const Value* const base = data != nullptr ? field_of(*data, u"base") : nullptr;
  if (_char_label != nullptr) {
    const Value* name = nullish_field(list_v, u"name");
    if (name == nullptr && base != nullptr) name = nullish_field(*base, u"name");
    if (name == nullptr) name = field_of(list_v, u"oid");
    _char_label->set_text(name != nullptr ? to_string(*name) : std::u16string());
  }
  if (_face != nullptr) {
    std::u16string head;
    if (base != nullptr) head = nullish_str_or(*base, u"head", u"");
    if (head.empty()) head = built_in_imgs::kRFACE;
    _face->set_src(head);
  }
  start_move(0);
}

void MoveTableLogic::start_move(int index) {
  const Value list_v = list();
  if (!truthy(list_v)) return;
  _list_ref = list_v;
  const Array* const moves = array_field(list_v, u"moves");
  const int len = moves != nullptr ? static_cast<int>(moves->size()) : 0;
  _move_index = len != 0 ? ((index % len) + len) % len : 0;
  const Value move = moves != nullptr && static_cast<size_t>(_move_index) < moves->size()
                         ? moves->at(static_cast<size_t>(_move_index))
                         : Value();
  rebuild_scene(list_v, move);
  if (truthy(field_or(move, u"frame"))) enter_move_frame(to_string(field_or(move, u"frame")));
  build_steps(move);
  _step_index = 0;
  const Value* const sd = nullish_field(move, u"start_delay");
  _move_time = -(sd != nullptr ? to_number(*sd) : nullish_num_or(list_v, u"start_delay", kStartDelay));
  _end_wait = 0;
  _phase = 0;
  update_move_labels(list_v, move);
}

void MoveTableLogic::rebuild_scene(const Value& list_v, const Value& move) {
  world().clear();
  lfw().change_bg(nullish_str_or(list_v, u"bg", kDefaultBg));
  world().camera().undest();
  world().camera().unlock();
  _entities.clear();
  _entity_specs.clear();
  _actor = spawn_actor(list_v, move);
  const Value* const ents = nullish_field(move, u"entities") != nullptr
                                ? nullish_field(move, u"entities")
                                : nullish_field(list_v, u"entities");
  spawn_entities(ents != nullptr ? *ents : Value());
}

void MoveTableLogic::enter_move_frame(const std::u16string& id) {
  if (_actor == nullptr) return;
  _actor->set_mp(kDemoMp);
  _actor->enter_frame_by_id(Value(id), true);
}

void MoveTableLogic::set_move_keys(UINode* node, const Value& move) {
  if (node == nullptr) return;
  const Value* const key = field_of(move, u"keys");
  node->set_text(key != nullptr && truthy(*key) ? to_string(lfw().string(*key))
                                                : std::u16string());
}

void MoveTableLogic::set_move_name(UINode* node, const Value& move, const std::u16string& prefix) {
  if (node == nullptr) return;
  const Value* const key = field_of(move, u"name");
  node->set_text(key != nullptr && truthy(*key) ? prefix + to_string(lfw().string(*key))
                                                : std::u16string());
}

void MoveTableLogic::update_move_labels(const Value& list_v, const Value& move) {
  set_move_name(_name_label, move, std::u16string());
  set_move_keys(_keys_label, move);
  if (_desc_label != nullptr) {
    std::u16string desc;
    const Value* const d = field_of(move, u"desc");
    const Value* const mp = field_of(move, u"mp");
    if (d != nullptr && truthy(*d)) {
      desc = to_string(lfw().string(*d));
    } else if (mp != nullptr && truthy(*mp)) {
      desc = to_string(lfw().string(Value(u"move_table.mp"))) + u": " + to_string(*mp);
    }
    _desc_label->set_text(desc);
  }
  const Array* const moves = array_field(list_v, u"moves");
  for (size_t i = 0; i < _rows.size(); ++i) {
    const Value row = moves != nullptr && i < moves->size() ? moves->at(i) : Value();
    const bool current = static_cast<int>(i) == _move_index;
    UINode* const row_node = _rows[i];
    if (row_node != nullptr) row_node->set_visible(truthy(row));
    UINode* const name_node = _row_names[i];
    if (name_node != nullptr) {
      name_node->set_opacity(current ? 1.0 : 0.55);
      set_move_name(name_node, row, current ? std::u16string(kRowCursor)
                                            : std::u16string(kRowIndent));
    }
    UINode* const keys_node = _row_keys[i];
    if (keys_node != nullptr) {
      keys_node->set_opacity(current ? 1.0 : 0.55);
      set_move_keys(keys_node, row);
    }
  }
  const size_t cur = static_cast<size_t>(_move_index);
  if (cur < _rows.size() && _rows[cur] != nullptr) _rows[cur]->set_focused(true);
}

void MoveTableLogic::click_row(size_t index) {
  if (_scroll != nullptr && _scroll->dragging()) return;
  const Array* const moves = array_field(list(), u"moves");
  const size_t len = moves != nullptr ? moves->size() : 0;
  if (index >= len) return;
  start_move(static_cast<int>(index));
}

void MoveTableLogic::build_steps(const Value& move) {
  _steps.clear();
  push_steps(field_or(move, u"seq"), std::nullopt);
  for (size_t i = 0; i < _entity_specs.size(); ++i) {
    const Value& spec = _entity_specs[i];
    const Value* const time = field_of(spec, u"time");
    if (time != nullptr) {
      KeyStep step;
      step.at = to_number(*time);
      step.kind = StepKind::Spawn;
      step.entity = i;
      step.spec = spec;
      _steps.push_back(step);
    }
    push_steps(field_or(spec, u"seq"), i);
  }
  // TS `Array.prototype.sort` 是**稳定**的（README 早列为雷）⇒ 这里必须 stable_sort：
  // 同一 `at` 上的 Up/Down/Click 次序（以及 Spawn 与同刻步骤的相对次序）都靠它。
  std::stable_sort(_steps.begin(), _steps.end(),
                   [](const KeyStep& a, const KeyStep& b) { return a.at < b.at; });
}

void MoveTableLogic::push_steps(const Value& seq, std::optional<size_t> entity) {
  const Array* const arr = as_array(seq);
  if (arr == nullptr || arr->empty()) return;
  double at = 0;
  for (size_t i = 0; i < arr->size(); ++i) {
    const Value& step = arr->at(i);
    const Value* const t = nullish_field(step, u"time");
    at = t != nullptr ? to_number(*t) : at + kDefaultStepWait;
    const double loop = std::max(1.0, js_round(nullish_num_or(step, u"loop", 1.0)));
    const double gap = nullish_num_or(step, u"gap", kDefaultLoopGap);
    const Value* const keyups = field_of(step, u"keyups");
    const Value* const keydowns = field_of(step, u"keydowns");
    const Value* const clicks = field_of(step, u"clicks");
    for (int k = 0; k < static_cast<int>(loop); ++k) {
      const double tk = at + static_cast<double>(k) * gap;
      if (keyups != nullptr && truthy(*keyups)) {
        KeyStep s;
        s.at = tk;
        s.kind = StepKind::Up;
        s.keys = to_string(*keyups);
        s.entity = entity;
        _steps.push_back(s);
      }
      if (keydowns != nullptr && truthy(*keydowns)) {
        KeyStep s;
        s.at = tk;
        s.kind = StepKind::Down;
        s.keys = to_string(*keydowns);
        s.entity = entity;
        _steps.push_back(s);
      }
      if (clicks != nullptr && truthy(*clicks)) {
        KeyStep s;
        s.at = tk;
        s.kind = StepKind::Click;
        s.keys = to_string(*clicks);
        s.entity = entity;
        _steps.push_back(s);
      }
    }
    at += (loop - 1.0) * gap;
  }
}

void MoveTableLogic::exec_step(const KeyStep& step) {
  if (step.kind == StepKind::Spawn) {
    if (as_object(step.spec) != nullptr) {
      spawn_entity(step.spec, step.entity.value_or(_entities.size()));
    }
    return;
  }
  Entity* entity = _actor;
  if (step.entity.has_value()) {
    entity = *step.entity < _entities.size() ? _entities[*step.entity] : nullptr;
  }
  if (entity == nullptr) return;
  controller::BaseController* const ctrl = entity->ctrl();
  if (ctrl == nullptr) return;
  const std::vector<std::u16string> keys = parse_keys(step.keys, entity);
  if (keys.empty()) return;
  if (step.kind == StepKind::Click) {
    ctrl->click(keys);
  } else if (step.kind == StepKind::Down) {
    ctrl->key_down(keys);
  } else {
    ctrl->key_up(keys);
  }
}

std::vector<std::u16string> MoveTableLogic::parse_keys(const std::u16string& str,
                                                       const Entity* entity) const {
  const double facing_raw =
      entity != nullptr ? entity->facing : static_cast<double>(FacingFlag::R);
  const int facing = facing_raw > 0 ? 1 : -1;
  std::vector<std::u16string> ret;
  for (const char16_t c : str) {
    switch (c) {
      case u'L': ret.push_back(gk::kL); break;
      case u'R': ret.push_back(gk::kR); break;
      case u'U': ret.push_back(gk::kU); break;
      case u'D': ret.push_back(gk::kD); break;
      case u'a': ret.push_back(gk::ka); break;
      case u'j': ret.push_back(gk::kj); break;
      case u'd': ret.push_back(gk::kd); break;
      case u'F': ret.push_back(facing > 0 ? gk::kR : gk::kL); break;
      case u'B': ret.push_back(facing > 0 ? gk::kL : gk::kR); break;
      default: break;
    }
  }
  return ret;
}

Entity* MoveTableLogic::spawn_actor(const Value& list_v, const Value& move) {
  const Value oid = field_or(list_v, u"oid");
  const Value* const data = lfw().datas().find(oid);
  if (data == nullptr) {
    lfw().warn(u"[MoveTableLogic] fighter data not found: " + to_string(oid));
    return nullptr;
  }
  Entity* const entity = lfw().create_entity(world(), *data);
  if (entity == nullptr) return nullptr;
  Background* const bg = world().bg();
  const double x = nullish_num_or(list_v, u"x", kActorX + nullish_num_or(move, u"dx", 0.0));
  const double z = nullish_num_or(
      list_v, u"z", bg != nullptr ? (bg->near_plane() + bg->far_plane()) / 2 : kActorX);
  entity->set_position(Value(x), Value(0.0), Value(z));
  entity->facing = nullish_num_or(list_v, u"facing", static_cast<double>(FacingFlag::R));
  entity->as_key_role(Value(false));
  entity->attach();
  entity->set_team(nullish_str_or(list_v, u"team", team_enum::kTeam_1));
  entity->set_mp_max(kDemoMp);
  entity->set_mp(kDemoMp);
  entity->ctrl_visible = 1;
  if (truthy(field_or(move, u"stat_bar"))) entity->stat_bar = 1;
  const Value* const hp = field_of(move, u"hp");
  if (hp != nullptr) entity->set_hp(to_number(*hp));
  return entity;
}

void MoveTableLogic::spawn_entities(const Value& spec_v) {
  const Array* const arr = as_array(spec_v);
  std::vector<Value> specs;
  if (arr != nullptr) {
    for (size_t i = 0; i < arr->size(); ++i) specs.push_back(arr->at(i));
  }
  _entity_specs.insert(_entity_specs.end(), specs.begin(), specs.end());
  _entity_fit = calc_entity_fit(specs);
  for (size_t i = 0; i < specs.size(); ++i) {
    // TS `spec[i].time === undefined` ⇒ 有 `time` 的等 `Spawn` 步骤到点再出。
    if (field_of(specs[i], u"time") == nullptr) spawn_entity(specs[i], i);
  }
}

double MoveTableLogic::clamp_x(double x) const {
  return std::min(std::max(x, kDemoRectLeft + kDemoPad), kDemoRectRight - kDemoPad);
}

double MoveTableLogic::calc_entity_fit(const std::vector<Value>& spec) const {
  if (_actor == nullptr || spec.empty()) return 1;
  const int facing = _actor->facing > 0 ? 1 : -1;
  const double ax = _actor->position.x;
  const double left_room = std::max(ax - (kDemoRectLeft + kDemoPad), 0.0);
  const double right_room = std::max(kDemoRectRight - kDemoPad - ax, 0.0);
  double far_l = 0;
  double far_r = 0;
  for (const Value& s : spec) {
    const double dx =
        nullish_num_or(s, u"x", ax + nullish_num_or(s, u"dx", kDefaultEntityDx) * facing) - ax;
    if (dx > far_r) far_r = dx;
    if (-dx > far_l) far_l = -dx;
  }
  const double fit_l = far_l > left_room ? left_room / far_l : 1;
  const double fit_r = far_r > right_room ? right_room / far_r : 1;
  return std::min(fit_l, fit_r);
}

void MoveTableLogic::spawn_entity(const Value& s, size_t index) {
  if (_actor == nullptr) return;
  const int facing = _actor->facing > 0 ? 1 : -1;
  const double ax = _actor->position.x;
  const double az = _actor->position.z;
  const Value oid = field_or(s, u"oid");
  const Value* const data = lfw().datas().find_object(oid);
  if (data == nullptr) {
    lfw().warn(u"[MoveTableLogic] demo entity data not found: " + to_string(oid));
    return;
  }
  Entity* const entity = lfw().create_entity(world(), *data);
  if (entity == nullptr) return;
  const double raw_x = nullish_num_or(s, u"x", ax + nullish_num_or(s, u"dx", kDefaultEntityDx) * facing);
  const double x = clamp_x(ax + (raw_x - ax) * _entity_fit);
  const double z = nullish_num_or(s, u"z", az + nullish_num_or(s, u"dz", 0.0));
  entity->set_position(Value(x), Value(nullish_num_or(s, u"y", 0.0)), Value(z));
  entity->facing = nullish_num_or(s, u"facing",
                                  x >= ax ? static_cast<double>(FacingFlag::L)
                                          : static_cast<double>(FacingFlag::R));
  entity->as_key_role(Value(false));
  entity->attach();
  entity->set_team(nullish_str_or(s, u"team", team_enum::kTeam_2));
  const Value* const hp = field_of(s, u"hp");
  if (hp == nullptr) {
    entity->set_hp_max(kDefaultEntityHp);
    entity->set_hp(kDefaultEntityHp);
  } else {
    entity->set_hp(to_number(*hp));
  }
  if (truthy(field_or(s, u"stat_bar"))) entity->stat_bar = 1;
  if (truthy(field_or(s, u"frame"))) {
    entity->set_mp(kDemoMp);
    entity->enter_frame_by_id(field_or(s, u"frame"), true);
  }
  if (truthy(field_or(s, u"tired"))) {
    const std::u16string tired_id = find_tired_frame_id(*data);
    if (!tired_id.empty()) entity->enter_frame_by_id(Value(tired_id), true);
  }
  if (index >= _entities.size()) _entities.resize(index + 1, nullptr);
  _entities[index] = entity;
}

}
}
