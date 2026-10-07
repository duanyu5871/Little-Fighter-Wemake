#include "lfw/dat_translator/xml/merge_by_tag.h"

#include <vector>

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

// `Object.assign(dst, src)` 的 port：按 src 的键序写入（已有键只换值不挪位置）。
void assign_into(Value& dst, const Value& src) {
  Object* d = as_object(dst);
  const Object* s = as_object(src);
  if (d == nullptr || s == nullptr) return;
  for (const std::u16string& k : s->keys()) {
    const Value* v = s->get(k);
    if (v != nullptr) d->set(k, *v);
  }
}

}

Value merge_by_tag(const IXMLElement& el, const std::u16string& tag,
                   const std::function<Value(const IXMLElement&)>& parser) {
  return merge_by_tag(el, tag, parser, Value());
}

Value merge_by_tag(const IXMLElement& el, const std::u16string& tag,
                   const std::function<Value(const IXMLElement&)>& parser, Value target) {
  const std::vector<IXMLElement*> children = el.children_by_tag(tag);
  if (children.empty()) return Value();
  Value ret = parser(*children[0]);
  for (size_t i = 1; i < children.size(); ++i) {
    assign_into(ret, parser(*children[i]));
  }
  if (truthy(target)) assign_into(target, ret);
  return ret;
}

}
}
}
