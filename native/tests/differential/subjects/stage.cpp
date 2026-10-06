// `stage/Expressions` + `stage/Status` 与 `bg/Background` + `bg/Layer` 的 C++ 侧台面，
// op 与 `subjects/stage.ts` 一一对应。用例：`cases/stage/expr.txt`、`cases/stage/bg.txt`。
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/bg/background.h"
#include "lfw/core/value.h"
#include "lfw/stage/expressions.h"
#include "lfw/stage/status.h"
#include "lfw/utils/container_help/field_or.h"

#include "trace_util.h"

namespace {

using trace::num_hex;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::vector<std::string> g_log;

void push(const std::string& line) { g_log.push_back(line); }

std::string num(double d) { return num_hex(d); }

std::string vstr(const lfw::Value& v) { return to_ascii(render_value(v)); }

lfw::Value number_of(const std::vector<std::string>& t, size_t& i, const std::string& op,
                     int lineno) {
  const lfw::Value v = parse_value(t, i);
  if (std::get_if<double>(&v) == nullptr) {
    std::fprintf(stderr, "%s expects a number literal at line %d\n", op.c_str(), lineno);
    std::exit(2);
  }
  return v;
}

// ---------------------------------------------------------------- Expressions 侧

// 假表达式：`run` 按脚本吐真假值（跑完最后一个就一直重复），并把每次调用记进日志。
class FakeExpr : public lfw::stage::IExpression<lfw::Value> {
 public:
  FakeExpr(size_t index, std::vector<bool> script)
      : _index(index), _script(std::move(script)) {}

  bool run(const lfw::Value& arg) override {
    push("call:" + std::to_string(_index) + ":arg=" + vstr(arg));
    if (_script.empty()) return false;
    const bool v = _script[_cursor < _script.size() ? _cursor : _script.size() - 1];
    if (_cursor < _script.size() - 1) _cursor++;
    return v;
  }

 private:
  size_t _index = 0;
  std::vector<bool> _script;
  size_t _cursor = 0;
};

lfw::stage::Expressions<lfw::Value> g_exp;
std::vector<std::shared_ptr<FakeExpr>> g_items;
lfw::Value g_arg;

lfw::stage::Expressions<lfw::Value>::Items items_of() {
  lfw::stage::Expressions<lfw::Value>::Items out;
  for (const std::shared_ptr<FakeExpr>& it : g_items) out.push_back(it);
  return out;
}

std::string flag(bool b) { return b ? "1" : "0"; }

void dump_expr() {
  push("dump|n=" + std::to_string(g_exp.list().size()) + "|i=" + num(g_exp.index()) +
       "|first=" + flag(g_exp.is_first()) + "|last=" + flag(g_exp.is_last()));
}

void dump_status() {
  push(std::string("status|") + to_ascii(lfw::status::kRunning) + "|" +
       to_ascii(lfw::status::kCompleted) + "|" + to_ascii(lfw::status::kEnd) + "|" +
       std::to_string(lfw::status_entries().size()));
}

// ---------------------------------------------------------------- Background 侧

lfw::Value g_data;
std::unique_ptr<lfw::Background> g_bg;

void dump_bg() {
  if (!g_bg) {
    push("bg|none");
    return;
  }
  const lfw::Background& b = *g_bg;
  push("bg|id=" + vstr(b.id()) + "|name=" + vstr(b.name()) + "|left=" + num(b.left()) +
       "|right=" + num(b.right()) + "|near=" + num(b.near_plane()) + "|far=" + num(b.far_plane()) +
       "|width=" + num(b.width()) + "|height=" + num(b.height()) + "|depth=" + num(b.depth()) +
       "|mid=" + num(b.middle().x) + "," + num(b.middle().z) + "|zoom=" + num(b.zoom_x()) + "," +
       num(b.zoom_y()) + "," + num(b.zoom_z()) + "|n=" + std::to_string(b.layers().size()) +
       "|ut=" + num(b.update_times()) + "|di=" + num(b.layer_data_index()));
}

void dump_layer(double index) {
  if (!g_bg) {
    push("layer|none");
    return;
  }
  const std::vector<std::shared_ptr<lfw::Layer>>& ls = g_bg->layers();
  if (index < 0 || index >= static_cast<double>(ls.size())) {
    push("layer|oob");
    return;
  }
  const lfw::Layer& l = *ls[static_cast<size_t>(index)];
  const lfw::Value& info = l.info();
  push("layer|di=" + num(l.data_index()) + "|li=" + num(l.loop_index()) + "|x=" +
       num(lfw::to_number(lfw::field_or(info, u"x"))) + "|y=" +
       num(lfw::to_number(lfw::field_or(info, u"y"))) + "|file=" +
       vstr(lfw::field_or(info, u"file")) + "|vis=" + flag(l.visible()) + "|st=" +
       flag(l.is_static()));
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_stage <case-file>\n");
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
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    // Expressions / Status
    if (op == "it") {
      std::vector<bool> script;
      while (i < t.size()) {
        const lfw::Value v = parse_value(t, i);
        const bool* b = std::get_if<bool>(&v);
        if (b == nullptr) {
          std::fprintf(stderr, "it expects boolean literals at line %d\n", lineno);
          return 2;
        }
        script.push_back(*b);
      }
      g_items.push_back(std::make_shared<FakeExpr>(g_items.size(), script));
    } else if (op == "arg") {
      g_arg = parse_value(t, i);
    } else if (op == "run") {
      push("run=" + flag(g_exp.run(g_arg)));
    } else if (op == "flow") {
      push("flow=" + flag(g_exp.flow(g_arg)));
    } else if (op == "next") {
      g_exp.next();
    } else if (op == "resetsame") {
      g_exp.reset(g_exp.list());
    } else if (op == "resetcopy") {
      const lfw::stage::Expressions<lfw::Value>::Items copy = items_of();
      g_exp.reset(copy);
    } else if (op == "expdump") {
      dump_expr();
    } else if (op == "status") {
      dump_status();
      // Background / Layer
    } else if (op == "data") {
      g_data = parse_value(t, i);
    } else if (op == "new") {
      g_bg = std::make_unique<lfw::Background>(nullptr, g_data);
    } else if (op == "bgdump") {
      dump_bg();
    } else if (op == "layer") {
      const lfw::Value idx = number_of(t, i, op, lineno);
      dump_layer(*std::get_if<double>(&idx));
    } else if (op == "upd") {
      if (g_bg) g_bg->update();
    } else if (op == "disp") {
      if (g_bg) g_bg->dispose();
    } else if (op == "lset") {
      const lfw::Value idx = number_of(t, i, op, lineno);
      const std::string field = to_ascii(trace::key_of(t[i++]));
      const lfw::Value value = parse_value(t, i);
      const lfw::Value layers = lfw::field_or(g_data, u"layers");
      lfw::Value* const arr_v = const_cast<lfw::Value*>(&layers);
      lfw::Array* const arr = lfw::as_array(*arr_v);
      const size_t at = static_cast<size_t>(*std::get_if<double>(&idx));
      if (arr == nullptr || at >= arr->size()) {
        std::fprintf(stderr, "lset out of range at line %d\n", lineno);
        return 2;
      }
      lfw::Object* const o = lfw::as_object(arr->at(at));
      if (o == nullptr) {
        std::fprintf(stderr, "lset needs an object layer at line %d\n", lineno);
        return 2;
      }
      o->set(to_u16(field), value);
    } else {
      std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
      return 2;
    }

    if (i != t.size()) {
      std::fprintf(stderr, "trailing token(s) at line %d: %s\n", lineno, raw.c_str());
      return 2;
    }
    for (const std::string& l : g_log) std::printf("%s\n", l.c_str());
    g_log.clear();
  }
  return 0;
}
