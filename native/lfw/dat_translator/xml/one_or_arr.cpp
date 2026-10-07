#include "lfw/dat_translator/xml/one_or_arr.h"

#include <memory>

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

Value strs_to_value(const std::vector<std::u16string>& strs) {
  auto arr = std::make_shared<Array>();
  for (const std::u16string& s : strs) arr->push_back(Value(s));
  return Value(std::move(arr));
}

}

Value one_or_arr(const Value& mess) {
  const Array* a = as_array(mess);
  if (a == nullptr) return mess;
  if (a->size() == 1) return a->at(0);
  return mess;
}

Value non_empty(const Value& mess) {
  const Array* a = as_array(mess);
  if (a != nullptr && a->size() != 0) return mess;
  return Value();
}

Value one_or_arr(const std::optional<std::vector<std::u16string>>& mess) {
  if (!mess) return Value();
  return one_or_arr(strs_to_value(*mess));
}

Value non_empty(const std::optional<std::vector<std::u16string>>& mess) {
  if (!mess) return Value();
  return non_empty(strs_to_value(*mess));
}

}
}
}
