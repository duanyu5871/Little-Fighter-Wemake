#include "lfw/dat_translator/float_scaling_entity.h"

#include <cstddef>
#include <string>

#include "lfw/core/value.h"
#include "lfw/dat_translator/cookers.h"
#include "lfw/utils/container_help/traversal.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/round_float.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {
namespace {

Value* member_value(Value& v, const char16_t* key) {
  Object* o = as_object(v);
  if (o == nullptr) return nullptr;
  return const_cast<Value*>(o->get(std::u16string(key)));
}

Object* member_object(Value& v, const char16_t* key) {
  Value* p = member_value(v, key);
  return p != nullptr ? as_object(*p) : nullptr;
}

Value* member_array(Value& v, const char16_t* key) {
  Value* p = member_value(v, key);
  if (p == nullptr || !truthy(*p)) return nullptr;
  return as_array(*p) != nullptr ? p : nullptr;
}

void round_truthy_field(Object& o, const char16_t* key) {
  const Value* cur = o.get(std::u16string(key));
  if (cur == nullptr || !truthy(*cur)) return;
  o.set(std::u16string(key), Value(round_float(to_number(*cur))));
}

void scale_fields(Object& o, const char16_t* const* keys, size_t count) {
  for (size_t i = 0; i < count; ++i) scale_num_field(o, keys[i]);
}

void scale_elements(Value& container, const char16_t* const* keys, size_t count) {
  Array* a = as_array(container);
  if (a == nullptr) return;
  for (size_t i = 0; i < a->size(); ++i) {
    Object* o = as_object(a->at(i));
    if (o == nullptr) continue;
    scale_fields(*o, keys, count);
  }
}

void scale_frame(Value& frame) {
  Object* f = as_object(frame);
  if (f == nullptr) return;
  static const char16_t* const kRounded[] = {u"dvx", u"dvy", u"dvz", u"acc_x", u"acc_y",
                                             u"acc_z", u"ctrl_x", u"ctrl_y", u"ctrl_z"};
  for (const char16_t* key : kRounded) round_truthy_field(*f, key);

  Object* dataset = member_object(frame, u"dataset");
  if (dataset != nullptr) {
    round_truthy_field(*dataset, u"friction_x");
    round_truthy_field(*dataset, u"friction_z");
    round_truthy_field(*dataset, u"gravity");
  }

  if (Value* itr = member_array(frame, u"itr")) {
    Array* a = as_array(*itr);
    for (size_t i = 0; i < a->size(); ++i) float_scaling_itr(a->at(i));
  }

  if (Object* cp = member_object(frame, u"cpoint")) {
    static const char16_t* const kKeys[] = {u"throwvx", u"throwvy", u"throwvz"};
    scale_fields(*cp, kKeys, 3);
  }
  if (Object* wp = member_object(frame, u"wpoint")) {
    static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz"};
    scale_fields(*wp, kKeys, 3);
  }
  if (Value* opoint = member_array(frame, u"opoint")) {
    static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz", u"speedz"};
    scale_elements(*opoint, kKeys, 4);
  }
}

void scale_actions(Value& actions) {
  traversal(actions, [](const std::u16string&, Value& action) {
    if (!truthy(action)) return;
    Object* a = as_object(action);
    if (a == nullptr) return;
    const Value* eray = a->get(u"e_ray");
    if (eray == nullptr || !truthy(*eray)) return;
    static const char16_t* const kKeys[] = {u"x",     u"z",     u"min_x", u"max_x",
                                            u"min_z", u"max_z", u"max_d"};
    scale_elements(const_cast<Value&>(*eray), kKeys, 7);
  });
}

}

void scale_num_field(Object& o, const char16_t* key) {
  const Value* cur = o.get(std::u16string(key));
  if (cur == nullptr || !is_num(*cur)) return;
  o.set(std::u16string(key), Value(floor(10000.0 * to_number(*cur))));
}

Value float_scaling_entity(Value& ret) {
  Object* r = as_object(ret);
  if (r == nullptr) return ret;

  Value* frames = member_value(ret, u"frames");
  if (frames != nullptr && truthy(*frames)) {
    traversal(*frames, [](const std::u16string&, Value& frame) {
      if (!truthy(frame)) return;
      scale_frame(frame);
    });
  }

  Value* base_value = member_value(ret, u"base");
  Object* base = base_value != nullptr ? as_object(*base_value) : nullptr;
  if (base != nullptr) {
    static const char16_t* const kBaseKeys[] = {
        u"jump_height",   u"jump_distance", u"jump_distancez", u"dash_height",
        u"dash_distance", u"dash_distancez", u"rowing_height", u"rowing_distance",
        u"weight"};
    scale_fields(*base, kBaseKeys, 9);

    Value* brokens = member_value(*base_value, u"brokens");
    if (brokens != nullptr && truthy(*brokens)) {
      static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz", u"speedz"};
      scale_elements(*brokens, kKeys, 4);
    }
    Value* bot = member_value(*base_value, u"bot");
    if (bot != nullptr) {
      Value* actions = member_value(*bot, u"actions");
      if (actions != nullptr && truthy(*actions)) scale_actions(*actions);
    }
  }
  return ret;
}

}
}
