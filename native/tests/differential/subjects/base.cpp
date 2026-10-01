#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/base/fsm.h"
#include "lfw/base/no_emit_callbacks.h"

#include "trace_util.h"

namespace {

using trace::esc;
using trace::Line;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_double;
using trace::to_long;
using trace::to_u16;

lfw::Callbacks g_cb;
std::vector<std::unique_ptr<lfw::NoEmitCallbacks::Listener>> g_listeners;
std::map<std::string, lfw::NoEmitCallbacks::Listener*> g_by_id;
std::string g_reemit_key;

void emit_line(const std::string& s) { std::printf("%s\n", s.c_str()); }

lfw::Value key_value(const std::string& tok) {
  bool numeric = !tok.empty();
  for (char c : tok) {
    if ((c < '0' || c > '9') && c != '-') numeric = false;
  }
  if (numeric) return lfw::Value(to_double(tok));
  return lfw::Value(to_u16(tok));
}

std::string g_selfadd_id;
std::string g_selfadd_key;

void fire(const std::string& id, const std::string& key, const lfw::NoEmitCallbacks::Payloads& args) {
  Line out;
  out.add(std::string_view("L")).add(id).add(key);
  for (const lfw::Value& a : args) out.add(to_ascii(render_value(a)));
  out.out();
  if (!g_reemit_key.empty() && g_reemit_key == key) {
    lfw::NoEmitCallbacks::Listener* self = g_by_id[id];
    if (self != nullptr) g_cb.call(to_u16(key), {});
  }
  if (!g_selfadd_key.empty() && g_selfadd_key == key) {
    g_cb.once(to_u16(key), [id](const lfw::NoEmitCallbacks::Payloads&) { emit_line("LX " + id); });
  }
}

class TestState : public lfw::IState {
 public:
  TestState(std::string k, std::string t, bool has_t)
      : _k(std::move(k)), _t(std::move(t)), _has(has_t), _kv(key_value(_k)) {}
  lfw::Value key() const override { return _kv; }
  std::optional<lfw::Value> update(double dt) override {
    (void)dt;
    if (!_has) return std::nullopt;
    return lfw::Value(to_u16(_t));
  }
  void enter() override { emit_line("E " + _k); }
  void leave() override { emit_line("LV " + _k); }
  std::string dump_key() const { return _k; }

 private:
  std::string _k;
  std::string _t;
  bool _has;
  lfw::Value _kv;
};

std::vector<std::unique_ptr<TestState>> g_states;
std::unique_ptr<lfw::FSM> g_fsm;
lfw::FSMStateSnapshot g_snap;
bool g_has_snap = false;

TestState* state_at(const std::string& key) {
  for (std::unique_ptr<TestState>& s : g_states) {
    if (s->dump_key() == key) return s.get();
  }
  return nullptr;
}

std::string key_text(lfw::IState* s) { return s == nullptr ? "-" : esc(lfw::to_string(s->key())); }

void fsm_dump(const char* tag) {
  Line out;
  out.add(std::string_view(tag)).add(g_fsm != nullptr ? esc(g_fsm->name()) : std::string("-"));
  out.add(key_text(g_fsm != nullptr ? g_fsm->state() : nullptr));
  out.add(key_text(g_fsm != nullptr ? g_fsm->prev_state() : nullptr));
  out.add(to_ascii(render_value(lfw::Value(g_fsm != nullptr ? g_fsm->time() : 0.0))));
  out.add(to_ascii(render_value(lfw::Value(g_fsm != nullptr ? g_fsm->state_time() : 0.0))));
  out.out();
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_base <case-file>\n");
    return 2;
  }

  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open case file: %s\n", argv[1]);
    return 2;
  }

  lfw::callbacks_warn() = [](const std::u16string& msg) {
    emit_line("WARN " + esc(msg));
  };

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    raw = trace::strip_comment(raw);
    const std::vector<std::string> t = split_ws(raw);
    if (t.empty()) continue;

    const std::string& op = t[0];
    size_t i = 1;

    if (op == "add") {
      const std::string id = t[i++];
      auto l = std::make_unique<lfw::NoEmitCallbacks::Listener>();
      while (i < t.size()) {
        const std::string key = t[i++];
        l->handlers.push_back({to_u16(key), [id, key](const lfw::NoEmitCallbacks::Payloads& args) {
                                 fire(id, key, args);
                               }});
      }
      lfw::NoEmitCallbacks::Listener* raw_ptr = l.get();
      g_by_id[id] = raw_ptr;
      g_listeners.push_back(std::move(l));
      g_cb.add(raw_ptr);

    } else if (op == "once") {
      const std::string id = t[i++];
      const std::string key = t[i++];
      auto l = std::make_unique<lfw::NoEmitCallbacks::Listener>();
      l->handlers.push_back({to_u16(key), [id, key](const lfw::NoEmitCallbacks::Payloads& args) {
                               fire(id, key, args);
                             }});
      l->once = true;
      lfw::NoEmitCallbacks::Listener* raw_ptr = l.get();
      g_by_id[id] = raw_ptr;
      g_listeners.push_back(std::move(l));
      g_cb.add(raw_ptr);

    } else if (op == "on") {
      const std::string id = t[i++];
      const std::string key = t[i++];
      g_cb.on(to_u16(key), [id, key](const lfw::NoEmitCallbacks::Payloads& args) {
        fire(id, key, args);
      });

    } else if (op == "del") {
      auto it = g_by_id.find(t[i++]);
      if (it != g_by_id.end() && it->second != nullptr) g_cb.del(it->second);

    } else if (op == "clear") {
      g_cb.clear();

    } else if (op == "call") {
      const std::string key = t[i++];
      lfw::NoEmitCallbacks::Payloads args;
      while (i < t.size()) args.push_back(parse_value(t, i));
      g_cb.call(to_u16(key), args);

    } else if (op == "keys") {
      Line out;
      out.add(std::string_view("KS"));
      for (const std::u16string& k : g_cb.keys()) out.add(esc(k));
      out.out();

    } else if (op == "count") {
      const std::string key = t[i++];
      emit_line("CNT " + key + " " + std::to_string(g_cb.listener_count(to_u16(key))));

    } else if (op == "pend") {
      const std::string key = t[i++];
      emit_line("PEND " + key + " " + std::to_string(g_cb.pending_count(to_u16(key))));

    } else if (op == "loop") {
      g_reemit_key = t[i++];

    } else if (op == "selfadd") {
      g_selfadd_id = t[i++];
      g_selfadd_key = t[i++];

    } else if (op == "stoploop") {
      g_reemit_key.clear();

    } else if (op == "fsm") {
      const std::string sub = t[i++];
      if (sub == "init") {
        g_fsm = std::make_unique<lfw::FSM>(to_u16(t[i++]));
        g_fsm->log = [](const std::u16string& msg) { emit_line("LOG " + esc(msg)); };
      } else if (sub == "state") {
        const std::string key = t[i++];
        std::string name = t[i++];
        const std::string target = t[i++];
        const bool has_target = target != "~";
        auto s = std::make_unique<TestState>(key, target, has_target);
        if (name != "~") s->name = to_u16(name);
        TestState* raw_ptr = s.get();
        g_states.push_back(std::move(s));
        g_fsm->add(raw_ptr);
      } else if (sub == "use") {
        g_fsm->use(key_value(t[i++]));
      } else if (sub == "reset") {
        g_fsm->reset(key_value(t[i++]));
      } else if (sub == "update") {
        const double dt = to_double(t[i++]);
        const long times = to_long(t[i++]);
        for (long k = 0; k < times; ++k) g_fsm->update(dt);
      } else if (sub == "listen") {
        const std::string id = t[i++];
        g_fsm->callbacks().on(u"on_state_changed", [id](const std::vector<lfw::FSM*>& args) {
          lfw::FSM* f = args[0];
          Line out;
          out.add(std::string_view("S")).add(id).add(esc(f->name()));
          out.add(key_text(f->state()));
          out.out();
        });
      } else if (sub == "dump") {
        fsm_dump("F");
      } else if (sub == "snap") {
        g_snap = g_fsm->to_snapshot();
        g_has_snap = true;
        fsm_dump("SN");
      } else if (sub == "restore") {
        if (g_has_snap) g_fsm->from_snapshot(g_snap);
        fsm_dump("F");
      } else {
        std::fprintf(stderr, "line %d: unknown fsm sub '%s'\n", lineno, sub.c_str());
        return 2;
      }

    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  return 0;
}
