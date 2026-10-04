#include <algorithm>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/controller/base_controller.h"
#include "lfw/core/js_num.h"
#include "lfw/core/value.h"
#include "lfw/entity/entity.h"
#include "lfw/entity/entity_snapshot.h"
#include "lfw/entity/summary_mgr.h"
#include "lfw/world_dataset.h"

#include "trace_util.h"

namespace {

using lfw::Entity;
using lfw::Value;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;
std::unique_ptr<lfw::WorldDataset> g_dataset;
Value g_bg_dataset;
// `env data` fills the `lfw.datas.find` table (`read_snapshot` looks a data id up).
Value g_datas = Value(std::make_shared<lfw::Object>());
std::u16string g_team = u"1";
int g_id_counter = 0;
std::vector<std::unique_ptr<lfw::controller::BaseController>> g_ctrls;
std::unique_ptr<Entity> g_entity;
std::unique_ptr<Entity> g_buddy;

std::string render(const Value& v) { return to_ascii(render_value(v)); }
std::string s_of(const std::u16string& s) { return to_ascii(s); }

// Snapshot buffers: `run snapbuf` fills them from `to_snapshot`, `run snappoke`
// edits a slot and `run snapapply` feeds them back through `read_snapshot`.
std::vector<Value> g_snap_nums(static_cast<std::size_t>(lfw::entity::num_slots()));
std::vector<std::u16string> g_snap_strs(static_cast<std::size_t>(lfw::entity::str_slots()));

std::string render_num_slots(const std::vector<Value>& nums) {
  std::string s;
  for (std::size_t k = 0; k < nums.size(); ++k) {
    if (k != 0) s += ",";
    s += render(nums[k]);
  }
  return s;
}

std::string render_str_slots(const std::vector<std::u16string>& strs) {
  std::string s;
  for (std::size_t k = 0; k < strs.size(); ++k) {
    if (k != 0) s += ",";
    s += render(Value(strs[k]));
  }
  return s;
}

std::size_t nslot_index(const std::string& name) {
  for (const lfw::EnumNumberEntry& e : lfw::entity::nslot_entries()) {
    if (s_of(e.name) == name) return static_cast<std::size_t>(e.value);
  }
  std::fprintf(stderr, "unknown nslot '%s'\n", name.c_str());
  std::exit(2);
}

std::size_t sslot_index(const std::string& name) {
  for (const lfw::EnumNumberEntry& e : lfw::entity::sslot_entries()) {
    if (s_of(e.name) == name) return static_cast<std::size_t>(e.value);
  }
  std::fprintf(stderr, "unknown sslot '%s'\n", name.c_str());
  std::exit(2);
}

Value field_of(const Value& v, const std::u16string& key) {
  const lfw::Object* o = lfw::as_object(v);
  if (o == nullptr) return Value();
  const Value* p = o->get(key);
  return p != nullptr ? *p : Value();
}

std::u16string text_of(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : lfw::to_string(v);
}

// `v ?? null` for the nullable slots (`dismiss_time`): nullish → `nullopt`.
std::optional<double> opt_num_of(const Value& v) {
  if (std::holds_alternative<std::monostate>(v) || std::holds_alternative<lfw::NullTag>(v)) {
    return std::nullopt;
  }
  return std::optional<double>(lfw::to_number(v));
}

// `catching` / `catcher` / `bearer` / `holding` render as `{id}` (or `null`) so the
// two sides print the same thing without dumping a whole entity.
Value id_ref(const Entity* e) {
  if (e == nullptr) return Value(lfw::NullTag{});
  lfw::Object o;
  o.set(u"id", Value(e->id));
  return Value(std::make_shared<lfw::Object>(o));
}

std::u16string ctrl_mark(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return u"u";
  if (lfw::truthy(field_of(v, u"__is_human_ctrl__"))) return u"human";
  if (lfw::truthy(field_of(v, u"__is_bot_ctrl__"))) return u"bot";
  return u"base";
}

std::u16string ctrl_mark(const lfw::controller::BaseController* c) {
  if (c == nullptr) return u"u";
  if (c->is_human()) return u"human";
  if (c->is_bot()) return u"bot";
  return u"base";
}

std::string who(const Entity& e, const std::vector<Value>& args, std::size_t idx) {
  if (idx >= args.size()) return "?";
  const std::u16string id = lfw::to_string(field_of(args[idx], u"id"));
  return id == e.id ? "self" : "?";
}

class Host : public lfw::IEntityHost {
 public:
  Value world_dataset(const std::u16string& key) const override { return g_dataset->get(key); }

  Value bg_dataset(const std::u16string& key) const override {
    return field_of(g_bg_dataset, key);
  }

  std::u16string new_team() const override { return g_team; }

  std::u16string new_id() override {
    ++g_id_counter;
    return u"e" + lfw::number_to_string(static_cast<double>(g_id_counter));
  }

  lfw::controller::BaseController* acquire_ctrl() override {
    g_log.push_back("acquire_ctrl");
    return make_ctrl(0);
  }

  void release_ctrl(lfw::controller::BaseController* c) override {
    g_log.push_back("release_ctrl:" + s_of(ctrl_mark(c)));
  }

  void mark_players_alive(bool alive) override {
    g_log.push_back("mark_players_alive:" + render(Value(alive)));
  }

  void enter_frame(const Value& nf) override {
    g_log.push_back("enter_frame:" + render(nf));
  }

  void apply_opoints(const Value& opoints) override {
    g_log.push_back("apply_opoints:" + render(opoints));
  }

  void play_sound(const Value& sounds) override {
    g_log.push_back("play_sound:" + render(sounds));
  }

  // `world.lfw.datas.find(id)`
  Value find_data(const std::u16string& id) const override {
    return field_of(g_datas, id);
  }

  // `world.entity_map.get(id) ?? null` — answered from the live harness entities so a
  // `reset` (new id) never leaves a stale lookup behind.
  Entity* find_entity(const std::u16string& id) const override {
    if (g_entity != nullptr && g_entity->id == id) return g_entity.get();
    if (g_buddy != nullptr && g_buddy->id == id) return g_buddy.get();
    return nullptr;
  }

  // kind: 0 = base (the factory's `InvalidController`), 1 = human (`LocalController`),
  // 2 = bot (`BotController`), 3 = human without a player name; the flags/`set_kind`
  // pair is what `is_*_ctrl` reads.
  lfw::controller::BaseController* make_ctrl(int kind) {
    auto c = std::make_unique<lfw::controller::BaseController>();
    c->set_kind(kind == 1 || kind == 3, kind == 2);
    c->player_id = kind == 3 ? u"9" : u"7";
    if (kind == 1) {
      lfw::Object player;
      player.set(u"id", Value(7.0));
      player.set(u"name", Value(std::u16string(u"P7")));
      c->player = Value(std::make_shared<lfw::Object>(player));
    } else if (kind == 3) {
      lfw::Object player;
      player.set(u"id", Value(9.0));
      c->player = Value(std::make_shared<lfw::Object>(player));
    }
    lfw::controller::BaseController* raw = c.get();
    g_ctrls.push_back(std::move(c));
    return raw;
  }
};

std::unique_ptr<Host> g_host;

void bind_callbacks(Entity& e) {
  e.callbacks.on(u"on_hp_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_hp_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" + render(a[2]));
  });
  e.callbacks.on(u"on_hp_max_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_hp_max_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_mp_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_mp_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" + render(a[2]));
  });
  e.callbacks.on(u"on_mp_max_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_mp_max_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_hp_r_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_hp_r_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_team_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_team_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_name_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_name_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_dead", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_dead:" + who(e, a, 0));
  });
  e.callbacks.on(u"on_ctrl_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_ctrl_changed:" + s_of(ctrl_mark(a[0])) + ":" + s_of(ctrl_mark(a[1])) +
                    ":" + who(e, a, 2));
  });
  e.callbacks.on(u"on_reserve_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_reserve_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_resting_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_resting_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_resting_max_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_resting_max_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_fall_value_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_fall_value_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_fall_value_max_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_fall_value_max_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_defend_value_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_defend_value_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_defend_value_max_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_defend_value_max_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_toughness_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_toughness_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_toughness_max_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_toughness_max_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
  e.callbacks.on(u"on_catch_time_max_changed", [&e](const lfw::Callbacks::Payloads& a) {
    g_log.push_back("on_catch_time_max_changed:" + who(e, a, 0) + ":" + render(a[1]) + ":" +
                    render(a[2]));
  });
}

// Harness-side peek at the stat slots TS keeps private; the TS harness reads the same
// names off the real entity (see PROTOCOL).
Value stat_slots(const Entity& e) {
  lfw::Object o;
  o.set(u"catch_time", Value(e.catch_time()));
  o.set(u"toughness_r_value", Value(e.toughness_r_value()));
  o.set(u"fall_r_value", Value(e.fall_r_value()));
  o.set(u"defend_r_value", Value(e.defend_r_value()));
  o.set(u"toughness_r_tick_max", Value(e.toughness_r_tick_max()));
  o.set(u"hp_r_tick_max", Value(e.hp_r_tick_max()));
  o.set(u"mp_r_tick_max", Value(e.mp_r_tick_max()));
  o.set(u"fall_r_tick_max", Value(e.fall_r_tick_max()));
  o.set(u"defend_r_tick_max", Value(e.defend_r_tick_max()));
  return Value(std::make_shared<lfw::Object>(o));
}

std::string join(const std::vector<std::string>& xs) {
  std::string out;
  for (std::size_t i = 0; i < xs.size(); ++i) {
    if (i) out += ",";
    out += xs[i];
  }
  return out;
}

Value arr_of(const std::vector<Value>& xs) {
  lfw::Array a;
  for (const Value& v : xs) a.push_back(v);
  return Value(std::make_shared<lfw::Array>(a));
}

Value role_probe(const Entity& e) {
  return arr_of({Value(e.name_visible), Value(e.wakeup_invuln), Value(e.dead_gone)});
}

Value vec3_value(const lfw::Vector3& v) {
  lfw::Object o;
  o.set(u"x", Value(v.x));
  o.set(u"y", Value(v.y));
  o.set(u"z", Value(v.z));
  return Value(std::make_shared<lfw::Object>(o));
}

bool get_num(const Entity& e, const std::string& name, double& out) {
  if (name == "lifetime") out = e.lifetime();
  else if (name == "spawn_time") out = e.spawn_time();
  else if (name == "render_effect_time") out = e.render_effect_time();
  else if (name == "outline_alpha") out = e.outline_alpha();
  else if (name == "outline_width") out = e.outline_width();
  else if (name == "mix_strength") out = e.mix_strength();
  else if (name == "greyscale") out = e.greyscale();
  else if (name == "ground_y") out = e.ground_y();
  else if (name == "mounted") out = e.mounted();
  else if (name == "ghosted") out = e.ghosted();
  else if (name == "reserve") out = e.reserve();
  else if (name == "toughness_resting_max") out = e.toughness_resting_max();
  else if (name == "resting_max") out = e.resting_max();
  else if (name == "resting") out = e.resting();
  else if (name == "fall_value") out = e.fall_value();
  else if (name == "toughness") out = e.toughness();
  else if (name == "toughness_max") out = e.toughness_max();
  else if (name == "toughness_resting") out = e.toughness_resting();
  else if (name == "catch_time_max") out = e.catch_time_max();
  else if (name == "fall_value_max") out = e.fall_value_max();
  else if (name == "defend_value") out = e.defend_value();
  else if (name == "defend_value_max") out = e.defend_value_max();
  else if (name == "defend_ratio") out = e.defend_ratio();
  else if (name == "mp") out = e.mp();
  else if (name == "hp_r") out = e.hp_r();
  else if (name == "hp") out = e.hp();
  else if (name == "mp_max") out = e.mp_max();
  else if (name == "hp_max") out = e.hp_max();
  else if (name == "blinking") out = e.blinking();
  else if (name == "invisible") out = e.invisible();
  else if (name == "invulnerable") out = e.invulnerable();
  else if (name == "arest") out = e.arest();
  else if (name == "gravity") out = e.gravity();
  else if (name == "itr_motionless") out = e.itr_motionless();
  else if (name == "weight") out = e.weight();
  else if (name == "base_type") out = e.base_type();
  else if (name == "type") out = e.type();
  else if (name == "variant") out = e.variant;
  else if (name == "wait") out = e.wait;
  else if (name == "stat_bar") out = e.stat_bar;
  else if (name == "facing") out = e.facing;
  else if (name == "motionless") out = e.motionless;
  else if (name == "shaking") out = e.shaking;
  else if (name == "fallinjury") out = e.fallinjury;
  else if (name == "throwinjury") out = e.throwinjury;
  else if (name == "name_visible") out = e.name_visible;
  else if (name == "wakeup_invuln") out = e.wakeup_invuln;
  else if (name == "dead_gone") out = e.dead_gone;
  else if (name == "ctrl_visible") out = e.ctrl_visible;
  else if (name == "puppet") out = e.puppet ? 1.0 : 0.0;
  else if (name == "is_on_ground") out = e.is_on_ground ? 1.0 : 0.0;
  else if (name == "jumping.x") out = e.jumping.x;
  else if (name == "jumping.y") out = e.jumping.y;
  else if (name == "jumping.z") out = e.jumping.z;
  else if (name == "jumping.t") out = e.jumping.t;
  else if (name == "aabb_min_x") out = e.aabb_min_x;
  else if (name == "aabb_max_x") out = e.aabb_max_x;
  else if (name == "l_len") out = e.l_len;
  else if (name == "r_len") out = e.r_len;
  else if (name == "atom_time") out = e.atom_time();
  else if (name == "from_wait_block") out = e.from_wait_block() ? 1.0 : 0.0;
  else return false;
  return true;
}

bool get_value(const Entity& e, const std::string& name, Value& out) {
  if (name == "outline_color") out = Value(e.outline_color());
  else if (name == "mix_color") out = Value(e.mix_color());
  else if (name == "outline_enabled") out = e.outline_enabled();
  else if (name == "name") out = e.name();
  else if (name == "team") out = Value(e.team());
  else if (name == "origin_data_id") out = Value(e.origin_data_id());
  else if (name == "group") out = e.group();
  else if (name == "bot_ignore") out = e.bot_ignore();
  else if (name == "state") out = e.state();
  else if (name == "armor") out = e.armor;
  else if (name == "dead_join") out = e.dead_join;
  else if (name == "transforms") out = e.transforms;
  else if (name == "itr") out = e.itr();
  else if (name == "bdy") out = e.bdy();
  else if (name == "frame") out = e.frame;
  else if (name == "prev_frame") out = e.get_prev_frame();
  else if (name == "data") out = e.data();
  else if (name == "emitter") {
    const std::u16string* p = e.emitter();
    out = p != nullptr ? Value(*p) : Value();
  } else if (name == "src_emitter") {
    const std::u16string* p = e.src_emitter();
    out = p != nullptr ? Value(*p) : Value();
  } else if (name == "drink") out = e.drink != nullptr ? Value(true) : Value();
  else if (name == "id") out = Value(e.id);
  else if (name == "ref") out = e.ref();
  else if (name == "velocity") out = vec3_value(e.velocity);
  else if (name == "prev_velocity") out = vec3_value(e.prev_velocity);
  else if (name == "position") out = vec3_value(e.position);
  else if (name == "prev_position") out = vec3_value(e.prev_position);
  else if (name == "dvx") out = e.dvx();
  else if (name == "dvy") out = e.dvy();
  else if (name == "dvz") out = e.dvz();
  else if (name == "landing_frame") out = e.landing_frame();
  else if (name == "dismiss_time")
    out = e.dismiss_time.has_value() ? Value(*e.dismiss_time) : Value(lfw::NullTag{});
  else if (name == "dismiss_data") out = e.dismiss_data;
  else if (name == "catching") out = id_ref(e.catching);
  else if (name == "catcher") out = id_ref(e.catcher);
  else if (name == "bearer") out = id_ref(e.bearer);
  else if (name == "holding") out = id_ref(e.holding);
  else if (name == "ctrl")
    out = e.ctrl() != nullptr ? Value(ctrl_mark(e.ctrl())) : Value();
  else return false;
  return true;
}

bool set_num(Entity& e, const std::string& name, double v) {
  if (name == "outline_alpha") e.set_outline_alpha(v);
  else if (name == "atom_time") e.set_atom_time(v);
  else if (name == "outline_width") e.set_outline_width(v);
  else if (name == "mix_strength") e.set_mix_strength(v);
  else if (name == "greyscale") e.set_greyscale(v);
  else if (name == "ghosted") e.set_ghosted(v);
  else if (name == "reserve") e.set_reserve(v);
  else if (name == "toughness_resting_max") e.set_toughness_resting_max(v);
  else if (name == "resting_max") e.set_resting_max(v);
  else if (name == "resting") e.set_resting(v);
  else if (name == "fall_value") e.set_fall_value(v);
  else if (name == "toughness") e.set_toughness(v);
  else if (name == "toughness_max") e.set_toughness_max(v);
  else if (name == "toughness_resting") e.set_toughness_resting(v);
  else if (name == "catch_time_max") e.set_catch_time_max(v);
  else if (name == "fall_value_max") e.set_fall_value_max(v);
  else if (name == "defend_value") e.set_defend_value(v);
  else if (name == "defend_value_max") e.set_defend_value_max(v);
  else if (name == "defend_ratio") e.set_defend_ratio(v);
  else if (name == "mp") e.set_mp(v);
  else if (name == "hp_r") e.set_hp_r(v);
  else if (name == "hp") e.set_hp(v);
  else if (name == "mp_max") e.set_mp_max(v);
  else if (name == "hp_max") e.set_hp_max(v);
  else if (name == "blinking") e.set_blinking(v);
  else if (name == "invisible") e.set_invisible(v);
  else if (name == "invulnerable") e.set_invulnerable(v);
  else if (name == "arest") e.set_arest(v);
  else if (name == "variant") e.variant = v;
  else if (name == "wait") e.wait = v;
  else if (name == "stat_bar") e.stat_bar = v;
  else if (name == "facing") e.facing = v;
  else if (name == "motionless") e.motionless = v;
  else if (name == "shaking") e.shaking = v;
  else if (name == "fallinjury") e.fallinjury = v;
  else if (name == "throwinjury") e.throwinjury = v;
  else if (name == "name_visible") e.name_visible = v;
  else if (name == "wakeup_invuln") e.wakeup_invuln = v;
  else if (name == "dead_gone") e.dead_gone = v;
  else if (name == "ctrl_visible") e.ctrl_visible = v;
  else if (name == "puppet") e.puppet = lfw::truthy(Value(v));
  else if (name == "is_on_ground") e.is_on_ground = lfw::truthy(Value(v));
  else if (name == "jumping.x") e.jumping.x = v;
  else if (name == "jumping.y") e.jumping.y = v;
  else if (name == "jumping.z") e.jumping.z = v;
  else if (name == "jumping.t") e.jumping.t = v;
  else return false;
  return true;
}

bool set_value(Entity& e, const std::string& name, const Value& v) {
  if (name == "outline_color") e.set_outline_color(text_of(v));
  else if (name == "mix_color") e.set_mix_color(text_of(v));
  else if (name == "outline_enabled") e.set_outline_enabled(v);
  else if (name == "name") e.set_name(v);
  else if (name == "team") e.set_team(text_of(v));
  else if (name == "dismiss_time") e.dismiss_time = opt_num_of(v);
  else if (name == "dismiss_data") e.dismiss_data = v;
  else if (name == "landing_frame") e.set_landing_frame(v);
  else if (name == "transforms") e.transforms = v;
  else if (name == "dead_join") e.dead_join = v;
  else return false;
  return true;
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_entity <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  g_dataset = std::make_unique<lfw::WorldDataset>();
  g_host = std::make_unique<Host>();
  {
    lfw::Object o;
    g_bg_dataset = Value(std::make_shared<lfw::Object>(o));
  }

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    std::size_t i = 1;
    g_log.clear();
    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "dataset") {
        const std::u16string key = text_of(parse_value(t, i));
        g_dataset->set(key, parse_value(t, i));
      } else if (sub == "bg") {
        const std::u16string key = text_of(parse_value(t, i));
        lfw::Object* o = lfw::as_object(g_bg_dataset);
        o->set(key, parse_value(t, i));
      } else if (sub == "team") {
        g_team = text_of(parse_value(t, i));
      } else if (sub == "data") {
        const std::u16string id = text_of(parse_value(t, i));
        lfw::Object* o = lfw::as_object(g_datas);
        o->set(id, parse_value(t, i));
      } else {
        std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      std::printf("env %s\n", sub.c_str());
      continue;
    }
    if (op == "run") {
      const std::string& what = t[i++];
      if (what == "make") {
        g_entity = std::make_unique<Entity>(*g_host, parse_value(t, i));
        bind_callbacks(*g_entity);
        std::printf("run make || id=%s | %s\n", s_of(g_entity->id).c_str(),
                    join(g_log).c_str());
      } else if (what == "reset") {
        g_entity->reset(parse_value(t, i));
        std::printf("run reset || id=%s | %s\n", s_of(g_entity->id).c_str(),
                    join(g_log).c_str());
      } else if (what == "get") {
        const std::string& name = t[i++];
        double n = 0;
        Value v;
        if (get_num(*g_entity, name, n)) v = Value(n);
        else if (get_value(*g_entity, name, v)) {
        } else {
          std::fprintf(stderr, "unknown get '%s' at line %d\n", name.c_str(), lineno);
          return 2;
        }
        std::printf("run get %s || %s | v=%s\n", name.c_str(), join(g_log).c_str(),
                    render(v).c_str());
      } else if (what == "set") {
        const std::string& name = t[i++];
        const Value in_v = parse_value(t, i);
        double n = 0;
        Value out_v;
        if (set_value(*g_entity, name, in_v)) {
          get_value(*g_entity, name, out_v);
        } else if (set_num(*g_entity, name, lfw::to_number(in_v))) {
          get_num(*g_entity, name, n);
          out_v = Value(n);
        } else {
          std::fprintf(stderr, "unknown set '%s' at line %d\n", name.c_str(), lineno);
          return 2;
        }
        std::printf("run set %s %s || %s | v=%s\n", name.c_str(), render(in_v).c_str(),
                    join(g_log).c_str(), render(out_v).c_str());
      } else if (what == "slots") {
        std::printf("run slots || %s | v=%s\n", join(g_log).c_str(),
                    render(stat_slots(*g_entity)).c_str());
      } else if (what == "armor") {
        g_entity->reset_armor();
        std::printf("run armor || %s | v=%s\n", join(g_log).c_str(),
                    render(stat_slots(*g_entity)).c_str());
      } else if (what == "catch") {
        g_entity->set_catch_time(lfw::to_number(parse_value(t, i)));
        std::printf("run catch || %s | v=%s\n", join(g_log).c_str(),
                    render(stat_slots(*g_entity)).c_str());
      } else if (what == "addcatch") {
        g_entity->add_catch_time(lfw::to_number(parse_value(t, i)));
        std::printf("run addcatch || %s | v=%s\n", join(g_log).c_str(),
                    render(stat_slots(*g_entity)).c_str());
      } else if (what == "catching") {
        g_entity->set_catching(nullptr);
        std::printf("run catching || %s\n", join(g_log).c_str());
      } else if (what == "role") {
        g_entity->as_key_role(parse_value(t, i));
        std::printf("run role || %s | v=%s\n", join(g_log).c_str(), render(role_probe(*g_entity)).c_str());
      } else if (what == "autorole") {
        g_entity->auto_key_role();
        std::printf("run autorole || %s | v=%s\n", join(g_log).c_str(),
                    render(role_probe(*g_entity)).c_str());
      } else if (what == "dataset") {
        const Value key = parse_value(t, i);
        std::printf("run dataset %s || %s | v=%s\n", render(key).c_str(), join(g_log).c_str(),
                    render(g_entity->dataset(text_of(key))).c_str());
      } else if (what == "itrfall") {
        const Value itr = parse_value(t, i);
        std::printf("run itrfall %s || %s | v=%s\n", render(itr).c_str(), join(g_log).c_str(),
                    render(g_entity->itr_fall(itr)).c_str());
      } else if (what == "ctrl") {
        const std::string& kind = t[i++];
        if (kind == "none") g_entity->set_ctrl(nullptr);
        else if (kind == "base") g_entity->set_ctrl(g_host->make_ctrl(0));
        else if (kind == "base_released") {
          lfw::controller::BaseController* c = g_host->make_ctrl(0);
          c->keys.d.hit(Value(1.0), 0.0);
          g_entity->set_ctrl(c);
        } else if (kind == "human") g_entity->set_ctrl(g_host->make_ctrl(1));
        else if (kind == "human_bare") g_entity->set_ctrl(g_host->make_ctrl(3));
        else if (kind == "bot") g_entity->set_ctrl(g_host->make_ctrl(2));
        else if (kind == "same") g_entity->set_ctrl(g_entity->ctrl());
        else {
          std::fprintf(stderr, "unknown ctrl '%s' at line %d\n", kind.c_str(), lineno);
          return 2;
        }
        std::printf("run ctrl %s || %s | v=%s\n", kind.c_str(), join(g_log).c_str(),
                    s_of(ctrl_mark(g_entity->ctrl())).c_str());
      } else if (what == "frame") {
        g_entity->frame = parse_value(t, i);
        std::printf("run frame %s || %s | v=%s\n", render(g_entity->frame).c_str(),
                    join(g_log).c_str(), render(g_entity->frame).c_str());
      } else if (what == "pos") {
        const double x = lfw::to_number(parse_value(t, i));
        const double y = lfw::to_number(parse_value(t, i));
        const double z = lfw::to_number(parse_value(t, i));
        g_entity->position.set(x, y, z);
        std::printf("run pos %s %s %s || %s | p=%s\n", render(Value(x)).c_str(),
                    render(Value(y)).c_str(), render(Value(z)).c_str(), join(g_log).c_str(),
                    render(vec3_value(g_entity->position)).c_str());
      } else if (what == "ground") {
        const double y = lfw::to_number(parse_value(t, i));
        g_entity->set_ground_y(y);
        std::printf("run ground %s || %s | g=%s\n", render(Value(y)).c_str(),
                    join(g_log).c_str(), render(Value(g_entity->ground_y())).c_str());
      } else if (what == "link") {
        const std::string& field = t[i++];
        const std::string& to = t[i++];
        Entity* v = to == "self"   ? g_entity.get()
                    : to == "buddy" ? g_buddy.get()
                                    : nullptr;
        if (field == "bearer") g_entity->bearer = v;
        else if (field == "catcher") g_entity->catcher = v;
        else if (field == "holding") g_entity->holding = v;
        else if (field == "catching") g_entity->catching = v;
        else {
          std::fprintf(stderr, "unknown link '%s' at line %d\n", field.c_str(), lineno);
          return 2;
        }
        std::printf("run link %s %s || %s | b=%s c=%s\n", field.c_str(), to.c_str(),
                    join(g_log).c_str(),
                    render(Value(g_entity->bearer != nullptr)).c_str(),
                    render(Value(g_entity->catcher != nullptr)).c_str());
      } else if (what == "setvel") {
        const Value x = parse_value(t, i);
        const Value y = parse_value(t, i);
        const Value z = parse_value(t, i);
        g_entity->set_velocity(x, y, z);
        std::printf("run setvel %s %s %s || %s | v=%s pv=%s g=%s\n", render(x).c_str(),
                    render(y).c_str(), render(z).c_str(), join(g_log).c_str(),
                    render(vec3_value(g_entity->velocity)).c_str(),
                    render(vec3_value(g_entity->prev_velocity)).c_str(),
                    render(Value(g_entity->is_on_ground)).c_str());
      } else if (what == "leaveground") {
        g_entity->leave_ground();
        std::printf("run leaveground || %s | p=%s g=%s\n", join(g_log).c_str(),
                    render(vec3_value(g_entity->position)).c_str(),
                    render(Value(g_entity->is_on_ground)).c_str());
      } else if (what == "gravity") {
        g_entity->handle_gravity();
        std::printf("run gravity || %s | v=%s\n", join(g_log).c_str(),
                    render(vec3_value(g_entity->velocity)).c_str());
      } else if (what == "gdecay") {
        const Value factor = parse_value(t, i);
        if (std::holds_alternative<std::monostate>(factor))
          g_entity->handle_ground_velocity_decay();
        else
          g_entity->handle_ground_velocity_decay(lfw::to_number(factor));
        std::printf("run gdecay %s || %s | v=%s pv=%s\n", render(factor).c_str(),
                    join(g_log).c_str(), render(vec3_value(g_entity->velocity)).c_str(),
                    render(vec3_value(g_entity->prev_velocity)).c_str());
      } else if (what == "vdecay") {
        const Value accx = parse_value(t, i);
        const Value accz = parse_value(t, i);
        const Value factor = parse_value(t, i);
        g_entity->handle_velocity_decay(
            accx,
            std::holds_alternative<std::monostate>(accz) ? std::nullopt
                                                         : std::optional<Value>(accz),
            std::holds_alternative<std::monostate>(factor) ? 1.0
                                                           : lfw::to_number(factor));
        std::printf("run vdecay %s %s %s || %s | v=%s pv=%s\n", render(accx).c_str(),
                    render(accz).c_str(), render(factor).c_str(), join(g_log).c_str(),
                    render(vec3_value(g_entity->velocity)).c_str(),
                    render(vec3_value(g_entity->prev_velocity)).c_str());
      } else if (what == "velocity") {
        const Value vinfo = parse_value(t, i);
        g_entity->update_velocity(vinfo);
        std::printf("run velocity %s || %s | v=%s pv=%s g=%s\n", render(vinfo).c_str(),
                    join(g_log).c_str(), render(vec3_value(g_entity->velocity)).c_str(),
                    render(vec3_value(g_entity->prev_velocity)).c_str(),
                    render(Value(g_entity->is_on_ground)).c_str());
      } else if (what == "land") {
        const bool self = t[i] == "self";
        const Value v = self ? g_entity->frame : parse_value(t, i);
        g_entity->set_landing_frame(v);
        std::printf("run land %s || %s | v=%s\n", self ? "self" : render(v).c_str(),
                    join(g_log).c_str(), render(g_entity->landing_frame()).c_str());
      } else if (what == "keys") {
        const double lr = trace::to_double(t[i++]);
        const double ud = trace::to_double(t[i++]);
        const double jd = trace::to_double(t[i++]);
        lfw::controller::BaseController* c = g_host->make_ctrl(0);
        if (lr > 0) c->keys.R.hit(Value(1.0), 0.0);
        else if (lr < 0) c->keys.L.hit(Value(1.0), 0.0);
        if (ud > 0) c->keys.D.hit(Value(1.0), 0.0);
        else if (ud < 0) c->keys.U.hit(Value(1.0), 0.0);
        if (jd > 0) c->keys.j.hit(Value(1.0), 0.0);
        else if (jd < 0) c->keys.d.hit(Value(1.0), 0.0);
        g_entity->set_ctrl(c);
        std::printf("run keys %s %s %s || %s | lr=%d ud=%d jd=%d\n",
                    render(Value(lr)).c_str(), render(Value(ud)).c_str(),
                    render(Value(jd)).c_str(), join(g_log).c_str(), c->LR(), c->UD(),
                    c->jd());
      } else if (what == "buddy") {
        g_buddy = std::make_unique<Entity>(*g_host, parse_value(t, i));
        std::printf("run buddy || id=%s | %s\n", s_of(g_buddy->id).c_str(),
                    join(g_log).c_str());
      } else if (what == "buddyset") {
        const std::string& name = t[i++];
        const Value in_v = parse_value(t, i);
        double n = 0;
        Value out_v;
        if (set_value(*g_buddy, name, in_v)) get_value(*g_buddy, name, out_v);
        else if (set_num(*g_buddy, name, lfw::to_number(in_v))) {
          get_num(*g_buddy, name, n);
          out_v = Value(n);
        } else {
          std::fprintf(stderr, "unknown buddyset '%s' at line %d\n", name.c_str(), lineno);
          return 2;
        }
        std::printf("run buddyset %s %s || %s | v=%s\n", name.c_str(), render(in_v).c_str(),
                    join(g_log).c_str(), render(out_v).c_str());
      } else if (what == "prev") {
        const Value v = parse_value(t, i);
        g_entity->set_prev_frame(v);
        std::printf("run prev %s || %s | v=%s\n", render(v).c_str(), join(g_log).c_str(),
                    render(g_entity->get_prev_frame()).c_str());
      } else if (what == "findframe") {
        const Value id = parse_value(t, i);
        std::printf("run findframe %s || %s | v=%s\n", render(id).c_str(),
                    join(g_log).c_str(),
                    render(g_entity->find_frame_by_id(id)).c_str());
      } else if (what == "autoframe") {
        std::printf("run autoframe || %s | v=%s\n", join(g_log).c_str(),
                    render(g_entity->find_auto_frame()).c_str());
      } else if (what == "align") {
        const Value fid = parse_value(t, i);
        const Value src = parse_value(t, i);
        const Value dst = parse_value(t, i);
        std::printf("run align %s %s %s || %s | v=%s\n", render(fid).c_str(),
                    render(src).c_str(), render(dst).c_str(), join(g_log).c_str(),
                    render(g_entity->find_align_frame(text_of(fid), src, dst)).c_str());
      } else if (what == "suddenframe") {
        std::printf("run suddenframe || %s | v=%s\n", join(g_log).c_str(),
                    render(g_entity->get_sudden_death_frame()).c_str());
      } else if (what == "caughtframe") {
        const Value v = g_entity->get_caught_end_frame();
        std::printf("run caughtframe || %s | v=%s p=%s\n", join(g_log).c_str(),
                    render(v).c_str(), render(vec3_value(g_entity->position)).c_str());
      } else if (what == "facingflag") {
        const Value f = parse_value(t, i);
        std::printf("run facingflag %s || %s | v=%s f=%s\n", render(f).c_str(),
                    join(g_log).c_str(),
                    render(Value(g_entity->handle_facing_flag(f))).c_str(),
                    render(Value(g_entity->facing)).c_str());
      } else if (what == "waitflag") {
        const Value w = parse_value(t, i);
        const Value f = parse_value(t, i);
        const std::optional<Value> frame =
            std::holds_alternative<std::monostate>(f) ? std::nullopt
                                                      : std::optional<Value>(f);
        std::printf("run waitflag %s %s || %s | v=%s w=%s\n", render(w).c_str(),
                    render(f).c_str(), join(g_log).c_str(),
                    render(Value(g_entity->handle_wait_flag(w, frame))).c_str(),
                    render(Value(g_entity->wait)).c_str());
      } else if (what == "framewait") {
        const Value f = parse_value(t, i);
        std::printf("run framewait %s || %s | v=%s\n", render(f).c_str(),
                    join(g_log).c_str(),
                    render(Value(g_entity->get_frame_wait(f))).c_str());
      } else if (what == "waitblock") {
        const double b = trace::to_double(t[i++]);
        g_entity->set_from_wait_block(b != 0);
        std::printf("run waitblock %s || %s | v=%s\n", render(Value(b)).c_str(),
                    join(g_log).c_str(),
                    render(Value(g_entity->from_wait_block() ? 1.0 : 0.0)).c_str());
      } else if (what == "summaries") {
        const std::vector<std::pair<std::u16string, std::shared_ptr<lfw::Summary>>>& items =
            lfw::summary_mgr().items();
        std::string s;
        for (const auto& kv : items) {
          s += " " + s_of(kv.first) + ":" + render(kv.second->hp_lost()) + "/" +
               render(kv.second->mp_usage());
        }
        std::printf("run summaries || %s | graves=%zu items%s\n", join(g_log).c_str(),
                    lfw::summary_mgr().grave_count(), s.c_str());
      } else if (what == "mark" || what == "delmark") {
        const Value key = parse_value(t, i);
        const std::optional<Value> value =
            what == "mark"
                ? std::optional<Value>(parse_value(t, i))
                : std::nullopt;
        const std::optional<Value> guard =
            i < t.size() ? std::optional<Value>(parse_value(t, i)) : std::nullopt;
        const bool ok = what == "mark"
                            ? g_entity->set_mark(text_of(key), text_of(*value), guard)
                            : g_entity->del_mark(text_of(key), guard);
        std::string marks;
        {
          std::vector<std::pair<std::string, std::string>> entries;
          for (const auto& kv : g_entity->marks) {
            entries.emplace_back(s_of(kv.first), s_of(kv.second));
          }
          // `std::map` is key-ordered, but the TS side has to sort a `Map` by hand;
          // sorting here too keeps the two dumps identical.
          std::sort(entries.begin(), entries.end());
          for (const auto& kv : entries) {
            if (!marks.empty()) marks += ",";
            marks += kv.first + ":" + kv.second;
          }
        }
        std::printf("run %s %s %s %s || %s | v=%s marks=%s\n", what.c_str(),
                    render(key).c_str(),
                    value.has_value() ? render(*value).c_str() : "-",
                    guard.has_value() ? render(*guard).c_str() : "-", join(g_log).c_str(),
                    render(Value(ok)).c_str(), marks.c_str());
      } else if (what == "ally") {
        const std::string& which = t[i++];
        const Entity* other = which == "buddy" ? g_buddy.get() : g_entity.get();
        std::printf("run ally %s || %s | v=%s team=%s other=%s\n", which.c_str(),
                    join(g_log).c_str(), render(Value(g_entity->is_ally(*other))).c_str(),
                    render(Value(g_entity->team())).c_str(),
                    other != nullptr ? render(Value(other->team())).c_str() : "-");
      } else if (what == "emit") {
        const std::string& idx_tok = t[i++];
        const double idx = trace::to_double(idx_tok);
        const Value id_value = parse_value(t, i);
        if (idx < 0 || std::floor(idx) != idx) {
          std::fprintf(stderr, "bad emitter index at line %d\n", lineno);
          return 2;
        }
        const std::size_t k = static_cast<std::size_t>(idx);
        if (g_entity->emitters.size() <= k) g_entity->emitters.resize(k + 1);
        g_entity->emitters[k] = text_of(id_value);
        std::printf("run emit %s %s || %s | n=%zu\n", idx_tok.c_str(),
                    render(id_value).c_str(), join(g_log).c_str(),
                    g_entity->emitters.size());
      } else if (what == "emitid") {
        const std::string& idx_tok = t[i++];
        const std::string& which = t[i++];
        const double idx = trace::to_double(idx_tok);
        const Entity* target = which == "buddy" ? g_buddy.get() : g_entity.get();
        const std::u16string eid = target != nullptr ? target->id : std::u16string();
        if (idx < 0 || std::floor(idx) != idx) {
          std::fprintf(stderr, "bad emitter index at line %d\n", lineno);
          return 2;
        }
        const std::size_t k = static_cast<std::size_t>(idx);
        if (g_entity->emitters.size() <= k) g_entity->emitters.resize(k + 1);
        g_entity->emitters[k] = eid;
        std::printf("run emitid %s %s || %s | n=%zu\n", idx_tok.c_str(), which.c_str(),
                    join(g_log).c_str(), g_entity->emitters.size());
      } else if (what == "getemitter") {
        const std::string& idx_tok = t[i++];
        Entity* e = g_entity->get_emitter(trace::to_double(idx_tok));
        std::printf("run getemitter %s || %s | v=%s\n", idx_tok.c_str(), join(g_log).c_str(),
                    e != nullptr ? render(id_ref(e)).c_str() : "u");
      } else if (what == "opointz") {
        const std::string& which = t[i++];
        const Entity* other = which == "buddy"   ? g_buddy.get()
                              : which == "null" ? nullptr
                                                : g_entity.get();
        const Value opoint = parse_value(t, i);
        std::printf("run opointz %s %s || %s | v=%s state=%s\n", which.c_str(),
                    render(opoint).c_str(), join(g_log).c_str(),
                    render(g_entity->get_opoint_speed_z(other, opoint)).c_str(),
                    render(g_entity->state()).c_str());
      } else if (what == "rec") {
        const std::string& which = t[i++];
        if (which == "stat") g_entity->stat_recovering();
        else if (which == "hp") g_entity->hp_recovering();
        else if (which == "mp") g_entity->mp_recovering();
        else if (which == "toughness") g_entity->toughness_recovering();
        else if (which == "fall") g_entity->fall_value_recovering();
        else if (which == "defend") g_entity->defend_value_recovering();
        else {
          std::fprintf(stderr, "unknown rec '%s' at line %d\n", which.c_str(), lineno);
          return 2;
        }
        std::printf(
            "run rec %s || %s | hp=%s hpr=%s mp=%s mpmax=%s r=%s t=%s tr=%s fv=%s dv=%s\n",
            which.c_str(), join(g_log).c_str(), render(Value(g_entity->hp())).c_str(),
            render(Value(g_entity->hp_r())).c_str(), render(Value(g_entity->mp())).c_str(),
            render(Value(g_entity->mp_max())).c_str(), render(Value(g_entity->resting())).c_str(),
            render(Value(g_entity->toughness())).c_str(),
            render(Value(g_entity->toughness_resting())).c_str(),
            render(Value(g_entity->fall_value())).c_str(),
            render(Value(g_entity->defend_value())).c_str());
      } else if (what == "snap" || what == "snapbuf" || what == "snapapply") {
        if (what == "snapapply") g_entity->read_snapshot(g_snap_nums, g_snap_strs);
        std::vector<Value> nums(static_cast<std::size_t>(lfw::entity::num_slots()));
        std::vector<std::u16string> strs(static_cast<std::size_t>(lfw::entity::str_slots()));
        g_entity->to_snapshot(nums, strs);
        if (what == "snapbuf") {
          g_snap_nums = nums;
          g_snap_strs = strs;
        }
        std::printf("run %s || %s | n=%s s=%s\n", what.c_str(), join(g_log).c_str(),
                    render_num_slots(nums).c_str(), render_str_slots(strs).c_str());
      } else if (what == "snappoke") {
        const std::string& name = t[i++];
        const Value v = parse_value(t, i);
        g_snap_nums[nslot_index(name)] = v;
        std::printf("run snappoke %s %s || %s | v=%s\n", name.c_str(),
                    render(g_snap_nums[nslot_index(name)]).c_str(), join(g_log).c_str(),
                    render(g_snap_nums[nslot_index(name)]).c_str());
      } else if (what == "snappokestr") {
        const std::string& name = t[i++];
        const Value v = parse_value(t, i);
        g_snap_strs[sslot_index(name)] = text_of(v);
        std::printf("run snappokestr %s %s || %s | v=%s\n", name.c_str(),
                    render(Value(g_snap_strs[sslot_index(name)])).c_str(), join(g_log).c_str(),
                    render(Value(g_snap_strs[sslot_index(name)])).c_str());
      } else if (what == "snappokeid") {
        const std::string& name = t[i++];
        const std::string& which = t[i++];
        Entity* target = which == "buddy" ? g_buddy.get() : g_entity.get();
        const std::u16string poke_id = target != nullptr ? target->id : std::u16string();
        g_snap_strs[sslot_index(name)] = poke_id;
        std::printf("run snappokeid %s %s || %s | v=%s\n", name.c_str(), which.c_str(),
                    join(g_log).c_str(), render(Value(poke_id)).c_str());
      } else if (what == "copy") {
        const Value v = parse_value(t, i);
        const bool added = g_entity->add_copy(text_of(v));
        std::printf("run copy %s || %s | added=%s\n", render(v).c_str(), join(g_log).c_str(),
                    render(Value(added)).c_str());
      } else if (what == "hook") {
        const std::string& sub = t[i++];
        if (sub == "dead") {
          g_entity->state_on_dead = [] { g_log.push_back("state_on_dead"); };
        } else if (sub == "gravity") {
          const Value v = parse_value(t, i);
          g_entity->state_get_gravity = [v]() { return v; };
        } else if (sub == "frameid") {
          if (t[i] == "echo") {
            ++i;
            g_entity->state_find_frame_by_id = [](const Value& v) { return v; };
          } else {
            const Value v = parse_value(t, i);
            g_entity->state_find_frame_by_id = [v](const Value&) { return v; };
          }
        } else if (sub == "autoframe") {
          const Value v = parse_value(t, i);
          g_entity->state_get_auto_frame = [v]() { return v; };
        } else if (sub == "sudden") {
          const Value v = parse_value(t, i);
          g_entity->state_get_sudden_death_frame = [v]() { return v; };
        } else if (sub == "caught") {
          const Value v = parse_value(t, i);
          g_entity->state_get_caught_end_frame = [v]() { return v; };
        } else if (sub == "none") {
          g_entity->state_on_dead = nullptr;
          g_entity->state_get_gravity = nullptr;
          g_entity->state_find_frame_by_id = nullptr;
          g_entity->state_get_auto_frame = nullptr;
          g_entity->state_get_sudden_death_frame = nullptr;
          g_entity->state_get_caught_end_frame = nullptr;
        } else {
          std::fprintf(stderr, "unknown hook '%s' at line %d\n", sub.c_str(), lineno);
          return 2;
        }
        std::printf("run hook %s || %s\n", sub.c_str(), join(g_log).c_str());
      } else {
        std::fprintf(stderr, "unknown run '%s' at line %d\n", what.c_str(), lineno);
        return 2;
      }
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
