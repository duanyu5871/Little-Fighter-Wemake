#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/cases.h"
#include "lfw/core/value.h"
#include "lfw/entity/drink_info.h"
#include "lfw/state/character_state_drink.h"
#include "lfw/state/state_burning.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/mersenne_twister.h"

#include "trace_util.h"

namespace {

using lfw::Array;
using lfw::Object;
using lfw::Value;
using lfw::DrinkInfo;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::state::CharacterState_Drink;
using lfw::state::IStateEntity;
using lfw::state::State_Base;
using lfw::state::State_Burning;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;
lfw::MersenneTwister g_mt(0.0);

std::string render(const Value& v) { return to_ascii(render_value(v)); }
std::string s_of(const std::u16string& s) { return to_ascii(s); }
std::string num(double d) { return render(Value(d)); }

std::string join(const std::vector<std::string>& xs) {
  std::string out;
  for (size_t i = 0; i < xs.size(); ++i) {
    if (i) out += ",";
    out += xs[i];
  }
  return out;
}

struct FakeEnt : IStateEntity {
  std::u16string _id;
  Value _data;
  Value _indexes;
  Value _frames;
  Value _onlanding;
  Value _wdata;
  Value _state;
  Value _hbtype;
  bool _catcher = false;
  bool _on_ground = false;
  double _hp = 50;
  double _hpr = 0;
  Value _hp_max = Value(50.0);
  Value _mp = Value(0.0);
  Value _mp_max = Value(0.0);
  double _px = 0;
  double _py = 0;
  double _pz = 0;
  Value _vx;
  Value _vy;
  Value _vz;
  Value _facing;
  Value _bounced;
  bool _has_holding = false;
  Value _hold_hp;
  Value _hold_hp_r;
  std::shared_ptr<DrinkInfo> _drink;
  Value _mtrange;

  explicit FakeEnt(std::u16string id) : _id(std::move(id)) {}

  const std::u16string& id() const override { return _id; }
  Value data() const override { return _data; }
  Value state() const override { return _state; }
  void position(double& x, double& y, double& z) const override {
    x = _px;
    y = _py;
    z = _pz;
  }
  void set_position(double x, double y, double z) override {
    _px = x;
    _py = y;
    _pz = z;
  }
  double frame_centery() const override { return 0; }
  double frame_height() const override { return 0; }
  double frame_pic_h() const override { return 0; }
  void set_frame(const Value& info) override {
    g_log.push_back(s_of(_id) + ":set_frame:" + render(lfw::field_or(info, u"id")));
  }
  void buffs_set(const std::u16string& key, Buff*) override {
    g_log.push_back(s_of(_id) + ":buffs_set:" + s_of(key));
  }
  void buffs_delete(const std::u16string& key) override {
    g_log.push_back(s_of(_id) + ":buffs_delete:" + s_of(key));
  }
  void set_outline_alpha(double v) override {
    g_log.push_back(s_of(_id) + ":outline_alpha:" + num(v));
  }
  void set_outline_width(double v) override {
    g_log.push_back(s_of(_id) + ":outline_width:" + num(v));
  }
  void set_outline_color(const std::u16string& v) override {
    g_log.push_back(s_of(_id) + ":outline_color:" + render(Value(v)));
  }
  void enter_frame_by_id(const std::u16string& id) override {
    g_log.push_back(s_of(_id) + ":enter_frame_by_id:" + render(Value(id)));
  }
  void attach(bool on) override {
    g_log.push_back(s_of(_id) + ":attach:" + std::string(on ? "1" : "0"));
  }
  Value velocity_x() const override { return _vx; }
  double velocity_y() const override { return lfw::to_number(_vy); }
  Value velocity_z() const override { return _vz; }
  Value hp() const override { return Value(_hp); }
  void set_hp(const Value& v) override { _hp = lfw::to_number(v); }
  Value hp_r() const override { return Value(_hpr); }
  void set_hp_r(const Value& v) override { _hpr = lfw::to_number(v); }
  Value hp_max() const override { return _hp_max; }
  Value mp() const override { return _mp; }
  void set_mp(const Value& v) override { _mp = v; }
  Value mp_max() const override { return _mp_max; }
  Value facing() const override { return _facing; }
  void set_facing(const Value& v) override { _facing = v; }
  Value bounced() const override { return _bounced; }
  void set_bounced(const Value& v) override { _bounced = v; }
  Value data_frames() const override { return _frames; }
  Value frame_on_landing() const override { return _onlanding; }
  bool is_on_ground() const override { return _on_ground; }
  bool has_catcher() const override { return _catcher; }
  void catcher_drop_catching() override {
    g_log.push_back(s_of(_id) + ":catcher_drop_catching");
  }
  void drop_holding() override { g_log.push_back(s_of(_id) + ":drop_holding"); }
  bool has_holding() const override { return _has_holding; }
  Value holding_base_type() const override { return _hbtype; }
  DrinkInfo* holding_drink() const override { return _drink.get(); }
  void holding_set_hp(const Value& v) override { _hold_hp = v; }
  void holding_set_hp_r(const Value& v) override { _hold_hp_r = v; }
  void holding_set_velocity(const Value& x, const Value& y, const Value& z) override {
    g_log.push_back(s_of(_id) + ":holding_set_velocity:" + render(x) + ":" + render(y) + ":" +
                    render(z));
  }
  Value holding_mt_range(double lo, double hi) override {
    g_log.push_back(s_of(_id) + ":holding_mt_range:" + num(lo) + ":" + num(hi));
    return _mtrange;
  }
  void holding_mt_mark(const std::u16string& mark) override {
    g_mt.mark = mark;
    g_log.push_back(s_of(_id) + ":holding_mt_mark:" + render(Value(mark)));
  }
  Value world_dataset(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":world_dataset:" + s_of(key));
    return lfw::field_or(_wdata, key.c_str());
  }
  bool has_data_indexes() const override { return lfw::truthy(_indexes); }
  Value data_indexes_on_ground() const override {
    return lfw::field_or(_indexes, u"on_ground");
  }
  Value data_indexes_throwings() const override {
    return lfw::field_or(_indexes, u"throwings");
  }
  Value data_indexes_in_the_skys() const override {
    return lfw::field_or(_indexes, u"in_the_skys");
  }
  Value data_indexes_default() const override {
    return lfw::field_or(_indexes, u"default");
  }
  Value data_indexes_heavy_obj_walk() const override {
    return lfw::field_or(_indexes, u"heavy_obj_walk");
  }
  Value data_indexes_landing_1() const override {
    return lfw::field_or(_indexes, u"landing_1");
  }
  Value data_indexes_landing_2() const override {
    return lfw::field_or(_indexes, u"landing_2");
  }
  Value data_indexes_bouncing() const override {
    return lfw::field_or(_indexes, u"bouncing");
  }
  Value data_indexes_lying() const override {
    return lfw::field_or(_indexes, u"lying");
  }
  Value data_indexes_falling() const override {
    return lfw::field_or(_indexes, u"falling");
  }
  Value dataset(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":dataset:" + s_of(key));
    return Value();
  }
  void enter_frame(const Value& frame) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(frame));
  }
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" +
                    render(z));
  }
  void handle_ground_velocity_decay() override {
    g_log.push_back(s_of(_id) + ":handle_ground_velocity_decay");
  }
  void set_shaking(const Value& v) override {
    g_log.push_back(s_of(_id) + ":set_shaking:" + render(v));
  }
  void set_motionless(const Value& v) override {
    g_log.push_back(s_of(_id) + ":set_motionless:" + render(v));
  }
  void play_sound(const Value& sounds) override {
    g_log.push_back(s_of(_id) + ":play_sound:" + render(sounds));
  }
  void apply_opoints(const std::vector<Value>& opoints) override {
    g_log.push_back(s_of(_id) + ":apply_opoints:" + std::to_string(opoints.size()));
  }
};

FakeEnt* g_ent = nullptr;
Value g_state;
Value g_velocity;
State_Base* g_obj = nullptr;
std::u16string g_cls = u"burning";

std::string state_text() {
  const FakeEnt& e = *g_ent;
  const Value drink = e._drink != nullptr ? e._drink->to_snapshot() : Value();
  return "hp=" + num(e._hp) + " hpr=" + num(e._hpr) + " hpmax=" + render(e._hp_max) +
         " mp=" + render(e._mp) + " mpmax=" + render(e._mp_max) + " state=" +
         render(g_obj != nullptr ? g_obj->state() : Value()) + " bounced=" +
         render(e._bounced) + " facing=" + render(e._facing) + " holding=" +
         (e._has_holding ? "1" : "0") + " hhp=" + render(e._hold_hp) +
         " hhpr=" + render(e._hold_hp_r) + " drink=" + render(drink) + " pos=[" + num(e._px) +
         ":" + num(e._py) + ":" + num(e._pz) + "]";
}

void bind() {
  static BuffEnv keep;
  keep.find_entity = [](const std::u16string&) -> lfw::buff::IBuffEntity* { return nullptr; };
  keep.create_entity = [](const Value&) -> lfw::buff::IBuffEntity* { return nullptr; };
  keep.find_data = [](const std::u16string&) -> Value { return Value(); };
  keep.world_buffs_set = [](const std::u16string&, Buff*) {};
  keep.world_buffs_get = [](const std::u16string&) -> Buff* { return nullptr; };
  keep.create_buff = [](const std::u16string&, const std::u16string&) -> Buff* {
    return nullptr;
  };
  lfw::state::StateEnv senv;
  senv.buff_env = &keep;
  lfw::state::set_state_env(senv);
}

std::u16string value_text(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_burning_drink <case-file>\n");
    return 2;
  }
  bind();
  g_ent = new FakeEnt(u"E1");

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
    g_log.clear();
    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "cls") {
        g_cls = value_text(parse_value(t, i));
      } else if (sub == "state") {
        g_state = parse_value(t, i);
      } else if (sub == "data") {
        g_ent->_data = parse_value(t, i);
      } else if (sub == "indexes") {
        g_ent->_indexes = parse_value(t, i);
      } else if (sub == "frames") {
        g_ent->_frames = parse_value(t, i);
      } else if (sub == "onlanding") {
        g_ent->_onlanding = parse_value(t, i);
      } else if (sub == "wdata") {
        g_ent->_wdata = parse_value(t, i);
      } else if (sub == "vstate") {
        g_ent->_state = parse_value(t, i);
      } else if (sub == "hbtype") {
        g_ent->_hbtype = parse_value(t, i);
      } else if (sub == "catcher") {
        g_ent->_catcher = lfw::truthy(parse_value(t, i));
      } else if (sub == "onground") {
        g_ent->_on_ground = lfw::truthy(parse_value(t, i));
      } else if (sub == "hp") {
        g_ent->_hp = trace::to_double(t[i++]);
      } else if (sub == "hpr") {
        g_ent->_hpr = trace::to_double(t[i++]);
      } else if (sub == "hpmax") {
        g_ent->_hp_max = parse_value(t, i);
      } else if (sub == "mp") {
        g_ent->_mp = parse_value(t, i);
      } else if (sub == "mpmax") {
        g_ent->_mp_max = parse_value(t, i);
      } else if (sub == "pos") {
        const Value v = parse_value(t, i);
        g_ent->_px = lfw::to_number(lfw::field_or(v, u"x"));
        g_ent->_py = lfw::to_number(lfw::field_or(v, u"y"));
        g_ent->_pz = lfw::to_number(lfw::field_or(v, u"z"));
      } else if (sub == "vx") {
        g_ent->_vx = parse_value(t, i);
      } else if (sub == "vy") {
        g_ent->_vy = parse_value(t, i);
      } else if (sub == "vz") {
        g_ent->_vz = parse_value(t, i);
      } else if (sub == "facing") {
        g_ent->_facing = parse_value(t, i);
      } else if (sub == "bounced") {
        g_ent->_bounced = parse_value(t, i);
      } else if (sub == "holding") {
        g_ent->_has_holding = lfw::truthy(parse_value(t, i));
      } else if (sub == "holdhp") {
        g_ent->_hold_hp = parse_value(t, i);
      } else if (sub == "holdhpr") {
        g_ent->_hold_hp_r = parse_value(t, i);
      } else if (sub == "drink") {
        const Value v = parse_value(t, i);
        if (std::holds_alternative<std::monostate>(v)) {
          g_ent->_drink = nullptr;
        } else {
          g_ent->_drink = std::make_shared<DrinkInfo>(v);
        }
      } else if (sub == "mtrange") {
        g_ent->_mtrange = parse_value(t, i);
      } else if (sub == "vel") {
        g_velocity = parse_value(t, i);
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
        delete g_obj;
        if (g_cls == u"drink") {
          g_obj = new CharacterState_Drink(g_state);
        } else {
          g_obj = new State_Burning(g_state);
        }
        std::printf("run make || %s | s=%s | %s\n", join(g_log).c_str(),
                    render(g_obj->state()).c_str(), state_text().c_str());
      } else if (what == "default") {
        delete g_obj;
        if (g_cls == u"drink") {
          g_obj = new CharacterState_Drink();
        } else {
          g_obj = new State_Burning();
        }
        std::printf("run default || %s | s=%s | %s\n", join(g_log).c_str(),
                    render(g_obj->state()).c_str(), state_text().c_str());
      } else if (what == "enter") {
        if (g_obj->enter) g_obj->enter(*g_ent, Value());
        std::printf("run enter || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "update") {
        g_obj->update(*g_ent);
        std::printf("run update || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "leave") {
        g_obj->leave(*g_ent, Value());
        std::printf("run leave || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "landing") {
        if (g_obj->on_landing) g_obj->on_landing(*g_ent, g_velocity);
        std::printf("run landing || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "mtmark") {
        std::printf("run mtmark || %s | mark=%s\n", join(g_log).c_str(),
                    render(Value(g_mt.mark)).c_str());
      } else {
        std::fprintf(stderr, "unknown run '%s' at line %d\n", what.c_str(), lineno);
        return 2;
      }
      continue;
    }
    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  delete g_obj;
  return 0;
}
