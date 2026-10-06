// `Camera`（`src/LFW/Camera.ts`）的 C++ 侧台面，op 与 `subjects/camera.ts` 一一对应。
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/camera.h"
#include "lfw/core/value.h"

#include "trace_util.h"

namespace {

using trace::bits_hex;
using trace::key_of;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::vector<std::string> g_log;

void push(const std::string& line) { g_log.push_back(line); }

std::string hex_or_nan(double d) { return std::isnan(d) ? "nan" : bits_hex(d); }

std::string pair(const lfw::Vector2* v) {
  if (v == nullptr) return "z";
  return hex_or_nan(v->x) + "," + hex_or_nan(v->y);
}

// 假世界：三个对象（`stage` / `bg` / `dataset`），对应 TS 台面里那三个 bag。
// 只记 `world.stage` / `world.bg` / `world.dataset` 这几次属性读。
class FakeWorld : public lfw::ICameraWorld {
 public:
  lfw::Value stage;
  lfw::Value bg;
  lfw::Value dataset;

  lfw::Value world_stage() override {
    push("w:stage");
    return stage;
  }
  lfw::Value world_bg() override {
    push("w:bg");
    return bg;
  }
  lfw::Value world_dataset() override {
    push("w:dataset");
    return dataset;
  }
};

FakeWorld g_world;
std::unique_ptr<lfw::Camera> g_camera;

lfw::Object* bag_of(const std::string& name) {
  lfw::Value* v = name == "stage" ? &g_world.stage : name == "bg" ? &g_world.bg
                                       : name == "dataset" ? &g_world.dataset
                                                           : nullptr;
  if (v == nullptr) return nullptr;
  if (std::holds_alternative<std::monostate>(*v)) *v = lfw::Value(std::make_shared<lfw::Object>());
  return lfw::as_object(*v);
}

double number_of(const std::vector<std::string>& t, size_t& i, const std::string& op, int lineno) {
  const lfw::Value v = parse_value(t, i);
  const double* const d = std::get_if<double>(&v);
  if (d == nullptr) {
    std::fprintf(stderr, "%s expects a number literal at line %d\n", op.c_str(), lineno);
    std::exit(2);
  }
  return *d;
}

void dump() {
  const lfw::Camera& c = *g_camera;
  push("dump|dest=" + pair(&c.destination) + "|pos=" + pair(&c.position) +
       "|vel=" + pair(&c.velocity) + "|locked=" + pair(c.locked()) +
       "|dested=" + pair(c.dested()));
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_camera <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  g_camera = std::make_unique<lfw::Camera>(&g_world);

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    if (op == "sf") {
      const std::string bag_name = t[i++];
      lfw::Object* const bag = bag_of(bag_name);
      if (bag == nullptr) {
        std::fprintf(stderr, "bad bag '%s' at line %d\n", bag_name.c_str(), lineno);
        return 2;
      }
      const std::string field = to_ascii(key_of(t[i++]));
      bag->set(to_u16(field), parse_value(t, i));
    } else if (op == "new") {
      g_camera = std::make_unique<lfw::Camera>(&g_world);
    } else if (op == "dump") {
      dump();
    } else if (op == "reset") {
      g_camera->reset();
    } else if (op == "undest") {
      g_camera->undest();
    } else if (op == "unlock") {
      g_camera->unlock();
    } else if (op == "jx") {
      g_camera->jump_x(number_of(t, i, op, lineno));
    } else if (op == "jy") {
      g_camera->jump_y(number_of(t, i, op, lineno));
    } else if (op == "dest") {
      // 注意：两个 `number_of` 必须分两句读（C++ 不保证实参求值顺序，MSVC 是右到左）。
      const double x = number_of(t, i, op, lineno);
      const double y = number_of(t, i, op, lineno);
      g_camera->dest(x, y);
    } else if (op == "lock") {
      const double x = number_of(t, i, op, lineno);
      const double y = number_of(t, i, op, lineno);
      g_camera->lock(x, y);
    } else if (op == "pos") {
      g_camera->position.x = number_of(t, i, op, lineno);
      g_camera->position.y = number_of(t, i, op, lineno);
    } else if (op == "dset") {
      g_camera->destination.x = number_of(t, i, op, lineno);
      g_camera->destination.y = number_of(t, i, op, lineno);
    } else if (op == "vel") {
      g_camera->velocity.x = number_of(t, i, op, lineno);
      g_camera->velocity.y = number_of(t, i, op, lineno);
    } else if (op == "update") {
      g_camera->update();
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
