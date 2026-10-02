#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/defines/enum_entries.h"
#include "lfw/entity/calc_v.h"
#include "lfw/entity/entity_snapshot.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/entity/face_helper.h"
#include "lfw/entity/find_frame_direction.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

using Check = bool (*)(const lfw::Value&);

const std::pair<const char*, Check>* check_table(size_t& count) {
  static const std::pair<const char*, Check> kTable[] = {
      {"fighter_data", lfw::entity::is_fighter_data},
      {"weapon_data", lfw::entity::is_weapon_data},
      {"entity_data", lfw::entity::is_entity_data},
      {"ball_data", lfw::entity::is_ball_data},
      {"fighter", lfw::entity::is_fighter},
      {"ball", lfw::entity::is_ball},
      {"weapon", lfw::entity::is_weapon},
      {"entity", lfw::entity::is_entity},
      {"boss", lfw::entity::is_boss},
      {"object", lfw::entity::is_object},
      {"object_data", lfw::entity::is_object_data},
      {"bg_data", lfw::entity::is_bg_data},
      {"base_ctrl", lfw::entity::is_base_ctrl},
      {"bot_ctrl", lfw::entity::is_bot_ctrl},
      {"human_ctrl", lfw::entity::is_human_ctrl},
      {"ball_ctrl", lfw::entity::is_ball_ctrl},
  };
  count = sizeof(kTable) / sizeof(kTable[0]);
  return kTable;
}

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

std::string slot_line(const char* tag, const std::vector<lfw::EnumNumberEntry>& entries) {
  std::string line = tag;
  for (const lfw::EnumNumberEntry& e : entries) {
    line += " ";
    line += to_ascii(e.name);
    line += "=";
    line += std::to_string(static_cast<int>(e.value));
  }
  return line;
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_entity_helpers <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::string line = trace::strip_comment(raw);
    const std::vector<std::string> t = split_ws(line);
    if (t.empty()) continue;
    const std::string& op = t[0];

    if (op == "ns") {
      emit(slot_line("ns", lfw::entity::nslot_entries()));
      continue;
    }
    if (op == "ss") {
      emit(slot_line("ss", lfw::entity::sslot_entries()));
      continue;
    }

    size_t i = 1;
    auto need = [&](size_t n) {
      if (t.size() - i < n) {
        std::fprintf(stderr, "not enough operands for '%s' at line %d\n", op.c_str(), lineno);
        std::exit(2);
      }
    };

    if (op == "cv") {
      need(5);
      const lfw::Value cur = parse_value(t, i);
      const lfw::Value val = parse_value(t, i);
      const lfw::Value mode = parse_value(t, i);
      const lfw::Value acc = parse_value(t, i);
      const lfw::Value dir = parse_value(t, i);
      const double out = lfw::entity::calc_v(lfw::to_number(cur), lfw::to_number(val), mode, acc,
                                             dir);
      emit("cv " + render(lfw::Value(out)));
    } else if (op == "fd") {
      need(2);
      const lfw::Value frame = parse_value(t, i);
      const lfw::Value pair = parse_value(t, i);
      emit("fd " + render(lfw::Value(lfw::entity::find_direction(frame, pair))));
    } else if (op == "sf") {
      need(2);
      const lfw::Value a = parse_value(t, i);
      const lfw::Value b = parse_value(t, i);
      emit("sf " + render(lfw::Value(lfw::entity::same_face(a, b))));
    } else if (op == "tf") {
      need(1);
      const lfw::Value f = parse_value(t, i);
      emit("tf " + render(lfw::entity::turn_face(f)));
    } else if (op == "tc") {
      need(2);
      const std::string key = t[1];
      i = 2;
      const lfw::Value v = parse_value(t, i);
      size_t count = 0;
      const std::pair<const char*, Check>* table = check_table(count);
      Check fn = nullptr;
      for (size_t k = 0; k < count; ++k) {
        if (key == table[k].first) fn = table[k].second;
      }
      if (fn == nullptr) {
        std::fprintf(stderr, "unknown check '%s' at line %d\n", key.c_str(), lineno);
        return 2;
      }
      emit("tc " + key + " " + (fn(v) ? "b1" : "b0"));
    } else if (op == "non") {
      need(1);
      emit("non " + render(lfw::entity::num_or_null(parse_value(t, i))));
    } else if (op == "tri") {
      need(1);
      emit("tri " + render(lfw::Value(lfw::entity::to_tri(parse_value(t, i)))));
    } else if (op == "ftri") {
      need(1);
      emit("ftri " + render(lfw::entity::from_tri(parse_value(t, i))));
    } else if (op == "nslots") {
      emit("nslots " + render(lfw::Value(lfw::entity::num_slots())));
    } else if (op == "sslots") {
      emit("sslots " + render(lfw::Value(lfw::entity::str_slots())));
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
    }

    if (i != t.size()) {
      std::fprintf(stderr, "trailing token(s) at line %d\n", lineno);
      return 2;
    }
  }
  return 0;
}
