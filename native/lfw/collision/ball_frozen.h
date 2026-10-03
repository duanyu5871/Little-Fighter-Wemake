#pragma once

#include <functional>
#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct IFrozenEntity {
  virtual ~IFrozenEntity() = default;
  virtual Value data() const = 0;
  virtual Value group() const = 0;
  virtual Value state() const = 0;
  virtual Value frame() const = 0;
  virtual Value facing() const = 0;
  virtual double position_x() const = 0;
  virtual double position_y() const = 0;
  virtual double position_z() const = 0;
  virtual bool spawn(const Value& opoint, const Value& face) = 0;
  virtual void enter_frame(const Value& info) = 0;
};

struct BallFrozenEnv {
  std::function<bool(const IFrozenEntity& e)> is_ball;
  std::function<bool(const IFrozenEntity& e)> is_fighter;
};

const BallFrozenEnv& ball_frozen_env();
void set_ball_frozen_env(const BallFrozenEnv& env);

bool handle_ball_frozen(IFrozenEntity& a, IFrozenEntity& v, const Value& itr);

}
}
