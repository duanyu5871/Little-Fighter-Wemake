#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/core/js_string.h"
#include "lfw/core/value.h"

namespace lfw {

struct FSMStateSnapshot {
  std::u16string name;
  std::optional<Value> state_key;
  std::optional<Value> prev_state_key;
  double time = 0;
  double state_time = 0;
};

class FSM;

class IState {
 public:
  std::optional<std::u16string> name;
  virtual ~IState() = default;
  virtual Value key() const = 0;
  virtual std::optional<Value> update(double dt) {
    (void)dt;
    return std::nullopt;
  }
  virtual void enter() {}
  virtual void leave() {}
};

class FSM {
 public:
  using Snapshot = FSMStateSnapshot;

  static std::function<void(const std::u16string&)>& default_log() {
    static std::function<void(const std::u16string&)> f;
    return f;
  }

  std::function<void(const std::u16string&)> log;

  explicit FSM(std::u16string name = std::u16string()) : _name(std::move(name)) { log = default_log(); }

  const std::u16string& name() const { return _name; }
  void set_name(std::u16string v) { _name = std::move(v); }

  const std::vector<std::pair<Value, IState*>>& states() const { return _state_map; }

  CallbacksT<FSM*>& callbacks() { return _callbacks; }

  IState* state() const { return _state; }
  IState* prev_state() const { return _prev_state; }
  double time() const { return _time; }
  void set_time(double v) { _time = v; }
  double state_time() const { return _state_time; }

  FSM& logger(std::function<void(const std::u16string&)> fn) {
    log = std::move(fn);
    return *this;
  }

  IState* get(const Value& key) const {
    for (const std::pair<Value, IState*>& kv : _state_map) {
      if (strict_equals(kv.first, key)) return kv.second;
    }
    return nullptr;
  }

  template <typename... Ss>
  FSM& add(Ss*... states) {
    (add_one(states), ...);
    return *this;
  }

  FSM& use(const Value& key) {
    set_state(get(key));
    return *this;
  }

  FSM& reset(const Value& key) {
    _state = nullptr;
    _prev_state = nullptr;
    _time = 0;
    _state_time = 0;
    return use(key);
  }

  void set_state(IState* next_state) {
    _prev_state = _state;
    if (_state != nullptr) _state->leave();
    _state_time = 0;
    _state = next_state;
    if (next_state != nullptr) next_state->enter();
    if (log) {
      log(u"[" + _name + u"::state] " + state_label(_prev_state) + u" ==> " + state_label(_state));
    }
    _callbacks.call(u"on_state_changed", {this});
  }

  void update(double dt) {
    _time += dt;
    _state_time += dt;
    IState* curr = _state;
    if (curr == nullptr) return;

    const std::optional<Value> next_key = curr->update(dt);
    if (!next_key.has_value()) return;

    IState* next_state = get(*next_key);
    if (next_state == nullptr) return;

    set_state(next_state);
  }

  Snapshot to_snapshot() const {
    Snapshot s;
    s.name = _name;
    s.state_key = _state != nullptr ? std::optional<Value>(_state->key()) : std::nullopt;
    s.prev_state_key = _prev_state != nullptr ? std::optional<Value>(_prev_state->key()) : std::nullopt;
    s.time = _time;
    s.state_time = _state_time;
    return s;
  }

  FSM& from_snapshot(const Snapshot& s) {
    _name = s.name;
    _time = s.time;
    _state_time = s.state_time;
    _prev_state = s.prev_state_key.has_value() ? get(*s.prev_state_key) : nullptr;
    _state = s.state_key.has_value() ? get(*s.state_key) : nullptr;
    return *this;
  }

 private:
  void add_one(IState* s) {
    for (size_t i = 0; i < _state_map.size(); ++i) {
      if (strict_equals(_state_map[i].first, s->key())) {
        _state_map[i].second = s;
        return;
      }
    }
    _state_map.emplace_back(s->key(), s);
  }

  static std::u16string state_label(IState* s) {
    if (s == nullptr) return u"undefined";
    const Value key = s->key();
    const std::u16string name = s->name.has_value() ? *s->name : to_string(key);
    if (equals(Value(name), key)) return name;
    return name + u"(" + to_string(key) + u")";
  }

  std::u16string _name;
  CallbacksT<FSM*> _callbacks;
  std::vector<std::pair<Value, IState*>> _state_map;
  IState* _prev_state = nullptr;
  IState* _state = nullptr;
  double _time = 0;
  double _state_time = 0;
};

}
