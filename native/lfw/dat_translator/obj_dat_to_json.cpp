#include "lfw/dat_translator/obj_dat_to_json.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/cook_frames.h"
#include "lfw/dat_translator/entity_data.h"
#include "lfw/dat_translator/entity_kinds.h"
#include "lfw/dat_translator/make_fighter_data.h"
#include "lfw/dat_translator/string_matchers.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/utils/container_help/set_obj_field.h"
#include "lfw/utils/string_help.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace dat_translator {

namespace {

bool is_ns(char16_t c) { return !is_str_white_space(c); }

bool is_digit(char16_t c) { return c >= u'0' && c <= u'9'; }

std::vector<std::u16string> split_lines_raw(const std::u16string& s) {
  std::vector<std::u16string> out;
  std::u16string cur;
  for (char16_t c : s) {
    if (c == u'\n') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

std::u16string double_backslash_to_slash(const std::u16string& s) {
  std::u16string t;
  t.reserve(s.size());
  size_t i = 0;
  while (i < s.size()) {
    if (i + 1 < s.size() && s[i] == u'\\' && s[i + 1] == u'\\') {
      t.push_back(u'/');
      i += 2;
    } else {
      t.push_back(s[i]);
      ++i;
    }
  }
  return t;
}

Value field_at(const Value& v, const char16_t* key) {
  const Object* o = as_object(v);
  if (o == nullptr) return Value();
  const Value* p = o->get(std::u16string(key));
  return p != nullptr ? *p : Value();
}

bool starts_with(const std::u16string& s, const char16_t* prefix) {
  const std::u16string p(prefix);
  return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

bool bmp_to_png_match(const std::u16string& s) {
  return s.size() >= 4 && s.compare(s.size() - 3, 3, u"bmp") == 0;
}

std::u16string bmp_to_png(const std::u16string& s) {
  if (!bmp_to_png_match(s)) return s;
  return s.substr(0, s.size() - 4) + std::u16string(u".png");
}

std::u16string slash_to_forward(const std::u16string& s) {
  std::u16string t = s;
  for (char16_t& c : t) {
    if (c == u'\\') c = u'/';
  }
  return t;
}

bool match_key_word(const std::u16string& s, const char16_t* key, std::u16string& out) {
  const std::u16string k(key);
  const size_t p = s.find(k);
  if (p == std::u16string::npos) return false;
  size_t i = p + k.size();
  while (i < s.size() && is_str_white_space(s[i])) ++i;
  size_t j = i;
  while (j < s.size() && is_ns(s[j])) ++j;
  out = s.substr(i, j - i);
  return true;
}

bool match_number(const std::u16string& s, size_t pos, size_t& end) {
  size_t i = pos;
  if (i < s.size() && (s[i] == u'+' || s[i] == u'-')) ++i;
  const size_t sign_end = i;
  size_t k = i;
  while (k < s.size() && is_digit(s[k])) ++k;
  if (k < s.size() && s[k] == u'.') {
    size_t d = k + 1;
    size_t d_end = d;
    while (d_end < s.size() && is_digit(s[d_end])) ++d_end;
    if (d_end > d) {
      end = d_end;
      return true;
    }
  }
  size_t d_end = sign_end;
  while (d_end < s.size() && is_digit(s[d_end])) ++d_end;
  if (d_end > sign_end) {
    end = d_end;
    return true;
  }
  return false;
}

bool match_key_colon_number(const std::u16string& s, std::u16string& key, size_t& num_start,
                            size_t& num_end) {
  for (size_t p = 0; p <= s.size(); ++p) {
    size_t m = 0;
    while (p + m < s.size() && is_ns(s[p + m])) ++m;
    for (size_t g = m + 1; g-- > 0;) {
      size_t i = p + g;
      while (i < s.size() && is_str_white_space(s[i])) ++i;
      if (i < s.size() && s[i] == u':') {
        ++i;
        while (i < s.size() && is_str_white_space(s[i])) ++i;
        size_t e = 0;
        if (match_number(s, i, e)) {
          key = s.substr(p, g);
          num_start = i;
          num_end = e;
          return true;
        }
      }
    }
  }
  return false;
}

bool match_key_colon_value(const std::u16string& s, std::u16string& key, std::u16string& out) {
  for (size_t p = 0; p <= s.size(); ++p) {
    size_t m = 0;
    while (p + m < s.size() && is_ns(s[p + m])) ++m;
    for (size_t g = m + 1; g-- > 0;) {
      size_t i = p + g;
      while (i < s.size() && is_str_white_space(s[i])) ++i;
      if (i < s.size() && s[i] == u':') {
        ++i;
        while (i < s.size() && is_str_white_space(s[i])) ++i;
        size_t j = i;
        while (j < s.size() && is_ns(s[j])) ++j;
        key = s.substr(p, g);
        out = s.substr(i, j - i);
        return true;
      }
    }
  }
  return false;
}

bool match_key_number(const std::u16string& s, std::u16string& key, size_t& num_start,
                      size_t& num_end) {
  for (size_t p = 0; p <= s.size(); ++p) {
    size_t m = 0;
    while (p + m < s.size() && is_ns(s[p + m])) ++m;
    for (size_t g = m + 1; g-- > 0;) {
      size_t i = p + g;
      while (i < s.size() && is_str_white_space(s[i])) ++i;
      size_t e = 0;
      if (match_number(s, i, e)) {
        key = s.substr(p, g);
        num_start = i;
        num_end = e;
        return true;
      }
    }
  }
  return false;
}

bool match_key_value(const std::u16string& s, std::u16string& key, std::u16string& out) {
  for (size_t p = 0; p <= s.size(); ++p) {
    size_t m = 0;
    while (p + m < s.size() && is_ns(s[p + m])) ++m;
    for (size_t g = m + 1; g-- > 0;) {
      size_t i = p + g;
      while (i < s.size() && is_str_white_space(s[i])) ++i;
      size_t j = i;
      while (j < s.size() && is_ns(s[j])) ++j;
      key = s.substr(p, g);
      out = s.substr(i, j - i);
      return true;
    }
  }
  return false;
}

}

ObjDatToJsonResult obj_dat_to_json(const std::u16string& text_in, const Value& dat_index) {
  ObjDatToJsonResult res;
  const std::u16string text = double_backslash_to_slash(text_in);
  const std::optional<std::u16string> block =
      match_block_once(text, std::u16string(u"<bmp_begin>"), std::u16string(u"<bmp_end>"));
  if (!block.has_value()) {
    res.error = u"[dat_to_json] failed, 3";
    return res;
  }

  Object base;
  base.set(u"name", Value(std::u16string()));
  base.set(u"files", Value(std::make_shared<Object>()));

  Value ctx = make_obj({{u"index", dat_index},
                        {u"base", Value(std::make_shared<Object>(base))},
                        {u"text", Value(text)},
                        {u"frames", Value(std::make_shared<Object>())},
                        {u"data", Value()}});
  Object* ctx_o = as_object(ctx);
  Object* base_o = ctx_o != nullptr ? const_cast<Object*>(as_object(field_at(ctx, u"base"))) : nullptr;
  if (base_o == nullptr) {
    res.error = u"[dat_to_json] failed, 3";
    return res;
  }

  const std::u16string body = js_trim(*block);
  for (const std::u16string& line : split_lines_raw(body)) {
    std::u16string word;
    if (match_key_word(line, u"name:", word)) {
      base_o->set(u"name", Value(word));
      continue;
    }
    if (match_key_word(line, u"head:", word)) {
      base_o->set(u"head", Value(slash_to_forward(bmp_to_png(word))));
      continue;
    }
    if (match_key_word(line, u"small:", word)) {
      base_o->set(u"small", Value(slash_to_forward(bmp_to_png(word))));
      continue;
    }
    if (starts_with(line, u"file(")) {
      const Value* files_v = base_o->get(u"files");
      const Object* files_o = files_v != nullptr ? as_object(*files_v) : nullptr;
      const size_t file_id = files_o != nullptr ? files_o->keys().size() : 0;
      Object file;
      file.set(u"id", Value(number_to_string(static_cast<double>(file_id))));
      file.set(u"path", Value(std::u16string()));
      file.set(u"row", Value(0.0));
      file.set(u"col", Value(0.0));
      file.set(u"cell_w", Value(0.0));
      file.set(u"cell_h", Value(0.0));
      for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(line)) {
        const std::u16string& key = kv.first;
        const std::u16string& value = kv.second;
        if (starts_with(key, u"file")) {
          file.set(u"path", Value(slash_to_forward(bmp_to_png(value))));
        } else if (key == u"w") {
          file.set(u"cell_w", to_number(Value(value)));
        } else if (key == u"h") {
          file.set(u"cell_h", to_number(Value(value)));
        } else {
          file.set(key, to_number(Value(value)));
        }
      }
      Value files_holder = files_v != nullptr ? *files_v : Value();
      const std::u16string file_key = number_to_string(static_cast<double>(file_id));
      base_o->set(u"files",
                  set_obj_field(files_holder, file_key, Value(std::make_shared<Object>(file))));
      continue;
    }

    std::u16string key;
    size_t ns = 0;
    size_t ne = 0;
    if (match_key_colon_number(line, key, ns, ne)) {
      base_o->set(key, to_number(Value(line.substr(ns, ne - ns))));
      continue;
    }
    std::u16string value;
    if (match_key_colon_value(line, key, value)) {
      base_o->set(key, Value(value));
      continue;
    }
    if (match_key_number(line, key, ns, ne)) {
      base_o->set(key, to_number(Value(line.substr(ns, ne - ns))));
      continue;
    }
    if (match_key_value(line, key, value)) {
      base_o->set(key, Value(value));
      continue;
    }
  }

  ctx_o = as_object(ctx);
  const Value frames = cook_frames(ctx);
  if (ctx_o != nullptr) ctx_o->set(u"frames", frames);

  const std::u16string type = to_string(field_at(dat_index, u"type"));
  Value data;
  if (type == u"0") {
    data = make_fighter_data(ctx);
  } else if (type == u"1" || type == u"2" || type == u"4" || type == u"6") {
    data = make_weapon_data(ctx);
  } else if (type == u"3") {
    data = make_ball_data(ctx);
  } else {
    data = make_entity_data(ctx);
  }
  ctx_o = as_object(ctx);
  if (ctx_o != nullptr) ctx_o->set(u"data", data);
  post_process_obj_data(ctx);

  res.ok = true;
  res.data = data;
  return res;
}

}
}
