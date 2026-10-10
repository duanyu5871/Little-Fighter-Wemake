#include "lfw/ui/component/alignment.h"

#include <cmath>

#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace ui {

namespace {

// `Number(v) || 0`：NaN（含 undefined/非数字串）与 0 同效。
double num_or_0(const Value& v) {
  const double n = to_number(v);
  return std::isnan(n) ? 0.0 : n;
}

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

}

const ClazzTag* Alignment::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& Alignment::TAGS() {
  static const std::vector<std::u16string> tags{u"Alignment"};
  return tags;
}

void Alignment::on_start() {
  const Value& raw = props_holder.raw();
  const Value* const followed_v = field_of(raw, u"followed");
  const Value* const offset_v = field_of(raw, u"offset");

  const auto which = [&](const char16_t* key) -> Value {
    if (followed_v == nullptr || is_nullish(*followed_v)) return Value(u"parent");
    if (std::holds_alternative<std::u16string>(*followed_v)) return *followed_v;
    const Value* const sub = field_of(*followed_v, key);
    return sub != nullptr && !is_nullish(*sub) ? *sub : Value(u"parent");
  };
  followed.l = find_node(which(u"l"));
  followed.r = find_node(which(u"r"));
  followed.t = find_node(which(u"t"));
  followed.b = find_node(which(u"b"));

  const std::optional<std::u16string> follower_v = props_holder.str(u"follower");
  follower = find_node(follower_v.has_value() ? Value(*follower_v) : Value());

  align.l = props_holder.str(u"left");
  align.r = props_holder.str(u"right");
  align.t = props_holder.str(u"top");
  align.b = props_holder.str(u"bottom");
  once = props_holder.bool_(u"once").value_or(false);

  offset.l = offset_v != nullptr ? num_or_0(field_or(*offset_v, u"l")) : 0.0;
  offset.r = offset_v != nullptr ? num_or_0(field_or(*offset_v, u"r")) : 0.0;
  offset.t = offset_v != nullptr ? num_or_0(field_or(*offset_v, u"t")) : 0.0;
  offset.b = offset_v != nullptr ? num_or_0(field_or(*offset_v, u"b")) : 0.0;
}

void Alignment::update(double dt) {
  (void)dt;
  if (follower == nullptr) return;
  bool has_l = false;
  bool has_r = false;
  bool has_t = false;
  bool has_b = false;
  double _l = 0.0;
  double _r = 0.0;
  double _t = 0.0;
  double _b = 0.0;

  const double cx = follower->center.x;
  const double cy = follower->center.y;
  const Vector3 gp = follower->global_pos();
  double x = gp.x;
  double y = gp.y;
  double z = gp.z;
  double w = follower->w();
  double h = follower->h();

  const auto align_of = [](const std::optional<std::u16string>& v) -> const char16_t* {
    return v.has_value() ? v->c_str() : u"";
  };

  if (align.r.has_value() && followed.r != nullptr && followed.r != follower) {
    const double fx = followed.r->global_pos().x;
    const UINode::Cross& c = followed.r->cross();
    if (std::u16string(align_of(align.r)) == u"left") {
      _r = fx + c.left;
      has_r = true;
    } else if (std::u16string(align_of(align.r)) == u"right") {
      _r = fx + c.right;
      has_r = true;
    }
  }

  if (align.l.has_value() && followed.l != nullptr && followed.l != follower) {
    const double fx = followed.l->global_pos().x;
    const UINode::Cross& c = followed.l->cross();
    const std::u16string a = align_of(align.l);
    if (a == u"left") {
      _l = fx + c.left;
      has_l = true;
    } else if (a == u"right") {
      _l = fx + c.right;
      has_l = true;
    } else if (a == u"mid") {
      _l = fx + c.mid_x - 0.5 * w;
      has_l = true;
      has_r = false;
    }
  }

  if (align.b.has_value() && followed.b != nullptr && followed.b != follower) {
    const double fy = followed.b->global_pos().y;
    const UINode::Cross& c = followed.b->cross();
    if (std::u16string(align_of(align.b)) == u"top") {
      _b = fy + c.top;
      has_b = true;
    } else if (std::u16string(align_of(align.b)) == u"bottom") {
      _b = fy + c.bottom;
      has_b = true;
    }
  }

  if (align.t.has_value() && followed.t != nullptr && followed.t != follower) {
    const double fy = followed.t->global_pos().y;
    const UINode::Cross& c = followed.t->cross();
    const std::u16string a = align_of(align.t);
    if (a == u"top") {
      _t = fy + c.top;
      has_t = true;
    } else if (a == u"bottom") {
      _t = fy + c.bottom;
      has_t = true;
    } else if (a == u"mid") {
      _t = fy + c.mid_y - 0.5 * h;
      has_t = true;
      has_b = false;
    }
  }

  if (has_l && has_r) w = (_r - _l) - offset.l + offset.r;
  if (has_l) {
    x = _l + cx * w + offset.l;
  } else if (has_r) {
    x = _r - (1 - cx) * w + offset.r;
  }
  if (has_t && has_b) h = (_b - _t) - offset.t + offset.b;
  if (has_t) {
    y = _t + cy * h + offset.t;
  } else if (has_b) {
    y = _b - (1 - cy) * h + offset.b;
  }

  follower->move_to_global(x, y, z);
  follower->resize(w, h);
  if (once) node.del_components(*this);
}

}
}
