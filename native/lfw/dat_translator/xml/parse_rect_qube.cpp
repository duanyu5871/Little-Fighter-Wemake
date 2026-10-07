#include "lfw/dat_translator/xml/parse_rect_qube.h"

#include <memory>
#include <optional>
#include <vector>

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

Value from_nums(const std::vector<double>& v) {
  auto o = std::make_shared<Object>();
  o->set(u"x", Value(v[0]));
  o->set(u"y", Value(v[1]));
  o->set(u"w", Value(v[2]));
  o->set(u"h", Value(v[3]));
  o->set(u"z", v.size() > 4 ? Value(v[4]) : Value());
  o->set(u"l", v.size() > 5 ? Value(v[5]) : Value());
  return Value(std::move(o));
}

}

Value parse_rect_qube(const IXMLElement& el) {
  const std::optional<std::vector<double>> rect = el.nums_attr(u"rect");
  if (rect && rect->size() >= 4) return from_nums(*rect);
  const std::optional<std::vector<double>> qube = el.nums_attr(u"qube");
  if (qube && qube->size() >= 4) return from_nums(*qube);
  return Value(std::make_shared<Object>());
}

}
}
}
