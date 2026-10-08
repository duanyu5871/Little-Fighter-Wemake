#include "lfw/ui/component/random_visible.h"

#include <memory>
#include <utility>
#include <vector>

#include "lfw/ui/uinode.h"
#include "lfw/utils/math/range.h"

namespace lfw {
namespace ui {

namespace {

Randoming::Ptr make_randoming(RandomVisible& owner) {
  const long last = static_cast<long>(owner.node.children().size()) - 1;
  const std::optional<std::vector<double>> r = range(0.0, static_cast<double>(last), 1.0);
  std::vector<Value> src;
  if (r.has_value()) {
    src.reserve(r->size());
    for (const double d : *r) src.push_back(Value(d));
  }
  return Randoming::create(u"RandomVisible", std::move(src), nullptr, Value(false));
}

}

const ClazzTag* RandomVisible::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& RandomVisible::TAGS() {
  static const std::vector<std::u16string> tags{u"RandomVisible"};
  return tags;
}

std::map<std::u16string, Randoming::Ptr>& RandomVisible::by_group() {
  static std::map<std::u16string, Randoming::Ptr> m;
  return m;
}

std::map<const RandomVisible*, Randoming::Ptr>& RandomVisible::by_self() {
  static std::map<const RandomVisible*, Randoming::Ptr> m;
  return m;
}

std::optional<std::u16string> RandomVisible::group() { return str(0); }

Randoming& RandomVisible::randoming() {
  const std::optional<std::u16string> g = group();
  if (g.has_value() && !g->empty()) {
    std::map<std::u16string, Randoming::Ptr>& m = by_group();
    std::map<std::u16string, Randoming::Ptr>::iterator it = m.find(*g);
    if (it == m.end()) it = m.emplace(*g, make_randoming(*this)).first;
    return *it->second;
  }
  std::map<const RandomVisible*, Randoming::Ptr>& m = by_self();
  std::map<const RandomVisible*, Randoming::Ptr>::iterator it = m.find(this);
  if (it == m.end()) it = m.emplace(this, make_randoming(*this)).first;
  return *it->second;
}

void RandomVisible::on_resume() {
  const double idx = to_number(randoming().get());
  const std::vector<UINode*>& kids = node.children();
  for (size_t i = 0; i < kids.size(); ++i) {
    kids[i]->set_visible(static_cast<double>(i) == idx);
  }
}

}
}
