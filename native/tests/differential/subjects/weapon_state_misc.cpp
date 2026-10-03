#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/state/weapon_state_misc.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using lfw::Array;
using lfw::Object;
using lfw::Value;
using lfw::buff::Buff;
using lfw::buff::BuffEnv;
using lfw::state::IStateEntity;
using lfw::state::WeaponState_Base;
using lfw::state::WeaponState_InTheSky;
using lfw::state::WeaponState_OnGround;
using lfw::state::WeaponState_OnHand;
using lfw::state::WeaponState_Throwing;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::vector<std::string> g_log;

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
  Value _team;
  Value _new_team;
  Value _motionless;
  bool _has_bearer = false;
  Value _bmotion;
  Value _dh;
  Value _dropping;
  double _hp = 0;
  double _hpr = 0;
  Value _base;
  Value _wt;
  Value _behavior;
  Value _fid;
  Value _onlanding;
  Value _vstate;
  Value _velx;
  Value _vely;
  Value _velz;
  Value _align;
  Value _indexes_flag;
  Value _throw_on_ground;
  Value _just_on_ground;
  Value _isky;
  Value _ithrow;
  Value _gval;

  explicit FakeEnt(std::u16string id) : _id(std::move(id)) {}

  const std::u16string& id() const override { return _id; }
  void position(double& x, double& y, double& z) const override {
    x = 0;
    y = 0;
    z = 0;
  }
  void set_position(double x, double y, double z) override {
    (void)x;
    (void)y;
    (void)z;
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
  Value data() const override {
    Object o;
    Object indexes;
    if (lfw::truthy(_indexes_flag)) {
      indexes.set(u"throw_on_ground", _throw_on_ground);
      indexes.set(u"just_on_ground", _just_on_ground);
      indexes.set(u"in_the_skys", _isky);
      indexes.set(u"throwings", _ithrow);
      o.set(u"indexes", Value(std::make_shared<Object>(indexes)));
    }
    o.set(u"base", _base);
    return Value(std::make_shared<Object>(o));
  }
  Value state() const override { return _vstate; }
  Value dataset(const std::u16string& key) const override {
    g_log.push_back(s_of(_id) + ":dataset:" + s_of(key));
    if (key == std::u16string(u"weapon_throwing_gravity")) return _gval;
    return Value();
  }
  Value hp() const override { return Value(_hp); }
  void set_hp(const Value& v) override { _hp = lfw::to_number(v); }
  Value hp_r() const override { return Value(_hpr); }
  void set_hp_r(const Value& v) override { _hpr = lfw::to_number(v); }
  Value team() const override { return _team; }
  void set_team(const Value& v) override { _team = v; }
  Value lfw_new_team() const override { return _new_team; }
  double velocity_y() const override { return lfw::to_number(_vely); }
  Value velocity_x() const override { return _velx; }
  Value velocity_z() const override { return _velz; }
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" +
                    render(z));
  }
  Value motionless() const override { return _motionless; }
  bool has_bearer() const override { return _has_bearer; }
  Value bearer_motionless() const override { return _bmotion; }
  void set_bearer_motionless(const Value& v) override { _bmotion = v; }
  Value drop_hurted() const override { return _dh; }
  void set_drop_hurted(const Value& v) override { _dh = v; }
  void set_dropping(bool v) override { _dropping = Value(v); }
  void leave_ground() override { g_log.push_back(s_of(_id) + ":leave_ground"); }
  Value frame_on_landing() const override { return _onlanding; }
  Value frame_behavior() const override { return _behavior; }
  Value frame_id() const override { return _fid; }
  Value data_base() const override { return _base; }
  Value base_type() const override { return _wt; }
  Value data_indexes_throw_on_ground() const override {
    return lfw::truthy(_indexes_flag) ? _throw_on_ground : Value();
  }
  Value data_indexes_just_on_ground() const override {
    return lfw::truthy(_indexes_flag) ? _just_on_ground : Value();
  }
  Value data_indexes_in_the_skys() const override {
    return lfw::truthy(_indexes_flag) ? _isky : Value();
  }
  Value data_indexes_throwings() const override {
    return lfw::truthy(_indexes_flag) ? _ithrow : Value();
  }
  void enter_frame(const Value& frame) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(frame));
  }
  Value find_align_frame(const Value& fid, const Value& throwings,
                         const Value& in_the_skys) override {
    g_log.push_back(s_of(_id) + ":find_align_frame:" + render(fid) + ":" + render(throwings) +
                    ":" + render(in_the_skys));
    return _align;
  }
  void handle_ground_velocity_decay() override {
    g_log.push_back(s_of(_id) + ":handle_ground_velocity_decay");
  }
};

FakeEnt* g_ent = nullptr;
Value g_state;
Value g_velocity;
WeaponState_Base* g_obj = nullptr;
std::u16string g_cls = u"onground";

std::u16string value_text(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

std::string state_text() {
  const FakeEnt& e = *g_ent;
  return "team=" + render(e._team) + " dh=" + render(e._dh) + " hp=" + num(e._hp) +
         " hpr=" + num(e._hpr) + " motionless=" + render(e._motionless) +
         " bmotion=" + render(e._bmotion) + " dropping=" + render(e._dropping);
}

void bind() {
  static BuffEnv keep;
  keep.find_entity = [](const std::u16string&) -> lfw::buff::IBuffEntity* { return nullptr; };
  keep.create_entity = [](const Value&) -> lfw::buff::IBuffEntity* { return nullptr; };
  keep.find_data = [](const std::u16string&) -> Value { return Value(); };
  keep.world_buffs_set = [](const std::u16string&, Buff*) {};
  keep.world_buffs_get = [](const std::u16string&) -> Buff* { return nullptr; };
  keep.create_buff = [](const std::u16string&, const std::u16string&) -> Buff* { return nullptr; };
  lfw::state::StateEnv senv;
  senv.buff_env = &keep;
  lfw::state::set_state_env(senv);
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_weapon_state_misc <case-file>\n");
    return 2;
  }
  bind();
  g_ent = new FakeEnt(u"W1");
  g_state = Value(static_cast<double>(0));

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
      } else if (sub == "newteam") {
        g_ent->_new_team = parse_value(t, i);
      } else if (sub == "team") {
        g_ent->_team = parse_value(t, i);
      } else if (sub == "motionless") {
        g_ent->_motionless = parse_value(t, i);
      } else if (sub == "bearer") {
        g_ent->_has_bearer = lfw::truthy(parse_value(t, i));
      } else if (sub == "bmotion") {
        g_ent->_bmotion = parse_value(t, i);
      } else if (sub == "dh") {
        g_ent->_dh = parse_value(t, i);
      } else if (sub == "dropping") {
        g_ent->_dropping = Value(lfw::truthy(parse_value(t, i)));
      } else if (sub == "hp") {
        g_ent->_hp = trace::to_double(t[i++]);
      } else if (sub == "hpr") {
        g_ent->_hpr = trace::to_double(t[i++]);
      } else if (sub == "base") {
        g_ent->_base = parse_value(t, i);
      } else if (sub == "wt") {
        g_ent->_wt = parse_value(t, i);
      } else if (sub == "behavior") {
        g_ent->_behavior = parse_value(t, i);
      } else if (sub == "fid") {
        g_ent->_fid = parse_value(t, i);
      } else if (sub == "onlanding") {
        g_ent->_onlanding = parse_value(t, i);
      } else if (sub == "vstate") {
        g_ent->_vstate = parse_value(t, i);
      } else if (sub == "vx") {
        g_ent->_velx = parse_value(t, i);
      } else if (sub == "vy") {
        g_ent->_vely = parse_value(t, i);
      } else if (sub == "vz") {
        g_ent->_velz = parse_value(t, i);
      } else if (sub == "align") {
        g_ent->_align = parse_value(t, i);
      } else if (sub == "indexes") {
        g_ent->_indexes_flag = parse_value(t, i);
      } else if (sub == "throwg") {
        g_ent->_throw_on_ground = parse_value(t, i);
      } else if (sub == "justg") {
        g_ent->_just_on_ground = parse_value(t, i);
      } else if (sub == "isky") {
        g_ent->_isky = parse_value(t, i);
      } else if (sub == "ithrow") {
        g_ent->_ithrow = parse_value(t, i);
      } else if (sub == "gval") {
        g_ent->_gval = parse_value(t, i);
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
        if (g_cls == u"onhand") {
          g_obj = new WeaponState_OnHand(g_state);
        } else if (g_cls == u"throwing") {
          g_obj = new WeaponState_Throwing(g_state);
        } else if (g_cls == u"inthesky") {
          g_obj = new WeaponState_InTheSky(g_state);
        } else {
          g_obj = new WeaponState_OnGround(g_state);
        }
        std::printf("run make || %s | s=%s | %s\n", join(g_log).c_str(),
                    render(g_obj->state()).c_str(), state_text().c_str());
      } else if (what == "enter") {
        if (g_obj->enter) g_obj->enter(*g_ent, Value());
        std::printf("run enter || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "update") {
        g_obj->update(*g_ent);
        std::printf("run update || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "landing") {
        if (g_obj->on_landing) g_obj->on_landing(*g_ent, g_velocity);
        std::printf("run landing || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "preupdate") {
        if (g_obj->pre_update) g_obj->pre_update(*g_ent);
        std::printf("run preupdate || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "gravity") {
        const Value r = g_obj->get_gravity ? g_obj->get_gravity(*g_ent) : Value();
        std::printf("run gravity || %s | r=%s | %s\n", join(g_log).c_str(), render(r).c_str(),
                    state_text().c_str());
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
