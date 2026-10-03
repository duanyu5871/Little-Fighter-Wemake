#pragma once

#include <functional>
#include <string>

#include "lfw/collision/fall.h"
#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct INbdyNormalEntity : IFallEntity {
  virtual Value state() const = 0;
  virtual double position_y() const = 0;
  virtual double ground_y() const = 0;
  virtual Value data_indexes_dizzy() const = 0;
  virtual Value data_indexes_grand_injured() const = 0;
  virtual Value data_indexes_injured() const = 0;
  virtual Value cpoint_backhurtact() const = 0;
  virtual Value cpoint_fronthurtact() const = 0;
};

struct NbdyNormalEnv {
  std::function<INbdyNormalEntity*(const std::u16string& id)> find_entity;
  std::function<bool(const INbdyNormalEntity& e)> is_fighter;
  std::function<bool(Collision& c)> is_fall;
  std::function<void(const Value& x, const Value& y, const Value& z, const Value& type)> spark;
  std::function<ItrVelocity(Collision& c)> calc_velocity;
};

void handle_itr_normal_bdy_normal(Collision& c);

const NbdyNormalEnv& nbdy_normal_env();
void set_nbdy_normal_env(const NbdyNormalEnv& env);

}
}
