#include "lfw/controller/creators.h"

#include "lfw/bot/bot_controller.h"
#include "lfw/controller/ball_controller.h"

namespace lfw {
namespace controller {

namespace {

class BotCtrlCreator : public ICtrlCreator {
 public:
  BaseController* create(const std::u16string& player_id, Entity*) const override {
    bot::BotController* const c = new bot::BotController();
    c->player_id = player_id;
    return c;
  }
};

class BallCtrlCreator : public ICtrlCreator {
 public:
  BaseController* create(const std::u16string& player_id, Entity*) const override {
    BallController* const c = new BallController();
    c->player_id = player_id;
    return c;
  }
};

}

const ICtrlCreator* bot_controller_creator() {
  static const BotCtrlCreator kCreator;
  return &kCreator;
}

const ICtrlCreator* ball_controller_creator() {
  static const BallCtrlCreator kCreator;
  return &kCreator;
}

}
}
