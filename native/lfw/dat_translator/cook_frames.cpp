#include "lfw/dat_translator/cook_frames.h"

#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_num.h"
#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/cookers.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/make_frame_state.h"
#include "lfw/dat_translator/next_frame.h"
#include "lfw/dat_translator/string_matchers.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/all_enums.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/labels.h"
#include "lfw/defines/speed_ctrl.h"
#include "lfw/defines/speed_mode.h"
#include "lfw/utils/math/round_float.h"
#include "lfw/utils/string_help.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {

namespace {

bool is_line_term(char16_t c) {
  return c == u'\n' || c == u'\r' || c == u'\u2028' || c == u'\u2029';
}

bool defined(const Value& v) { return !std::holds_alternative<std::monostate>(v); }

Value to_arr(const std::vector<Value>& items) {
  Array a;
  for (const Value& v : items) a.push_back(v);
  return Value(std::make_shared<Array>(a));
}

struct FrameMatch {
  std::u16string id;
  std::u16string name;
  std::u16string content;
  size_t end = 0;
};

std::optional<FrameMatch> match_frame(const std::u16string& text, size_t from) {
  size_t search = from;
  for (;;) {
    const size_t p0 = text.find(u"<frame>", search);
    if (p0 == std::u16string::npos) return std::nullopt;
    const size_t i = p0 + 7;
    size_t j = i;
    while (j < text.size() && is_str_white_space(text[j])) ++j;
    if (j == i) {
      search = p0 + 1;
      continue;
    }
    size_t k = j;
    while (k < text.size() && !is_str_white_space(text[k])) ++k;
    if (k >= text.size()) {
      search = p0 + 1;
      continue;
    }
    size_t m = k;
    while (m < text.size() && is_str_white_space(text[m])) ++m;
    const size_t e = text.find(u"<frame_end>", m);
    if (e == std::u16string::npos || e < m + 1) {
      search = p0 + 1;
      continue;
    }
    size_t line_end = m;
    while (line_end < text.size() && !is_line_term(text[line_end])) ++line_end;
    const size_t line_len = line_end - m;
    const size_t limit = e - m - 1;
    const size_t name_len = line_len < limit ? line_len : limit;
    if (e - m - name_len < 1) {
      search = p0 + 1;
      continue;
    }
    FrameMatch out;
    out.id = text.substr(j, k - j);
    out.name = text.substr(m, name_len);
    out.content = text.substr(m + name_len, e - m - name_len);
    out.end = e + 10;
    return out;
  }
}

struct Dvxyz {
  Value vxm;
  Value dvx;
  Value ctrl_x;
};

Dvxyz cook_dvxyz(const Value& v) {
  if (!not_zero_num(v)) return Dvxyz{Value(), Value(), Value()};
  if (strict_equals(v, n(550))) {
    return Dvxyz{en(SpeedMode::Fixed), n(0), en(SpeedCtrl::None)};
  }
  return Dvxyz{Value(), n(round_float(to_number(v))), Value()};
}

Value field_of(const Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  return v != nullptr ? *v : Value();
}

double num_of(const Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  return v != nullptr ? to_number(*v) : 0.0;
}

Value parse_colon_fields(const std::u16string& text) {
  Object fields;
  for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(text)) {
    const double num = to_number(Value(kv.second));
    if (is_num(num)) fields.set(kv.first, Value(num));
    else fields.set(kv.first, Value(kv.second));
  }
  return Value(std::make_shared<Object>(fields));
}

bool array_has_len(const Value& v) {
  const Array* a = as_array(v);
  return a != nullptr && a->size() > 0;
}

}

Value cook_frames(const Value& ctx) {
  std::u16string text;
  {
    const Object* co = as_object(ctx);
    if (co == nullptr) return Value(std::make_shared<Object>(Object()));
    const Value* tv = co->get(u"text");
    if (tv == nullptr || !is_str(*tv)) return Value(std::make_shared<Object>(Object()));
    text = std::get<std::u16string>(*tv);
  }

  const Object* ctx_obj = as_object(ctx);
  const Value* base_v = ctx_obj != nullptr ? ctx_obj->get(u"base") : nullptr;
  const Object* base = base_v != nullptr ? as_object(*base_v) : nullptr;
  const Value files_default = Value(std::make_shared<Object>());
  const Value* files_v = base != nullptr ? base->get(u"files") : nullptr;
  const Value files_value = files_v != nullptr ? *files_v : files_default;
  const Object* files = as_object(files_value);

  Object frames;
  size_t pos = 0;
  for (;;) {
    const std::optional<FrameMatch> mm = match_frame(text, pos);
    if (!mm.has_value()) break;
    pos = mm->end;

    const std::u16string frame_id = mm->id;
    const std::u16string frame_name = mm->name;
    std::u16string content = mm->content;

    const TakeSectionsResult r1 = take_sections(content, u"bdy:", u"bdy_end:");
    std::vector<Value> bdy_list = r1.sections;
    content = r1.remains;

    const TakeSectionsResult r2 = take_sections(content, u"itr:", u"itr_end:");
    std::vector<Value> itr_list = r2.sections;
    content = r2.remains;

    const TakeSectionsResult r3 = take_sections(content, u"opoint:", u"opoint_end:");
    std::vector<Value> opoint_list = r3.sections;
    content = r3.remains;

    const TakeSectionsResult r4 = take_sections(content, u"wpoint:", u"wpoint_end:");
    std::vector<Value> wpoint_list = r4.sections;
    content = r4.remains;

    const TakeSectionsResult r5 = take_sections(content, u"bpoint:", u"bpoint_end:");
    std::vector<Value> bpoint_list = r5.sections;
    content = r5.remains;

    const TakeSectionsResult r6 = take_sections(content, u"cpoint:", u"cpoint_end:");
    std::vector<Value> cpoint_list = r6.sections;
    content = r6.remains;

    Value fields_v = parse_colon_fields(content);
    Object* fields = as_object(fields_v);

    const Value raw_next = fields != nullptr ? take(*fields, u"next") : Value();
    const Value next = get_next_frame_by_raw_id(raw_next, u"repeat", u"", nullptr);
    const Value pic_idx = fields != nullptr ? take(*fields, u"pic") : Value();
    Value frame_pic_info;
    const Object* entity_pic_info = nullptr;

    Value pic = pic_idx;
    if (files != nullptr) {
      for (const std::u16string& key : files->keys()) {
        const Value* fv = files->get(key);
        const Object* fo = fv != nullptr ? as_object(*fv) : nullptr;
        entity_pic_info = fo;
        const double row = fo != nullptr ? num_of(*fo, u"row") : 0.0;
        const double col = fo != nullptr ? num_of(*fo, u"col") : 0.0;
        if (to_number(pic) < row * col) break;
        pic = n(to_number(pic) - row * col);
      }
    }

    Value frame_error;
    if (entity_pic_info != nullptr) {
      const double row = num_of(*entity_pic_info, u"row");
      const double cell_w = num_of(*entity_pic_info, u"cell_w");
      const double cell_h = num_of(*entity_pic_info, u"cell_h");
      const Value tex = field_of(*entity_pic_info, u"id");
      frame_pic_info = make_obj({{u"tex", tex},
                                 {u"x", n((cell_w + 1) * std::fmod(to_number(pic), row))},
                                 {u"y", n((cell_h + 1) * js_floor(to_number(pic) / row))},
                                 {u"w", n(cell_w)},
                                 {u"h", n(cell_h)}});
    } else {
      Object err;
      err.set(u"msg", s(u"entity_pic_info not found!"));
      err.set(u"files", files_value);
      err.set(u"pic_idx", pic_idx);
      frame_error = Value(std::make_shared<Object>(err));
    }

    const double wait = to_number(fields != nullptr ? take(*fields, u"wait") : Value()) * 2 + 2;
    Object frame_o;
    frame_o.set(u"id", Value(frame_id));
    frame_o.set(u"name", Value(frame_name));
    frame_o.set(u"pic", frame_pic_info);
    frame_o.set(u"wait", n(wait));
    frame_o.set(u"next", next);
    const Object* pic_obj = as_object(frame_pic_info);
    frame_o.set(u"width", pic_obj != nullptr ? field_of(*pic_obj, u"w") : n(0));
    frame_o.set(u"height", pic_obj != nullptr ? field_of(*pic_obj, u"h") : n(0));
    if (fields != nullptr) {
      for (const std::u16string& k : fields->keys()) {
        const Value* fv = fields->get(k);
        if (fv != nullptr) frame_o.set(k, *fv);
      }
    }

    Value frame = Value(std::make_shared<Object>(frame_o));
    Object* fo = as_object(frame);

    for (Value& bdy : bdy_list) cook_bdy(bdy, frame);
    for (Value& itr : itr_list) cook_itr(itr, frame);
    for (Value& opoint : opoint_list) cook_opoint(opoint, frame);
    for (Value& wpoint : wpoint_list) cook_wpoint(wpoint, frame);
    for (Value& cpoint : cpoint_list) cook_cpoint(cpoint, frame);

    if (!itr_list.empty()) fo->set(u"itr", to_arr(itr_list));
    if (!bdy_list.empty()) fo->set(u"bdy", to_arr(bdy_list));
    if (!opoint_list.empty()) fo->set(u"opoint", to_arr(opoint_list));
    if (!wpoint_list.empty()) fo->set(u"wpoint", wpoint_list[0]);
    if (!cpoint_list.empty()) fo->set(u"cpoint", cpoint_list[0]);
    if (!bpoint_list.empty()) fo->set(u"bpoint", bpoint_list[0]);

    const Value* state_v = fo->get(u"state");
    if (state_v != nullptr) {
      const Value state_name = defines::js_enum_get(u"StateEnum", *state_v);
      if (truthy(state_name)) {
        fo->set(u"state_name", Value(u"StateEnum." + to_string(state_name)));
      }
    }

    if (truthy(frame_error)) fo->set(u"__ERROR__", frame_error);

    const double raw_next_num = to_number(raw_next);
    if ((raw_next_num >= 1100 && raw_next_num <= 1299) ||
        (raw_next_num <= -1100 && raw_next_num >= -1299)) {
      const double vv = 2 * (js_abs(raw_next_num) - 1100);
      fo->set(u"invisible", n(vv));
      fo->set(u"invulnerable", n(vv));
      fo->set(u"blinking", n(vv + 120));
    }

    const Value* itr_field = fo->get(u"itr");
    if (itr_field == nullptr || !array_has_len(*itr_field)) fo->remove(u"itr");
    const Value* bdy_field = fo->get(u"bdy");
    if (bdy_field == nullptr || !array_has_len(*bdy_field)) fo->remove(u"bdy");
    const Value* opoint_field = fo->get(u"opoint");
    if (opoint_field == nullptr || !array_has_len(*opoint_field)) fo->remove(u"opoint");
    const Value* wpoint_field = fo->get(u"wpoint");
    if (wpoint_field == nullptr || !truthy(*wpoint_field)) fo->remove(u"wpoint");
    const Value* bpoint_field = fo->get(u"bpoint");
    if (bpoint_field == nullptr || !truthy(*bpoint_field)) fo->remove(u"bpoint");
    const Value* cpoint_field = fo->get(u"cpoint");
    if (cpoint_field == nullptr || !truthy(*cpoint_field)) fo->remove(u"cpoint");

    const Value sound = take(*fo, u"sound");
    if (is_str(sound)) {
      fo->set(u"sound",
              Value(replace_all(std::get<std::u16string>(sound), u'\\', u'/') + u".mp3"));
    }

    frames.set(frame_id, frame);

    Value dircontrol;
    if (!cpoint_list.empty()) {
      Object* cp = as_object(cpoint_list[0]);
      if (cp != nullptr) dircontrol = take(*cp, u"dircontrol");
    }
    if (truthy(dircontrol)) {
      const Value* hit_field = fo->get(u"hit");
      if (hit_field == nullptr || !truthy(*hit_field)) {
        fo->set(u"hit", Value(std::make_shared<Object>()));
      }
      const Object* hit_c = as_object(*fo->get(u"hit"));
      Object* hit_o = hit_c != nullptr ? const_cast<Object*>(hit_c) : nullptr;
      const Value item = make_obj({{u"wait", s(u"i")}, {u"facing", en(FacingFlag::Backward)}});
      if (hit_o != nullptr && strict_equals(dircontrol, n(1))) {
        const Value* b = hit_o->get(u"B");
        hit_o->set(u"B", add_next_frame(b != nullptr ? *b : Value(), {item}));
      } else if (hit_o != nullptr) {
        const Value* f = hit_o->get(u"F");
        hit_o->set(u"F", add_next_frame(f != nullptr ? *f : Value(), {item}));
      }
    }

    const Value vx = take(*fo, u"dvx");
    const Value vy = take(*fo, u"dvy");
    const Value vz = take(*fo, u"dvz");
    const Dvxyz cx = cook_dvxyz(vx);
    const Dvxyz cy = cook_dvxyz(vy);
    const Dvxyz cz = cook_dvxyz(vz);

    if (defined(cx.vxm)) fo->set(u"vxm", cx.vxm);
    if (defined(cx.dvx)) fo->set(u"dvx", cx.dvx);
    if (defined(cx.ctrl_x)) fo->set(u"ctrl_x", cx.ctrl_x);

    if (strict_equals(vy, n(550))) fo->set(u"gravity_enabled", Value(false));
    if (defined(cy.vxm)) fo->set(u"vym", cy.vxm);
    if (defined(cy.dvx)) fo->set(u"dvy", cy.dvx);
    if (defined(cy.ctrl_x)) fo->set(u"ctrl_y", cy.ctrl_x);

    if (defined(cz.vxm)) fo->set(u"vzm", cz.vxm);
    if (defined(cz.dvx)) fo->set(u"dvz", cz.dvx);
    if (defined(cz.ctrl_x)) fo->set(u"ctrl_z", cz.ctrl_x);
    else if (not_zero_num(vz)) fo->set(u"ctrl_z", n(1));

    const Value* itr_last = fo->get(u"itr");
    if (itr_last != nullptr) {
      Array* arr = const_cast<Array*>(as_array(*itr_last));
      if (arr != nullptr) {
        for (size_t i = 0; i < arr->size(); ++i) {
          Object* io = as_object(arr->at(i));
          if (io == nullptr) continue;
          const Value* kind = io->get(u"kind");
          if (kind == nullptr || !strict_equals(*kind, en(ItrKind::SuperPunchMe))) continue;
          const Value* vrest = io->get(u"vrest");
          const double frame_wait = num_of(*fo, u"wait");
          if (vrest == nullptr || !truthy(*vrest) || to_number(*vrest) < frame_wait) {
            io->set(u"vrest", n(frame_wait));
          }
        }
      }
    }

    make_frame_state(frame);
  }

  return Value(std::make_shared<Object>(frames));
}

}
}
