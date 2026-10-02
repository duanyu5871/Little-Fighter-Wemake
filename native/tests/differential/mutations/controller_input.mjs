// controller_input 变异规格
// 覆盖 native/lfw/defines/game_key.h(GKLabels/AGK/CONFLICTS_KEY_MAP 三表与查表函数)、
// native/lfw/controller/key_status.h(读取器 time/u_time/used)、
// native/lfw/controller/controller_key_status.{h,cpp}(7 槽/查找/重置/快照往返)、
// native/lfw/controller/controller_result.cpp(clear/fire/fire2 与 resolver 缝)。
//
// 不可变异类(记录，不计入):
//   1. to_snapshot/from_snapshot 的边界守卫(n > k)：TS 对短数组直接抛错，
//      差分无法喂入短数组，故守卫阈值改动不可观测(与既有 bounds-check 类同源)。
//   2. slot 成员声明顺序(L/R/U/D/d/j/a 成员本身)：不影响任何可观测输出。
//   3. KeyStatus::key()：同类型成员只有 _key 一个，无同型错误取值可注入。
//   4. controller_result.cpp 的 `_resolve ? ... : Value()` 空函数守卫：
//      去掉后会 std::bad_function_call(_HAS_EXCEPTIONS=0)，属崩溃类变异。
//
// 过程记录:
//   - 首轮 66/72，6 条 `ck reset 漏 X` 幸存：用例在第一次 reset 时只有 L 槽有数据，
//     其余槽全 0，重置它们不改变任何观测。已在 7 槽都有值之后补一次 reset+raw。
//   - 修正一处真实漂移: ControllerResult::fire 的守卫必须是 JS 真值判定
//     (`if (!truthy(result)) return false;`)，否则 fire(nf=0) 会错误地成功。
export default {
  subject: "controller_input",
  mutations: [
    { note: "gk label L", file: "native/lfw/defines/game_key.h", from: `{gk::kL, u"<"},`, to: `{gk::kL, u">"},` },
    { note: "gk label R", file: "native/lfw/defines/game_key.h", from: `{gk::kR, u">"},`, to: `{gk::kR, u"<"},` },
    { note: "gk label U", file: "native/lfw/defines/game_key.h", from: `{gk::kU, u"^"},`, to: `{gk::kU, u"v"},` },
    { note: "gk label D", file: "native/lfw/defines/game_key.h", from: `{gk::kD, u"v"},`, to: `{gk::kD, u"^"},` },
    { note: "gk label a", file: "native/lfw/defines/game_key.h", from: `{gk::ka, u"A"},`, to: `{gk::ka, u"a"},` },
    { note: "gk label j", file: "native/lfw/defines/game_key.h", from: `{gk::kj, u"J"},`, to: `{gk::kj, u"j"},` },
    { note: "gk label d", file: "native/lfw/defines/game_key.h", from: `{gk::kd, u"D"},`, to: `{gk::kd, u"d"},` },
    {
      note: "agk 顺序 U/D",
      file: "native/lfw/defines/game_key.h",
      from: `                                                 gk::kU, gk::kD, gk::kj, gk::ka};`,
      to: `                                                 gk::kD, gk::kU, gk::kj, gk::ka};`,
    },
    {
      note: "agk 顺序 d 提前",
      file: "native/lfw/defines/game_key.h",
      from: `static const std::vector<const char16_t*> t = {gk::kd, gk::kL, gk::kR,`,
      to: `static const std::vector<const char16_t*> t = {gk::kL, gk::kd, gk::kR,`,
    },
    { note: "conflicts L", file: "native/lfw/defines/game_key.h", from: `{gk::kL, gk::kR},`, to: `{gk::kL, gk::kU},` },
    { note: "conflicts R", file: "native/lfw/defines/game_key.h", from: `{gk::kR, gk::kL},`, to: `{gk::kR, gk::kD},` },
    { note: "conflicts U", file: "native/lfw/defines/game_key.h", from: `{gk::kU, gk::kD},`, to: `{gk::kU, gk::kL},` },
    { note: "conflicts D", file: "native/lfw/defines/game_key.h", from: `{gk::kD, gk::kU},`, to: `{gk::kD, gk::kR},` },
    { note: "conflicts a 非空", file: "native/lfw/defines/game_key.h", from: `{gk::ka, nullptr},`, to: `{gk::ka, gk::kL},` },
    { note: "conflicts j 非空", file: "native/lfw/defines/game_key.h", from: `{gk::kj, nullptr},`, to: `{gk::kj, gk::kL},` },
    { note: "conflicts d 非空", file: "native/lfw/defines/game_key.h", from: `{gk::kd, nullptr},`, to: `{gk::kd, gk::kL},` },
    {
      note: "gk_label_of 比较取反",
      file: "native/lfw/defines/game_key.h",
      from: `    if (std::u16string(e.first) == key) return std::u16string(e.second);`,
      to: `    if (std::u16string(e.first) != key) return std::u16string(e.second);`,
    },
    {
      note: "gk_label_of 兜底非空",
      file: "native/lfw/defines/game_key.h",
      from: `  return std::u16string();\n}\n\ninline const char16_t* conflicts_key_of`,
      to: `  return std::u16string(u"?");\n}\n\ninline const char16_t* conflicts_key_of`,
    },
    {
      note: "conflicts_key_of 比较取反",
      file: "native/lfw/defines/game_key.h",
      from: `    if (std::u16string(e.first) == key) return e.second;`,
      to: `    if (std::u16string(e.first) != key) return e.second;`,
    },
    {
      note: "conflicts_key_of 兜底非空",
      file: "native/lfw/defines/game_key.h",
      from: `    if (std::u16string(e.first) == key) return e.second;\n  }\n  return nullptr;\n}`,
      to: `    if (std::u16string(e.first) == key) return e.second;\n  }\n  return gk::kL;\n}`,
    },
    {
      note: "KeyStatus::time 取值错",
      file: "native/lfw/controller/key_status.h",
      from: `  double time() const { return _d_time; }`,
      to: `  double time() const { return _u_time; }`,
    },
    {
      note: "KeyStatus::u_time 取值错",
      file: "native/lfw/controller/key_status.h",
      from: `  double u_time() const { return _u_time; }`,
      to: `  double u_time() const { return _d_time; }`,
    },
    {
      note: "KeyStatus::used 恒 0",
      file: "native/lfw/controller/key_status.h",
      from: `  double used() const { return _used; }`,
      to: `  double used() const { return 0; }`,
    },
    {
      note: "ck 构造 L 用错键",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `    : L(Value(std::u16string(gk::kL))),`,
      to: `    : L(Value(std::u16string(gk::kR))),`,
    },
    {
      note: "ck 构造 R 用错键",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `      R(Value(std::u16string(gk::kR))),`,
      to: `      R(Value(std::u16string(gk::kU))),`,
    },
    {
      note: "ck 构造 U 用错键",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `      U(Value(std::u16string(gk::kU))),`,
      to: `      U(Value(std::u16string(gk::kD))),`,
    },
    {
      note: "ck 构造 D 用错键",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `      D(Value(std::u16string(gk::kD))),`,
      to: `      D(Value(std::u16string(gk::kd))),`,
    },
    {
      note: "ck 构造 d 用错键",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `      d(Value(std::u16string(gk::kd))),`,
      to: `      d(Value(std::u16string(gk::ka))),`,
    },
    {
      note: "ck 构造 j 用错键",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `      j(Value(std::u16string(gk::kj))),`,
      to: `      j(Value(std::u16string(gk::kd))),`,
    },
    {
      note: "ck 构造 a 用错键",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `      a(Value(std::u16string(gk::ka))) {}`,
      to: `      a(Value(std::u16string(gk::kj))) {}`,
    },
    {
      note: "ck slot 不收 a",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  const std::vector<KeyStatus*> all = {&L, &R, &U, &D, &d, &j, &a};`,
      to: `  const std::vector<KeyStatus*> all = {&L, &R, &U, &D, &d, &j};`,
    },
    {
      note: "ck slot 不收 L",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  const std::vector<KeyStatus*> all = {&L, &R, &U, &D, &d, &j, &a};`,
      to: `  const std::vector<KeyStatus*> all = {&R, &U, &D, &d, &j, &a};`,
    },
    {
      note: "ck slot 比较取反",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `        std::get<std::u16string>(k) == key) {`,
      to: `        std::get<std::u16string>(k) != key) {`,
    },
    {
      note: "ck const slot 返回空",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  return const_cast<ControllerKeyStatus*>(this)->slot(key);`,
      to: `  return nullptr;`,
    },
    {
      note: "ck reset 漏 L",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  L.reset();\n  R.reset();`,
      to: `  R.reset();\n  R.reset();`,
    },
    {
      note: "ck reset 漏 R",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  R.reset();\n  U.reset();`,
      to: `  U.reset();\n  U.reset();`,
    },
    {
      note: "ck reset 漏 U",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  U.reset();\n  D.reset();`,
      to: `  D.reset();\n  D.reset();`,
    },
    {
      note: "ck reset 漏 D",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  D.reset();\n  d.reset();`,
      to: `  d.reset();\n  d.reset();`,
    },
    {
      note: "ck reset 漏 d",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  d.reset();\n  j.reset();`,
      to: `  j.reset();\n  j.reset();`,
    },
    {
      note: "ck reset 漏 j",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  j.reset();\n  a.reset();`,
      to: `  a.reset();\n  a.reset();`,
    },
    {
      note: "ck reset 漏 a",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  a.reset();\n}`,
      to: `  L.reset();\n}`,
    },
    {
      note: "ck snap 槽 0 错",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  snap.push_back(L.to_snapshot());`,
      to: `  snap.push_back(R.to_snapshot());`,
    },
    {
      note: "ck snap 槽 1 错",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  snap.push_back(R.to_snapshot());`,
      to: `  snap.push_back(U.to_snapshot());`,
    },
    {
      note: "ck snap 槽 2 错",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  snap.push_back(U.to_snapshot());`,
      to: `  snap.push_back(D.to_snapshot());`,
    },
    {
      note: "ck snap 槽 3 错",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  snap.push_back(D.to_snapshot());`,
      to: `  snap.push_back(d.to_snapshot());`,
    },
    {
      note: "ck snap 槽 4 错",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  snap.push_back(d.to_snapshot());`,
      to: `  snap.push_back(j.to_snapshot());`,
    },
    {
      note: "ck snap 槽 5 错",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  snap.push_back(j.to_snapshot());`,
      to: `  snap.push_back(a.to_snapshot());`,
    },
    {
      note: "ck snap 槽 6 错",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  snap.push_back(a.to_snapshot());`,
      to: `  snap.push_back(j.to_snapshot());`,
    },
    {
      note: "ck 空数组判定取反",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  const size_t n = arr != nullptr ? arr->size() : 0;`,
      to: `  const size_t n = arr == nullptr ? arr->size() : 0;`,
    },
    {
      note: "ck load L 索引",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  L.from_snapshot(n > 0 ? arr->at(0) : Value());`,
      to: `  L.from_snapshot(n > 0 ? arr->at(1) : Value());`,
    },
    {
      note: "ck load R 索引",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  R.from_snapshot(n > 1 ? arr->at(1) : Value());`,
      to: `  R.from_snapshot(n > 1 ? arr->at(2) : Value());`,
    },
    {
      note: "ck load U 索引",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  U.from_snapshot(n > 2 ? arr->at(2) : Value());`,
      to: `  U.from_snapshot(n > 2 ? arr->at(3) : Value());`,
    },
    {
      note: "ck load D 索引",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  D.from_snapshot(n > 3 ? arr->at(3) : Value());`,
      to: `  D.from_snapshot(n > 3 ? arr->at(4) : Value());`,
    },
    {
      note: "ck load d 索引",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  d.from_snapshot(n > 4 ? arr->at(4) : Value());`,
      to: `  d.from_snapshot(n > 4 ? arr->at(5) : Value());`,
    },
    {
      note: "ck load j 索引",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  j.from_snapshot(n > 5 ? arr->at(5) : Value());`,
      to: `  j.from_snapshot(n > 5 ? arr->at(6) : Value());`,
    },
    {
      note: "ck load a 索引",
      file: "native/lfw/controller/controller_key_status.cpp",
      from: `  a.from_snapshot(n > 6 ? arr->at(6) : Value());`,
      to: `  a.from_snapshot(n > 6 ? arr->at(5) : Value());`,
    },
    {
      note: "res clear 后 result 非空",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  _result = Value();\n  _time = 0;`,
      to: `  _result = Value(std::u16string(u"x"));\n  _time = 0;`,
    },
    {
      note: "res clear 后 time 非 0",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  _time = 0;\n  _keys.clear();`,
      to: `  _time = 1;\n  _keys.clear();`,
    },
    {
      note: "res clear 后 keys 非空",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  _keys.clear();\n  _kind.clear();`,
      to: `  _keys = u"x";\n  _kind.clear();`,
    },
    {
      note: "res clear 后 kind 非空",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  _kind.clear();\n  return *this;`,
      to: `  _kind = u"x";\n  return *this;`,
    },
    {
      note: "res fire 用 nf 绕过 resolver",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  const Value result = _resolve ? _resolve(nf) : Value();`,
      to: `  const Value result = _resolve ? nf : Value();`,
    },
    {
      note: "res fire 守卫取反",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  if (!truthy(result)) return false;`,
      to: `  if (truthy(result)) return false;`,
    },
    {
      note: "res fire 不存 result",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  if (!truthy(result)) return false;\n  _result = result;\n  _time = time;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
      to: `  if (!truthy(result)) return false;\n  _result = Value();\n  _time = time;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
    },
    {
      note: "res fire 不存 time",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  if (!truthy(result)) return false;\n  _result = result;\n  _time = time;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
      to: `  if (!truthy(result)) return false;\n  _result = result;\n  _time = 0;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
    },
    {
      note: "res fire 不存 keys",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  if (!truthy(result)) return false;\n  _result = result;\n  _time = time;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
      to: `  if (!truthy(result)) return false;\n  _result = result;\n  _time = time;\n  _keys = kind;\n  _kind = kind;\n  return true;`,
    },
    {
      note: "res fire 不存 kind",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  if (!truthy(result)) return false;\n  _result = result;\n  _time = time;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
      to: `  if (!truthy(result)) return false;\n  _result = result;\n  _time = time;\n  _keys = keys;\n  _kind = keys;\n  return true;`,
    },
    {
      note: "res fire 恒 false",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  _kind = kind;\n  return true;\n}\n\nbool ControllerResult::fire2`,
      to: `  _kind = kind;\n  return false;\n}\n\nbool ControllerResult::fire2`,
    },
    {
      note: "res fire2 不存 result",
      file: "native/lfw/controller/controller_result.cpp",
      from: `                             const std::u16string& kind) {\n  _result = result;\n  _time = time;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
      to: `                             const std::u16string& kind) {\n  _result = Value();\n  _time = time;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
    },
    {
      note: "res fire2 不存 time",
      file: "native/lfw/controller/controller_result.cpp",
      from: `                             const std::u16string& kind) {\n  _result = result;\n  _time = time;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
      to: `                             const std::u16string& kind) {\n  _result = result;\n  _time = 0;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
    },
    {
      note: "res fire2 不存 keys",
      file: "native/lfw/controller/controller_result.cpp",
      from: `                             const std::u16string& kind) {\n  _result = result;\n  _time = time;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
      to: `                             const std::u16string& kind) {\n  _result = result;\n  _time = time;\n  _keys = u"z";\n  _kind = kind;\n  return true;`,
    },
    {
      note: "res fire2 不存 kind",
      file: "native/lfw/controller/controller_result.cpp",
      from: `                             const std::u16string& kind) {\n  _result = result;\n  _time = time;\n  _keys = keys;\n  _kind = kind;\n  return true;`,
      to: `                             const std::u16string& kind) {\n  _result = result;\n  _time = time;\n  _keys = keys;\n  _kind = u"z";\n  return true;`,
    },
    {
      note: "res fire2 恒 false",
      file: "native/lfw/controller/controller_result.cpp",
      from: `  _keys = keys;\n  _kind = kind;\n  return true;\n}\n\n}`,
      to: `  _keys = keys;\n  _kind = kind;\n  return false;\n}\n\n}`,
    },
  ],
};
