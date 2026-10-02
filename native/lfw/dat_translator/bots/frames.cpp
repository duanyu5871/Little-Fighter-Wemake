#include "lfw/dat_translator/bots/frames.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/range.h"

namespace lfw {
namespace dat_translator {
namespace bots {

namespace {

Value make_array(std::vector<Value> items) {
  return Value(std::make_shared<Array>(std::move(items)));
}

Value text(const std::u16string& s) { return Value(s); }

std::vector<Value> number_series(double from, double to) {
  std::vector<Value> out;
  const std::optional<std::vector<double>> r = range(from, to);
  if (!r.has_value()) return out;
  for (double d : *r) out.push_back(Value(d));
  return out;
}

std::vector<Value> prefixed_series(const std::u16string& prefix, double from, double to) {
  std::vector<Value> out;
  const std::optional<std::vector<double>> r = range(from, to);
  if (!r.has_value()) return out;
  for (double d : *r) out.push_back(Value(prefix + number_to_string(d)));
  return out;
}

}

Value frames_object() {
  Object o;
  o.set(u"walkings", make_array(prefixed_series(u"walking_", 0.0, 5.0)));
  o.set(u"standings", make_array(prefixed_series(u"", 0.0, 3.0)));
  o.set(u"runnings", make_array(prefixed_series(u"running_", 0.0, 3.0)));
  o.set(u"punchs", make_array(number_series(60.0, 69.0)));
  o.set(u"rowings", make_array(number_series(103.0, 107.0)));
  o.set(u"super_punch", make_array(number_series(70.0, 79.0)));
  o.set(u"defends", make_array({text(u"110"), text(u"111")}));
  return Value(std::make_shared<Object>(o));
}

}
}
}
