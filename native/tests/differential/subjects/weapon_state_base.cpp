#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/state/weapon_state_base.h"
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
  Value _indexes_flag;
  Value _ionground;
  Value _ithrow;
  Value _isky;
  Value _frames;
  Value _onlanding;
  Value _base;
  Value _wt;
  bool _on_ground = false;
  Value _dh;
  Value _state;
  Value _fid;
  double _hp = 0;
  double _hpr = 0;
  Value _align;

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
  Value velocity_x() const override { return Value(); }
  Value velocity_z() const override { return Value(); }
  Value data_frames() const override { return _frames; }
  bool has_data_indexes() const override { return lfw::truthy(_indexes_flag); }
  Value data_indexes_on_ground() const override { return _ionground; }
  Value data_indexes_throwings() const override { return _ithrow; }
  Value data_indexes_in_the_skys() const override { return _isky; }
  Value frame_on_landing() const override { return _onlanding; }
  bool is_on_ground() const override { return _on_ground; }
  void enter_frame(const Value& frame) override {
    g_log.push_back(s_of(_id) + ":enter_frame:" + render(frame));
  }
  Value data_base() const override { return _base; }
  Value base_type() const override { return _wt; }
  Value drop_hurted() const override { return _dh; }
  void set_drop_hurted(const Value& v) override { _dh = v; }
  Value hp() const override { return Value(_hp); }
  void set_hp(const Value& v) override { _hp = lfw::to_number(v); }
  Value hp_r() const override { return Value(_hpr); }
  void set_hp_r(const Value& v) override { _hpr = lfw::to_number(v); }
  void set_velocity(const Value& x, const Value& y, const Value& z) override {
    g_log.push_back(s_of(_id) + ":set_velocity:" + render(x) + ":" + render(y) + ":" +
                    render(z));
  }
  void leave_ground() override { g_log.push_back(s_of(_id) + ":leave_ground"); }
  Value state() const override { return _state; }
  Value frame_id() const override { return _fid; }
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
Value g_nf;
WeaponState_Base* g_obj = nullptr;

std::string state_text() {
  const FakeEnt& e = *g_ent;
  return "hp=" + num(e._hp) + " hpr=" + num(e._hpr) + " dh=" + render(e._dh);
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

std::u16string value_text(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_weapon_state_base <case-file>\n");
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
      if (sub == "state") {
        g_state = parse_value(t, i);
      } else if (sub == "indexes") {
        g_ent->_indexes_flag = parse_value(t, i);
      } else if (sub == "ionground") {
        g_ent->_ionground = parse_value(t, i);
      } else if (sub == "ithrow") {
        g_ent->_ithrow = parse_value(t, i);
      } else if (sub == "isky") {
        g_ent->_isky = parse_value(t, i);
      } else if (sub == "frames") {
        g_ent->_frames = parse_value(t, i);
      } else if (sub == "onlanding") {
        g_ent->_onlanding = parse_value(t, i);
      } else if (sub == "base") {
        g_ent->_base = parse_value(t, i);
      } else if (sub == "wt") {
        g_ent->_wt = parse_value(t, i);
      } else if (sub == "onground") {
        g_ent->_on_ground = lfw::truthy(parse_value(t, i));
      } else if (sub == "dh") {
        g_ent->_dh = parse_value(t, i);
      } else if (sub == "vstate") {
        g_ent->_state = parse_value(t, i);
      } else if (sub == "fid") {
        g_ent->_fid = parse_value(t, i);
      } else if (sub == "hp") {
        g_ent->_hp = trace::to_double(t[i++]);
      } else if (sub == "hpr") {
        g_ent->_hpr = trace::to_double(t[i++]);
      } else if (sub == "vel") {
        g_velocity = parse_value(t, i);
      } else if (sub == "nf") {
        g_nf = parse_value(t, i);
      } else if (sub == "align") {
        g_ent->_align = parse_value(t, i);
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
        g_obj = new WeaponState_Base(g_state);
        std::printf("run make || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "auto") {
        const Value f = g_obj->get_auto_frame ? g_obj->get_auto_frame(*g_ent) : Value();
        std::printf("run auto || %s | fid=%s | %s\n", join(g_log).c_str(), render(f).c_str(),
                    state_text().c_str());
      } else if (what == "landing") {
        if (g_obj->on_landing) g_obj->on_landing(*g_ent, g_velocity);
        std::printf("run landing || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "update") {
        g_obj->update(*g_ent);
        std::printf("run update || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "leaveground") {
        if (g_obj->on_leave_ground) g_obj->on_leave_ground(*g_ent);
        std::printf("run leaveground || %s | %s\n", join(g_log).c_str(), state_text().c_str());
      } else if (what == "rebound") {
        g_obj->hit_ground_rebouncing(*g_ent, g_nf, g_velocity);
        std::printf("run rebound || %s | %s\n", join(g_log).c_str(), state_text().c_str());
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
