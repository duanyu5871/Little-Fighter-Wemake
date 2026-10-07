// `utils/schema` 家族（4W）的差分台面：`SchemaValidator` 对生成表（`defines/schemas_gen.h`）
// 与台面自搭 schema 的校验，外加 `check_stage_info` / `check_phase_info` 两个薄包装。
//
// 文法（两侧逐字一致）：
//   nv <vid>                          新建校验器（TS `new SchemaValidator()`）
//   sch <sid> <TableName>             绑定生成表里的 schema（名字 = TS 原名，如 Schema_ITerrainInfo）
//   schv <sid> <value-literal>        绑定手搭 schema（值字面量见 trace_util：u/z/b/n/s/a/o）
//   val <vid> <sid> <value-literal>   校验：`v|<vid>|<sid>|b0/1|e=N|w=N`，
//                                     逐条 `ve|<vid>|<i>|<esc>` / `vw|…`，最后 `vv|<vid>|<校验后的值>`
//   rz <vid>                          reset：`rz|<vid>|e=N|w=N`（reset 之后的计数）
//   cst <value-literal>               `check_stage_info`：`cst|b0/1|e=N` + `ce|<i>|<esc>`
//   cph <stage-lit> <info-lit>        `check_phase_info`（stage / idx 形参在 TS 里没用到）
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "lfw/defines/schemas_gen.h"
#include "lfw/loader/check_stage_info.h"
#include "lfw/utils/schema/validate_schema.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

void fail(const char* what, const std::string& detail) {
  std::fprintf(stderr, "%s: %s\n", what, detail.c_str());
  std::exit(2);
}

const lfw::Value* find_schema_table(const std::string& name) {
  for (const lfw::SchemaTableRef& t : lfw::all_schema_table_refs()) {
    if (to_ascii(t.name) == name) return &t.get();
  }
  return nullptr;
}

void print_messages(const char* tag, const std::string& vid,
                    const std::vector<std::u16string>& ms) {
  for (size_t j = 0; j < ms.size(); ++j) {
    std::printf("%s|%s|%zu|%s\n", tag, vid.c_str(), j, trace::esc(ms[j]).c_str());
  }
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_schema <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  std::map<std::string, lfw::Value> schemas;
  std::map<std::string, lfw::schema::SchemaValidator> validators;

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    if (op == "nv") {
      validators[t[i++]] = lfw::schema::SchemaValidator();
    } else if (op == "sch") {
      const std::string sid = t[i++];
      const std::string name = t[i++];
      const lfw::Value* s = find_schema_table(name);
      if (s == nullptr) fail("unknown schema table", name);
      schemas[sid] = *s;
    } else if (op == "schv") {
      const std::string sid = t[i++];
      schemas[sid] = parse_value(t, i);
    } else if (op == "val") {
      const std::string vid = t[i++];
      const std::string sid = t[i++];
      const auto sv = schemas.find(sid);
      if (sv == schemas.end()) fail("unbound schema", sid);
      const auto vv = validators.find(vid);
      if (vv == validators.end()) fail("unbound validator", vid);
      lfw::Value value = parse_value(t, i);
      const bool b = vv->second.validate(value, sv->second);
      std::printf("v|%s|%s|%s|e=%zu|w=%zu\n", vid.c_str(), sid.c_str(), b ? "b1" : "b0",
                  vv->second.errors().size(), vv->second.warnings().size());
      print_messages("ve", vid, vv->second.errors());
      print_messages("vw", vid, vv->second.warnings());
      std::printf("vv|%s|%s\n", vid.c_str(), to_ascii(render_value(value)).c_str());
    } else if (op == "rz") {
      const std::string vid = t[i++];
      const auto vv = validators.find(vid);
      if (vv == validators.end()) fail("unbound validator", vid);
      vv->second.reset();
      std::printf("rz|%s|e=%zu|w=%zu\n", vid.c_str(), vv->second.errors().size(),
                  vv->second.warnings().size());
    } else if (op == "cst") {
      const lfw::Value value = parse_value(t, i);
      std::vector<std::u16string> errs;
      const bool b = lfw::loader::check_stage_info(value, &errs);
      std::printf("cst|%s|e=%zu\n", b ? "b1" : "b0", errs.size());
      print_messages("ce", "-", errs);
    } else if (op == "cph") {
      const lfw::Value stage = parse_value(t, i);
      const lfw::Value info = parse_value(t, i);
      std::vector<std::u16string> errs;
      const bool b = lfw::loader::check_phase_info(stage, info, 0, &errs);
      std::printf("cph|%s|e=%zu\n", b ? "b1" : "b0", errs.size());
      print_messages("ce", "-", errs);
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
