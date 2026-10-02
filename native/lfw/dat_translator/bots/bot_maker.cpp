#include "lfw/dat_translator/bots/bot_maker.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/spread_assign.h"

namespace lfw {
namespace dat_translator {
namespace bots {

namespace {

std::vector<std::pair<std::u16string, BotMakerFactory>>& registry() {
  static std::vector<std::pair<std::u16string, BotMakerFactory>> r;
  return r;
}

Value field_of(const Value& v, const char16_t* key) {
  const Object* o = as_object(v);
  const Value* p = o != nullptr ? o->get(std::u16string(key)) : nullptr;
  return p != nullptr ? *p : Value();
}

Object& child_object(Value& host, const char16_t* key) {
  Object* h = as_object(host);
  const Value* cur = h->get(std::u16string(key));
  if (cur == nullptr || !truthy(*cur)) {
    h->set(std::u16string(key), Value(std::make_shared<Object>()));
  }
  return *const_cast<Object*>(as_object(*h->get(std::u16string(key))));
}

}

Value as_action(const Value& v) { return v; }

Value as_action(const EditBotActionFunc& f) { return f(nullptr); }

BotMaker::BotMaker(const char16_t* oid) {
  Object o;
  o.set(u"id", Value(std::u16string(oid)));
  o.set(u"oid", Value(std::u16string(oid)));
  o.set(u"actions", Value(std::make_shared<Object>()));
  _bot = Value(std::make_shared<Object>(o));
}

Object& BotMaker::frames() { return child_object(_bot, u"frames"); }

Object& BotMaker::states() { return child_object(_bot, u"states"); }

BotMaker& BotMaker::set_actions(std::initializer_list<Value> actions) {
  const Value actions_value = field_of(_bot, u"actions");
  Object* act = const_cast<Object*>(as_object(actions_value));
  if (act == nullptr) return *this;
  for (const Value& a : actions) {
    const Object* o = as_object(a);
    if (o == nullptr) continue;
    act->set(to_string(field_of(a, u"action_id")), a);
  }
  return *this;
}

BotMaker& BotMaker::set_frames(const Value& frame_ids, const Value& action_ids) {
  frames().set(to_string(frame_ids), action_ids);
  return *this;
}

BotMaker& BotMaker::set_states(const Value& state_ids, const Value& action_ids) {
  states().set(to_string(state_ids), action_ids);
  return *this;
}

BotMaker& BotMaker::set_dataset(const Value& dataset) {
  Object* b = as_object(_bot);
  b->set(u"dataset", spread_assign(field_of(_bot, u"dataset"), dataset));
  return *this;
}

void BotMaker::register_maker(const char16_t* oid, BotMakerFactory fn) {
  const std::u16string key(oid);
  for (std::pair<std::u16string, BotMakerFactory>& e : registry()) {
    if (e.first == key) {
      e.second = fn;
      return;
    }
  }
  registry().emplace_back(key, fn);
}

const std::vector<std::pair<std::u16string, BotMakerFactory>>& BotMaker::makers() {
  return registry();
}

}
}
}
