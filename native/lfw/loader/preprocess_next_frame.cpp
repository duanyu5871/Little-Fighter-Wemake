#include "lfw/loader/preprocess_next_frame.h"

#include <variant>

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

}

bool preprocess_next_frame(Value& nf) {
  const Array* a = as_array(nf);
  if (a != nullptr) {
    const size_t n = a->size();
    for (size_t i = 0; i < n; ++i) {
      Value item = a->at(i);
      if (!preprocess_next_frame(item)) return false;
    }
    return true;
  }
  if (is_nullish(nf)) return false;
  return true;
}

}
}
