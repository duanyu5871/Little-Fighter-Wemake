#pragma once

#include <cstdint>

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace lfw {

class Array;
class Object;

struct NullTag {};

using Value = std::variant<std::monostate, NullTag, bool, double, std::u16string,
                           std::shared_ptr<Array>, std::shared_ptr<Object>>;

class Array {
 public:
  Array() = default;
  explicit Array(std::vector<Value> items) : _items(std::move(items)) {}

  size_t size() const { return _items.size(); }
  bool empty() const { return _items.empty(); }
  const Value& at(size_t i) const { return _items[i]; }
  Value& at(size_t i) { return _items[i]; }
  void push_back(Value v) { _items.push_back(std::move(v)); }
  // `a.splice(i, 1)`：只按调用者保证的下标删除。
  void remove_at(size_t i) { _items.erase(_items.begin() + static_cast<std::ptrdiff_t>(i)); }
  const std::vector<Value>& items() const { return _items; }

 private:
  std::vector<Value> _items;
};

class Object {
 public:
  Object() = default;

  size_t size() const { return _ints.size() + _strs.size(); }
  bool empty() const { return _ints.empty() && _strs.empty(); }

  bool has(const std::u16string& key) const;
  const Value* get(const std::u16string& key) const;
  void set(const std::u16string& key, Value v);
  bool remove(const std::u16string& key);
  std::vector<std::u16string> keys() const;

 private:
  std::map<uint32_t, Value> _ints;
  std::vector<std::pair<std::u16string, Value>> _strs;
};

bool truthy(const Value& v);
const char* type_of(const Value& v);
bool is_array(const Value& v);
const Array* as_array(const Value& v);
Array* as_array(Value& v);
const Object* as_object(const Value& v);
Object* as_object(Value& v);
std::vector<std::u16string> object_keys(const Value& v);

std::u16string array_join(const Array& a);
Value to_primitive(const Value& v);
double to_number(const Value& v);
std::u16string to_string(const Value& v);
bool strict_equals(const Value& a, const Value& b);
bool equals(const Value& a, const Value& b);

std::optional<bool> less_than(const Value& a, const Value& b);
bool lt(const Value& a, const Value& b);
bool gt(const Value& a, const Value& b);
bool le(const Value& a, const Value& b);
bool ge(const Value& a, const Value& b);

}
