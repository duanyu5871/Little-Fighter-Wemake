// `stage/Expressions` + `stage/Status` 与 `bg/Background` + `bg/Layer` 的 C++ 侧台面，
// op 与 `subjects/stage.ts` 一一对应。用例：`cases/stage/expr.txt`、`cases/stage/bg.txt`。
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/bg/background.h"
#include "lfw/core/value.h"
#include "lfw/stage/expressions.h"
#include "lfw/stage/item.h"
#include "lfw/stage/stage.h"
#include "lfw/stage/status.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using trace::num_hex;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::vector<std::string> g_log;

void push(const std::string& line) { g_log.push_back(line); }

std::string num(double d) { return num_hex(d); }

std::string vstr(const lfw::Value& v) { return to_ascii(render_value(v)); }

lfw::Value number_of(const std::vector<std::string>& t, size_t& i, const std::string& op,
                     int lineno) {
  const lfw::Value v = parse_value(t, i);
  if (std::get_if<double>(&v) == nullptr) {
    std::fprintf(stderr, "%s expects a number literal at line %d\n", op.c_str(), lineno);
    std::exit(2);
  }
  return v;
}

// `number_of` 的 double 版：`*std::get_if<double>(&number_of(...))` 是取**临时量**的地址
// （本刀踩过），所以一律走这个。
double number_arg(const std::vector<std::string>& t, size_t& i, const std::string& op,
                  int lineno) {
  const lfw::Value v = number_of(t, i, op, lineno);
  const double* const d = std::get_if<double>(&v);
  if (d == nullptr) {
    std::fprintf(stderr, "%s expects a number literal at line %d\n", op.c_str(), lineno);
    std::exit(2);
  }
  return *d;
}

// ---------------------------------------------------------------- Expressions 侧

// 假表达式：`run` 按脚本吐真假值（跑完最后一个就一直重复），并把每次调用记进日志。
class FakeExpr : public lfw::stage::IExpression<lfw::Value> {
 public:
  FakeExpr(size_t index, std::vector<bool> script)
      : _index(index), _script(std::move(script)) {}

  bool run(const lfw::Value& arg) override {
    push("call:" + std::to_string(_index) + ":arg=" + vstr(arg));
    if (_script.empty()) return false;
    const bool v = _script[_cursor < _script.size() ? _cursor : _script.size() - 1];
    if (_cursor < _script.size() - 1) _cursor++;
    return v;
  }

 private:
  size_t _index = 0;
  std::vector<bool> _script;
  size_t _cursor = 0;
};

lfw::stage::Expressions<lfw::Value> g_exp;
std::vector<std::shared_ptr<FakeExpr>> g_items;
lfw::Value g_arg;

lfw::stage::Expressions<lfw::Value>::Items items_of() {
  lfw::stage::Expressions<lfw::Value>::Items out;
  for (const std::shared_ptr<FakeExpr>& it : g_items) out.push_back(it);
  return out;
}

std::string flag(bool b) { return b ? "1" : "0"; }

void dump_expr() {
  push("dump|n=" + std::to_string(g_exp.list().size()) + "|i=" + num(g_exp.index()) +
       "|first=" + flag(g_exp.is_first()) + "|last=" + flag(g_exp.is_last()));
}

void dump_status() {
  push(std::string("status|") + to_ascii(lfw::status::kRunning) + "|" +
       to_ascii(lfw::status::kCompleted) + "|" + to_ascii(lfw::status::kEnd) + "|" +
       std::to_string(lfw::status_entries().size()));
}

// ---------------------------------------------------------------- Background 侧

lfw::Value g_data;
std::unique_ptr<lfw::Background> g_bg;

void dump_bg() {
  if (!g_bg) {
    push("bg|none");
    return;
  }
  const lfw::Background& b = *g_bg;
  push("bg|id=" + vstr(b.id()) + "|name=" + vstr(b.name()) + "|left=" + num(b.left()) +
       "|right=" + num(b.right()) + "|near=" + num(b.near_plane()) + "|far=" + num(b.far_plane()) +
       "|width=" + num(b.width()) + "|height=" + num(b.height()) + "|depth=" + num(b.depth()) +
       "|mid=" + num(b.middle().x) + "," + num(b.middle().z) + "|zoom=" + num(b.zoom_x()) + "," +
       num(b.zoom_y()) + "," + num(b.zoom_z()) + "|n=" + std::to_string(b.layers().size()) +
       "|ut=" + num(b.update_times()) + "|di=" + num(b.layer_data_index()));
}

void dump_layer(double index) {
  if (!g_bg) {
    push("layer|none");
    return;
  }
  const std::vector<std::shared_ptr<lfw::Layer>>& ls = g_bg->layers();
  if (index < 0 || index >= static_cast<double>(ls.size())) {
    push("layer|oob");
    return;
  }
  const lfw::Layer& l = *ls[static_cast<size_t>(index)];
  const lfw::Value& info = l.info();
  push("layer|di=" + num(l.data_index()) + "|li=" + num(l.loop_index()) + "|x=" +
       num(lfw::to_number(lfw::field_or(info, u"x"))) + "|y=" +
       num(lfw::to_number(lfw::field_or(info, u"y"))) + "|file=" +
       vstr(lfw::field_or(info, u"file")) + "|vis=" + flag(l.visible()) + "|st=" +
       flag(l.is_static()));
}

// ---------------------------------------------------------------- Item 侧

// 假实体：`IItemEntity` 正好是 `Item` 用到的那一面 ⇒ 只把每次设置记进日志。
class FakeItemEntity : public lfw::stage::IItemEntity {
 public:
  FakeItemEntity(std::string label, lfw::Value data)
      : _label(std::move(label)), _data(std::move(data)) {
    auto ref = std::make_shared<lfw::Object>();
    ref->set(u"data", _data);  // `is_fighter(e)` / `is_weapon(e)` 读的是 `e.data`
    _ref = lfw::Value(ref);
  }

  const std::string& label() const { return _label; }

  lfw::Value ref() const override { return _ref; }
  const lfw::Value& data() const override { return _data; }
  // `Stage::kill_*` 会读 `e.team`（TS 里是属性读，故台面**不打日志**）；值由 `set_team` 记下。
  lfw::Value team() const override { return _team; }
  lfw::Callbacks& callbacks() override { return _callbacks; }

  void set_outline_color(const lfw::Value& v) override { push(_label + ":outline=" + vstr(v)); }
  void set_stat_bar(double v) override { push(_label + ":stat_bar=" + num(v)); }
  void set_wakeup_invuln(double v) override { push(_label + ":wakeup_invuln=" + num(v)); }
  void set_dead_gone(double v) override { push(_label + ":dead_gone=" + num(v)); }
  void set_reserve(const lfw::Value& v) override { push(_label + ":reserve=" + vstr(v)); }
  void set_hp(double v) override { push(_label + ":hp=" + num(v)); }
  void set_hp_r(double v) override { push(_label + ":hp_r=" + num(v)); }
  void set_hp_max(double v) override { push(_label + ":hp_max=" + num(v)); }
  void set_mp(double v) override { push(_label + ":mp=" + num(v)); }
  void set_mp_max(double v) override { push(_label + ":mp_max=" + num(v)); }
  void set_name(const lfw::Value& v) override { push(_label + ":name=" + vstr(v)); }
  void set_team(const lfw::Value& v) override {
    _team = v;
    push(_label + ":team=" + vstr(v));
  }
  void set_facing(const lfw::Value& v) override { push(_label + ":facing=" + vstr(v)); }
  void set_dead_join(lfw::Value v) override { push(_label + ":dead_join=" + vstr(v)); }
  void set_position(const lfw::Value& x, const lfw::Value& y, const lfw::Value& z) override {
    push(_label + ":pos=" + vstr(x) + "," + vstr(y) + "," + vstr(z));
  }
  void attach() override { push(_label + ":attach"); }
  void enter_frame_by_id(const lfw::Value& v) override { push(_label + ":frame_id=" + vstr(v)); }
  void enter_frame(const lfw::Value& v) override { push(_label + ":frame=" + vstr(v)); }

  // 台面自己触发那两件事（`Item` 靠它们出列）。
  void fire_dead() { _callbacks.call(u"on_dead", {_ref}); }
  void fire_team_changed() { _callbacks.call(u"on_team_changed", {_ref, lfw::Value(), lfw::Value()}); }

 private:
  std::string _label;
  lfw::Value _data;
  lfw::Value _ref;
  lfw::Value _team = lfw::Value(std::u16string());
  lfw::Callbacks _callbacks;
};

class FakeItemHost : public lfw::stage::IItemHost {
 public:
  double far_value = 0.0;
  double near_value = 0.0;
  lfw::Value team_value = lfw::Value(std::u16string());
  bool aboss = false;
  lfw::Value diff_value = lfw::Value(2.0);
  lfw::MersenneTwister mt_value{0.0};
  std::vector<std::pair<lfw::Value, lfw::Value>> datas;
  std::vector<std::pair<std::u16string, std::shared_ptr<lfw::Randoming>>> groups;
  std::vector<std::unique_ptr<FakeItemEntity>> entities;

  double far_plane() const override {
    push("h:far");
    return far_value;
  }
  double near_plane() const override {
    push("h:near");
    return near_value;
  }
  lfw::Value team() const override {
    push("h:team");
    return team_value;
  }
  bool all_boss_dead() override {
    push("h:aboss");
    return aboss;
  }
  lfw::Value difficulty() const override {
    push("h:diff=" + vstr(diff_value));
    return diff_value;
  }
  lfw::MersenneTwister* mt() override { return &mt_value; }

  lfw::Value datas_find(const lfw::Value& oid) override {
    for (const std::pair<lfw::Value, lfw::Value>& kv : datas) {
      if (lfw::strict_equals(kv.first, oid)) {
        push("h:find=" + vstr(kv.second));
        return kv.second;
      }
    }
    push("h:find=u");
    return lfw::Value();
  }

  std::shared_ptr<lfw::Randoming> datas_randoming_by_group(const lfw::Value& oid) override {
    const std::u16string key = lfw::to_string(oid);
    for (const std::pair<std::u16string, std::shared_ptr<lfw::Randoming>>& kv : groups) {
      if (kv.first == key) {
        push("h:group=" + to_ascii(key) + ":" + std::to_string(kv.second->src().size()));
        return kv.second;
      }
    }
    push("h:group=" + to_ascii(key) + ":0");
    return lfw::Randoming::create(key, {}, &mt_value);
  }

  lfw::stage::IItemEntity* create_entity_with_bot(const lfw::Value& data) override {
    const std::string label = "e" + std::to_string(entities.size());
    push("h:create=" + vstr(data));
    entities.push_back(std::make_unique<FakeItemEntity>(label, data));
    return entities.back().get();
  }

  FakeItemEntity* by_label(const std::string& label) {
    for (const std::unique_ptr<FakeItemEntity>& e : entities) {
      if (e->label() == label) return e.get();
    }
    return nullptr;
  }
};

FakeItemHost g_host;
std::unique_ptr<lfw::stage::Item> g_item;
lfw::Value g_phase;
lfw::Value g_info;

void dump_item() {
  if (!g_item) {
    push("item|none");
    return;
  }
  std::string objs;
  for (const lfw::stage::IItemEntity* e : g_item->objects()) {
    if (!objs.empty()) objs += ",";
    objs += static_cast<const FakeItemEntity*>(e)->label();
  }
  push("item|rel=" + flag(g_item->released()) + "|f=" + flag(g_item->is_fighter()) + "|times=" +
       (g_item->times.has_value() ? num(*g_item->times) : std::string("u")) + "|data=" +
       vstr(g_item->data) + "|objs=" + (objs.empty() ? std::string("-") : objs) + "|delay=" +
       num(g_item->end_delay().value()) + "|rq=" + flag(static_cast<bool>(g_item->randoming)));
}

// ---------------------------------------------------------------- Stage 侧

// 假实体：`IStageEntity` 正好是 `Stage` 用到的那一面。**getter 静默**（TS 那边都是属性读）、
// **setter 打日志**（`hp = hp_r = …` 这类赋值顺序与次数才是可比量）。
class FakeStageEntity : public lfw::stage::IStageEntity {
 public:
  explicit FakeStageEntity(std::string label) : _label(std::move(label)) {}

  const std::string& label() const { return _label; }

  void set_data(const lfw::Value& v) { _data = v; }
  void set_team(const lfw::Value& v) { _team = v; }
  void set_ctrl(const lfw::Value& v) { _ctrl = v; }
  void set_hp_value(double v) { _hp = v; }
  void set_hp_max(double v) { _hp_max = v; }
  void set_hp_r_value(double v) { _hp_r = v; }
  void set_mp_value(double v) { _mp = v; }
  void set_mp_max(double v) { _mp_max = v; }
  void set_mounted(double v) { _mounted = v; }
  void set_x(double v) { _x = v; }
  void set_bearer(FakeStageEntity* v) { _bearer = v; }

  lfw::Value ref() const override {
    auto o = std::make_shared<lfw::Object>();
    o->set(u"data", _data);
    return lfw::Value(o);
  }
  const lfw::Value& data() const override { return _data; }
  lfw::Value team() const override { return _team; }
  lfw::Value ctrl() const override { return _ctrl; }
  lfw::Value facing() const override { return lfw::Value(_facing); }
  void set_facing(const lfw::Value& v) override {
    _facing = lfw::to_number(v);
    push(_label + ":facing=" + vstr(v));
  }
  double hp() const override { return _hp; }
  void set_hp(double v) override {
    _hp = v;
    push(_label + ":hp=" + num(v));
  }
  double hp_max() const override { return _hp_max; }
  double hp_r() const override { return _hp_r; }
  void set_hp_r(double v) override {
    _hp_r = v;
    push(_label + ":hp_r=" + num(v));
  }
  double mp() const override { return _mp; }
  void set_mp(double v) override {
    _mp = v;
    push(_label + ":mp=" + num(v));
  }
  double mp_max() const override { return _mp_max; }
  bool mounted() const override { return lfw::truthy(lfw::Value(_mounted)); }
  double position_x() const override { return _x; }
  void set_position(const lfw::Value& x, const lfw::Value& y, const lfw::Value& z) override {
    push(_label + ":pos=" + vstr(x) + "," + vstr(y) + "," + vstr(z));
  }
  lfw::stage::IStageEntity* bearer() const override { return _bearer; }

 private:
  std::string _label;
  lfw::Value _data;
  lfw::Value _team = lfw::Value(std::u16string());
  lfw::Value _ctrl;
  double _facing = 1.0;
  double _hp = 0.0;
  double _hp_max = 0.0;
  double _hp_r = 0.0;
  double _mp = 0.0;
  double _mp_max = 0.0;
  double _mounted = 0.0;
  double _x = 0.0;
  FakeStageEntity* _bearer = nullptr;
};

// 假表达式：`run` 的脚本来自 phase / dialog 数据里的 `__test` 数组（`Expressions` 的游标
// 语义已经被 `expr` 用例钉住，这里只关心「谁的表达式被跑了、跑了几次」）。
class FakeStageExpr : public lfw::stage::IExpression<lfw::stage::Stage> {
 public:
  FakeStageExpr(size_t index, std::vector<bool> script)
      : _index(index), _script(std::move(script)) {}

  bool run(const lfw::stage::Stage& arg) override {
    (void)arg;
    push("scall:" + std::to_string(_index));
    if (_script.empty()) return false;
    const bool v = _script[_cursor < _script.size() ? _cursor : _script.size() - 1];
    if (_cursor < _script.size() - 1) ++_cursor;
    return v;
  }

 private:
  size_t _index = 0;
  std::vector<bool> _script;
  size_t _cursor = 0;
};

class FakeStageWorld;

class FakeStageLfw : public lfw::stage::IStageLfw {
 public:
  lfw::MersenneTwister mt_value{0.0};
  std::vector<std::pair<lfw::Value, lfw::Value>> backgrounds;
  std::vector<std::pair<lfw::Value, lfw::Value>> stages;
  std::vector<lfw::Value> players;
  double team_counter = 0.0;

  lfw::MersenneTwister* mt() override { return &mt_value; }

  lfw::Value datas_backgrounds_find(const lfw::Value& id) override {
    // TS 是 `datas.backgrounds.find(v => v.id === bid)`，而 `datas.backgrounds` 是**数组**；
    // 台面的等价物是「按数据自己的 `id` 查」，重复 id 时取**最后**登记的那条（与 TS 台面用
    // `Map` 覆盖一致）。
    const lfw::Value* found = nullptr;
    for (const std::pair<lfw::Value, lfw::Value>& kv : backgrounds) {
      if (lfw::strict_equals(lfw::field_or(kv.second, u"id"), id)) found = &kv.second;
    }
    if (found != nullptr) {
      push("h:bgfind=" + vstr(lfw::field_or(*found, u"id")));
      return *found;
    }
    push("h:bgfind=u");
    return lfw::Value();
  }

  lfw::Value datas_stages_find(const lfw::Value& id) override {
    const lfw::Value* found = nullptr;
    for (const std::pair<lfw::Value, lfw::Value>& kv : stages) {
      if (lfw::strict_equals(lfw::field_or(kv.second, u"id"), id)) found = &kv.second;
    }
    if (found != nullptr) {
      push("h:stagefind=" + vstr(lfw::field_or(*found, u"id")));
      return *found;
    }
    push("h:stagefind=u");
    return lfw::Value();
  }

  std::u16string new_team() override {
    team_counter += 1.0;
    const std::u16string v = u"team_" + to_u16(std::to_string(static_cast<int>(team_counter)));
    push("h:newteam=" + to_ascii(v));
    return v;
  }

  bool players_has(const lfw::Value& player_id) const override {
    push("h:players=" + vstr(player_id));
    for (const lfw::Value& p : players) {
      if (lfw::strict_equals(p, player_id)) return true;
    }
    return false;
  }

  std::function<void()> sounds_play_bgm(const lfw::Value& music) override {
    push("h:playbgm=" + vstr(music));
    return []() { push("h:stopbgm"); };
  }

  void sounds_stop_bgm() override { push("h:stopbgm_now"); }

  void sounds_play(const lfw::Value& path, const lfw::Value& x, const lfw::Value& y,
                   const lfw::Value& z) override {
    push("h:sound=" + vstr(path) + "," + vstr(x) + "," + vstr(y) + "," + vstr(z));
  }

  void sounds_play_with_load(const lfw::Value& path) override {
    push("h:loadplay=" + vstr(path));
  }

  lfw::stage::Expressions<lfw::stage::Stage>::Items end_testers(const lfw::Value& owner) override;

  lfw::Value datas_find(const lfw::Value& oid) override { return g_host.datas_find(oid); }
  std::shared_ptr<lfw::Randoming> datas_randoming_by_group(const lfw::Value& oid) override {
    return g_host.datas_randoming_by_group(oid);
  }
  lfw::stage::IItemEntity* create_entity_with_bot(const lfw::Value& data) override {
    return g_host.create_entity_with_bot(data);
  }
};

using StageItems = lfw::stage::Expressions<lfw::stage::Stage>::Items;

// TS 台面把 `__test` 就地转成对象上的 `__end_testers`（实例数组）；`Value` 装不下实例
// ⇒ 端口改成往对象上挂一个**序号**，实例存在这张表里。这样「同一份数据对象」永远拿到
// 同一批实例（游标共享），与 TS 的引用语义一致 —— 也不能像以前那样拿对象**指针**当
// 缓存键：对象释放后地址会被复用，命中错表会随运行而变。
std::vector<StageItems> g_expr_pool;

void make_testers(lfw::Value& owner) {
  lfw::Object* const o = lfw::as_object(owner);
  if (o == nullptr) return;
  StageItems items;
  const lfw::Array* const arr = lfw::as_array(lfw::field_or(owner, u"__test"));
  if (arr != nullptr) {
    for (size_t i = 0; i < arr->size(); ++i) {
      std::vector<bool> flags;
      const lfw::Array* const inner = lfw::as_array(arr->at(i));
      if (inner == nullptr) {
        flags.push_back(lfw::truthy(arr->at(i)));
      } else {
        for (size_t k = 0; k < inner->size(); ++k) flags.push_back(lfw::truthy(inner->at(k)));
      }
      items.push_back(std::make_shared<FakeStageExpr>(i, flags));
    }
  }
  g_expr_pool.push_back(std::move(items));
  o->set(u"__end_testers", lfw::Value(static_cast<double>(g_expr_pool.size() - 1)));
}

void prepare_data(lfw::Value& data) {
  lfw::Value phases_v = lfw::field_or(data, u"phases");
  lfw::Array* const phases = lfw::as_array(phases_v);
  if (phases == nullptr) return;
  for (size_t i = 0; i < phases->size(); ++i) {
    lfw::Value& p = phases->at(i);
    make_testers(p);
    lfw::Value dialogs_v = lfw::field_or(p, u"dialogs");
    lfw::Array* const dialogs = lfw::as_array(dialogs_v);
    if (dialogs == nullptr) continue;
    for (size_t k = 0; k < dialogs->size(); ++k) make_testers(dialogs->at(k));
  }
}

class FakeStageWorld : public lfw::stage::IStageWorld {
 public:
  std::unique_ptr<lfw::Background> bg_value;
  lfw::stage::Stage* self = nullptr;
  std::vector<lfw::stage::IStageEntity*> entity_list;
  std::vector<lfw::stage::IStageEntity*> puppet_list;
  lfw::Value diff_value = lfw::Value(2.0);
  double cam_x = 0.0;

  lfw::World* world_ptr() const override { return nullptr; }

  lfw::Background* bg() const override {
    push("h:bg");
    return bg_value.get();
  }

  void set_bg(std::unique_ptr<lfw::Background> bg) override {
    // 旧 bg 的层数也打出来：`change_bg` 里的 `prev_bg->dispose()` 只有靠这个才看得见。
    const std::string old_info =
        bg_value ? ("|old=" + vstr(bg_value->id()) + ":n=" +
                    std::to_string(bg_value->layers().size()))
                 : std::string("|old=u");
    push("h:setbg=" + (bg ? vstr(bg->id()) : std::string("u")) + old_info);
    bg_value = std::move(bg);
  }

  lfw::stage::Stage* stage() const override {
    push("h:stage=" + flag(self != nullptr));
    return self;
  }

  std::vector<lfw::stage::IStageEntity*>& entities() override {
    push("h:ents=" + std::to_string(entity_list.size()));
    return entity_list;
  }

  std::vector<lfw::stage::IStageEntity*>& puppets() override {
    push("h:pupts=" + std::to_string(puppet_list.size()));
    return puppet_list;
  }

  void del_entities(const std::vector<lfw::stage::IStageEntity*>& es) override {
    std::string names;
    for (const lfw::stage::IStageEntity* e : es) {
      if (!names.empty()) names += ",";
      names += static_cast<const FakeStageEntity*>(e)->label();
    }
    push("h:del=" + (names.empty() ? std::string("-") : names));
  }

  lfw::Value difficulty() const override {
    push("h:diff=" + vstr(diff_value));
    return diff_value;
  }

  void camera_jump_x(const lfw::Value& x) override {
    cam_x = lfw::to_number(x);
    push("h:camjump=" + vstr(x));
  }
};

FakeStageLfw g_slfw;
FakeStageWorld g_sworld;
lfw::Value g_stage_data;
std::unique_ptr<lfw::stage::Stage> g_stage;
std::vector<std::unique_ptr<FakeStageEntity>> g_sentities;

lfw::stage::Expressions<lfw::stage::Stage>::Items FakeStageLfw::end_testers(
    const lfw::Value& owner) {
  const lfw::Value id = lfw::field_or(owner, u"__end_testers");
  const double* const d = std::get_if<double>(&id);
  if (d == nullptr) return StageItems();
  const size_t idx = static_cast<size_t>(*d);
  if (idx >= g_expr_pool.size()) return StageItems();
  return g_expr_pool[idx];
}

FakeStageEntity* stage_entity(const std::string& label) {
  for (const std::unique_ptr<FakeStageEntity>& e : g_sentities) {
    if (e->label() == label) return e.get();
  }
  return nullptr;
}

void dump_stage() {
  if (!g_stage) {
    push("stage|none");
    return;
  }
  lfw::stage::Stage& s = *g_stage;
  std::string objs;
  for (const std::unique_ptr<lfw::stage::Item>& it : s.items) {
    if (!objs.empty()) objs += ",";
    objs += "[" + std::to_string(it->objects().size()) + "]";
  }
  const lfw::Value fsm_state =
      s.fsm().state() != nullptr ? s.fsm().state()->key() : lfw::Value();
  push("stage|id=" + vstr(lfw::Value(s.id())) + "|name=" + vstr(lfw::Value(s.name())) +
       "|title=" + vstr(s.title()) + "|team=" + vstr(s.team()) + "|ph=" + num(s.phase_idx()) +
       "|fin=" + flag(s.is_stage_finish()) + "|cf=" + flag(s.is_chapter_finish()) +
       "|pt=" + num(s.phase_time) + "|dt=" + num(s.dialog_time) + "|di=" + num(s.dialog_idx()) +
       "|dlg=" + (lfw::truthy(s.dialog()) ? vstr(lfw::field_or(s.dialog(), u"id"))
                                          : std::string("u")) +
       "|fsm=" + vstr(fsm_state) + "|ft=" + num(s.fsm().time()) +
       "|L=" + num(s.left) + "|R=" + num(s.right) + "|n=" + num(s.near_plane()) +
       "|f=" + num(s.far_plane()) + "|w=" + num(s.width) + "|d=" + num(s.depth) +
       "|mid=" + num(s.middle.x) + "," + num(s.middle.z) + "|pl=" + num(s.player_l) +
       "|pr=" + num(s.player_r) + "|cl=" + num(s.cam_l) + "|cr=" + num(s.cam_r) +
       "|el=" + num(s.enemy_l) + "|er=" + num(s.enemy_r) + "|dkl=" + num(s.drink_l) +
       "|dkr=" + num(s.drink_r) +
       "|bg=" + (g_sworld.bg_value ? vstr(g_sworld.bg_value->id()) : std::string("u")) +
       "|items=" + (objs.empty() ? std::string("-") : objs));
}

void dump_stage_quest() {
  if (!g_stage) {
    push("squest|none");
    return;
  }
  lfw::stage::Stage& s = *g_stage;
  // ❗ `push("..." + f() + g() + ...)` 里那些**有副作用**的调用不能写在拼接链里：`operator+`
  // 是函数调用，参数求值顺序**未指定**（MSVC 从右往左）⇒ 宿主日志的顺序会与 TS 的模板字符串
  // 插值（严格从左往右）不同。这里一律先按 TS 的顺序求到局部量。
  const double ce_v = s.ce();
  const bool pend = s.is_phase_end();
  const bool dend = s.is_dialog_end();
  const bool abd = s.all_boss_dead();
  const bool afd = s.all_fighter_dead();
  const bool dcl = s.dialog_cleared();
  const bool goto_next = s.should_goto_next_stage();
  const bool wp = s.world_pause();
  const bool cd = s.control_disabled();
  const bool wrd = s.weapon_rain_disabled();
  const lfw::Value next = s.next_stage();
  push("squest|ce=" + num(ce_v) + "|pend=" + flag(pend) +
       "|dend=" + flag(dend) + "|abd=" + flag(abd) +
       "|afd=" + flag(afd) + "|dcl=" + flag(dcl) +
       "|goto=" + flag(goto_next) + "|wp=" + flag(wp) +
       "|cd=" + flag(cd) + "|wrd=" + flag(wrd) +
       "|next=" + vstr(next));
}

// 按 `n` 走 `update()`（TS 侧同名 op 一一对应）。
void stage_update(double n) {
  for (double k = 0.0; k < n; k += 1.0) {
    if (g_stage) g_stage->update();
  }
}

// 回调里只打「能不能分辨出是谁」的摘要（整对象渲染会牵扯键序）。
std::string phase_brief(const lfw::Value& v) {
  return lfw::truthy(v) ? vstr(lfw::field_or(v, u"id")) : std::string("u");
}

std::string dlg_brief(const lfw::Value& v) {
  const lfw::Value list = lfw::field_or(v, u"list");
  const lfw::Array* const a = lfw::as_array(list);
  return vstr(lfw::field_or(v, u"index")) + "/" + std::to_string(a != nullptr ? a->size() : 0);
}

void listen_stage(lfw::stage::Stage* s) {
  using Args = std::vector<lfw::stage::StageCallbackArgs>;
  s->callbacks().on(u"on_stage_finish", [](const Args&) { push("cb:stage_finish"); });
  s->callbacks().on(u"on_chapter_finish", [](const Args&) { push("cb:chapter_finish"); });
  s->callbacks().on(u"on_requrie_goto_next_stage", [](const Args&) { push("cb:goto_next"); });
  s->callbacks().on(u"on_phase_changed", [](const Args& a) {
    push("cb:phase=" + phase_brief(a[0].a) + "," + phase_brief(a[0].b));
  });
  s->callbacks().on(u"on_dialogs_changed", [](const Args& a) {
    push("cb:dlg=" + dlg_brief(a[0].a) + "," + dlg_brief(a[0].b));
  });
}

}

int main(int argc, char** argv) {
  lfw::stage::Stage::set_warn([](const std::u16string& where, const std::u16string& text) {
    push("warn:" + to_ascii(where) + ":" + to_ascii(text));
  });
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_stage <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    // Expressions / Status
    if (op == "it") {
      std::vector<bool> script;
      while (i < t.size()) {
        const lfw::Value v = parse_value(t, i);
        const bool* b = std::get_if<bool>(&v);
        if (b == nullptr) {
          std::fprintf(stderr, "it expects boolean literals at line %d\n", lineno);
          return 2;
        }
        script.push_back(*b);
      }
      g_items.push_back(std::make_shared<FakeExpr>(g_items.size(), script));
    } else if (op == "arg") {
      g_arg = parse_value(t, i);
    } else if (op == "run") {
      push("run=" + flag(g_exp.run(g_arg)));
    } else if (op == "flow") {
      push("flow=" + flag(g_exp.flow(g_arg)));
    } else if (op == "next") {
      g_exp.next();
    } else if (op == "resetsame") {
      g_exp.reset(g_exp.list());
    } else if (op == "resetcopy") {
      const lfw::stage::Expressions<lfw::Value>::Items copy = items_of();
      g_exp.reset(copy);
    } else if (op == "expdump") {
      dump_expr();
    } else if (op == "status") {
      dump_status();
      // Background / Layer
    } else if (op == "data") {
      g_data = parse_value(t, i);
    } else if (op == "new") {
      g_bg = std::make_unique<lfw::Background>(nullptr, g_data);
    } else if (op == "bgdump") {
      dump_bg();
    } else if (op == "layer") {
      const lfw::Value idx = number_of(t, i, op, lineno);
      dump_layer(*std::get_if<double>(&idx));
    } else if (op == "upd") {
      if (g_bg) g_bg->update();
    } else if (op == "disp") {
      if (g_bg) g_bg->dispose();
    } else if (op == "lset") {
      const lfw::Value idx = number_of(t, i, op, lineno);
      const std::string field = to_ascii(trace::key_of(t[i++]));
      const lfw::Value value = parse_value(t, i);
      const lfw::Value layers = lfw::field_or(g_data, u"layers");
      lfw::Value* const arr_v = const_cast<lfw::Value*>(&layers);
      lfw::Array* const arr = lfw::as_array(*arr_v);
      const size_t at = static_cast<size_t>(*std::get_if<double>(&idx));
      if (arr == nullptr || at >= arr->size()) {
        std::fprintf(stderr, "lset out of range at line %d\n", lineno);
        return 2;
      }
      lfw::Object* const o = lfw::as_object(arr->at(at));
      if (o == nullptr) {
        std::fprintf(stderr, "lset needs an object layer at line %d\n", lineno);
        return 2;
      }
      o->set(to_u16(field), value);
    } else if (op == "mtseed") {
      const lfw::Value seed_v = number_of(t, i, op, lineno);
      g_host.mt_value.reset(*std::get_if<double>(&seed_v));
    } else if (op == "datas") {
      const lfw::Value oid = parse_value(t, i);
      const lfw::Value v = parse_value(t, i);
      g_host.datas.push_back({oid, v});
    } else if (op == "datasgroup") {
      const std::string oid = t[i++];
      const std::string name = t[i++];
      std::vector<lfw::Value> src;
      while (i < t.size()) src.push_back(parse_value(t, i));
      g_host.groups.push_back(
          {to_u16(oid), lfw::Randoming::create(to_u16(name), src, &g_host.mt_value)});
    } else if (op == "far") {
      const lfw::Value far_v = number_of(t, i, op, lineno);
      g_host.far_value = *std::get_if<double>(&far_v);
    } else if (op == "near") {
      const lfw::Value near_v = number_of(t, i, op, lineno);
      g_host.near_value = *std::get_if<double>(&near_v);
    } else if (op == "team") {
      g_host.team_value = parse_value(t, i);
    } else if (op == "aboss") {
      g_host.aboss = truthy(parse_value(t, i));
    } else if (op == "diff") {
      g_host.diff_value = parse_value(t, i);
    } else if (op == "phase") {
      g_phase = parse_value(t, i);
    } else if (op == "info") {
      g_info = parse_value(t, i);
    } else if (op == "newitem") {
      g_item = std::make_unique<lfw::stage::Item>(&g_host, g_phase, g_info);
    } else if (op == "upd") {
      if (g_item) g_item->update();
    } else if (op == "updn") {
      const lfw::Value n_v = number_of(t, i, op, lineno);
      const double n = *std::get_if<double>(&n_v);
      for (double k = 0.0; k < n; k += 1.0) {
        if (g_item) g_item->update();
      }
    } else if (op == "spawn") {
      push(std::string("spawn=") + flag(g_item && g_item->spawn()));
    } else if (op == "rel") {
      if (g_item) g_item->release();
    } else if (op == "itemdump") {
      dump_item();
    } else if (op == "dead" || op == "teamchg") {
      FakeItemEntity* const e = g_host.by_label(t[i++]);
      if (e == nullptr) {
        std::fprintf(stderr, "no such entity at line %d\n", lineno);
        return 2;
      }
      if (op == "dead") e->fire_dead();
      else e->fire_team_changed();
      // Stage 侧
    } else if (op == "sbgfind") {
      const lfw::Value id = parse_value(t, i);
      const lfw::Value v = parse_value(t, i);
      g_slfw.backgrounds.push_back({id, v});
    } else if (op == "sstagefind") {
      const lfw::Value id = parse_value(t, i);
      const lfw::Value v = parse_value(t, i);
      g_slfw.stages.push_back({id, v});
    } else if (op == "sbg") {
      g_sworld.bg_value = std::make_unique<lfw::Background>(nullptr, parse_value(t, i));
    } else if (op == "schangebg") {
      if (g_stage) g_stage->change_bg(parse_value(t, i));
    } else if (op == "sdiff") {
      g_sworld.diff_value = parse_value(t, i);
    } else if (op == "splayer") {
      g_slfw.players.push_back(parse_value(t, i));
    } else if (op == "stmseed") {
      g_slfw.mt_value.reset(number_arg(t, i, op, lineno));
    } else if (op == "smtmark") {
      push("mtmark=" + vstr(lfw::Value(g_slfw.mt_value.mark)));
    } else if (op == "steamlike") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_team(g_stage ? g_stage->team() : lfw::Value());
      push("h:teamlike=" + vstr(e->team()));
    } else if (op == "sprop") {
      g_sworld.self = g_stage.get();
    } else if (op == "sent") {
      g_sentities.push_back(std::make_unique<FakeStageEntity>(t[i++]));
    } else if (op == "sentdata") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_data(parse_value(t, i));
    } else if (op == "sentce") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      const lfw::Value ce_v = parse_value(t, i);
      lfw::Value data = e->data();
      if (lfw::as_object(data) == nullptr) {
        data = lfw::Value(std::make_shared<lfw::Object>());
      }
      lfw::Object* const o = lfw::as_object(data);
      auto base = std::make_shared<lfw::Object>();
      const lfw::Object* const po = lfw::as_object(lfw::field_or(*o, u"base"));
      if (po != nullptr) {
        for (const std::u16string& k : po->keys()) base->set(k, lfw::field_or(*po, k.c_str()));
      }
      base->set(u"ce", ce_v);
      o->set(u"base", lfw::Value(base));
      e->set_data(data);
    } else if (op == "sentteam") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_team(parse_value(t, i));
    } else if (op == "sentctrl") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_ctrl(parse_value(t, i));
    } else if (op == "senthp") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_hp_value(number_arg(t, i, op, lineno));
    } else if (op == "senthpmax") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_hp_max(number_arg(t, i, op, lineno));
    } else if (op == "senthpr") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_hp_r_value(number_arg(t, i, op, lineno));
    } else if (op == "sentmp") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_mp_value(number_arg(t, i, op, lineno));
    } else if (op == "sentmpmax") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_mp_max(number_arg(t, i, op, lineno));
    } else if (op == "sentmounted") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_mounted(number_arg(t, i, op, lineno));
    } else if (op == "sentx") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_x(number_arg(t, i, op, lineno));
    } else if (op == "sentbearer") {
      FakeStageEntity* const e = stage_entity(t[i++]);
      if (e == nullptr) return 2;
      e->set_bearer(stage_entity(t[i++]));
    } else if (op == "sentities") {
      g_sworld.entity_list.clear();
      while (i < t.size()) {
        FakeStageEntity* const e = stage_entity(t[i++]);
        if (e == nullptr) return 2;
        g_sworld.entity_list.push_back(e);
      }
    } else if (op == "spuppets") {
      g_sworld.puppet_list.clear();
      while (i < t.size()) {
        FakeStageEntity* const e = stage_entity(t[i++]);
        if (e == nullptr) return 2;
        g_sworld.puppet_list.push_back(e);
      }
    } else if (op == "sdata") {
      g_stage_data = parse_value(t, i);
    } else if (op == "snew") {
      prepare_data(g_stage_data);
      g_stage = std::make_unique<lfw::stage::Stage>(&g_sworld, &g_slfw, g_stage_data);
      listen_stage(g_stage.get());
    } else if (op == "sfree") {
      g_stage.reset();
    } else if (op == "sdump") {
      dump_stage();
    } else if (op == "squest") {
      dump_stage_quest();
    } else if (op == "sphase") {
      if (g_stage) g_stage->enter_phase(number_arg(t, i, op, lineno));
    } else if (op == "supd") {
      stage_update(i < t.size() ? number_arg(t, i, op, lineno) : 1.0);
    } else if (op == "sdisp") {
      if (g_stage) g_stage->dispose();
    } else if (op == "skill") {
      const std::string which = t[i++];
      if (g_stage) {
        if (which == "all") g_stage->kill_all();
        else if (which == "soldiers") g_stage->kill_soliders();
        else if (which == "boss") g_stage->kill_boss();
        else if (which == "others") g_stage->kill_others();
        else return 2;
      }
    } else if (op == "spushd") {
      lfw::Value more = parse_value(t, i);
      // `spushd` 传进来的那些对话框没走过 `prepare_data` ⇒ 就地补上 `__end_testers`
      // （TS 台面同此）。
      lfw::Array* const arr = lfw::as_array(more);
      if (arr != nullptr) {
        for (size_t k = 0; k < arr->size(); ++k) make_testers(arr->at(k));
      }
      if (g_stage) g_stage->push_dialogs(more);
    } else if (op == "snextd") {
      if (g_stage) g_stage->next_dialog();
    } else if (op == "scleard") {
      if (g_stage) g_stage->clear_dialogs();
    } else if (op == "sstopbgm") {
      if (g_stage) g_stage->stop_bgm();
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
    }

    if (i != t.size()) {
      std::fprintf(stderr, "trailing token(s) at line %d: %s\n", lineno, raw.c_str());
      return 2;
    }
    for (const std::string& l : g_log) std::printf("%s\n", l.c_str());
    g_log.clear();
  }
  return 0;
}
