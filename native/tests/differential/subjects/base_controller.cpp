#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lfw/controller/base_controller.h"
#include "lfw/core/value.h"

#include "trace_util.h"

namespace {

using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;

lfw::controller::CtrlEnv g_env;
lfw::controller::BaseController g_ctl;
std::vector<std::string> g_log;

void emit(const std::string& line) { std::printf("%s\n", line.c_str()); }

std::string render(const lfw::Value& v) { return to_ascii(render_value(v)); }

std::string flag(bool b) { return b ? "b1" : "b0"; }

std::string num(double d) { return render(lfw::Value(d)); }

std::string result_text(const lfw::Value& v) {
  return std::holds_alternative<std::monostate>(v) ? std::string("-") : render(v);
}

std::string fields_text(const lfw::controller::ControllerResult& r) {
  return render(lfw::Value(r.time())) + " " + render(lfw::Value(r.keys())) + " " +
         render(lfw::Value(r.kind())) + " " + result_text(r.result());
}

void bind_env() {
  g_env.get_next_frame = [](const lfw::Value& nf) {
    return lfw::to_string(nf) == u"none" ? lfw::Value() : nf;
  };
  g_env.world_etc = [](double x, double y, double z, const std::u16string& etc) {
    g_log.push_back("etc:" + num(x) + "," + num(y) + "," + num(z) + "," + to_ascii(etc));
  };
  g_env.team_come = [](const std::u16string& t, double x, double y, double z) {
    g_log.push_back("come:" + to_ascii(t) + "," + num(x) + "," + num(y) + "," + num(z));
  };
  g_env.team_stay = [](const std::u16string& t) { g_log.push_back("stay:" + to_ascii(t)); };
  g_env.team_move = [](const std::u16string& t) { g_log.push_back("move:" + to_ascii(t)); };
  g_env.team_follow = []() { g_log.push_back("follow"); };
  g_ctl.set_env(&g_env);
  g_ctl.result.set_resolver(g_env.get_next_frame);
}

std::vector<std::u16string> keys_of(const std::vector<std::string>& t, size_t i) {
  std::vector<std::u16string> ks;
  for (; i < t.size(); ++i) ks.push_back(trace::parse_js_string_literal(t[i]));
  return ks;
}

std::string keys_text() {
  return to_ascii(g_ctl.key_list_raw()) + "/" + to_ascii(g_ctl.key_list());
}

std::string update_text() {
  const lfw::controller::ControllerResult& r = g_ctl.result;
  return num(g_ctl.time()) + " " + render(lfw::Value(static_cast<double>(g_ctl.LR()))) + " " +
         render(lfw::Value(static_cast<double>(g_ctl.UD()))) + " " +
         render(lfw::Value(static_cast<double>(g_ctl.jd()))) + " " +
         keys_text() + " " + render(lfw::Value(r.time())) + " " + render(lfw::Value(r.keys())) +
         " " + render(lfw::Value(r.kind())) + " " + result_text(r.result()) + " " +
         render(g_ctl.keys.to_snapshot());
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_base_controller <case-file>\n");
    return 2;
  }
  bind_env();

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
    if (t.size() < 2) {
      std::fprintf(stderr, "too few operands at line %d\n", lineno);
      return 2;
    }
    size_t i = 1;

    if (op == "env") {
      const std::string& sub = t[i++];
      if (sub == "hit_dur") {
        g_env.key_hit_duration = trace::to_double(t[i++]);
      } else if (sub == "dbl_int") {
        g_env.double_click_interval = trace::to_double(t[i++]);
      } else if (sub == "facing") {
        g_env.facing = trace::to_double(t[i++]);
      } else if (sub == "alive") {
        g_env.alive = t[i++] == "1";
      } else if (sub == "human") {
        const bool h = t[i++] == "1";
        g_ctl.set_kind(h, g_ctl.is_bot());
      } else if (sub == "bot") {
        const bool b = t[i++] == "1";
        g_ctl.set_kind(g_ctl.is_human(), b);
      } else if (sub == "team") {
        g_env.team = trace::parse_js_string_literal(t[i++]);
      } else if (sub == "pos") {
        g_env.px = trace::to_double(t[i++]);
        g_env.py = trace::to_double(t[i++]);
        g_env.pz = trace::to_double(t[i++]);
      } else if (sub == "fstate") {
        g_env.frame_state = trace::to_double(t[i++]);
      } else {
        const lfw::Value v = parse_value(t, i);
        if (sub == "pre") {
          g_env.pre_hitkeys = v;
        } else if (sub == "post") {
          g_env.post_hitkeys = v;
        } else if (sub == "hf") {
          g_env.hit = v;
        } else if (sub == "hl") {
          g_env.hld = v;
        } else if (sub == "kd") {
          g_env.kd = v;
        } else if (sub == "ku") {
          g_env.ku = v;
        } else if (sub == "seq") {
          g_env.seq_map = v;
        } else if (sub == "tpre") {
          g_env.transform_pre_seq_map = v;
        } else if (sub == "tpost") {
          g_env.transform_post_seq_map = v;
        } else if (sub == "dpre") {
          g_env.data_pre_seq_map = v;
        } else if (sub == "dpost") {
          g_env.data_post_seq_map = v;
        } else {
          std::fprintf(stderr, "unknown env '%s' at line %d\n", sub.c_str(), lineno);
          return 2;
        }
      }
      emit("env " + sub);
      continue;
    }

    if (op == "ctl") {
      const std::string& sub = t[i++];
      if (sub == "new") {
        g_ctl = lfw::controller::BaseController();
        bind_env();
      } else if (sub == "reset") {
        g_ctl.reset();
        emit("ctl reset " + fields_text(g_ctl.result) + " " +
             render(g_ctl.keys.to_snapshot()) + " " +
             render(lfw::Value(static_cast<double>(g_ctl.queue.size()))) + " " +
             render(g_ctl.dbc.to_snapshot()));
        continue;
      } else if (sub == "start" || sub == "hold" || sub == "end" || sub == "db_hit" ||
                 sub == "click" || sub == "dbl_click" || sub == "kd" || sub == "ku" ||
                 sub == "ck") {
        const std::vector<std::u16string> ks = keys_of(t, i);
        i = t.size();
        if (sub == "start") {
          g_ctl.start(ks);
        } else if (sub == "hold") {
          g_ctl.hold(ks);
        } else if (sub == "end") {
          g_ctl.end(ks);
        } else if (sub == "db_hit") {
          g_ctl.db_hit(ks);
        } else if (sub == "click") {
          g_ctl.click(ks);
        } else if (sub == "dbl_click") {
          g_ctl.dbl_click(ks);
        } else if (sub == "kd") {
          g_ctl.kd(ks);
        } else if (sub == "ku") {
          g_ctl.ku(ks);
        } else {
          g_ctl.ck(ks);
        }
      } else if (sub == "update") {
        g_ctl.update();
        emit("ctl update " + update_text());
        continue;
      } else if (sub == "tst") {
        const std::u16string type = trace::parse_js_string_literal(t[i++]);
        const std::u16string key = trace::parse_js_string_literal(t[i++]);
        emit("ctl tst " + to_ascii(type) + " " + to_ascii(key) + " " + flag(g_ctl.tst(type, key)));
        continue;
      } else if (sub == "flags") {
        const std::u16string key = trace::parse_js_string_literal(t[i++]);
        emit("ctl flags " + to_ascii(key) + " " + flag(g_ctl.is_start(key)) + " " +
             flag(g_ctl.is_hit(key)) + " " + flag(g_ctl.is_hold(key)) + " " +
             flag(g_ctl.is_end(key)));
        continue;
      } else if (sub == "dbhit") {
        const std::u16string key = trace::parse_js_string_literal(t[i++]);
        const bool hit = g_ctl.is_db_hit(key);
        emit("ctl dbhit " + to_ascii(key) + " " + flag(hit) + " " +
             render(g_ctl.dbc.to_snapshot()));
        continue;
      } else if (sub == "lr" || sub == "rl" || sub == "ud" || sub == "du" || sub == "jd" ||
                 sub == "dj") {
        double v = 0;
        if (sub == "lr") v = g_ctl.LR();
        if (sub == "rl") v = g_ctl.RL();
        if (sub == "ud") v = g_ctl.UD();
        if (sub == "du") v = g_ctl.DU();
        if (sub == "jd") v = g_ctl.jd();
        if (sub == "dj") v = g_ctl.dj();
        emit("ctl " + sub + " " + render(lfw::Value(v)));
        continue;
      } else if (sub == "envdump") {
        emit("ctl envdump " + num(g_env.key_hit_duration) + " " +
             num(g_env.double_click_interval) + " " + num(g_env.facing) + " " +
             num(g_env.frame_state) + " " + flag(g_ctl.is_human()) + " " +
             flag(g_ctl.is_bot()) + " " + flag(g_env.alive));
        continue;
      } else if (sub == "q") {
        emit("ctl q " + render(lfw::Value(static_cast<double>(g_ctl.queue.size()))));
        continue;
      } else if (sub == "keys") {
        emit("ctl keys " + render(g_ctl.keys.to_snapshot()));
        continue;
      } else if (sub == "dbs") {
        emit("ctl dbs " + render(g_ctl.dbc.to_snapshot()));
        continue;
      } else if (sub == "kraw") {
        emit("ctl kraw " + keys_text());
        continue;
      } else if (sub == "seqtest") {
        const std::u16string s = trace::parse_js_string_literal(t[i++]);
        emit("ctl seqtest " + to_ascii(s) + " " + flag(g_ctl.sequence_keys_test(s)));
        continue;
      } else if (sub == "sametest") {
        const std::u16string s = trace::parse_js_string_literal(t[i++]);
        emit("ctl sametest " + to_ascii(s) + " " + flag(g_ctl.sametime_keys_test(s)));
        continue;
      } else if (sub == "log") {
        std::string s = "ctl log";
        for (const std::string& e : g_log) s += " " + e;
        emit(s);
        g_log.clear();
        continue;
      } else {
        std::fprintf(stderr, "unknown ctl sub '%s' at line %d\n", sub.c_str(), lineno);
        return 2;
      }
      if (i != t.size()) {
        std::fprintf(stderr, "unexpected trailing token '%s' at line %d\n", t[i].c_str(), lineno);
        return 2;
      }
      continue;
    }

    std::fprintf(stderr, "unknown op '%s' at line %d\n", op.c_str(), lineno);
    return 2;
  }
  return 0;
}
