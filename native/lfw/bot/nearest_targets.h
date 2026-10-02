#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace bot {

class BotTarget {
 public:
  BotTarget(Value entity, double distance, Value defendable)
      : entity(std::move(entity)), distance(distance), defendable(std::move(defendable)) {}

  Value facing() const;
  Value x() const;

  Value entity;
  double distance;
  Value defendable;
};

class NearestTargets {
 public:
  explicit NearestTargets(double max) : max_(max) {}

  const BotTarget* get() const;
  void look(const Value& self, const Value& other, const Value& defendable = Value());
  void del(const std::function<bool(const BotTarget&)>& condition);
  void sort(const Value& self);
  void clear();

  double max() const { return max_; }
  const std::vector<BotTarget>& targets() const { return targets_; }
  const std::vector<Value>& entities() const { return entities_; }

 private:
  bool has_entity(const Value& v) const;
  void add_entity(const Value& v);
  void del_entity(const Value& v);

  double max_;
  std::vector<BotTarget> targets_;
  std::vector<Value> entities_;
};

}
}
