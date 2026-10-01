#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace lfw {

class Array;

struct NullTag {};

using Value =
    std::variant<std::monostate, NullTag, bool, double, std::u16string, std::shared_ptr<Array>>;

class Array {
 public:
  Array() = default;

  size_t size() const { return _items.size(); }
  bool empty() const { return _items.empty(); }
  const Value& at(size_t i) const { return _items[i]; }
  Value& at(size_t i) { return _items[i]; }
  void push_back(Value v) { _items.push_back(std::move(v)); }
  const std::vector<Value>& items() const { return _items; }

 private:
  std::vector<Value> _items;
};

bool truthy(const Value& v);
const char* type_of(const Value& v);
bool is_array(const Value& v);
const Array* as_array(const Value& v);
Array* as_array(Value& v);

}
