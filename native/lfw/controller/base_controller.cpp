#include "lfw/controller/base_controller.h"

#include <memory>
#include <variant>

#include "lfw/defines/game_key.h"
#include "lfw/defines/state_enum.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace controller {
namespace {

Value seq_etc(const char16_t* etc) {
  Object o;
  o.set(u"etc", Value(std::u16string(etc)));
  return Value(std::make_shared<Object>(o));
}

Value coalesce(const Value& a, const Value& b) {
  if (std::holds_alternative<std::monostate>(a) || std::holds_alternative<NullTag>(a)) {
    return b;
  }
  return a;
}

Value obj_get(const Value& v, const std::u16string& key) {
  const Object* o = as_object(v);
  if (o == nullptr) return Value();
  const Value* p = o->get(key);
  return p == nullptr ? Value() : *p;
}

void use_each(ControllerKeyStatus& keys, const std::u16string& seq) {
  for (const char16_t c : seq) {
    KeyStatus* p = keys.slot(std::u16string(1, c));
    if (p != nullptr) p->use();
  }
}

}

BaseController::BaseController()
    : _seq_djdj(u"djdj", seq_etc(u"0")),
      _seq_dddd(u"dddd", seq_etc(u"2")),
      _seq_dada(u"dada", seq_etc(u"4")),
      _seq_djjj(u"djjj", seq_etc(u"8")) {}

int BaseController::LR() const {
  const bool has_l = !keys.L.is_end() || keys.L.is_start(time());
  const bool has_r = !keys.R.is_end() || keys.R.is_start(time());
  return has_l == has_r ? 0 : (has_r ? 1 : -1);
}

int BaseController::UD() const {
  const bool has_u = !keys.U.is_end() || keys.U.is_start(time());
  const bool has_d = !keys.D.is_end() || keys.D.is_start(time());
  return has_u == has_d ? 0 : (has_d ? 1 : -1);
}

int BaseController::jd() const {
  const bool has_d = !keys.d.is_end() || keys.d.is_start(time());
  const bool has_j = !keys.j.is_end() || keys.j.is_start(time());
  return has_d == has_j ? 0 : (has_d ? -1 : 1);
}

void BaseController::reset_key_list() {
  _key_list.clear();
  _readable_key_list.clear();
}

void BaseController::reset() {
  _time.reset();
  keys.reset();
  dbc.reset();
  result.clear();
  queue.clear();
  reset_key_list();
  std::vector<SeqKeys*> all = {&_seq_djdj, &_seq_dddd, &_seq_dada, &_seq_djjj};
  for (SeqKeys* v : all) v->reset();
}

BaseController& BaseController::start(const std::vector<std::u16string>& ks) {
  for (const std::u16string& k : ks) queue.emplace_back(Status::DOWN, k);
  return *this;
}

BaseController& BaseController::hold(const std::vector<std::u16string>& ks) {
  for (const std::u16string& k : ks) queue.emplace_back(Status::HOLD, k);
  return *this;
}

BaseController& BaseController::end(const std::vector<std::u16string>& ks) {
  for (const std::u16string& k : ks) queue.emplace_back(Status::UP, k);
  return *this;
}

BaseController& BaseController::db_hit(const std::vector<std::u16string>& ks) {
  start(ks);
  end(ks);
  start(ks);
  return *this;
}

BaseController& BaseController::click(const std::vector<std::u16string>& ks) {
  for (const std::u16string& k : ks) {
    start({k});
    end({k});
  }
  return *this;
}

BaseController& BaseController::dbl_click(const std::vector<std::u16string>& ks) {
  for (const std::u16string& k : ks) {
    start({k});
    end({k});
    start({k});
    end({k});
  }
  return *this;
}

BaseController& BaseController::key_down(const std::vector<std::u16string>& ks) {
  for (const std::u16string& k : ks) {
    if (is_end(k)) start({k});
  }
  return *this;
}

BaseController& BaseController::key_up(const std::vector<std::u16string>& ks) {
  for (const std::u16string& k : ks) {
    if (!is_end(k)) end({k});
  }
  return *this;
}

bool BaseController::is_hold(const std::u16string& k) const {
  const KeyStatus* p = slot(k);
  return p != nullptr && p->is_hld(time(), _env->key_hit_duration);
}

bool BaseController::is_hit(const std::u16string& k) const {
  const KeyStatus* p = slot(k);
  return p != nullptr && p->is_hit(time(), _env->key_hit_duration);
}

bool BaseController::is_end(const std::u16string& k) const {
  const KeyStatus* p = slot(k);
  return p != nullptr && p->is_end();
}

bool BaseController::is_start(const std::u16string& k) const {
  const KeyStatus* p = slot(k);
  return p != nullptr && p->is_start(time());
}

bool BaseController::is_db_hit(const std::u16string& k) {
  DoubleClick* d = dbc.slot(k);
  const double down_time = d->time();
  const Value d0 = d->data(0);
  const Value d1 = d->data(1);
  const bool ret = down_time > 0 && time() - down_time <= _env->key_hit_duration;
  if (!ret) return false;
  if (!truthy(d0) || !truthy(d1)) return true;
  const double st = static_cast<double>(StateEnum::Standing);
  const double wk = static_cast<double>(StateEnum::Walking);
  const double f0 = to_number(field_or(d0, u"fstate"));
  if ((f0 == st || f0 == wk) && (f0 == st || f0 == wk)) return true;
  if (k == u"L" && (to_number(field_or(d0, u"facing")) != -1 ||
                    to_number(field_or(d1, u"facing")) != -1)) {
    d->step();
    return false;
  }
  if (k == u"R" && (to_number(field_or(d0, u"facing")) != 1 ||
                    to_number(field_or(d1, u"facing")) != 1)) {
    d->step();
    return false;
  }
  return true;
}

bool BaseController::tst(const std::u16string& type, const std::u16string& key) {
  const char16_t* conflict = conflicts_key_of(key);
  if (conflict != nullptr && !is_end(std::u16string(conflict))) return false;
  KeyStatus* p = slot(key);
  if (type == u"kd") return !is_end(key) || p->time() == time();
  if (type == u"ku") return is_end(key) || p->u_time() == time();
  if (type == u"dbl") return is_db_hit(key);
  if (type == u"hit") {
    return p->is_hit(time(), _env->key_hit_duration) && !truthy(Value(p->used()));
  }
  return p->is_hld(time(), _env->key_hit_duration);
}

const ControllerResult& BaseController::update() {
  _time.add();
  const CtrlEnv& env = *_env;
  const std::u16string F = env.facing == 1 ? u"R" : u"L";
  const std::u16string B = env.facing == 1 ? u"L" : u"R";
  if (!queue.empty()) {
    std::u16string key_downs;
    for (const auto& item : queue) {
      const Status status = item.first;
      const std::u16string& gk = item.second;
      if (status == Status::UP) {
        if (is_end(gk)) continue;
        slot(gk)->end(time());
      } else if (status == Status::DOWN) {
        if (!is_end(gk)) continue;
        key_downs += gk;
        if (gk == u"d") {
          _key_list = gk;
          _readable_key_list = gk_label_of(gk);
        } else if (!_key_list.empty() && _key_list[0] == u'd') {
          _key_list += gk;
          _readable_key_list += gk_label_of(gk);
        }
        slot(gk)->hit(Value(time()), time());
        const char16_t* ck = conflicts_key_of(gk);
        if (ck != nullptr) dbc.slot(std::u16string(ck))->reset();
        DoubleClick* d = dbc.slot(gk);
        if (!d->fired()) {
          Object o;
          o.set(u"fstate", Value(env.frame_state));
          o.set(u"facing", Value(env.facing));
          d->press(time(), Value(std::make_shared<Object>(o)), env.double_click_interval);
        }
      } else {
        slot(gk)->hit(Value(time() - env.key_hit_duration), time());
      }
    }
    if (_is_human && !key_downs.empty() && env.alive) {
      std::vector<SeqKeys*> all = {&_seq_djdj, &_seq_dddd, &_seq_dada, &_seq_djjj};
      for (SeqKeys* v : all) {
        v->press(key_downs);
        if (!truthy(Value(v->hit()))) continue;
        const std::u16string etc = to_string(field_or(v->data(), u"etc"));
        if (env.world_etc) env.world_etc(env.px, env.py, env.pz, etc);
        if (etc == u"0" && env.team_come) env.team_come(env.team, env.px, env.py, env.pz);
        if (etc == u"2" && env.team_stay) env.team_stay(env.team);
        if (etc == u"4" && env.team_move) env.team_move(env.team);
        if (etc == u"8" && env.team_follow) env.team_follow();
        v->reset();
      }
    }
    queue.clear();
  }

  const Value& kd = env.kd;
  const Value& hit = env.hit;
  const Value& ku = env.ku;
  const Value& hld = env.hld;

  ControllerResult& ret = result;
  ret.clear();

  if (truthy(kd) && ret.time() == 0) {
    if (truthy(obj_get(kd, u"F")) && tst(u"kd", F)) {
      ret.fire(obj_get(kd, u"F"), slot(F)->time(), F, u"kd");
    }
    if (truthy(obj_get(kd, u"B")) && tst(u"kd", B)) {
      ret.fire(obj_get(kd, u"B"), slot(B)->time(), B, u"kd");
    }
  }
  if (truthy(ku) && ret.time() == 0) {
    if (truthy(obj_get(ku, u"F")) && tst(u"ku", F)) {
      ret.fire(obj_get(ku, u"F"), slot(F)->time(), F, u"ku");
    }
    if (truthy(obj_get(ku, u"B")) && tst(u"ku", B)) {
      ret.fire(obj_get(ku, u"B"), slot(B)->time(), B, u"ku");
    }
  }
  if (truthy(env.pre_hitkeys) && ret.time() == 0) {
    if (truthy(obj_get(env.pre_hitkeys, u"F")) && tst(u"hit", F) &&
        ret.fire(obj_get(env.pre_hitkeys, u"F"), slot(F)->time(), F, u"hit")) {
      slot(F)->use();
    }
    if (truthy(obj_get(env.pre_hitkeys, u"B")) && tst(u"hit", B) &&
        ret.fire(obj_get(env.pre_hitkeys, u"B"), slot(B)->time(), B, u"hit")) {
      slot(B)->use();
    }
  }
  if (truthy(hit) && ret.time() == 0) {
    if (truthy(obj_get(hit, u"F")) && tst(u"hit", F) &&
        ret.fire(obj_get(hit, u"F"), slot(F)->time(), F, u"hit")) {
      slot(F)->use();
    }
    if (truthy(obj_get(hit, u"B")) && tst(u"hit", B) &&
        ret.fire(obj_get(hit, u"B"), slot(B)->time(), B, u"hit")) {
      slot(B)->use();
    }
  }
  if (truthy(env.post_hitkeys) && ret.time() == 0) {
    if (truthy(obj_get(env.post_hitkeys, u"F")) && tst(u"hit", F) &&
        ret.fire(obj_get(env.post_hitkeys, u"F"), slot(F)->time(), F, u"hit")) {
      slot(F)->use();
    }
    if (truthy(obj_get(env.post_hitkeys, u"B")) && tst(u"hit", B) &&
        ret.fire(obj_get(env.post_hitkeys, u"B"), slot(B)->time(), B, u"hit")) {
      slot(B)->use();
    }
  }
  if (truthy(env.pre_hitkeys)) {
    if (truthy(obj_get(env.pre_hitkeys, u"FF")) && tst(u"dbl", F)) {
      ret.fire(obj_get(env.pre_hitkeys, u"FF"), dbc.slot(F)->time(), F, u"dbl");
    }
    if (truthy(obj_get(env.pre_hitkeys, u"BB")) && tst(u"dbl", B)) {
      ret.fire(obj_get(env.pre_hitkeys, u"BB"), dbc.slot(B)->time(), B, u"dbl");
    }
  }
  if (truthy(hit)) {
    if (truthy(obj_get(hit, u"FF")) && tst(u"dbl", F)) {
      ret.fire(obj_get(hit, u"FF"), dbc.slot(F)->time(), F, u"dbl");
    }
    if (truthy(obj_get(hit, u"BB")) && tst(u"dbl", B)) {
      ret.fire(obj_get(hit, u"BB"), dbc.slot(B)->time(), B, u"dbl");
    }
  }
  if (truthy(env.post_hitkeys)) {
    if (truthy(obj_get(env.post_hitkeys, u"FF")) && tst(u"dbl", F)) {
      ret.fire(obj_get(env.post_hitkeys, u"FF"), dbc.slot(F)->time(), F, u"dbl");
    }
    if (truthy(obj_get(env.post_hitkeys, u"BB")) && tst(u"dbl", B)) {
      ret.fire(obj_get(env.post_hitkeys, u"BB"), dbc.slot(B)->time(), B, u"dbl");
    }
  }
  if (truthy(hld)) {
    if (truthy(obj_get(hld, u"F")) && tst(u"hld", F)) {
      ret.fire(obj_get(hld, u"F"), slot(F)->time(), F, u"hld");
    }
    if (truthy(obj_get(hld, u"B")) && tst(u"hld", B)) {
      ret.fire(obj_get(hld, u"B"), slot(B)->time(), B, u"hld");
    }
  }

  for (const char16_t* name : all_game_keys()) {
    const std::u16string nm(name);
    KeyStatus* key = slot(nm);
    if (check_key_act(kd, u"kd", key, false)) break;
    if (check_key_act(ku, u"ku", key, false)) break;
    if (check_key_act(env.pre_hitkeys, u"hit", key, true)) break;
    if (check_key_act(hit, u"hit", key, true)) break;
    if (check_key_act(env.post_hitkeys, u"hit", key, true)) break;
    if (check_key_act(hld, u"hld", key, false)) break;
    DoubleClick* d = dbc.slot(nm);
    if (d->fired()) d->set_fired(false);
  }
  do {
    const Value m1 = coalesce(env.transform_pre_seq_map, env.data_pre_seq_map);
    if (truthy(m1) && check_hit_seqs(m1, ret)) break;
    if (truthy(env.seq_map) && check_hit_seqs(env.seq_map, ret)) break;
    const Value m2 = coalesce(env.transform_post_seq_map, env.data_post_seq_map);
    if (truthy(m2) && check_hit_seqs(m2, ret)) break;
  } while (false);

  if (_key_list.size() >= 10) reset_key_list();
  return ret;
}

bool BaseController::check_key_act(const Value& map, const std::u16string& kind,
                                  KeyStatus* key, bool use) {
  if (!truthy(map)) return false;
  if (result.time() != 0) return false;

  const std::u16string name = to_string(key->key());
  const Value act = obj_get(map, name);
  if (truthy(act) && tst(kind, name)) {
    if (result.fire(act, key->time(), name, kind)) {
      if (use) key->use();
      return true;
    }
  }

  if (kind != u"hit") return false;

  const std::u16string keykey = name + name;
  const Value act2 = obj_get(map, keykey);
  if (truthy(act2) && tst(u"dbl", name)) {
    return result.fire(act2, dbc.slot(name)->time(), name, u"dbl");
  }
  return false;
}

bool BaseController::check_hit_seqs(const Value& seqs, ControllerResult& out) {
  const Object* o = as_object(seqs);
  if (keys.d.is_hit(time(), _env->key_hit_duration) && o != nullptr) {
    for (const std::u16string& seq : o->keys()) {
      const Value nf = obj_get(seqs, seq);
      if (seq.empty() || !truthy(nf)) continue;
      if (!sametime_keys_test(seq)) continue;
      use_each(keys, seq);
      if (out.fire(nf, time(), u"d" + seq, u"seq")) {
        use_each(keys, seq);
        reset_key_list();
        return true;
      }
    }
  }
  if (_is_bot) return false;
  if (_key_list.size() >= 3 && o != nullptr) {
    for (const std::u16string& seq : o->keys()) {
      const Value nf = obj_get(seqs, seq);
      if (seq.empty() || !truthy(nf)) continue;
      if (!sequence_keys_test(seq)) continue;
      if (out.fire(nf, time(), u"d" + seq, u"seq")) {
        use_each(keys, seq);
        reset_key_list();
        return true;
      }
    }
  }
  return false;
}

bool BaseController::sequence_keys_test(const std::u16string& str) const {
  if (_key_list.empty() || _key_list[0] != u'd') return false;
  for (size_t i = 0; i < str.size(); ++i) {
    if (i + 1 >= _key_list.size()) return false;
    const char16_t actual_key = _key_list[i + 1];
    char16_t expected_key = str[i];
    if (expected_key == u'F') {
      expected_key = _env->facing > 0 ? u'R' : u'L';
    } else if (expected_key == u'B') {
      expected_key = _env->facing < 0 ? u'R' : u'L';
    }
    if (expected_key != actual_key) return false;
  }
  return true;
}

bool BaseController::sametime_keys_test(const std::u16string& str) const {
  for (char16_t k : str) {
    if (k == u'F') {
      k = _env->facing > 0 ? u'R' : u'L';
    } else if (k == u'B') {
      k = _env->facing < 0 ? u'R' : u'L';
    }
    if (!is_hit(std::u16string(1, k))) return false;
  }
  return true;
}

}
}
