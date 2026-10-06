#include "lfw/dat_translator/cook_frame_indicator_info.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/type_cast.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {

namespace {

Object* as_mut(const Value& v) { return const_cast<Object*>(as_object(v)); }

const Value* key_of(const Object* o, const char16_t* key) {
  return o != nullptr ? o->get(std::u16string(key)) : nullptr;
}

Value field_at(const Value& v, const char16_t* key) {
  const Value* p = key_of(as_object(v), key);
  return p != nullptr ? *p : Value();
}

bool has_key(const Object* o, const char16_t* key) { return key_of(o, key) != nullptr; }

Value or_zero_if_undefined(const Value& v) {
  return std::holds_alternative<std::monostate>(v) ? Value(0.0) : v;
}

Value or_zero_if_falsy(const Value& v) { return truthy(v) ? v : Value(0.0); }

Value num(const Value& v) { return Value(to_number(v)); }

Value sub(const Value& a, const Value& b) { return Value(to_number(a) - to_number(b)); }

Value neg(const Value& a) { return Value(-to_number(a)); }

Value add(const Value& a, const Value& b) { return Value(to_number(a) + to_number(b)); }

Value div2(const Value& a) { return Value(to_number(a) / 2.0); }

Value make_qube(std::initializer_list<std::pair<const char16_t*, Value>> kv) {
  Object o;
  for (const std::pair<const char16_t*, Value>& p : kv) o.set(std::u16string(p.first), p.second);
  return Value(std::make_shared<Object>(o));
}

Value qube_x(const Value& q) { return field_at(q, u"x"); }

Value copy_with_x(const Value& src, const Value& x) {
  const Object* o = as_object(src);
  Object out;
  if (o != nullptr) {
    const std::vector<std::u16string> ks = o->keys();
    for (const std::u16string& k : ks) {
      out.set(k, k == u"x" ? x : *o->get(k));
    }
  }
  return Value(std::make_shared<Object>(out));
}

Value indicator_pair(const Value& q1, const Value& q2) {
  Object o;
  o.set(u"1", q1);
  o.set(u"-1", q2);
  return Value(std::make_shared<Object>(o));
}

bool set_indicator(const Value& target, const Value& q1, const Value& q2) {
  Object* t = as_mut(target);
  if (t == nullptr) {
    // TS 给数组挂属性是可以的（渲染里看不见）；给字符串 / 数字挂会抛。
    return as_array(target) != nullptr;
  }
  t->set(u"__indicator_info", indicator_pair(q1, q2));
  return true;
}

bool is_object_like(const Value& v) {
  return as_object(v) != nullptr || as_array(v) != nullptr;
}

// TS 的 `list?.forEach(fn)`：假值跳过；真值却不是数组时 `.forEach` 不是函数 ⇒ 抛。
// 每一项要么是对象（数组也行，`{ … } = o` 解构得动、也能挂属性），要么 TS 就抛。
// `ok` 是「没能照 TS 走完」的出口，三个调用点都紧跟一句 `if (!ok) return false;`。
void for_each_object(const Value& list, bool& ok, const std::function<void(Value&)>& fn) {
  if (!ok) return;
  if (!truthy(list)) return;
  Array* a = const_cast<Array*>(as_array(list));
  if (a == nullptr) {
    ok = false;
    return;
  }
  const size_t n = a->size();
  for (size_t i = 0; i < n; ++i) {
    Value item = a->at(i);
    if (!is_object_like(item)) {
      ok = false;
      return;
    }
    fn(item);
  }
}

}

bool cook_frame_indicator_info(Value& frame) {
  Object* f = as_object(frame);
  if (f == nullptr) return true;
  const Value pic = field_at(frame, u"pic");
  // `pic && "w" in pic`：`in` 对字符串 / 数字 / 布尔这类原始值会抛。
  if (truthy(pic) && !is_object_like(pic)) return false;
  const Object* pic_o = as_object(pic);
  const bool use_pic = pic_o != nullptr && has_key(pic_o, u"w");
  const Value w = use_pic ? field_at(pic, u"w") : field_at(frame, u"width");
  const Value h = use_pic ? field_at(pic, u"h") : field_at(frame, u"height");
  if (!truthy(w) || !truthy(h)) return true;

  const Value f1 = make_qube({{u"x", neg(field_at(frame, u"centerx"))},
                              {u"y", sub(field_at(frame, u"centery"), h)},
                              {u"w", w},
                              {u"h", h},
                              {u"z", Value(0.0)},
                              {u"l", Value(0.0)}});
  const Value f2 = copy_with_x(f1, sub(field_at(frame, u"centerx"), field_at(f1, u"w")));

  Object info;
  info.set(u"1", f1);
  info.set(u"-1", f2);
  f->set(u"__indicator_info", Value(std::make_shared<Object>(info)));

  const Value f1w = field_at(f1, u"w");
  const Value f1h = field_at(f1, u"h");
  const Value f1x = qube_x(f1);
  const Value f1y = field_at(f1, u"y");
  const Value f2x = qube_x(f2);

  bool ok = true;
  for_each_object(field_at(frame, u"opoint"), ok, [&](Value& o) {
    const Value rect1 = make_qube({{u"w", Value(2.0)},
                                   {u"h", Value(2.0)},
                                   {u"x", sub(add(f1x, field_at(o, u"x")), Value(1.0))},
                                   {u"y", sub(sub(add(f1y, f1h), field_at(o, u"y")), Value(1.0))},
                                   {u"z", or_zero_if_falsy(field_at(o, u"z"))},
                                   {u"l", Value(0.0)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), Value(2.0)), field_at(o, u"x")));
    set_indicator(o, rect1, rect2);
  });
  if (!ok) return false;

  const Value cpoint = field_at(frame, u"cpoint");
  if (truthy(cpoint)) {
    const Value ox = or_zero_if_undefined(field_at(cpoint, u"x"));
    const Value oy = or_zero_if_undefined(field_at(cpoint, u"y"));
    const Value oz = or_zero_if_falsy(or_zero_if_undefined(field_at(cpoint, u"z")));
    const Value rect1 = make_qube({{u"w", Value(2.0)},
                                   {u"h", Value(2.0)},
                                   {u"x", sub(add(f1x, ox), Value(1.0))},
                                   {u"y", sub(sub(add(f1y, f1h), oy), Value(1.0))},
                                   {u"z", oz},
                                   {u"l", Value(0.0)}});
    const Value rect2 = copy_with_x(rect1, sub(sub(add(f2x, f1w), Value(2.0)), ox));
    if (!set_indicator(cpoint, rect1, rect2)) return false;
  }

  const Value bpoint = field_at(frame, u"bpoint");
  if (truthy(bpoint)) {
    const Value rect1 = make_qube({{u"w", Value(2.0)},
                                   {u"h", Value(2.0)},
                                   {u"x", sub(add(f1x, field_at(bpoint, u"x")), Value(1.0))},
                                   {u"y",
                                    sub(sub(add(f1y, f1h), field_at(bpoint, u"y")), Value(1.0))},
                                   {u"z", or_zero_if_falsy(field_at(bpoint, u"z"))},
                                   {u"l", Value(0.0)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), Value(2.0)), field_at(bpoint, u"x")));
    if (!set_indicator(bpoint, rect1, rect2)) return false;
  }

  const Value wpoint = field_at(frame, u"wpoint");
  if (truthy(wpoint)) {
    const Value rect1 = make_qube({{u"w", Value(2.0)},
                                   {u"h", Value(2.0)},
                                   {u"x", sub(add(f1x, field_at(wpoint, u"x")), Value(1.0))},
                                   {u"y",
                                    sub(sub(add(f1y, f1h), field_at(wpoint, u"y")), Value(1.0))},
                                   {u"z", or_zero_if_falsy(field_at(wpoint, u"z"))},
                                   {u"l", Value(0.0)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), Value(2.0)), field_at(wpoint, u"x")));
    if (!set_indicator(wpoint, rect1, rect2)) return false;
  }

  for_each_object(field_at(frame, u"bdy"), ok, [&](Value& o) {
    const Value bw = or_zero_if_undefined(field_at(o, u"w"));
    const Value bh = or_zero_if_undefined(field_at(o, u"h"));
    const Value rect1 = make_qube({{u"w", bw},
                                   {u"h", bh},
                                   {u"z", or_zero_if_undefined(field_at(o, u"z"))},
                                   {u"l", or_zero_if_undefined(field_at(o, u"l"))},
                                   {u"x", add(f1x, or_zero_if_undefined(field_at(o, u"x")))},
                                   {u"y", sub(sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y"))), bh)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });
  if (!ok) return false;

  for_each_object(field_at(frame, u"itr"), ok, [&](Value& o) {
    const Value bw = or_zero_if_undefined(field_at(o, u"w"));
    const Value bh = or_zero_if_undefined(field_at(o, u"h"));
    const Value rect1 = make_qube({{u"w", bw},
                                   {u"h", bh},
                                   {u"z", or_zero_if_undefined(field_at(o, u"z"))},
                                   {u"l", or_zero_if_undefined(field_at(o, u"l"))},
                                   {u"x", add(f1x, or_zero_if_undefined(field_at(o, u"x")))},
                                   {u"y", sub(sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y"))), bh)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });
  if (!ok) return false;

  return true;
}

}
}
