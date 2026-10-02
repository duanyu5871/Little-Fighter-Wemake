#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/entity/summary.h"

namespace lfw {

class SummaryMgr {
 public:
  std::shared_ptr<Summary> get(const std::u16string& id);
  void clear();
  void release(const std::u16string& id);
  void add_damage_sum(const Value& a, const Value& value);
  void add_kill_sum(const Value& a, const Value& value = Value());
  void apply_damage(const Value& a, const Value& injury, const Value& v, const Value& prev_hp);

  const std::vector<std::pair<std::u16string, std::shared_ptr<Summary>>>& items() const {
    return _items;
  }
  std::size_t grave_count() const { return _graves.size(); }

 protected:
  std::shared_ptr<Summary> acquire(const std::u16string& id);

 private:
  std::shared_ptr<Summary>* find_item(const std::u16string& id);
  void del_item(const std::u16string& id);

  std::vector<std::shared_ptr<Summary>> _graves;
  std::vector<std::pair<std::u16string, std::shared_ptr<Summary>>> _items;
};

SummaryMgr& summary_mgr();

}
