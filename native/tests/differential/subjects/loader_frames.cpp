#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/loader/preprocess_bdy.h"
#include "lfw/loader/preprocess_frame.h"
#include "lfw/loader/preprocess_itr.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/read_nums.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

std::string probe_of(const lfw::Value& v, const char16_t* key) {
  const lfw::Object* o = lfw::as_object(v);
  if (o == nullptr) return "-";
  const lfw::Value* p = o->get(std::u16string(key));
  if (p == nullptr) return "-";
  return lfw::truthy(*p) ? "s" : "u";
}

std::string tester_probe(const lfw::Value& v) {
  std::string probe = probe_of(v, u"__tester");
  const lfw::Value acts = lfw::field_or(v, u"actions");
  const lfw::Array* a = lfw::as_array(acts);
  if (a == nullptr) return probe;
  for (size_t i = 0; i < a->size(); ++i) probe += "," + probe_of(a->at(i), u"tester");
  return probe;
}

std::string actions_probe(const lfw::Value& v) {
  std::string probe;
  const lfw::Array* a = lfw::as_array(lfw::field_or(v, u"actions"));
  if (a == nullptr) return probe;
  for (size_t i = 0; i < a->size(); ++i) probe += "," + probe_of(a->at(i), u"tester");
  return probe;
}

// `frame` 的探针：帧自己（恒 `-`）+ 每个 `bdy` / `itr` 的 `__tester` 与它们 `actions` 里的
// `tester`（顺序与 TS 侧逐字符对齐）。
std::string frame_probe(const lfw::Value& frame) {
  std::string probe = probe_of(frame, u"__tester");
  const char16_t* const keys[] = {u"bdy", u"itr"};
  for (const char16_t* key : keys) {
    probe += std::string("/") + to_ascii(std::u16string(key));
    const lfw::Array* const a = lfw::as_array(lfw::field_or(frame, key));
    if (a == nullptr) {
      probe += "=-";
      continue;
    }
    for (size_t i = 0; i < a->size(); ++i) {
      probe += "," + probe_of(a->at(i), u"__tester") + actions_probe(a->at(i));
    }
  }
  return probe;
}

// TS 侧把编译好的 `Expression` / `ValExpression` 挂在这些键上（端口存的是源串或不写），
// 两端都不可比，剥掉后再比其余字段。
void strip_compiled(lfw::Value& v) {
  if (lfw::Array* a = lfw::as_array(v)) {
    for (size_t i = 0; i < a->size(); ++i) strip_compiled(a->at(i));
    return;
  }
  lfw::Object* const o = lfw::as_object(v);
  if (o == nullptr) return;
  o->remove(u"__tester");
  o->remove(u"__judger");
  o->remove(u"tester");
  for (const char16_t* const key : {u"__gen_x", u"__gen_y", u"__gen_z", u"__gen_dvx", u"__gen_dvy",
                                    u"__gen_dvz", u"__gen_spread_x", u"__gen_spread_y",
                                    u"__gen_spread_z", u"__gen_facing"}) {
    o->remove(std::u16string(key));
  }
  const std::vector<std::u16string> keys = o->keys();
  for (const std::u16string& k : keys) {
    const lfw::Value* p = o->get(k);
    if (p == nullptr) continue;
    lfw::Value child = *p;
    strip_compiled(child);
  }
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_loader_frames <case-file>\n");
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
    size_t i = 1;

    if (op == "bdy" || op == "itr") {
      lfw::Value ctx = parse_value(t, i);
      std::u16string error;
      const bool ok = op == "bdy" ? lfw::loader::preprocess_bdy(ctx, error)
                                  : lfw::loader::preprocess_itr(ctx, error);
      const lfw::Value result = lfw::field_or(ctx, op == "bdy" ? u"bdy" : u"itr");
      const std::string probe = tester_probe(result);
      strip_compiled(ctx);
      std::string out = op + (ok ? " ok" : " throw");
      out += " " + render(lfw::field_or(ctx, u"data"));
      if (op == "bdy") out += " " + render(lfw::field_or(ctx, u"frame"));
      out += " " + render(result);
      out += " t=" + probe;
      if (!ok) {
        const std::string msg = to_ascii(error);
        out += " msg=" + (!msg.empty() && msg[0] == '[' ? msg : "-");
      }
      emit(out);
    } else if (op == "frame") {
      lfw::Value ctx = parse_value(t, i);
      std::u16string error;
      const bool ok = lfw::loader::preprocess_frame(ctx, error);
      const lfw::Value result = lfw::field_or(ctx, u"frame");
      const std::string probe = frame_probe(result);
      strip_compiled(ctx);
      std::string out = std::string("frame ") + (ok ? "ok" : "throw");
      out += " " + render(lfw::field_or(ctx, u"data"));
      out += " " + render(result);
      out += " t=" + probe;
      if (!ok) {
        const std::string msg = to_ascii(error);
        out += " msg=" + (!msg.empty() && msg[0] == '[' ? msg : "-");
      }
      emit(out);
    } else if (op == "rn") {
      const lfw::Value src = parse_value(t, i);
      const lfw::Value length = parse_value(t, i);
      const lfw::Value fallbacks = i < t.size() ? parse_value(t, i) : lfw::Value();
      std::u16string error;
      std::vector<lfw::Value> nums;
      const bool ok = lfw::read_nums(src, lfw::to_number(length), fallbacks, nums, &error);
      std::string out = std::string("rn ") + (ok ? "ok" : "throw") + " ";
      if (ok) {
        lfw::Array a;
        for (const lfw::Value& v : nums) a.push_back(v);
        out += render(lfw::Value(std::make_shared<lfw::Array>(a)));
      } else {
        out += "-";
      }
      out += " fb=" + render(fallbacks);
      out += " msg=" + (ok ? std::string("-") : to_ascii(error));
      emit(out);
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
    }
  }
  return 0;
}
