#include "lfw/dat_translator/xml/xml_x_frame_indexes.h"

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

// `indexes.falling?.[1]`：子对象在才取 `"1"` / `"-1"` 键，缺键给 undefined。
Value pair_at(const Value& pair, const char16_t* key) { return field_or(pair, key); }

// `ret.falling = { [1]: a, [-1]: b }`（a、b 都真值才给）。
Value pair_of(const std::optional<std::vector<std::u16string>>& a,
              const std::optional<std::vector<std::u16string>>& b) {
  if (!a || !b) return Value();
  auto o = std::make_shared<Object>();
  o->set(u"1", from_opt(a));
  o->set(u"-1", from_opt(b));
  return Value(std::move(o));
}

// `injured`/`lying` 那两对走的是 `get_str`（单个字符串）—— TS 就是这么写的；
// `if (a && b)` 对空串也算假（与写向的数组对不一样）。
Value pair_of_str(const std::optional<std::u16string>& a, const std::optional<std::u16string>& b) {
  if (!a || !b || a->empty() || b->empty()) return Value();
  auto o = std::make_shared<Object>();
  o->set(u"1", Value(*a));
  o->set(u"-1", Value(*b));
  return Value(std::move(o));
}

}  // namespace

std::shared_ptr<IXMLElement> xml_x_frame_indexes(IXML& xml, const Value& indexes,
                                                 const std::u16string& tag) {
  if (!truthy(indexes)) return nullptr;
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"default", field_or(indexes, u"default"));
  ret->set_attr(u"heavy_obj_walk", field_or(indexes, u"heavy_obj_walk"));
  ret->set_attr(u"landing_1", field_or(indexes, u"landing_1"));
  ret->set_attr(u"landing_2", field_or(indexes, u"landing_2"));
  ret->set_attr(u"dizzy", field_or(indexes, u"dizzy"));
  ret->set_attr(u"in_the_skys", field_or(indexes, u"in_the_skys"));
  ret->set_attr(u"throwings", field_or(indexes, u"throwings"));
  ret->set_attr(u"on_hands", field_or(indexes, u"on_hands"));
  ret->set_attr(u"falling_1", pair_at(field_or(indexes, u"falling"), u"1"));
  ret->set_attr(u"falling_2", pair_at(field_or(indexes, u"falling"), u"-1"));
  ret->set_attr(u"bouncing_1", pair_at(field_or(indexes, u"bouncing"), u"1"));
  ret->set_attr(u"bouncing_2", pair_at(field_or(indexes, u"bouncing"), u"-1"));
  ret->set_attr(u"critical_hit_1", pair_at(field_or(indexes, u"critical_hit"), u"1"));
  ret->set_attr(u"critical_hit_2", pair_at(field_or(indexes, u"critical_hit"), u"-1"));
  ret->set_attr(u"injured_1", pair_at(field_or(indexes, u"injured"), u"1"));
  ret->set_attr(u"injured_2", pair_at(field_or(indexes, u"injured"), u"-1"));
  ret->set_attr(u"grand_injured_1", pair_at(field_or(indexes, u"grand_injured"), u"1"));
  ret->set_attr(u"grand_injured_2", pair_at(field_or(indexes, u"grand_injured"), u"-1"));
  ret->set_attr(u"lying_1", pair_at(field_or(indexes, u"lying"), u"1"));
  ret->set_attr(u"lying_2", pair_at(field_or(indexes, u"lying"), u"-1"));
  ret->set_attr(u"fire", field_or(indexes, u"fire"));
  ret->set_attr(u"ice", field_or(indexes, u"ice"));
  ret->set_attr(u"on_ground", field_or(indexes, u"on_ground"));
  ret->set_attr(u"just_on_ground", field_or(indexes, u"just_on_ground"));
  ret->set_attr(u"throw_on_ground", field_or(indexes, u"throw_on_ground"));
  return ret;
}

Value xml_2_frame_indexes(const IXMLElement* el) {
  if (el == nullptr) return Value();
  Value ret = frame_indexes_new();
  Object* const o = as_object(ret);
  o->set(u"default", from_opt(get_str_or(*el, u"default", field_or(*o, u"default"))));
  o->set(u"heavy_obj_walk",
         from_opt(get_str_or(*el, u"heavy_obj_walk", field_or(*o, u"heavy_obj_walk"))));
  o->set(u"landing_1", from_opt(get_str_or(*el, u"landing_1", field_or(*o, u"landing_1"))));
  o->set(u"landing_2", from_opt(get_str_or(*el, u"landing_2", field_or(*o, u"landing_2"))));
  o->set(u"dizzy", from_opt(get_str_or(*el, u"dizzy", field_or(*o, u"dizzy"))));
  o->set(u"in_the_skys", from_opt(el->get_str_arr(u"in_the_skys")));
  o->set(u"throwings", from_opt(el->get_str_arr(u"throwings")));
  o->set(u"on_hands", from_opt(el->get_str_arr(u"on_hands")));
  {
    const Value v = pair_of(el->get_str_arr(u"falling_1"), el->get_str_arr(u"falling_2"));
    if (truthy(v)) o->set(u"falling", v);
  }
  {
    const Value v = pair_of(el->get_str_arr(u"bouncing_1"), el->get_str_arr(u"bouncing_2"));
    if (truthy(v)) o->set(u"bouncing", v);
  }
  {
    const Value v = pair_of(el->get_str_arr(u"critical_hit_1"), el->get_str_arr(u"critical_hit_2"));
    if (truthy(v)) o->set(u"critical_hit", v);
  }
  {
    const Value v = pair_of_str(el->get_str(u"injured_1"), el->get_str(u"injured_2"));
    if (truthy(v)) o->set(u"injured", v);
  }
  {
    const Value v = pair_of(el->get_str_arr(u"grand_injured_1"), el->get_str_arr(u"grand_injured_2"));
    if (truthy(v)) o->set(u"grand_injured", v);
  }
  {
    const Value v = pair_of_str(el->get_str(u"lying_1"), el->get_str(u"lying_2"));
    if (truthy(v)) o->set(u"lying", v);
  }
  o->set(u"fire", from_opt(el->get_str_arr(u"fire")));
  o->set(u"ice", from_opt(get_str_or(*el, u"ice", field_or(*o, u"ice"))));
  o->set(u"on_ground", from_opt(get_str_or(*el, u"on_ground", field_or(*o, u"on_ground"))));
  o->set(u"just_on_ground",
         from_opt(get_str_or(*el, u"just_on_ground", field_or(*o, u"just_on_ground"))));
  o->set(u"throw_on_ground",
         from_opt(get_str_or(*el, u"throw_on_ground", field_or(*o, u"throw_on_ground"))));
  delete_undefined(ret);
  reorder_fields(ret, frame_indexes_fields());
  return ret;
}

}
}
}
