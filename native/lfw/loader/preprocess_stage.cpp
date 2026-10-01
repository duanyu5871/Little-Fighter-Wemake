#include "lfw/loader/preprocess_stage.h"

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/fields.h"

namespace lfw {
namespace loader {

Value preprocess_stage_phase(Value& v) {
  Object* o = as_object(v);
  if (o == nullptr) return v;
  dat_translator::delete_undefined(v);
  reorder_fields(v, stage_phase_info_fields());
  return v;
}

Value preprocess_stage(Value& v) {
  Object* o = as_object(v);
  if (o == nullptr) return v;
  const Value* phases = o->get(u"phases");
  if (phases != nullptr) {
    if (Array* a = const_cast<Array*>(as_array(*phases))) {
      const size_t n = a->size();
      for (size_t i = 0; i < n; ++i) {
        Value item = a->at(i);
        preprocess_stage_phase(item);
      }
    }
  }
  dat_translator::delete_undefined(v);
  reorder_fields(v, stage_info_fields());
  return v;
}

}
}
