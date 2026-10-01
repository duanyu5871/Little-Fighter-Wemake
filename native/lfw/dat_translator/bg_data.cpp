#include "lfw/dat_translator/bg_data.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/js_num.h"
#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/colon_value_reader.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/string_matchers.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/background_group.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/string_help.h"
#include "lfw/utils/type_cast.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {
namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

double num_of(const Value& v) { return is_nullish(v) ? 0.0 : to_number(v); }

Value array_at(const Value& v, size_t i) {
  const Array* a = as_array(v);
  if (a == nullptr || i >= a->size()) return Value();
  return a->at(i);
}

double array_num(const Value& v, size_t i) {
  const Array* a = as_array(v);
  if (a == nullptr || i >= a->size()) return std::numeric_limits<double>::quiet_NaN();
  return to_number(a->at(i));
}

Value dat_index_id(const Value& index) {
  const Object* o = as_object(index);
  const Value* v = o != nullptr ? o->get(u"id") : nullptr;
  return v != nullptr ? *v : Value();
}

std::u16string dat_index_file(const Value& index) {
  const Object* o = as_object(index);
  const Value* v = o != nullptr ? o->get(u"file") : nullptr;
  return v != nullptr && is_str(*v) ? std::get<std::u16string>(*v) : std::u16string();
}

bool starts_with(const std::u16string& s, const std::u16string& prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::u16string replace_double_back_slash(const std::u16string& s) {
  std::u16string out;
  size_t i = 0;
  while (i < s.size()) {
    if (s[i] == u'\\' && i + 1 < s.size() && s[i + 1] == u'\\') {
      out.push_back(u'/');
      i += 2;
    } else {
      out.push_back(s[i]);
      ++i;
    }
  }
  return out;
}

std::u16string replace_single_back_slash(const std::u16string& s) {
  std::u16string out;
  for (char16_t c : s) out.push_back(c == u'\\' ? u'/' : c);
  return out;
}

std::u16string replace_bmp_suffix(const std::u16string& s) {
  if (s.size() >= 4 && s.compare(s.size() - 3, 3, u"bmp") == 0) {
    return s.substr(0, s.size() - 4) + u".png";
  }
  return s;
}

std::u16string rgb_of(int32_t r, int32_t g, int32_t b, int32_t bit5) {
  const int32_t r2 = r + ((r > 64 || r == 0) ? 7 : 0);
  const int32_t g2 = g + ((g > 64 || g == 0) ? 7 : 0) + ((bit5 != 0 && g > 80) ? 4 : 0);
  const int32_t b2 = b + ((b > 64 || b == 0) ? 7 : 0);
  std::u16string out = u"rgb(";
  out += to_string(Value(static_cast<double>(r2)));
  out.push_back(u',');
  out += to_string(Value(static_cast<double>(g2)));
  out.push_back(u',');
  out += to_string(Value(static_cast<double>(b2)));
  out.push_back(u')');
  return out;
}

std::u16string bg_color_translate(const Value& rect) {
  const std::u16string key = to_string(rect);
  if (key == u"4706") return u"rgb(16,79,16)";
  if (key == u"16835") return u"rgb(66,56,24)";
  if (key == u"21096") return u"rgb(90,78,75)";
  if (key == u"25356") return u"rgb(103,103,103)";
  if (key == u"29582") return u"rgb(119,119,119)";
  if (key == u"33580") return u"rgb(135,107,103)";
  if (key == u"37770") return u"rgb(154,110,90)";
  if (key == u"37773") return u"rgb(151,119,111)";
  if (key == u"34816") return u"rgb(143,7,7)";
  if (key == u"40179") return u"rgb(159,163,159)";
  if (key == u"40179b") return u"rgb(159,163,159)";
  if (is_str(rect)) return std::get<std::u16string>(rect);
  const int32_t n = js_to_int32(to_number(rect));
  const int32_t r = (n >> 11) << 3;
  const int32_t g = ((n >> 6) & 31) << 3;
  const int32_t b = (n & 31) << 3;
  return rgb_of(r, g, b, (n >> 5) & 1);
}

double field_num(const Object& o, const char16_t* key, double fallback) {
  const Value* v = o.get(std::u16string(key));
  if (v == nullptr) return fallback;
  const double n = to_number(*v);
  return n;
}

Value field_value(const Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  return v != nullptr ? *v : Value();
}

Value make_bg_layer(const std::u16string& block_str) {
  const std::vector<std::u16string> pieces = split_lines(js_trim(block_str));
  std::vector<std::u16string> kept;
  for (const std::u16string& p : pieces) {
    if (!p.empty()) kept.push_back(js_trim(p));
  }
  const std::u16string file = kept.size() > 0 ? kept[0] : std::u16string();
  const std::u16string remains = kept.size() > 1 ? kept[1] : std::u16string();

  Object fields;
  for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(remains)) {
    const Value v(kv.second);
    const std::optional<double> n = to_num(v);
    fields.set(kv.first, n.has_value() ? Value(*n) : v);
  }
  take(fields, u"transparency");
  const Value y = take(fields, u"y");

  Value layer = bg_layer_info_new();
  Object* lo = as_object(layer);
  const Value id = field_value(fields, u"id");
  const Value name = field_value(fields, u"name");
  const Value rect = field_value(fields, u"rect");
  const Value width = field_value(fields, u"width");
  const Value height = field_value(fields, u"height");
  const Value x = field_value(fields, u"x");
  const Value w = field_value(fields, u"w");
  const Value h = field_value(fields, u"h");
  const Value loop = field_value(fields, u"loop");
  const Value cc = field_value(fields, u"cc");
  const Value c1 = field_value(fields, u"c1");
  const Value c2 = field_value(fields, u"c2");
  if (truthy(id)) lo->set(u"id", Value(to_string(id)));
  if (truthy(name)) lo->set(u"name", Value(to_string(name)));
  if (truthy(rect)) {
    lo->set(u"file", Value());
    lo->set(u"absolute", n(1));
    lo->set(u"color", Value(bg_color_translate(rect)));
  } else {
    lo->set(u"file", Value(replace_single_back_slash(replace_bmp_suffix(file))));
    lo->set(u"absolute", Value());
    lo->set(u"color", Value());
  }
  lo->set(u"width", is_nullish(width) ? n(0) : width);
  lo->set(u"height", is_nullish(height) ? n(0) : height);
  lo->set(u"x", is_num(x) ? x : n(0));
  lo->set(u"y", n(defines::num(u"Defines.CLASSIC_SCREEN_HEIGHT") - num_of(y)));
  lo->set(u"z", n(0));
  lo->set(u"w", is_num(w) ? w : field_value(*lo, u"width"));
  lo->set(u"h", is_num(h) ? h : field_value(*lo, u"height"));
  lo->set(u"loop", is_nullish(loop) ? Value() : loop);
  lo->set(u"cc", is_num(cc) ? n(to_number(cc) * 2) : Value());
  lo->set(u"c1", is_num(c1) ? n(to_number(c1) * 2) : Value());
  lo->set(u"c2", is_num(c2) ? n(to_number(c2) * 2 + 1) : Value());
  reorder_fields(layer, bg_layer_info_fields());
  delete_undefined(layer);
  return layer;
}

}

Value make_bg_data(const Value& full_str, const Value& dat_index) {
  const std::u16string text = replace_double_back_slash(to_string(full_str));
  Object fields;
  ColonValueReader reader;
  reader.str(u"name").int_(u"width").int_2(u"zboundary").str(u"shadow").int_2(u"shadowsize");
  reader.read(text, fields);

  Value ret = bg_data_new();
  Object* ro = as_object(ret);
  const Value* base_ptr = ro != nullptr ? ro->get(u"base") : nullptr;
  Value info = base_ptr != nullptr ? *base_ptr : Value();
  Object* io = as_object(info);
  if (io != nullptr) {
    io->set(u"name", field_value(fields, u"name"));
    io->set(u"shadow", field_value(fields, u"shadow"));
    io->set(u"shadow_w", array_at(field_value(fields, u"shadowsize"), 0));
    io->set(u"shadow_h", array_at(field_value(fields, u"shadowsize"), 1));
    io->set(u"group", make_arr({s(background_group::kRegular)}));
    io->set(u"right", field_value(fields, u"width"));
    const double screen = defines::num(u"Defines.CLASSIC_SCREEN_HEIGHT");
    const Value zb = field_value(fields, u"zboundary");
    io->set(u"far", n(2 * (array_num(zb, 0) - screen)));
    io->set(u"near", n(2 * (array_num(zb, 1) - screen)));
  }
  const Value index_id = dat_index_id(dat_index);
  ro->set(u"id", is_nullish(index_id) ? field_value(fields, u"name") : index_id);
  ro->set(u"base", info);
  ro->set(u"layers", Value(std::make_shared<Array>(Array())));
  Object* base_obj = as_object(info);
  if (base_obj != nullptr) {
    const Value nm = field_value(*base_obj, u"name");
    if (is_str(nm)) base_obj->set(u"name", Value(replace_all(std::get<std::u16string>(nm), u'_', u' ')));
    const Value sh = field_value(*base_obj, u"shadow");
    if (is_str(sh)) {
      base_obj->set(u"shadow", Value(replace_single_back_slash(
                                     replace_bmp_suffix(std::get<std::u16string>(sh)))));
    }
  }
  const TakeBlocksResult blocks = take_blocks(text, u"layer:", u"layer_end");
  Array layers;
  for (const std::u16string& block_str : blocks.blocks) {
    Value layer = make_bg_layer(block_str);
    Object* lo = as_object(layer);
    if (lo != nullptr) lo->set(u"z", n(static_cast<double>(layers.size()) -
                                     static_cast<double>(blocks.blocks.size())));
    layers.push_back(layer);
  }
  ro->set(u"layers", Value(std::make_shared<Array>(layers)));
  if (base_obj != nullptr) {
    const std::u16string file = dat_index_file(dat_index);
    if (starts_with(file, u"bg/template")) {
      base_obj->set(u"group", Value(std::make_shared<Array>(Array())));
    } else {
      base_obj->set(u"group", make_arr({s(background_group::kRegular)}));
    }
  }
  reorder_fields(ret, bg_data_fields());
  return ret;
}

}
}
