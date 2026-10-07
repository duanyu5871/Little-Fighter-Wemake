#include "lfw/loader/preprocess_bg_data.h"

#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/schemas_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/schema/validate_schema.h"

namespace lfw {
namespace loader {

namespace {

Object* as_mut(const Value& v) { return const_cast<Object*>(as_object(v)); }

Value field_at(const Value& v, const char16_t* key) {
  const Object* o = as_object(v);
  if (o == nullptr) return Value();
  const Value* p = o->get(std::u16string(key));
  return p != nullptr ? *p : Value();
}

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

bool is_number(const Value& v) { return std::holds_alternative<double>(v); }

void or_assign(Object& target, const char16_t* key, const Value& value) {
  const Value* cur = target.get(std::u16string(key));
  if (cur == nullptr || is_nullish(*cur)) target.set(std::u16string(key), value);
}

double num_or_zero(const Value& v) { return is_number(v) ? std::get<double>(v) : 0.0; }

Value default_screen_height() {
  const Value* v = defines::find(std::u16string(u"Defines.MODERN_SCREEN_HEIGHT"));
  return v != nullptr ? *v : Value();
}

}

Value preprocess_bg_data(Value& data, std::vector<std::u16string>* warnings,
                         std::vector<std::u16string>* errors) {
  Object* root = as_object(data);
  if (root == nullptr) return data;

  Value base_v = field_at(data, u"base");
  if (Object* base = as_mut(base_v)) {
    reorder_fields(base_v, bg_info_fields());
    dat_translator::delete_undefined(base_v);
    base = as_mut(base_v);
    if (base != nullptr) or_assign(*base, u"height", default_screen_height());
  }

  const Value dataset_v = field_at(data, u"dataset");
  if (truthy(dataset_v)) {
    Value ds = dataset_v;
    reorder_fields(ds, world_dataset_fields());
    dat_translator::delete_undefined(ds);
  }

  const Value layers_v = field_at(data, u"layers");
  if (const Array* layers = as_array(layers_v)) {
    const size_t n = layers->size();
    for (size_t i = 0; i < n; ++i) {
      Value layer = layers->at(i);
      reorder_fields(layer, bg_layer_info_fields());
      dat_translator::delete_undefined(layer);
    }
  }

  const Value terrain_v = field_at(data, u"terrain");
  if (const Array* terrain = as_array(terrain_v)) {
    const size_t n = terrain->size();
    for (size_t i = 0; i < n; ++i) {
      Value t = terrain->at(i);
      reorder_fields(t, terrain_info_fields());
      dat_translator::delete_undefined(t);
      schema::SchemaValidator& sv = schema::SchemaValidator::Default();
      sv.validate(t, schema_i_terrain_info());
      if (!sv.warnings().empty() && warnings != nullptr) {
        warnings->insert(warnings->end(), sv.warnings().begin(), sv.warnings().end());
      }
      if (!sv.errors().empty() && errors != nullptr) {
        errors->insert(errors->end(), sv.errors().begin(), sv.errors().end());
      }
      sv.reset();
    }
  }

  Object* base = as_mut(field_at(data, u"base"));
  if (base != nullptr) {
    const Value shadowsize = field_at(field_at(data, u"base"), u"shadowsize");
    if (const Array* sa = as_array(shadowsize)) {
      const Value a = sa->size() > 0 ? sa->at(0) : Value();
      const Value b = sa->size() > 1 ? sa->at(1) : Value();
      or_assign(*base, u"shadow_w", Value(num_or_zero(a)));
      or_assign(*base, u"shadow_h", Value(num_or_zero(b)));
    }
    const Value zoom = field_at(field_at(data, u"base"), u"zoom");
    if (const Array* za = as_array(zoom)) {
      const Value a = za->size() > 0 ? za->at(0) : Value();
      const Value b = za->size() > 1 ? za->at(1) : Value();
      const Value c = za->size() > 2 ? za->at(2) : Value();
      or_assign(*base, u"zoom_x", Value(num_or_zero(a)));
      or_assign(*base, u"zoom_y", Value(num_or_zero(b)));
      or_assign(*base, u"zoom_z", Value(num_or_zero(c)));
    }
  }

  reorder_fields(data, bg_data_fields());
  dat_translator::delete_undefined(data);
  return data;
}

}
}
