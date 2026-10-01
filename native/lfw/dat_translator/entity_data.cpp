#include "lfw/dat_translator/entity_data.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/utils/container_help/traversal.h"

namespace lfw {
namespace dat_translator {
namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

std::u16string clean_file_name(const std::u16string& file) {
  std::u16string out;
  for (char16_t c : file) {
    const bool keep = (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z') ||
                      (c >= u'0' && c <= u'9') || c == u'_' || c == u'|';
    if (keep) out.push_back(c);
  }
  return out;
}

void take_frame_keys(Value& frame) {
  Object* o = as_object(frame);
  if (o == nullptr) return;
  take(*o, u"hit_a");
  take(*o, u"hit_j");
  take(*o, u"hit_d");
  take(*o, u"hit_Fa");
  take(*o, u"hit_Fj");
  take(*o, u"hit_Ua");
  take(*o, u"hit_Uj");
  take(*o, u"hit_Da");
  take(*o, u"hit_Dj");
  take(*o, u"hit_Ua");
  take(*o, u"hit_ja");
}

}

void make_frames_special(Value& ret) {
  Object* p = as_object(ret);
  if (p == nullptr) return;
  const Value* frames = p->get(u"frames");
  if (frames == nullptr) return;
  Value f = *frames;
  if (Array* a = as_array(f)) {
    const size_t n = a->size();
    for (size_t i = 0; i < n; ++i) take_frame_keys(a->at(i));
    return;
  }
  traversal(f, [](const std::u16string&, Value& item) { take_frame_keys(item); });
}

void make_entity_special(Value& ret) { (void)ret; }

void post_process_obj_data(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return;
  const Value* data_ptr = c->get(u"data");
  if (data_ptr == nullptr || !truthy(*data_ptr)) return;
  Value data = *data_ptr;
  make_frames_special(data);
  const Value* index_ptr = c->get(u"index");
  const Value index = index_ptr != nullptr ? *index_ptr : Value();
  const Object* idx = as_object(index);
  const Value* groups = idx != nullptr ? idx->get(u"groups") : nullptr;
  if (groups == nullptr || !truthy(*groups)) return;
  Object* dobj = as_object(data);
  if (dobj == nullptr) return;
  const Value* base = dobj->get(u"base");
  Value base_copy = base != nullptr ? *base : Value();
  Object* bobj = as_object(base_copy);
  if (bobj == nullptr) return;
  bobj->set(u"group", *groups);
}

Value make_entity_data(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return Value();
  const Value* base = c->get(u"base");
  const Value* frames = c->get(u"frames");
  const Value* index = c->get(u"index");
  Value info = base != nullptr ? *base : Value();
  const Value frames_value = frames != nullptr ? *frames : Value();
  const Value index_value = index != nullptr ? *index : Value();
  const Object* idx = as_object(index_value);
  Object* info_obj = as_object(info);
  if (info_obj != nullptr) {
    const Value* hash = idx != nullptr ? idx->get(u"hash") : nullptr;
    if (hash != nullptr && !is_nullish(*hash)) {
      info_obj->set(u"name", *hash);
    } else {
      const Value* file = idx != nullptr ? idx->get(u"file") : nullptr;
      info_obj->set(u"name", Value(clean_file_name(file != nullptr ? to_string(*file)
                                                                  : std::u16string())));
    }
  }
  const Value* id = idx != nullptr ? idx->get(u"id") : nullptr;
  Object out;
  out.set(u"id", id != nullptr ? *id : Value());
  out.set(u"type", en(EntityEnum::Entity));
  out.set(u"base", info);
  out.set(u"frames", frames_value);
  Value ret(std::make_shared<Object>(out));
  make_entity_special(ret);
  return ret;
}

}
}
