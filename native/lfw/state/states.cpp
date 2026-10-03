#include "lfw/state/states.h"

#include <variant>

#include "lfw/defines/entity_enum.h"

namespace lfw {
namespace state {
namespace {

constexpr const char16_t* kNumberPrefix = u"n:";
constexpr const char16_t* kStringPrefix = u"s:";

}

std::u16string States::encode_key(const Value& key) {
  const std::u16string* text = std::get_if<std::u16string>(&key);
  if (text != nullptr) return std::u16string(kStringPrefix) + *text;
  return std::u16string(kNumberPrefix) + to_string(key);
}

std::size_t States::index_of(const Value& key) const {
  const auto it = _index.find(encode_key(key));
  return it == _index.end() ? _entries.size() : it->second;
}

bool States::has(const Value& key) const { return index_of(key) != _entries.size(); }

State_Base* States::get(const Value& key) const {
  const std::size_t i = index_of(key);
  if (i == _entries.size()) return nullptr;
  return _entries[i].state.get();
}

void States::set(const Value& key, std::unique_ptr<State_Base> value,
                 std::u16string class_name) {
  const std::size_t i = index_of(key);
  if (i != _entries.size()) {
    // TS `States.set` / `States.add` hit `debugger` on a duplicate key: a re-set
    // key keeps its original position and only the value is replaced.
    _entries[i].state = std::move(value);
    _entries[i].class_name = std::move(class_name);
    return;
  }
  _index.emplace(encode_key(key), _entries.size());
  _entries.push_back(Entry{key, std::move(value), std::move(class_name)});
}

State_Base& States::fallback(const Value& type, double code) {
  const std::u16string state_key = to_string(type) + u"_" + to_string(Value(code));
  const Value key(state_key);
  if (State_Base* hit = get(key)) return *hit;
  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Fighter)))) {
    return make<CharacterState_Base>(key, Value(code));
  }
  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Weapon)))) {
    return make<WeaponState_Base>(key, Value(code));
  }
  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Ball)))) {
    return make<BallState_Base>(key, Value(code));
  }
  return make<State_Base>(key, Value(code));
}

}
}
