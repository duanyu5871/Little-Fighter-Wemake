#include "lfw/ui/component/flex.h"

#include "lfw/ui/component/flex_item.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace ui {

const ClazzTag* Flex::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& Flex::TAGS() {
  static const std::vector<std::u16string> tags{u"Flex"};
  return tags;
}

std::u16string Flex::direction() {
  return props_holder.str(u"direction", {u"row", u"column"}).value_or(u"row");
}

double Flex::gap() { return props_holder.num(u"gap").value_or(0.0); }

double Flex::row_gap() {
  const std::optional<double> v = props_holder.num(u"row_gap");
  return v.has_value() ? *v : gap();
}

double Flex::col_gap() {
  const std::optional<double> v = props_holder.num(u"col_gap");
  return v.has_value() ? *v : gap();
}

std::u16string Flex::align() {
  return props_holder.str(u"align", {u"start", u"center", u"end", u"stretch"}).value_or(u"start");
}

bool Flex::fit() { return props_holder.bool_(u"fit").value_or(false); }

bool Flex::fit_w() {
  const std::optional<bool> v = props_holder.bool_(u"fit_w");
  return v.has_value() ? *v : fit();
}

bool Flex::fit_h() {
  const std::optional<bool> v = props_holder.bool_(u"fit_h");
  return v.has_value() ? *v : fit();
}

std::array<double, 4> Flex::padding() {
  std::array<double, 4> out{0.0, 0.0, 0.0, 0.0};
  const std::optional<std::vector<Value>> nums = props_holder.nums(u"padding", 4);
  if (!nums.has_value()) return out;
  const size_t len = nums->size() < out.size() ? nums->size() : out.size();
  for (size_t i = 0; i < len; ++i) {
    if (const double* const d = std::get_if<double>(&(*nums)[i])) out[i] = *d;
  }
  return out;
}

void Flex::on_resume() { apply(); }

void Flex::update(double dt) {
  (void)dt;
  apply();
}

void Flex::apply() {
  const std::u16string dir = direction();
  const double col_gap_v = col_gap();
  const double row_gap_v = row_gap();
  const std::u16string align_v = align();
  const bool fit_h_v = fit_h();
  const bool fit_w_v = fit_w();
  const std::array<double, 4> pad = padding();
  const double padding_top = pad[0];
  const double padding_right = pad[1];
  const double padding_bottom = pad[2];
  const double padding_left = pad[3];

  double temp_x = padding_left;
  double temp_y = padding_top;
  const UINode::Cross& my_cross = node.cross();
  const std::vector<UINode*>& kids = node.children();
  const size_t len = kids.size();

  if (fit_w_v || fit_h_v) {
    double w = 0.0;
    double h = 0.0;
    for (size_t i = 0; i < len; ++i) {
      UINode* const child = kids[i];
      if (child == nullptr || !child->self_visible()) continue;
      const double child_w = child->size.x;
      const double child_h = child->size.y;
      if (fit_w_v && dir == u"row") w += child_w + (i < len - 1 ? row_gap_v : 0.0);
      else if (fit_w_v && dir == u"column") w = lfw::max(child_w, w);
      if (fit_h_v && dir == u"column") h += child_h + (i < len - 1 ? col_gap_v : 0.0);
      else if (fit_h_v && dir == u"row") h = lfw::max(child_h, h);
    }
    if (fit_w_v) {
      w += padding_left + padding_right;
      node.resize(w, node.h());
    }
    if (fit_h_v) {
      h += padding_top + padding_bottom;
      node.resize(node.w(), h);
    }
  }

  const double my_w = node.size.x;
  const double my_h = node.size.y;
  for (UINode* const child : node.children()) {
    if (child == nullptr || !child->self_visible()) continue;
    const double z = child->pos.z;
    const double child_w = child->w();
    const double child_h = child->h();
    const UINode::Cross& cross = child->cross();
    UIComponent* const comp = child->find_component(FlexItem::class_tag(),
                                                    [](UIComponent& v) { return v.enabled(); });
    FlexItem* const item = static_cast<FlexItem*>(comp);
    const std::u16string item_align = item != nullptr ? item->align().value_or(align_v) : align_v;
    if (dir == u"row") {
      const double item_x = padding_left + my_cross.left + (temp_x - cross.left);
      if (item_align == u"start") {
        child->move_to(item_x, padding_top + my_cross.top - cross.top, z);
      } else if (item_align == u"center") {
        child->move_to(item_x, my_cross.mid_y - cross.mid_y, z);
      } else if (item_align == u"end") {
        child->move_to(item_x, my_cross.bottom - cross.bottom - padding_bottom, z);
      } else if (item_align == u"stretch") {
        child->move_to(item_x, my_cross.top - cross.top, z);
        child->resize(child_w, my_h);
      }
      temp_x += child_w + row_gap_v;
    } else if (dir == u"column") {
      const double item_y = padding_top + my_cross.top + (temp_y - cross.top);
      if (item_align == u"start") {
        child->move_to(padding_left + my_cross.left - cross.left, item_y, z);
      } else if (item_align == u"center") {
        child->move_to(my_cross.mid_x - cross.mid_x, item_y, z);
      } else if (item_align == u"end") {
        child->move_to(my_cross.right - cross.right - padding_right, item_y, z);
      } else if (item_align == u"stretch") {
        child->move_to(padding_left + my_cross.left - cross.left, item_y, z);
        child->resize(my_w, child_h);
      }
      temp_y += child_h + col_gap_v;
    }
  }
}

}
}
