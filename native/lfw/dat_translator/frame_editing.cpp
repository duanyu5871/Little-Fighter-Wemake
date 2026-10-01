#include "lfw/dat_translator/frame_editing.h"

#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/next_frame.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {

namespace {

Value field_or_any(const Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  return v != nullptr ? *v : Value();
}

std::vector<std::u16string> keys_of(const Value& key) {
  std::vector<std::u16string> out;
  if (is_str(key)) {
    out.push_back(std::get<std::u16string>(key));
    return out;
  }
  const Array* a = as_array(key);
  if (a != nullptr) {
    for (const Value& v : a->items()) {
      if (is_str(v)) out.push_back(std::get<std::u16string>(v));
    }
  }
  return out;
}

Value cook_one(const Value& v, const Object* costs) {
  if (is_str(v) || is_num(v)) {
    return get_next_frame_by_raw_id(to_string(v), u"frame", u"hit", costs);
  }
  Object r = *as_object(v);
  Value out = Value(std::make_shared<Object>(r));
  cook_next_frame_cost(out, u"hit", costs);
  return out;
}

}

FrameEditing::FrameEditing(const Value& f, const Object* c) : frame(f), costs(c) {}

FrameEditing& FrameEditing::init(const Value& f) {
  frame = f;
  return *this;
}

FrameEditing& FrameEditing::keydown(const Value& key, const std::vector<Value>& nexts) {
  Object* fo = as_object(frame);
  if (fo == nullptr) return *this;
  const Value* kd = fo->get(u"key_down");
  if (kd == nullptr || !truthy(*kd)) fo->set(u"key_down", Value(std::make_shared<Object>()));
  std::vector<Value> cooks;
  for (const Value& v : nexts) cooks.push_back(cook_one(v, costs));
  Object* target = const_cast<Object*>(as_object(*fo->get(u"key_down")));
  if (target == nullptr) return *this;
  for (const std::u16string& k : keys_of(key)) {
    const Value* cur = target->get(k);
    target->set(k, add_next_frame(cur != nullptr ? *cur : Value(), cooks));
  }
  return *this;
}

FrameEditing& FrameEditing::hit(const Value& key, const std::vector<Value>& nexts) {
  Object* fo = as_object(frame);
  if (fo == nullptr) return *this;
  const Value* hv = fo->get(u"hit");
  if (hv == nullptr || !truthy(*hv)) fo->set(u"hit", Value(std::make_shared<Object>()));
  std::vector<Value> cooks;
  for (const Value& v : nexts) cooks.push_back(cook_one(v, costs));
  Object* target = const_cast<Object*>(as_object(*fo->get(u"hit")));
  if (target == nullptr) return *this;
  for (const std::u16string& k : keys_of(key)) {
    const Value* cur = target->get(k);
    target->set(k, add_next_frame(cur != nullptr ? *cur : Value(), cooks));
  }
  return *this;
}

FrameEditing& FrameEditing::seq(const std::u16string& key, const std::vector<Value>& nexts) {
  Object* fo = as_object(frame);
  if (fo == nullptr) return *this;
  const Value* sv = fo->get(u"seqs");
  if (sv == nullptr || !truthy(*sv)) fo->set(u"seqs", Value(std::make_shared<Object>()));
  std::vector<Value> cookeds;
  for (const Value& any : nexts) {
    if (!truthy(any)) continue;
    if (is_str(any) || is_num(any)) {
      cookeds.push_back(get_next_frame_by_raw_id(to_string(any), u"frame", u"hit", costs));
      continue;
    }
    const Object* ao = as_object(any);
    if (ao != nullptr) {
      Value t = Value(std::make_shared<Object>(*ao));
      cook_next_frame_cost(t, u"hit", costs);
      cookeds.push_back(t);
    }
  }
  Object* target = const_cast<Object*>(as_object(*fo->get(u"seqs")));
  if (target == nullptr) return *this;
  if (key.size() > 0 && key[0] == u'F') {
    const std::u16string k1 = u"L" + key.substr(1);
    const std::u16string k2 = u"R" + key.substr(1);
    std::vector<Value> left;
    std::vector<Value> right;
    for (const Value& nf : cookeds) {
      const Object* o = as_object(nf);
      const bool is_b = o != nullptr && equals(field_or_any(*o, u"facing"), en(FacingFlag::B));
      Object l = o != nullptr ? *o : Object();
      l.set(u"facing", en(is_b ? FacingFlag::R : FacingFlag::L));
      Object rr = o != nullptr ? *o : Object();
      rr.set(u"facing", en(is_b ? FacingFlag::L : FacingFlag::R));
      left.push_back(Value(std::make_shared<Object>(l)));
      right.push_back(Value(std::make_shared<Object>(rr)));
    }
    const Value* c1 = target->get(k1);
    target->set(k1, add_next_frame(c1 != nullptr ? *c1 : Value(), left));
    const Value* c2 = target->get(k2);
    target->set(k2, add_next_frame(c2 != nullptr ? *c2 : Value(), right));
  } else {
    const Value* cur = target->get(key);
    target->set(key, add_next_frame(cur != nullptr ? *cur : Value(), cookeds));
  }
  return *this;
}

}
}
