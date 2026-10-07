// `lfw/animation/*`（Loop / Animation / Delay / Easing / Periodic / 三角函数族 / Sequence）
// 的 C++ 侧台面，op 与 `subjects/animation.ts` 一一对应。
// 用例：`cases/animation/*.txt`。
//
// 约定：数值观察一律打**量化位**（`q_bits`，1e-3 粒度）—— `cos`/`sin`/`tan` 的实现库
// 不同（V8 vs UCRT）可能在 ulp 上不同；量化后结构性差异照样可见（用例里的差值远大于
// 1e-3 的格子）。**NaN 一律打 `nan`**：JS 只有一个 NaN 值，payload 不可观察（MSVC 的
// `strtod("NaN")` 与 V8 的位型不同）。布尔走 `render_value` 的 `b1`/`b0`，空序列的
// `curr` 打 `u`。
//
// op 一览：
//   mk <id> loop|anim
//   mk <id> delay <num>
//   mk <id> easing [begin] [end]
//   mk <id> sine|cosine|tangent [bottom] [height] [scale]
//   mk <id> seq [<animid>...]
//   set <id> <prop> <num|0|1>        prop: duration|time|value|direction|fill_mode|reverse|
//                                         times|count|offset|bottom|height|scale|val_1|val_2
//   seteasing <id> sine|linearity|quint
//   call <id> start [0|1] / end [0|1] / calc / update <dt> / auto_trip <0|1> <dt>
//   call <id> continue / reset / set <count> <times>   （后三个只给 loop）
//   seqpush <id> <animid>
//   get <id> <prop>                  打 `get|<id>|<prop>|<payload>`
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/animation/animation.h"
#include "lfw/animation/cosine.h"
#include "lfw/animation/delay.h"
#include "lfw/animation/easing.h"
#include "lfw/animation/loop.h"
#include "lfw/animation/periodic.h"
#include "lfw/animation/sequence.h"
#include "lfw/animation/sine.h"
#include "lfw/animation/tangent.h"
#include "lfw/utils/easing/ease_in_out_quint.h"
#include "lfw/utils/easing/ease_in_out_sine.h"
#include "lfw/utils/easing/ease_linearity.h"

#include "trace_util.h"

namespace {

using trace::esc;
using trace::render_value;
using trace::split_ws;
using trace::strip_comment;
using trace::to_ascii;
using trace::to_double;
using trace::to_flag;

std::string qb(double v) {
  if (std::isnan(v)) return "nan";
  return trace::q_bits(v);
}

struct Entry {
  std::unique_ptr<lfw::Animation> anim;
  std::unique_ptr<lfw::Loop> loop;
  std::string kind;  // loop | anim | delay | easing | sine | cosine | tangent | seq
};

std::map<std::string, Entry> g_items;

Entry& entry(const std::string& id) { return g_items.at(id); }
lfw::Animation& anim(const std::string& id) { return *entry(id).anim; }
lfw::Loop& loop_of(const std::string& id) { return *entry(id).loop; }

bool is_periodic(const std::string& kind) {
  return kind == "sine" || kind == "cosine" || kind == "tangent";
}

void fail(const char* msg, const std::string& what, int lineno) {
  std::fprintf(stderr, "%s '%s' at line %d\n", msg, what.c_str(), lineno);
  std::exit(2);
}

std::string bool_payload(bool b) { return to_ascii(render_value(lfw::Value(b))); }

void emit_get(const std::string& id, const std::string& prop, int lineno) {
  Entry& e = entry(id);
  std::string payload;
  if (e.kind == "loop") {
    if (prop == "count") payload = qb(e.loop->count());
    else if (prop == "times") payload = qb(e.loop->times());
    else if (prop == "done") payload = bool_payload(e.loop->done());
    else fail("unknown loop prop", prop, lineno);
  } else {
    lfw::Animation& a = *e.anim;
    if (prop == "value") payload = qb(a.value());
    else if (prop == "time") payload = qb(a.time());
    else if (prop == "duration") payload = qb(a.duration());
    else if (prop == "direction") payload = qb(a.direction());
    else if (prop == "fill_mode") payload = qb(a.fill_mode());
    else if (prop == "reverse") payload = bool_payload(a.reverse());
    else if (prop == "count") payload = qb(a.count());
    else if (prop == "times") payload = qb(a.times());
    else if (prop == "done") payload = bool_payload(a.done());
    else if (prop == "offset" || prop == "bottom" || prop == "height" || prop == "scale") {
      if (!is_periodic(e.kind)) fail("unknown anim prop", prop, lineno);
      const lfw::Periodic& p = static_cast<const lfw::Periodic&>(a);
      if (prop == "offset") payload = qb(p.offset);
      else if (prop == "bottom") payload = qb(p.bottom());
      else if (prop == "height") payload = qb(p.height());
      else payload = qb(p.scale());
    } else if (prop == "val_1" || prop == "val_2") {
      if (e.kind != "easing") fail("unknown anim prop", prop, lineno);
      const lfw::Easing& es = static_cast<const lfw::Easing&>(a);
      payload = qb(prop == "val_1" ? es.val_1() : es.val_2());
    } else if (prop == "n") {
      if (e.kind != "seq") fail("unknown anim prop", prop, lineno);
      payload = qb(static_cast<double>(static_cast<const lfw::Sequence&>(a).anims().size()));
    } else if (prop == "curr") {
      if (e.kind != "seq") fail("unknown anim prop", prop, lineno);
      const lfw::Sequence& s = static_cast<const lfw::Sequence&>(a);
      lfw::Animation* cur = s.curr_anim();
      long idx = -1;
      if (cur != nullptr) {
        for (size_t i = 0; i < s.anims().size(); ++i) {
          if (s.anims()[i] == cur) {
            idx = static_cast<long>(i);
            break;
          }
        }
      }
      payload = idx < 0 ? "u" : qb(static_cast<double>(idx));
    } else {
      fail("unknown anim prop", prop, lineno);
    }
  }
  std::printf("get|%s|%s|%s\n", id.c_str(), prop.c_str(), payload.c_str());
}

void set_prop(const std::string& id, const std::string& prop, const std::string& tok,
              int lineno) {
  Entry& e = entry(id);
  if (e.kind == "loop") {
    if (prop == "count") e.loop->set_count(to_double(tok));
    else if (prop == "times") e.loop->set_times(to_double(tok));
    else fail("unknown anim prop", prop, lineno);
    return;
  }
  lfw::Animation& a = *e.anim;
  if (prop == "duration") a.set_duration(to_double(tok));
  else if (prop == "time") a.set_time(to_double(tok));
  else if (prop == "value") a.set_value(to_double(tok));
  else if (prop == "direction") a.set_direction(to_double(tok));
  else if (prop == "fill_mode") a.set_fill_mode(to_double(tok));
  else if (prop == "reverse") a.set_reverse(to_flag(tok));
  else if (prop == "times") a.set_times(to_double(tok));
  else if (prop == "count") a.set_count(to_double(tok));
  else if (prop == "offset" || prop == "bottom" || prop == "height" || prop == "scale") {
    if (!is_periodic(e.kind)) fail("unknown anim prop", prop, lineno);
    lfw::Periodic& p = static_cast<lfw::Periodic&>(a);
    if (prop == "offset") p.set_offset(to_double(tok));
    else if (prop == "bottom") p.set_bottom(to_double(tok));
    else if (prop == "height") p.set_height(to_double(tok));
    else p.set_scale(to_double(tok));
  } else if (prop == "val_1" || prop == "val_2") {
    if (e.kind != "easing") fail("unknown anim prop", prop, lineno);
    lfw::Easing& es = static_cast<lfw::Easing&>(a);
    if (prop == "val_1") es.set_val_1(to_double(tok));
    else es.set_val_2(to_double(tok));
  } else {
    fail("unknown anim prop", prop, lineno);
  }
}

void call_op(const std::string& id, const std::vector<std::string>& rest, int lineno) {
  Entry& e = entry(id);
  const std::string& name = rest[0];
  if (e.kind == "loop") {
    if (name == "continue") {
      e.loop->continue_();
    } else if (name == "reset") {
      e.loop->reset();
    } else if (name == "set") {
      e.loop->set(to_double(rest[1]), to_double(rest[2]));
    } else {
      fail("unknown loop call", name, lineno);
    }
    return;
  }
  lfw::Animation& a = *e.anim;
  if (name == "start") {
    a.start(rest.size() > 1 ? std::optional<bool>(to_flag(rest[1])) : std::nullopt);
  } else if (name == "end") {
    a.end(rest.size() > 1 ? std::optional<bool>(to_flag(rest[1])) : std::nullopt);
  } else if (name == "calc") {
    a.calc();
  } else if (name == "update") {
    a.update(to_double(rest[1]));
  } else if (name == "auto_trip") {
    a.auto_trip(to_flag(rest[1]), to_double(rest[2]));
  } else if (name == "set") {
    // `Easing.set(begin, end)` / `Periodic.set(bottom, height, scale)`（后者的 set 会 calc）。
    if (e.kind == "easing") {
      static_cast<lfw::Easing&>(a).set(to_double(rest[1]), to_double(rest[2]));
    } else if (is_periodic(e.kind)) {
      static_cast<lfw::Periodic&>(a).set(to_double(rest[1]), to_double(rest[2]),
                                       to_double(rest[3]));
    } else {
      fail("unknown anim call", name, lineno);
    }
  } else {
    fail("unknown anim call", name, lineno);
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_animation <case-file>\n");
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
    const std::vector<std::string> t = split_ws(strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    if (op == "mk") {
      const std::string id = t[i++];
      const std::string kind = t[i++];
      Entry e;
      e.kind = kind;
      if (kind == "loop") {
        e.loop = std::make_unique<lfw::Loop>();
      } else if (kind == "anim") {
        e.anim = std::make_unique<lfw::Animation>();
      } else if (kind == "delay") {
        e.anim = std::make_unique<lfw::Delay>(to_double(t[i++]));
      } else if (kind == "easing") {
        const double begin = i < t.size() ? to_double(t[i++]) : 0;
        const double end = i < t.size() ? to_double(t[i++]) : 1;
        e.anim = std::make_unique<lfw::Easing>(begin, end);
      } else if (kind == "sine" || kind == "cosine" || kind == "tangent") {
        const double bottom = i < t.size() ? to_double(t[i++]) : 0;
        const double height = i < t.size() ? to_double(t[i++]) : 1;
        const double scale = i < t.size() ? to_double(t[i++]) : 1;
        if (kind == "sine") e.anim = std::make_unique<lfw::Sine>(bottom, height, scale);
        else if (kind == "cosine") e.anim = std::make_unique<lfw::Cosine>(bottom, height, scale);
        else e.anim = std::make_unique<lfw::Tangent>(bottom, height, scale);
      } else if (kind == "seq") {
        std::vector<lfw::Animation*> anims;
        for (; i < t.size(); ++i) anims.push_back(&anim(t[i]));
        e.anim = std::make_unique<lfw::Sequence>(anims);
      } else {
        fail("unknown mk kind", kind, lineno);
      }
      g_items[id] = std::move(e);
    } else if (op == "set") {
      const std::string id = t[i++];
      const std::string prop = t[i++];
      set_prop(id, prop, t[i++], lineno);
    } else if (op == "seteasing") {
      const std::string id = t[i++];
      const std::string name = t[i++];
      lfw::Easing& e = static_cast<lfw::Easing&>(anim(id));
      if (name == "sine") e.set_easing(lfw::ease_in_out_sine);
      else if (name == "linearity") e.set_easing(lfw::ease_linearity);
      else if (name == "quint") e.set_easing(lfw::ease_in_out_quint);
      else fail("unknown easing", name, lineno);
    } else if (op == "call") {
      const std::string id = t[i++];
      const std::vector<std::string> rest(t.begin() + static_cast<std::ptrdiff_t>(i), t.end());
      call_op(id, rest, lineno);
    } else if (op == "seqpush") {
      const std::string id = t[i++];
      static_cast<lfw::Sequence&>(anim(id)).anims().push_back(&anim(t[i++]));
    } else if (op == "get") {
      const std::string id = t[i++];
      const std::string prop = t[i++];
      emit_get(id, prop, lineno);
    } else {
      fail("unknown op", op, lineno);
    }
  }
  return 0;
}
