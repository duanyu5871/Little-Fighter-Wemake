#include "lfw/dat_translator/parase_indexes.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/string_matchers.h"
#include "lfw/defines/i_dat_index.h"
#include "lfw/defines/oid.h"
#include "lfw/utils/string_help.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {

namespace {

bool is_line_term(char16_t c) {
  return c == u'\n' || c == u'\r' || c == u'\u2028' || c == u'\u2029';
}

std::u16string replace_dat_suffix(const std::u16string& s, const std::u16string& ext) {
  if (s.size() < 4) return s;
  if (is_line_term(s[s.size() - 4])) return s;
  if (s.compare(s.size() - 3, 3, u"dat") != 0) return s;
  return s.substr(0, s.size() - 4) + ext;
}

std::u16string replace_txt_suffix(const std::u16string& s, const std::u16string& ext) {
  if (s.size() < 4) return s;
  if (is_line_term(s[s.size() - 4])) return s;
  if (s.compare(s.size() - 3, 3, u"txt") != 0) return s;
  return s.substr(0, s.size() - 4) + ext;
}

bool ends_with(const std::u16string& s, const std::u16string& tail) {
  return s.size() >= tail.size() && s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
}

std::vector<std::u16string> split_commas(const std::u16string& s) {
  std::vector<std::u16string> out;
  std::u16string cur;
  for (char16_t c : s) {
    if (c == u',') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

std::vector<std::u16string> non_empty_lines(const std::u16string& block) {
  std::vector<std::u16string> out;
  for (const std::u16string& line : split_lines(block)) {
    if (!line.empty()) out.push_back(line);
  }
  return out;
}

bool starts_with_hash(const std::u16string& s) { return !s.empty() && s[0] == u'#'; }

Value make_str_array(const std::vector<std::u16string>& items) {
  Array a;
  for (const std::u16string& s : items) a.push_back(Value(s));
  return Value(std::make_shared<Array>(a));
}

}

ParseIndexesResult parase_indexes(const Value& text, const std::u16string& suffix) {
  ParseIndexesResult r;
  if (!truthy(text) || !is_str(text)) {
    r.ok = false;
    r.error = u"[read_indexes] failed, text got empty!";
    return r;
  }
  const std::u16string s = std::get<std::u16string>(text);
  if (s.find(u"[NOT_READY]") != std::u16string::npos) {
    r.ok = false;
    r.error = u"[read_indexes] failed, '[NOT_READY]' mark still exists.";
    return r;
  }

  Array objects;
  const std::optional<std::u16string> object_block =
      match_block_once(s, u"<object>", u"<object_end>");
  if (object_block.has_value()) {
    for (const std::u16string& raw_line : split_lines(*object_block)) {
      if (raw_line.empty()) continue;
      const std::u16string line = js_trim(raw_line);
      if (starts_with_hash(line)) continue;
      Object item;
      item.set(u"id", Value(std::u16string()));
      item.set(u"type", Value(std::u16string(dat_type_enum::kInvalid)));
      item.set(u"file", Value(std::u16string()));
      item.set(u"src", Value(std::u16string()));
      for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(line)) {
        const std::u16string& name = kv.first;
        const std::u16string& value = kv.second;
        if (name == u"id" || name == u"alias" || name == u"skipped" || name == u"bot" ||
            name == u"type") {
          item.set(name, Value(value));
        } else if (name == u"groups") {
          item.set(name, make_str_array(split_commas(value)));
        } else if (name == u"file") {
          const std::u16string src = replace_all(value, u'\\', u'/');
          item.set(u"src", Value(src));
          item.set(name, Value(replace_dat_suffix(src, u".obj." + suffix)));
        }
      }
      const Value* alias_v = item.get(u"alias");
      const Value* id_v = item.get(u"id");
      if (alias_v != nullptr && id_v != nullptr && is_non_empty_str(*alias_v) &&
          is_non_empty_str(*id_v)) {
        const std::u16string keep_id = std::get<std::u16string>(*id_v);
        item.set(u"id", Value(std::get<std::u16string>(*alias_v)));
        item.set(u"alias", Value(keep_id));
      }
      const std::optional<std::u16string> hash = match_hash_end(line);
      if (hash.has_value() && !hash->empty()) item.set(u"hash", Value(*hash));
      const Value* hash_id = item.get(u"id");
      if (hash_id != nullptr && is_str(*hash_id)) {
        const std::u16string id = std::get<std::u16string>(*hash_id);
        if (id == oid::kWeapon_LouisArmourA) item.set(u"hash", Value(u"louis_limbs_armour"));
        else if (id == oid::kWeapon_Boomerang) item.set(u"hash", Value(u"boomerang"));
        else if (id == oid::kHenryArrow1) item.set(u"hash", Value(u"henry_arrow"));
        else if (id == oid::kRudolfWeapon) item.set(u"hash", Value(u"rudolf_weapon"));
      }
      objects.push_back(Value(std::make_shared<Object>(item)));
    }
  }

  Array backgrounds;
  const std::optional<std::u16string> background_block =
      match_block_once(s, u"<background>", u"<background_end>");
  if (background_block.has_value()) {
    for (const std::u16string& raw_line : split_lines(*background_block)) {
      if (raw_line.empty()) continue;
      const std::u16string line = js_trim(raw_line);
      if (starts_with_hash(line)) continue;
      Object item;
      item.set(u"id", Value(std::u16string()));
      item.set(u"type", Value(std::u16string(dat_type_enum::kBackground)));
      item.set(u"file", Value(std::u16string()));
      item.set(u"src", Value(std::u16string()));
      for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(line)) {
        const std::u16string& name = kv.first;
        const std::u16string& value = kv.second;
        if (name == u"id") {
          item.set(name, Value(u"bg_" + value));
        } else if (name == u"alias" || name == u"skipped") {
          item.set(name, Value(value));
        } else if (name == u"file") {
          const std::u16string src = replace_all(value, u'\\', u'/');
          item.set(u"src", Value(src));
          item.set(name, Value(replace_dat_suffix(src, u".bg." + suffix)));
        }
      }
      backgrounds.push_back(Value(std::make_shared<Object>(item)));
    }
  }

  Array stages;
  const std::optional<std::u16string> stage_block = match_block_once(s, u"<stage>", u"<stage_end>");
  if (stage_block.has_value()) {
    for (const std::u16string& raw_line : split_lines(*stage_block)) {
      if (raw_line.empty()) continue;
      const std::u16string line = js_trim(raw_line);
      if (starts_with_hash(line)) continue;
      Object item;
      item.set(u"id", Value(std::u16string()));
      item.set(u"type", Value(std::u16string(dat_type_enum::kStage)));
      item.set(u"file", Value(std::u16string()));
      item.set(u"src", Value(std::u16string()));
      for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(line)) {
        const std::u16string& name = kv.first;
        const std::u16string& value = kv.second;
        if (name == u"id" || name == u"alias" || name == u"skipped") {
          item.set(name, Value(value));
        } else if (name == u"file") {
          const std::u16string src = replace_all(value, u'\\', u'/');
          item.set(u"src", Value(src));
          if (ends_with(value, u".dat")) {
            item.set(name, Value(replace_dat_suffix(src, u".stage." + suffix)));
          } else if (ends_with(value, u".txt")) {
            item.set(name, Value(replace_txt_suffix(src, u".stage." + suffix)));
          } else {
            r.ok = false;
            r.error = u"file suffix not supported, must be \".dat\" or \".txt\"";
            return r;
          }
        }
      }
      stages.push_back(Value(std::make_shared<Object>(item)));
    }
  }

  Object lists;
  lists.set(u"objects", Value(std::make_shared<Array>(objects)));
  lists.set(u"backgrounds", Value(std::make_shared<Array>(backgrounds)));
  lists.set(u"stages", Value(std::make_shared<Array>(stages)));
  lists.set(u"bots", make_str_array({}));
  r.lists = Value(std::make_shared<Object>(lists));
  return r;
}

}
}
