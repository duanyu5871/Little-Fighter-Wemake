#pragma once

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/bots/bots_types.h"

namespace lfw {
namespace dat_translator {
namespace bots {

class BotMaker;

using BotMakerFactory = BotMaker (*)();

Value as_action(const Value& v);

Value as_action(const EditBotActionFunc& f);

class BotMaker {
 public:
  explicit BotMaker(const char16_t* oid);

  const Value& bot() const { return _bot; }
  Value& bot() { return _bot; }

  Object& frames();
  Object& states();

  BotMaker& set_actions(std::initializer_list<Value> actions);
  BotMaker& set_frames(const Value& frame_ids, const Value& action_ids);
  BotMaker& set_states(const Value& state_ids, const Value& action_ids);
  BotMaker& set_dataset(const Value& dataset);

  static void register_maker(const char16_t* oid, BotMakerFactory fn);

  static const std::vector<std::pair<std::u16string, BotMakerFactory>>& makers();

 private:
  Value _bot;
};

}
}
}
