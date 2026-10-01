#include "lfw/dat_translator/make_stage_info_list.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/string_matchers.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/difficulty.h"
#include "lfw/defines/stage_actions.h"
#include "lfw/utils/string_help.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {

namespace {

Value empty_obj() { return Value(std::make_shared<Object>()); }

Value field_or_any(const Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  return v != nullptr ? *v : Value();
}

std::u16string replace_double_backslash(const std::u16string& s) {
  std::u16string out;
  size_t i = 0;
  while (i < s.size()) {
    if (s[i] == u'\\' && i + 1 < s.size() && s[i + 1] == u'\\') {
      out.push_back(u'/');
      i += 2;
      continue;
    }
    out.push_back(s[i]);
    ++i;
  }
  return out;
}

std::u16string normalize_phase_end(const std::u16string& s) {
  std::u16string out;
  size_t i = 0;
  while (i < s.size()) {
    if (s.compare(i, 11, u"<phase_end>") == 0) {
      size_t j = i + 11;
      while (j < s.size() && (is_str_white_space(s[j]) || s[j] == u'|')) ++j;
      if (s.compare(j, 7, u"<stage>") == 0) {
        out += u"<phase_end><stage_end><stage>";
        i = j + 7;
        continue;
      }
    }
    out.push_back(s[i]);
    ++i;
  }
  return out;
}

std::vector<std::u16string> split_ln(const std::u16string& s) {
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

std::u16string collapse_ws_newlines(const std::u16string& s) {
  std::u16string out;
  size_t i = 0;
  while (i < s.size()) {
    if (is_str_white_space(s[i])) {
      size_t j = i;
      while (j < s.size() && is_str_white_space(s[j])) ++j;
      size_t nl = j;
      while (nl < s.size() && s[nl] == u'\n') ++nl;
      if (nl > j) {
        out.push_back(u'\n');
        i = nl;
        continue;
      }
    }
    out.push_back(s[i]);
    ++i;
  }
  return out;
}

std::u16string strip_stage_word(const std::u16string& s) {
  std::u16string out;
  size_t i = 0;
  while (i < s.size()) {
    if (i + 5 <= s.size() && s.compare(i, 5, u"stage") == 0) {
      i += 5;
      continue;
    }
    if (i + 5 <= s.size()) {
      std::u16string seg = s.substr(i, 5);
      for (char16_t& c : seg) {
        if (c >= u'A' && c <= u'Z') c = static_cast<char16_t>(c - u'A' + u'a');
      }
      if (seg == u"stage") {
        i += 5;
        continue;
      }
    }
    out.push_back(s[i]);
    ++i;
  }
  return out;
}

bool is_num_value(const Value& v) { return is_num(to_number(v)); }

std::vector<Value> as_value_array(const Value& v) {
  std::vector<Value> out;
  const Array* a = as_array(v);
  if (a != nullptr) out = a->items();
  return out;
}

Value find_by_id_strict(const std::vector<Value>& list, const Value& id) {
  for (const Value& v : list) {
    const Object* o = as_object(v);
    if (o == nullptr) continue;
    const Value* vid = o->get(u"id");
    if (vid != nullptr && strict_equals(*vid, id)) return v;
  }
  return Value();
}

Value make_difficulty_map(double easy, double normal, double difficult, double crazy) {
  return make_obj({{u"1", n(easy)},
                   {u"2", n(normal)},
                   {u"3", n(difficult)},
                   {u"4", n(crazy)}});
}

}

Value make_stage_info_list(const std::u16string& full_str) {
  std::u16string text = replace_double_backslash(full_str);
  text = normalize_phase_end(text);

  Array stage_infos;
  const TakeBlocksResult r0 = take_blocks(text, u"<stage>", u"<stage_end>");
  text = r0.remains;

  for (const std::u16string& stage_block : r0.blocks) {
    Value phases = Value(std::make_shared<Array>());
    Value stage_info = empty_obj();
    Object* si = as_object(stage_info);
    si->set(u"phases", phases);

    TakeBlocksResult r1 = take_blocks(stage_block, u"<phase>", u"<phase_end>");
    std::u16string stage_str = r1.remains;
    for (const std::u16string& phase_block : r1.blocks) {
      Value phase_info = empty_obj();
      Object* pi = as_object(phase_info);
      const std::u16string trimmed = js_trim(phase_block);
      for (const std::u16string& raw_line : split_ln(trimmed)) {
        const std::u16string line = js_trim(raw_line);
        if (line.empty()) continue;
        if (line.compare(0, 5, u"bound") == 0) {
          for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(line)) {
            if (kv.first == u"bound") {
              const Value num = Value(kv.second);
              if (is_num_value(num)) pi->set(u"bound", Value(to_number(num)));
            } else if (kv.first == u"music") {
              pi->set(u"music", Value(replace_all(kv.second, u'\\', u'/') + u".mp3"));
            }
          }
          const std::optional<std::u16string> hash = match_hash_end(line);
          pi->set(u"desc", hash.has_value() ? Value(js_trim(*hash)) : Value(u""));
        } else if (line.compare(0, 5, u"music") == 0) {
          for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(line)) {
            if (kv.first == u"music") {
              pi->set(u"music", Value(replace_all(kv.second, u'\\', u'/') + u".mp3"));
            }
          }
        } else if (line.compare(0, 2, u"id") == 0) {
          Object object;
          object.set(u"id", Value(std::make_shared<Array>()));
          object.set(u"x", field_or_any(*pi, u"bound"));
          if (line.find(u"<soldier>") != std::u16string::npos) object.set(u"is_soldier", Value(true));
          if (line.find(u"<boss>") != std::u16string::npos) object.set(u"is_boss", Value(true));
          for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(line)) {
            if (kv.first == u"id") {
              Array ids;
              ids.push_back(Value(kv.second));
              object.set(u"id", Value(std::make_shared<Array>(ids)));
            } else if (kv.first == u"act") {
              object.set(u"act", Value(kv.second));
            } else {
              const Value num = Value(kv.second);
              if (is_num_value(num)) object.set(kv.first, Value(to_number(num)));
            }
            const Value* xv = object.get(u"x");
            const bool has_x = xv != nullptr && truthy(*xv);
            const bool neg = has_x && to_number(*xv) < 0;
            object.set(u"facing", n(neg ? 1 : -1));
          }
          const Value* solo = object.get(u"is_soldier");
          const Value* times = object.get(u"times");
          if (solo != nullptr && truthy(*solo) && (times == nullptr || !truthy(*times))) {
            object.set(u"times", n(50));
          }
          const Value* objs = pi->get(u"objects");
          if (objs == nullptr || !truthy(*objs)) pi->set(u"objects", Value(std::make_shared<Array>()));
          Array* arr = const_cast<Array*>(as_array(*pi->get(u"objects")));
          if (arr != nullptr) arr->push_back(Value(std::make_shared<Object>(object)));
        }
      }
      Array* ph = const_cast<Array*>(as_array(phases));
      if (ph != nullptr) ph->push_back(phase_info);
    }

    std::u16string head = collapse_ws_newlines(stage_str);
    head = js_trim(head);
    for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(head)) {
      si->set(kv.first, Value(kv.second));
    }
    const double nid = to_number(field_or_any(*si, u"id"));
    const std::optional<std::u16string> hash = match_hash_end(head);
    const Value name_src = hash.has_value() ? Value(*hash) : field_or_any(*si, u"id");
    if (is_str(name_src)) {
      si->set(u"name", Value(js_trim(strip_stage_word(std::get<std::u16string>(name_src)))));
    } else if (!std::holds_alternative<std::monostate>(name_src)) {
      si->set(u"name", Value(js_trim(strip_stage_word(to_string(name_src)))));
    } else {
      si->set(u"name", Value());
    }

    if (std::fmod(nid, 10) == 0) {
      si->set(u"is_starting", Value(true));
      si->set(u"starting_name", Value(to_string(n(1 + nid / 10))));
    }

    Array* phase_arr = const_cast<Array*>(as_array(phases));
    const size_t phase_count = phase_arr != nullptr ? phase_arr->size() : 0;
    for (size_t i = 0; i < phase_count; ++i) {
      Object* p = as_object(phase_arr->at(i));
      if (p == nullptr) continue;
      const Value bound = field_or_any(*p, u"bound");
      const bool has_bound = !std::holds_alternative<std::monostate>(bound);
      p->set(u"enemy_r", n((has_bound ? to_number(bound) : 0) + 300));
      p->set(u"enemy_l", n(-300));
      Array on_end;
      on_end.push_back(Value(std::u16string(stage_actions::kEnterNextPhase)));
      p->set(u"on_end", Value(std::make_shared<Array>(on_end)));
      if (i == phase_count - 1) {
        Array last;
        last.push_back(Value(std::u16string(stage_actions::kLoopGoGoGoRight)));
        p->set(u"on_end", Value(std::make_shared<Array>(last)));
      } else if (i > 0) {
        Array on_start;
        on_start.push_back(Value(std::u16string(stage_actions::kGoGoGoRight)));
        p->set(u"on_start", Value(std::make_shared<Array>(on_start)));
      }
    }

    if (nid < 49 && phase_count > 0) {
      Object* p0 = as_object(phase_arr->at(0));
      if (p0 != nullptr) {
        const Value up = make_difficulty_map(200, 150, 100, 50);
        p0->set(u"respawn", up);
        p0->set(u"health_up", up);
        p0->set(u"mp_up", make_difficulty_map(500, 500, 500, 500));
      }
    }

    if (nid == 50) {
      for (size_t i = 0; i < phase_count; ++i) {
        Object* p = as_object(phase_arr->at(i));
        if (p == nullptr) continue;
        p->set(u"respawn", make_difficulty_map(200, 100, 5, 5));
        p->set(u"respawn_r", make_difficulty_map(500, 400, 300, 250));
        p->set(u"respawn_x", make_difficulty_map(100, 100, 100, 100));
      }
    }

    if (nid == 50) {
      si->set(u"starting_name", Value(u"Survival"));
      si->set(u"chapter", Value(u"survival"));
      si->set(u"bg", Value(u"bg_8"));
      si->set(u"title", Value(u"SURVIVAL STAGE"));
      for (size_t i = 0; i < phase_count; ++i) {
        Object* p = as_object(phase_arr->at(i));
        if (p == nullptr) continue;
        p->set(u"drink_l", n(0));
        p->set(u"drink_r", field_or_any(*p, u"bound"));
        p->set(u"title", Value(u"Survival Stage " + to_string(n(static_cast<double>(i)))));
        p->set(u"on_start", Value());
        Array on_end;
        on_end.push_back(Value(std::u16string(stage_actions::kEnterNextPhase)));
        p->set(u"on_end", Value(std::make_shared<Array>(on_end)));
      }
    }

    if (nid <= 9) {
      si->set(u"bg", Value(u"bg_2"));
      si->set(u"chapter", Value(u"chapter_1"));
      si->set(u"title", Value(u"STAGE 1-" + to_string(n(nid + 1))));
      if (nid < 9) si->set(u"next", Value(to_string(n(nid + 1))));
    } else if (nid <= 19) {
      si->set(u"bg", Value(u"bg_3"));
      si->set(u"chapter", Value(u"chapter_2"));
      si->set(u"title", Value(u"STAGE 2-" + to_string(n(nid + 1 - 10))));
      if (nid < 19) si->set(u"next", Value(to_string(n(nid + 1))));
    } else if (nid <= 29) {
      si->set(u"bg", Value(u"bg_5"));
      si->set(u"chapter", Value(u"chapter_3"));
      si->set(u"title", Value(u"STAGE 3-" + to_string(n(nid + 1 - 20))));
      if (nid < 29) si->set(u"next", Value(to_string(n(nid + 1))));
    } else if (nid <= 39) {
      si->set(u"bg", Value(u"bg_6"));
      si->set(u"chapter", Value(u"chapter_4"));
      si->set(u"title", Value(u"STAGE 4-" + to_string(n(nid + 1 - 30))));
      if (nid < 39) si->set(u"next", Value(to_string(n(nid + 1))));
    } else if (nid <= 49) {
      si->set(u"bg", Value(u"bg_7"));
      si->set(u"chapter", Value(u"chapter_5"));
      si->set(u"title", Value(u"STAGE 5-" + to_string(n(nid + 1 - 40))));
      if (nid < 49) si->set(u"next", Value(to_string(n(nid + 1))));
    }

    stage_infos.push_back(stage_info);
  }

  std::vector<Value> list = stage_infos.items();
  for (Value& v : list) {
    Object* si = as_object(v);
    if (si == nullptr) continue;
    const Value* phases_v = si->get(u"phases");
    Array* ph = phases_v != nullptr ? const_cast<Array*>(as_array(*phases_v)) : nullptr;
    if (ph == nullptr || ph->size() == 0) continue;
    Object* first = as_object(ph->at(0));
    if (first == nullptr) continue;
    first->set(u"cam_jump_to_x", n(0));
    first->set(u"player_jump_to_x", n(0));
    first->set(u"player_facing", n(1));
  }

  std::stable_sort(list.begin(), list.end(), [](const Value& a, const Value& b) {
    const Object* ao = as_object(a);
    const Object* bo = as_object(b);
    const double av = ao != nullptr ? to_number(field_or_any(*ao, u"id")) : 0.0;
    const double bv = bo != nullptr ? to_number(field_or_any(*bo, u"id")) : 0.0;
    return av < bv;
  });

  for (Value& v : list) {
    Object* s = as_object(v);
    if (s == nullptr) continue;
    const Value next = field_or_any(*s, u"next");
    if (!truthy(next)) continue;
    const Value hit = find_by_id_strict(list, next);
    if (as_object(hit) != nullptr) continue;
    s->remove(u"next");
    const double nid = to_number(field_or_any(*s, u"id"));
    if (nid <= 9) s->set(u"next", Value(u"10"));
    else if (nid <= 19) s->set(u"next", Value(u"20"));
    else if (nid <= 29) s->set(u"next", Value(u"30"));
    else if (nid <= 39) s->set(u"next", Value(u"40"));
    else if (nid <= 49) s->set(u"next", Value(u"end"));
  }

  for (Value& v : list) {
    Object* s = as_object(v);
    if (s == nullptr) continue;
    const Value next = field_or_any(*s, u"next");
    Value next_stage = find_by_id_strict(list, next);
    bool is_stage_end = strict_equals(next, Value(u"end")) || !truthy(next);
    if (!is_stage_end) {
      const Object* no = as_object(next_stage);
      const Value* chapter = no != nullptr ? no->get(u"chapter") : nullptr;
      const Value* mine = s->get(u"chapter");
      if (chapter == nullptr || mine == nullptr || !strict_equals(*chapter, *mine)) is_stage_end = true;
    }
    if (!is_stage_end) continue;
    const Value* phases_v = s->get(u"phases");
    Array* ph = phases_v != nullptr ? const_cast<Array*>(as_array(*phases_v)) : nullptr;
    if (ph == nullptr || ph->size() == 0) continue;
    Object* last = as_object(ph->at(ph->size() - 1));
    if (last == nullptr) continue;
    last->set(u"on_end", Value());
  }

  Array out;
  for (const Value& v : list) out.push_back(v);
  return Value(std::make_shared<Array>(out));
}

}
}
