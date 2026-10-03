#pragma once

#include <functional>
#include <string>

#include "lfw/collision/n_bdy_normal.h"
#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct INdbdyDefendEntity : INbdyNormalEntity {
  virtual Value defend_ratio() const = 0;
};

struct NbdDefendEnv {
  std::function<INdbdyDefendEntity*(const std::u16string& id)> find_entity;
  std::function<ItrVelocity(Collision& c)> calc_velocity;
  std::function<void(const Value& x, const Value& y, const Value& z, const Value& type)> spark;
  std::function<void(const std::u16string& handler_type, const Value& action)> dispatch;
};

void handle_itr_normal_bdy_defend(Collision& c);

const NbdDefendEnv& nbd_defend_env();
void set_nbd_defend_env(const NbdDefendEnv& env);

}
}
