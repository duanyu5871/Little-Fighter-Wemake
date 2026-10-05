// Mutation spec for the `base` differential subject.
//
// Subject: native/lfw/base/* + the `src/LFW/base/*` modules mirrored by the `base`
// harness (FSM / Callbacks / NoEmitCallbacks / ValExpression / Ticker / FPS).
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * Slice 2g (`base/team_color.h` + `base/get_short_file_size_txt.h` + `core` 的
//    `toFixed(1)`):
//    - `get_team_text_color` 里 `!color->empty()` 这一半不可观察：出货数据（`Defines.TeamInfoMap`）
//      每个队伍的 `txt_color` 都是非空字符串，而两侧读的是同一份数据、harness 也注入不了。
//    - 两个颜色函数对「字段缺失 / 非字符串」的分支不可达（同一原因）：TS 侧 `0` 会被当假值走
//      fallback、`1` 会原样返回数字，端口只认字符串。
//    - `get_team_outline_color` 的 `info.txt_outline_color` **没有** `?.`：Independent 那一项
//      缺字段时 TS 会抛 TypeError，端口回空串（数据里字段恒在 ⇒ 不可达）。
//    - `.replace(".0", "")` 用 `find` 还是 `rfind` 等价：`toFixed(1)` 的定点串与
//      `String(number)` 的指数串里 ".0" 最多出现一次。
//  * Slice 2f (`base/clock.h` + `Ticker` + `FPS`):
//    - `Ticker._schedule`'s `_pending` term in the guard is unobservable: every caller
//      either just cleared `_pending` (`start` / `_tick` / `resync` via `cancel`) or
//      cannot see a stale callback fire, so `_schedule` is never entered with
//      `_pending == true`.
//    - `_tick`'s `if (!_running) return;` needs a callback that still fires after
//      `stop()`, but `stop()` cancels the handle first (`FIRE -` in the harness).
//    - `IClock::del` on an unknown handle has no observable trace (both the port and
//      the fake hosts make it a no-op).
//    - The lower bound of `clamp(t0 - _last_step, 0, _base * 4)` needs the clock to go
//      backwards, which the fake clock in the case never does (the upper bound is
//      observable and listed).
//  * One candidate is withdrawn as equivalent by construction: `step_once`'s
//    `want = clamp((cost * safety) / _base, 1, max_span)` has its upper bound replaced
//    by the literal `2`. `_span` is clamped by the *same* `max_span` right after, and
//    whenever `want < max_span` the inner slew clamp makes `_span` reach exactly
//    `want`, so both `max_span` values give the same `_span` (it was the run's only
//    survivor; see DESIGN §59.4).
//
export default {
  subject: "base",
  mutations: [
    {
      note: "compact 之后没有把 head 归零（等价候选：size()-_head 写成 size() 在 flush 之外恒等价，不予采用）",
      file: "native/lfw/base/no_emit_callbacks.h",
      from: `      _pendings.erase(_pendings.begin(), _pendings.begin() + static_cast<std::ptrdiff_t>(_head));
      _head = 0;`,
      to: `      _pendings.erase(_pendings.begin(), _pendings.begin() + static_cast<std::ptrdiff_t>(_head));`,
    },
    {
      note: "单次 flush 上限边界 >= 写成 >",
      file: "native/lfw/base/no_emit_callbacks.h",
      from: `if (done >= kMaxPendingsPerFlush) {`,
      to: `if (done > kMaxPendingsPerFlush) {`,
    },
    {
      note: "溢出标志在队列耗尽后没有复位",
      file: "native/lfw/base/no_emit_callbacks.h",
      from: `      _overflow_warned = false;
      compact();
    }

    std::u16string _key;`,
      to: `      compact();
    }

    std::u16string _key;`,
    },
    {
      note: "once 的延迟删除条件取反",
      file: "native/lfw/base/no_emit_callbacks.h",
      from: `if (v->once) _waits.push_back({WaitKind::kDel, v});`,
      to: `if (!v->once) _waits.push_back({WaitKind::kDel, v});`,
    },
    {
      note: "每轮派发后不再应用延迟的 add/del",
      file: "native/lfw/base/no_emit_callbacks.h",
      from: `        handle_waits();
        _emiting = false;`,
      to: `        _emiting = false;`,
    },
    {
      note: "handler 查找用错 key",
      file: "native/lfw/base/no_emit_callbacks.h",
      from: `const Fn* f = find_handler(v, _key);`,
      to: `const Fn* f = find_handler(v, u"nope");`,
    },
    {
      note: "handler 查找不比对 key，直接取第一个",
      file: "native/lfw/base/no_emit_callbacks.h",
      from: `        if (h.key == key) return &h.fn;`,
      to: `        if (h.key != u"") return &h.fn;`,
    },
    {
      note: "插入顺序改成头插（Set 顺序语义被破坏）",
      file: "native/lfw/base/no_emit_callbacks.h",
      from: `      _set.push_back(v);
    }

    void set_remove(Listener* v) {`,
      to: `      _set.insert(_set.begin(), v);
    }

    void set_remove(Listener* v) {`,
    },
    {
      note: "keys() 顺序反转",
      file: "native/lfw/base/no_emit_callbacks.h",
      from: `    for (const Pack& pack : _packs) out.push_back(pack.key());`,
      to: `    for (const Pack& pack : _packs) out.insert(out.begin(), pack.key());`,
    },
    {
      note: "溢出提示文案被改写",
      file: "native/lfw/base/no_emit_callbacks.h",
      from: `u" still pending \\u2014 possible broadcast loop? Remaining jobs will be dispatched on the "`,
      to: `u" still pending - possible broadcast loop? Remaining jobs will be dispatched on the "`,
    },
    {
      note: "FSM 切状态时 state_time 没有归零",
      file: "native/lfw/base/fsm.h",
      from: `    _state_time = 0;
    _state = next_state;`,
      to: `    _state = next_state;`,
    },
    {
      note: "FSM 切状态时 prev_state 没有更新",
      file: "native/lfw/base/fsm.h",
      from: `    _prev_state = _state;
    if (_state != nullptr) _state->leave();`,
      to: `    if (_state != nullptr) _state->leave();`,
    },
    {
      note: "FSM 旧状态没有 leave",
      file: "native/lfw/base/fsm.h",
      from: `    if (_state != nullptr) _state->leave();`,
      to: ``,
    },
    {
      note: "FSM 新状态没有 enter",
      file: "native/lfw/base/fsm.h",
      from: `    if (next_state != nullptr) next_state->enter();`,
      to: ``,
    },
    {
      note: "FSM 总时长累积错误",
      file: "native/lfw/base/fsm.h",
      from: `    _time += dt;
    _state_time += dt;`,
      to: `    _time += dt * 2;
    _state_time += dt;`,
    },
    {
      note: "状态标签用严格相等（丢掉 name 与 key 的宽松相等语义）",
      file: "native/lfw/base/fsm.h",
      from: `    if (equals(Value(name), key)) return name;`,
      to: `    if (strict_equals(Value(name), key)) return name;`,
    },
    {
      note: "无当前状态时的标签文案改错",
      file: "native/lfw/base/fsm.h",
      from: `    if (s == nullptr) return u"undefined";`,
      to: `    if (s == nullptr) return u"?";`,
    },
    {
      note: "切状态日志格式改错",
      file: "native/lfw/base/fsm.h",
      from: `state_label(_prev_state) + u" ==> " + state_label(_state));`,
      to: `state_label(_prev_state) + u" -> " + state_label(_state));`,
    },
    {
      note: "snapshot 恢复时没有还原总时长",
      file: "native/lfw/base/fsm.h",
      from: `    _name = s.name;
    _time = s.time;`,
      to: `    _name = s.name;
    _time = 0;`,
    },
    {
      note: "状态切换不再通知 on_state_changed",
      file: "native/lfw/base/fsm.h",
      from: `    _callbacks.call(u"on_state_changed", {this});`,
      to: `    (void)this;`,
    },
    {
      note: "update 不再沿 update() 返回的 key 切状态",
      file: "native/lfw/base/fsm.h",
      from: `    IState* next_state = get(*next_key);
    if (next_state == nullptr) return;

    set_state(next_state);`,
      to: `    (void)next_key;`,
    },

    // ---------------------------------------------------------------------------
    // 切片 9o：`base/ValExpression` + `loader/preprocess_opoint`
    // ---------------------------------------------------------------------------
    {
      note: "valexpr：空白不剥离",
      file: "native/lfw/base/val_expression.h",
      from: `      if (!is_str_white_space(c)) text.push_back(c);`,
      to: `      if (true) text.push_back(c);`,
    },
    {
      note: "valexpr：tag 缺省值写错",
      file: "native/lfw/base/val_expression.h",
      from: `    tag = options.tag.empty() ? std::u16string(u"val_expr") : options.tag;`,
      to: `    tag = options.tag.empty() ? std::u16string(u"val") : options.tag;`,
    },
    {
      note: "valexpr：空 tag 不回落缺省值",
      file: "native/lfw/base/val_expression.h",
      from: `    tag = options.tag.empty() ? std::u16string(u"val_expr") : options.tag;`,
      to: `    tag = options.tag;`,
    },
    {
      note: "valexpr：空表达式不报错",
      file: "native/lfw/base/val_expression.h",
      from: `    if (text.empty()) {
      fail(u"empty expression");
      return;
    }`,
      to: `    if (false) {
      fail(u"empty expression");
      return;
    }`,
    },
    {
      note: "valexpr：尾部残留不报错",
      file: "native/lfw/base/val_expression.h",
      from: `    if (_index < text.size()) {
      fail(u"unexpected '" + text.substr(_index) + u"'");
      return;
    }`,
      to: `    if (false) {
      fail(u"unexpected '" + text.substr(_index) + u"'");
      return;
    }`,
    },
    {
      note: "valexpr：加法写成减法",
      file: "native/lfw/base/val_expression.h",
      from: `        return a + b;`,
      to: `        return a - b;`,
    },
    {
      note: "valexpr：减法写成加法",
      file: "native/lfw/base/val_expression.h",
      from: `                            return a - b;`,
      to: `                            return a + b;`,
    },
    {
      note: "valexpr：乘法写成除法",
      file: "native/lfw/base/val_expression.h",
      from: `        return a * b;`,
      to: `        return a / b;`,
    },
    {
      note: "valexpr：除法写成乘法",
      file: "native/lfw/base/val_expression.h",
      from: `                            return a / b;`,
      to: `                            return a * b;`,
    },
    {
      note: "valexpr：一元负号不取负",
      file: "native/lfw/base/val_expression.h",
      from: `    return Get([v](const Ctx& e) { return -v(e); });`,
      to: `    return Get([v](const Ctx& e) { return v(e); });`,
    },
    {
      note: "valexpr：乘除项不再接受一元符号（丢掉递归）",
      file: "native/lfw/base/val_expression.h",
      from: `  Get ParseTerm() {
    Get ret = ParseUnary();`,
      to: `  Get ParseTerm() {
    Get ret = ParsePrimary();`,
    },
    {
      note: "valexpr：左括号不检查右括号",
      file: "native/lfw/base/val_expression.h",
      from: `      if (at(_index) != u')') {
        fail(u"missing ')'");
        return nullptr;
      }
      ++_index;
      return v;`,
      to: `      if (false) {
        fail(u"missing ')'");
        return nullptr;
      }
      ++_index;
      return v;`,
    },
    {
      note: "valexpr：左括号不消费右括号",
      file: "native/lfw/base/val_expression.h",
      from: `      ++_index;
      return v;
    }`,
      to: `      return v;
    }`,
    },
    {
      note: "valexpr：数字判定把 9 排除",
      file: "native/lfw/base/val_expression.h",
      from: `inline bool is_digit(char16_t c) { return c >= u'0' && c <= u'9'; }`,
      to: `inline bool is_digit(char16_t c) { return c >= u'0' && c < u'9'; }`,
    },
    {
      note: "valexpr：标识符字符集漏掉下划线",
      file: "native/lfw/base/val_expression.h",
      from: `  return (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z') || (c >= u'0' && c <= u'9') ||
         c == u'_';`,
      to: `  return (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z') || (c >= u'0' && c <= u'9');`,
    },
    {
      note: "valexpr：小数部分不解析",
      file: "native/lfw/base/val_expression.h",
      from: `    if (at(_index) == u'.') {`,
      to: `    if (false) {`,
    },
    {
      note: "valexpr：不检查数值有限（NaN 也当合法）",
      file: "native/lfw/base/val_expression.h",
      from: `    if (!std::isfinite(value)) {`,
      to: `    if (false) {`,
    },
    {
      note: "valexpr：数字字面量多 1",
      file: "native/lfw/base/val_expression.h",
      from: `    return Get([value](const Ctx&) { return value; });`,
      to: `    return Get([value](const Ctx&) { return value + 1; });`,
    },
    {
      note: "valexpr：任何变量都返回 w",
      file: "native/lfw/base/val_expression.h",
      from: `      return it->second;
    }
    ++_index;`,
      to: `      return default_vars().at(u"w");
    }
    ++_index;`,
    },
    {
      note: "valexpr：自定义变量不覆盖默认（emplace）",
      file: "native/lfw/base/val_expression.h",
      from: `    for (const auto& kv : custom_vars) _vars[kv.first] = kv.second;`,
      to: `    for (const auto& kv : custom_vars) _vars.emplace(kv.first, kv.second);`,
    },
    {
      note: "valexpr：默认变量 w 读成了 height",
      file: "native/lfw/base/val_expression.h",
      from: `        {u"w", [](const Ctx& e) { return e.frame_var(u"width"); }},`,
      to: `        {u"w", [](const Ctx& e) { return e.frame_var(u"height"); }},`,
    },
    {
      note: "valexpr：默认变量 cy 读成了 centerx",
      file: "native/lfw/base/val_expression.h",
      from: `        {u"cy", [](const Ctx& e) { return e.frame_var(u"centery"); }},`,
      to: `        {u"cy", [](const Ctx& e) { return e.frame_var(u"centerx"); }},`,
    },
    {
      note: "valexpr：实参分隔符只吃一个逗号",
      file: "native/lfw/base/val_expression.h",
      from: `      while (at(_index) == u',') {`,
      to: `      if (at(_index) == u',') {`,
    },
    {
      note: "valexpr：调用不检查右括号",
      file: "native/lfw/base/val_expression.h",
      from: `    if (at(_index) != u')') {
      fail(u"missing ')' for '" + name + u"(...'");
      return nullptr;
    }`,
      to: `    if (false) {
      fail(u"missing ')' for '" + name + u"(...'");
      return nullptr;
    }`,
    },
    {
      note: "valexpr：rand 的参数个数上界放宽",
      file: "native/lfw/base/val_expression.h",
      from: `      if (args.size() != 2) {`,
      to: `      if (args.size() < 2) {`,
    },
    {
      note: "valexpr：rand 不写 mark",
      file: "native/lfw/base/val_expression.h",
      from: `        // TS: 先写 \`mark\`、再按**从左到右**求两个参数、最后抽取。
        mt.mark = tag_snapshot;
        const double lo = a(e);`,
      to: `        const double lo = a(e);`,
    },
    {
      note: "valexpr：rand 的两个参数求值顺序颠倒",
      file: "native/lfw/base/val_expression.h",
      from: `        const double lo = a(e);
        const double hi = b(e);
        return mt.range(lo, hi);`,
      to: `        const double hi = b(e);
        const double lo = a(e);
        return mt.range(lo, hi);`,
    },
    {
      note: "valexpr：pick 不写 mark",
      file: "native/lfw/base/val_expression.h",
      from: `        for (std::size_t i = 0; i < args.size(); ++i) (*scratch)[i] = args[i](e);
        MersenneTwister& mt = e.mt();
        mt.mark = tag_snapshot;`,
      to: `        for (std::size_t i = 0; i < args.size(); ++i) (*scratch)[i] = args[i](e);
        MersenneTwister& mt = e.mt();`,
    },
    {
      note: "valexpr：pick 不抽取，直接取第一个",
      file: "native/lfw/base/val_expression.h",
      from: `        const std::optional<double> v = mt.pick(*scratch);
        return v.has_value() ? *v : val_expr_detail::undefined_number();`,
      to: `        const std::optional<double> v = (*scratch)[0];
        return v.has_value() ? *v : val_expr_detail::undefined_number();`,
    },
    {
      note: "valexpr：pick 的实参倒序求值",
      file: "native/lfw/base/val_expression.h",
      from: `      return Get([tag_snapshot, args, scratch](const Ctx& e) -> double {
        for (std::size_t i = 0; i < args.size(); ++i) (*scratch)[i] = args[i](e);`,
      to: `      return Get([tag_snapshot, args, scratch](const Ctx& e) -> double {
        for (std::size_t i = args.size(); i-- > 0;) (*scratch)[i] = args[i](e);`,
    },
    {
      note: "valexpr：bag 的重填过滤条件取反",
      file: "native/lfw/base/val_expression.h",
      from: `              if (!taken->has_value() || v != **taken) cur->push_back(v);`,
      to: `              if (!taken->has_value() || v == **taken) cur->push_back(v);`,
    },
    {
      note: "valexpr：bag 抽走以后不删除",
      file: "native/lfw/base/val_expression.h",
      from: `        cur->erase(cur->begin() + static_cast<std::ptrdiff_t>(i));`,
      to: `        (void)i;`,
    },
    {
      note: "valexpr：bag 不记录 taken",
      file: "native/lfw/base/val_expression.h",
      from: `        *taken = got;
        return got;`,
      to: `        return got;`,
    },
    {
      note: "valexpr：bag 重填兜底填 0",
      file: "native/lfw/base/val_expression.h",
      from: `          if (cur->empty()) *cur = *scratch;`,
      to: `          if (cur->empty()) cur->push_back(0.0);`,
    },
    {
      note: "valexpr：bag 不写 mark",
      file: "native/lfw/base/val_expression.h",
      from: `        MersenneTwister& mt = e.mt();
        mt.mark = tag_snapshot;
        const double idx = mt.range(0.0, static_cast<double>(cur->size()));`,
      to: `        MersenneTwister& mt = e.mt();
        const double idx = mt.range(0.0, static_cast<double>(cur->size()));`,
    },
    {
      note: "valexpr：flip 的候选表顺序颠倒",
      file: "native/lfw/base/val_expression.h",
      from: `  static const std::vector<double> kValues{-1.0, 1.0};`,
      to: `  static const std::vector<double> kValues{1.0, -1.0};`,
    },
    {
      note: "valexpr：flip 不写 mark",
      file: "native/lfw/base/val_expression.h",
      from: `      return Get([tag_snapshot](const Ctx& e) -> double {
        MersenneTwister& mt = e.mt();
        mt.mark = tag_snapshot;
        const std::optional<double> v = mt.pick(val_expr_detail::flip_values());`,
      to: `      return Get([tag_snapshot](const Ctx& e) -> double {
        MersenneTwister& mt = e.mt();
        const std::optional<double> v = mt.pick(val_expr_detail::flip_values());`,
    },
    {
      note: "valexpr：round 用 floor",
      file: "native/lfw/base/val_expression.h",
      from: `      return Get([a](const Ctx& e) { return round(a(e)); });`,
      to: `      return Get([a](const Ctx& e) { return floor(a(e)); });`,
    },
    {
      note: "valexpr：round 不取整",
      file: "native/lfw/base/val_expression.h",
      from: `      return Get([a](const Ctx& e) { return round(a(e)); });`,
      to: `      return Get([a](const Ctx& e) { return a(e); });`,
    },
    {
      note: "valexpr：解析失败时 _get 回落成 1",
      file: "native/lfw/base/val_expression.h",
      from: `  Get _get = [](const Ctx&) { return 0.0; };`,
      to: `  Get _get = [](const Ctx&) { return 1.0; };`,
    },
    {
      note: "valexpr：解析成功也不装 _get",
      file: "native/lfw/base/val_expression.h",
      from: `    if (ret != nullptr) _get = ret;`,
      to: `    (void)ret;`,
    },
    {
      note: "valexpr：错误前缀改写",
      file: "native/lfw/base/val_expression.h",
      from: `    err = u"[ValExpression] " + tag + u": " + message + u" @" + val_expr_detail::dec(_index) +`,
      to: `    err = u"[ValExpr] " + tag + u": " + message + u" @" + val_expr_detail::dec(_index) +`,
    },
    {
      note: "valexpr：错误里缺 tag 分隔符",
      file: "native/lfw/base/val_expression.h",
      from: `    err = u"[ValExpression] " + tag + u": " + message + u" @" + val_expr_detail::dec(_index) +`,
      to: `    err = u"[ValExpression] " + tag + message + u" @" + val_expr_detail::dec(_index) +`,
    },
    {
      note: "valexpr：错误的位置分隔符改写",
      file: "native/lfw/base/val_expression.h",
      from: `    err = u"[ValExpression] " + tag + u": " + message + u" @" + val_expr_detail::dec(_index) +`,
      to: `    err = u"[ValExpression] " + tag + u": " + message + u" #" + val_expr_detail::dec(_index) +`,
    },
    {
      note: "valexpr：错误里的原文括号改写",
      file: "native/lfw/base/val_expression.h",
      from: `          u" in \\"" + text + u"\\"";`,
      to: `          u" in [" + text + u"]";`,
    },
    {
      note: "valexpr：下标十进制化用 9 取模",
      file: "native/lfw/base/val_expression.h",
      from: `    out.insert(out.begin(), static_cast<char16_t>(u'0' + (v % 10)));`,
      to: `    out.insert(out.begin(), static_cast<char16_t>(u'0' + (v % 9)));`,
    },
    {
      note: "valexpr：空表达式的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `      fail(u"empty expression");`,
      to: `      fail(u"empty");`,
    },
    {
      note: "valexpr：表达式提前结束的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `      fail(u"unexpected end of expression");`,
      to: `      fail(u"unexpected end");`,
    },
    {
      note: "valexpr：尾部残留的文案去掉引号",
      file: "native/lfw/base/val_expression.h",
      from: `      fail(u"unexpected '" + text.substr(_index) + u"'");`,
      to: `      fail(u"unexpected " + text.substr(_index));`,
    },
    {
      note: "valexpr：非法字符的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `    fail(u"unexpected character '" + std::u16string(1, *c) + u"'");`,
      to: `    fail(u"bad character '" + std::u16string(1, *c) + u"'");`,
    },
    {
      note: "valexpr：坏数字的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `      fail(u"bad number '" + raw + u"'");`,
      to: `      fail(u"bad numeric '" + raw + u"'");`,
    },
    {
      note: "valexpr：未知标识符的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `        fail(u"unknown identifier '" + name + u"'");`,
      to: `        fail(u"unknown ident '" + name + u"'");`,
    },
    {
      note: "valexpr：rand 参数个数的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `        fail(u"'rand' expects 2 arguments, got " + val_expr_detail::dec(args.size()));`,
      to: `        fail(u"'rand' wants 2 arguments, got " + val_expr_detail::dec(args.size()));`,
    },
    {
      note: "valexpr：pick 参数个数的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `        fail(u"'pick' expects at least 1 argument");`,
      to: `        fail(u"'pick' needs at least 1 argument");`,
    },
    {
      note: "valexpr：bag 参数个数的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `        fail(u"'bag' expects at least 1 argument");`,
      to: `        fail(u"'bag' needs at least 1 argument");`,
    },
    {
      note: "valexpr：flip 参数个数的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `        fail(u"'flip' expects no arguments");`,
      to: `        fail(u"'flip' takes no arguments");`,
    },
    {
      note: "valexpr：round 参数个数的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `        fail(u"'round' expects 1 argument, got " + val_expr_detail::dec(args.size()));`,
      to: `        fail(u"'round' wants 1 argument, got " + val_expr_detail::dec(args.size()));`,
    },
    {
      note: "valexpr：未知函数的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `    fail(u"unknown function '" + name + u"'");`,
      to: `    fail(u"unknown func '" + name + u"'");`,
    },
    {
      note: "valexpr：调用缺右括号的文案改写",
      file: "native/lfw/base/val_expression.h",
      from: `      fail(u"missing ')' for '" + name + u"(...'");`,
      to: `      fail(u"missing ')' of '" + name + u"(...'");`,
    },
    {
      note: "opoint：字段表前两项顺序颠倒",
      file: "native/lfw/loader/preprocess_opoint.h",
      from: `      {u"gen_x", u"__gen_x"},
      {u"gen_y", u"__gen_y"},`,
      to: `      {u"gen_y", u"__gen_y"},
      {u"gen_x", u"__gen_x"},`,
    },
    {
      note: "opoint：gen_x 写进 __gen_y",
      file: "native/lfw/loader/preprocess_opoint.h",
      from: `      {u"gen_x", u"__gen_x"},
      {u"gen_y", u"__gen_y"},`,
      to: `      {u"gen_x", u"__gen_y"},
      {u"gen_y", u"__gen_y"},`,
    },
    {
      note: "opoint：忽略 falsy 的 gen_* 源",
      file: "native/lfw/loader/preprocess_opoint.h",
      from: `    if (src == nullptr || !truthy(*src)) continue;`,
      to: `    if (src == nullptr) continue;`,
    },
    {
      note: "opoint：编译用的 tag 写成 __gen_* 名",
      file: "native/lfw/loader/preprocess_opoint.h",
      from: `    options.tag = field.first;`,
      to: `    options.tag = field.second;`,
    },
    {
      note: "opoint：结果表的键写成 gen_* 名",
      file: "native/lfw/loader/preprocess_opoint.h",
      from: `    out.emplace(field.second, std::move(expr));`,
      to: `    out.emplace(field.first, std::move(expr));`,
    },
    {
      note: "opoint：解析失败不告警",
      file: "native/lfw/loader/preprocess_opoint.h",
      from: `      if (warn) warn(expr.err);`,
      to: `      (void)expr;`,
    },
    {
      note: "opoint：解析失败也收进结果表",
      file: "native/lfw/loader/preprocess_opoint.h",
      from: `    if (expr.has_err) {`,
      to: `    if (false) {`,
    },

    // ---- 2f：base/Ticker ---------------------------------------------------------
    {
      note: "Ticker.start：重复 start 也重新计时（丢掉 _running 守卫）",
      file: "native/lfw/base/ticker.h",
      from: `    if (_running) return;
    _running = true;`,
      to: `    _running = true;`,
    },
    {
      note: "Ticker.start：首帧截止点不含一个步长",
      file: "native/lfw/base/ticker.h",
      from: `    _deadline = now + _base;`,
      to: `    _deadline = now;`,
    },
    {
      note: "Ticker.start：rate 不防 _base == 0",
      file: "native/lfw/base/ticker.h",
      from: `    _rate = _base > 0 ? 1000 / _base : 0;`,
      to: `    _rate = 1000 / _base;`,
    },
    {
      note: "Ticker.start：不复位 _span",
      file: "native/lfw/base/ticker.h",
      from: `    _span = 1;
    cost = 0;
    const double now = clock_now();`,
      to: `    cost = 0;
    const double now = clock_now();`,
    },
    {
      note: "Ticker.start：不复位 cost",
      file: "native/lfw/base/ticker.h",
      from: `    _span = 1;
    cost = 0;
    const double now`,
      to: `    _span = 1;
    const double now`,
    },
    {
      note: "Ticker.stop：不清 _paused",
      file: "native/lfw/base/ticker.h",
      from: `    _running = false;
    _paused = false;
    cancel();`,
      to: `    _running = false;
    cancel();`,
    },
    {
      note: "Ticker.stop：不 cancel（残留回调）",
      file: "native/lfw/base/ticker.h",
      from: `    if (!_running) return;
    _running = false;
    _paused = false;`,
      to: `    if (!_running) return;
    _running = false;`,
    },
    {
      note: "Ticker.pause：加 _running 守卫（TS 没有）",
      file: "native/lfw/base/ticker.h",
      from: `    if (_paused) return;
    _paused = true;
    cancel();`,
      to: `    if (_paused || !_running) return;
    _paused = true;
    cancel();`,
    },
    {
      note: "Ticker.pause：不 cancel",
      file: "native/lfw/base/ticker.h",
      from: `    _paused = true;
    cancel();
  }`,
      to: `    _paused = true;
  }`,
    },
    {
      note: "Ticker.resume：不检查 _paused",
      file: "native/lfw/base/ticker.h",
      from: `    if (!_running || !_paused) return;`,
      to: `    if (!_running) return;`,
    },
    {
      note: "Ticker.resume：不重读 step_ms",
      file: "native/lfw/base/ticker.h",
      from: `    _base = _opt->step_ms();
    const double now = clock_now();
    _deadline = max(now, _last_step + _base * _span);`,
      to: `    const double now = clock_now();
    _deadline = max(now, _last_step + _base * _span);`,
    },
    {
      note: "Ticker.resume：截止点丢掉 max（直接用 now）",
      file: "native/lfw/base/ticker.h",
      from: `    _deadline = max(now, _last_step + _base * _span);`,
      to: `    _deadline = now;`,
    },
    {
      note: "Ticker.resync：不检查 _running",
      file: "native/lfw/base/ticker.h",
      from: `    if (!_running) return;
    const double now = clock_now();
    _base = _opt->step_ms();
    _deadline = immediate ?`,
      to: `    const double now = clock_now();
    _base = _opt->step_ms();
    _deadline = immediate ?`,
    },
    {
      note: "Ticker.resync：immediate 分支取反",
      file: "native/lfw/base/ticker.h",
      from: `    if (!immediate) _last_step = now;`,
      to: `    if (immediate) _last_step = now;`,
    },
    {
      note: "Ticker.resync：不复位 rate 计数",
      file: "native/lfw/base/ticker.h",
      from: `    _rate_start = now;
    _rate_steps = 0;
    cancel();`,
      to: `    _rate_start = now;
    cancel();`,
    },
    {
      note: "Ticker.resync：不复位 rate 窗口起点",
      file: "native/lfw/base/ticker.h",
      from: `    if (!immediate) _last_step = now;
    _rate_start = now;
    _rate_steps = 0;`,
      to: `    if (!immediate) _last_step = now;
    _rate_steps = 0;`,
    },
    {
      note: "Ticker.cancel：不清 _pending",
      file: "native/lfw/base/ticker.h",
      from: `    _pending = false;
    if (_timer != 0) {`,
      to: `    if (_timer != 0) {`,
    },
    {
      note: "Ticker.cancel：不删 Clock 句柄",
      file: "native/lfw/base/ticker.h",
      from: `    if (_wake_id != 0) {
      clock_del(_wake_id);
      _wake_id = 0;
    }
  }`,
      to: `  }`,
    },
    {
      note: "Ticker.cancel：删完不清 _timer",
      file: "native/lfw/base/ticker.h",
      from: `      timeout_del(_timer);
      _timer = 0;`,
      to: `      timeout_del(_timer);`,
    },
    {
      note: "Ticker.schedule：sleep_threshold 边界 > 写成 >=",
      file: "native/lfw/base/ticker.h",
      from: `    if (delay > sleep_threshold && !clock_hidden()) {`,
      to: `    if (delay >= sleep_threshold && !clock_hidden()) {`,
    },
    {
      note: "Ticker.schedule：守卫丢掉 !_running（重入 stop 后还会再排一个回调）",
      file: "native/lfw/base/ticker.h",
      from: `    if (!_running || _pending || _paused) return;`,
      to: `    if (_pending || _paused) return;`,
    },
    {
      note: "Ticker.schedule：守卫丢掉 _paused（重入 pause 后还会再排一个回调）",
      file: "native/lfw/base/ticker.h",
      from: `    if (!_running || _pending || _paused) return;`,
      to: `    if (!_running || _pending) return;`,
    },
    {
      note: "Ticker.schedule：不看 Clock.hidden()",
      file: "native/lfw/base/ticker.h",
      from: `    if (delay > sleep_threshold && !clock_hidden()) {`,
      to: `    if (delay > sleep_threshold) {`,
    },
    {
      note: "Ticker.schedule：Timeout 延时不减 1ms",
      file: "native/lfw/base/ticker.h",
      from: `      _timer = timeout_add([this]() { tick(); }, delay - 1);`,
      to: `      _timer = timeout_add([this]() { tick(); }, delay);`,
    },
    {
      note: "Ticker.schedule：Timeout 分支不置 _pending",
      file: "native/lfw/base/ticker.h",
      from: `      _pending = true;
      _timer = timeout_add`,
      to: `      _timer = timeout_add`,
    },
    {
      note: "Ticker.schedule：Clock 分支不置 _pending",
      file: "native/lfw/base/ticker.h",
      from: `    _pending = true;
    _wake_id = clock_add`,
      to: `    _wake_id = clock_add`,
    },
    {
      note: "Ticker.tick：不清 _timer",
      file: "native/lfw/base/ticker.h",
      from: `    _pending = false;
    _timer = 0;
    _wake_id = 0;`,
      to: `    _pending = false;
    _wake_id = 0;`,
    },
    {
      note: "Ticker.tick：不清 _pending",
      file: "native/lfw/base/ticker.h",
      from: `    _pending = false;
    _timer = 0;`,
      to: `    _timer = 0;`,
    },
    {
      note: "Ticker.step_once：base 变化阈值 0.05 写成 0.5",
      file: "native/lfw/base/ticker.h",
      from: `    if (base > 0 && abs(base - _base) > base * 0.05) {`,
      to: `    if (base > 0 && abs(base - _base) > base * 0.5) {`,
    },
    {
      note: "Ticker.step_once：base 变化后不按 cost 重算 _span",
      file: "native/lfw/base/ticker.h",
      from: `      _span = clamp((cost * safety) / base, 1, max_span);`,
      to: `      _span = 1;`,
    },    {
      note: "Ticker.step_once：去掉 step_ms 未变时的 _base 跟进分支",
      file: "native/lfw/base/ticker.h",
      from: `    } else if (base > 0) {
      _base = base;
    }`,
      to: `    }`,
    },
    {
      note: "Ticker.step_once：未到截止点也步进（< 写成 <=）",
      file: "native/lfw/base/ticker.h",
      from: `    if (t0 < _deadline) return;`,
      to: `    if (t0 <= _deadline) return;`,
    },
    {
      note: "Ticker.step_once：dt 上限 4 个步长写成 8 个",
      file: "native/lfw/base/ticker.h",
      from: `    const double dt = clamp(t0 - _last_step, 0, _base * 4);`,
      to: `    const double dt = clamp(t0 - _last_step, 0, _base * 8);`,
    },
    {
      note: "Ticker.step_once：cost 首次不走 truthy 直取",
      file: "native/lfw/base/ticker.h",
      from: `    cost = truthy(Value(cost)) ? cost * 0.9 + spent * 0.1 : spent;`,
      to: `    cost = cost * 0.9 + spent * 0.1;`,
    },
    {
      note: "Ticker.step_once：cost 的 EMA 系数颠倒",
      file: "native/lfw/base/ticker.h",
      from: `    cost = truthy(Value(cost)) ? cost * 0.9 + spent * 0.1 : spent;`,
      to: `    cost = truthy(Value(cost)) ? cost * 0.8 + spent * 0.2 : spent;`,
    },
    {
      note: "Ticker.step_once：span 的 slew 夹取下界上界颠倒",
      file: "native/lfw/base/ticker.h",
      from: `    _span = clamp(_span + clamp(want - _span, -slew, slew), 1, max_span);`,
      to: `    _span = clamp(_span + clamp(want - _span, slew, -slew), 1, max_span);`,
    },
    {
      note: "Ticker.step_once：截止点推进不含 _span",
      file: "native/lfw/base/ticker.h",
      from: `    _deadline += _base * _span;`,
      to: `    _deadline += _base;`,
    },
    {
      note: "Ticker.step_once：rate 换算写成 ×100",
      file: "native/lfw/base/ticker.h",
      from: `      _rate = (_rate_steps * 1000) / el;`,
      to: `      _rate = (_rate_steps * 100) / el;`,
    },
    {
      note: "Ticker.step_once：rate 窗口边界 >= 写成 >",
      file: "native/lfw/base/ticker.h",
      from: `    if (el >= rate_window) {`,
      to: `    if (el > rate_window) {`,
    },
    {
      note: "Ticker.step_once：rate 窗口后不复位计数",
      file: "native/lfw/base/ticker.h",
      from: `      _rate_steps = 0;
      _rate_start = t0;`,
      to: `      _rate_start = t0;`,
    },
    {
      note: "Ticker.step_once：rate 窗口后不推进起点",
      file: "native/lfw/base/ticker.h",
      from: `      _rate_steps = 0;
      _rate_start = t0;
    }`,
      to: `      _rate_steps = 0;
    }`,
    },
    {
      note: "Ticker.step_once：滞后阈值不乘 max_lag_steps",
      file: "native/lfw/base/ticker.h",
      from: `    if (t1 - _deadline > _base * max_lag_steps) {`,
      to: `    if (t1 - _deadline > _base) {`,
    },
    {
      note: "Ticker.step_once：滞后重置不更新 _last_step",
      file: "native/lfw/base/ticker.h",
      from: `      _deadline = t1 + _base * _span;
      _last_step = t1;`,
      to: `      _deadline = t1 + _base * _span;`,
    },
    {
      note: "Ticker.step_once：滞后重置的截止点不含 _span",
      file: "native/lfw/base/ticker.h",
      from: `      _deadline = t1 + _base * _span;`,
      to: `      _deadline = t1;`,
    },
    {
      note: "Ticker.step_once：整块滞后重置去掉",
      file: "native/lfw/base/ticker.h",
      from: `    const double t1 = clock_now();
    if (t1 - _deadline > _base * max_lag_steps) {
      _deadline = t1 + _base * _span;
      _last_step = t1;
    }
  }`,
      to: `    const double t1 = clock_now();
    (void)t1;
  }`,
    },
    {
      note: "Ticker.step：不含 _span",
      file: "native/lfw/base/ticker.h",
      from: `  double step() const { return _base * _span; }`,
      to: `  double step() const { return _base; }`,
    },

    // ---- 2f：base/FPS ------------------------------------------------------------
    {
      note: "FPS：保留率上限放到 1",
      file: "native/lfw/base/fps.h",
      from: `  explicit FPS(double retention = 0.99) { _retention = clamp(retention, 0, 0.99); }`,
      to: `  explicit FPS(double retention = 0.99) { _retention = clamp(retention, 0, 1); }`,
    },
    {
      note: "FPS.update：EMA 的保留/新值系数颠倒",
      file: "native/lfw/base/fps.h",
      from: `      _duration = _duration * _retention + dt * (1 - _retention);`,
      to: `      _duration = _duration * (1 - _retention) + dt * _retention;`,
    },
    {
      note: "FPS.update：不做首帧直取（永远走 EMA）",
      file: "native/lfw/base/fps.h",
      from: `    if (truthy(Value(_duration))) {`,
      to: `    if (true) {`,
    },
    {
      note: "FPS.update：跳过 EMA（永远直取 dt）",
      file: "native/lfw/base/fps.h",
      from: `    if (truthy(Value(_duration))) {`,
      to: `    if (false) {`,
    },
    {
      note: "FPS.update：帧率不取倒数",
      file: "native/lfw/base/fps.h",
      from: `    _value = 1000 / _duration;`,
      to: `    _value = _duration;`,
    },
    {
      note: "FPS.reset：不清 _duration",
      file: "native/lfw/base/fps.h",
      from: `    _value = 0;
    _duration = 0;`,
      to: `    _value = 0;`,
    },

    // ---- 2f：base/clock.h 的槽助手 ------------------------------------------------
    {
      note: "clock_hidden：槽为空也报 hidden（写死 true）",
      file: "native/lfw/base/clock.h",
      from: `inline bool clock_hidden() { return clock() != nullptr && clock()->hidden(); }`,
      to: `inline bool clock_hidden() { return true; }`,
    },
    {
      note: "timeout_add：延时参数不往下传",
      file: "native/lfw/base/clock.h",
      from: `  return timeout() != nullptr ? timeout()->add(std::move(handler), delay) : 0;`,
      to: `  return timeout() != nullptr ? timeout()->add(std::move(handler), 0) : 0;`,
    },

    // ---- 2g：base/team_color.h（两队色）-----------------------------------------
    {
      note: "text 色：team 的查表退化成恒查 Independent",
      file: "native/lfw/base/team_color.h",
      from: `      team_color_detail::team_info(team), u"txt_color");`,
      to: `      team_color_detail::team_info(std::u16string(team_enum::kIndependent)), u"txt_color");`,
    },
    {
      note: "text 色：队色字段名取成描边色",
      file: "native/lfw/base/team_color.h",
      from: `      team_color_detail::team_info(team), u"txt_color");`,
      to: `      team_color_detail::team_info(team), u"txt_outline_color");`,
    },
    {
      note: "text 色：默认 fallback 的字段名取成描边色",
      file: "native/lfw/base/team_color.h",
      from: `      team_color_detail::team_info(std::u16string(team_enum::kIndependent)), u"txt_color");`,
      to: `      team_color_detail::team_info(std::u16string(team_enum::kIndependent)), u"txt_outline_color");`,
    },
    {
      note: "text 色：默认 fallback 拿的不是 Independent 而是 team 自己",
      file: "native/lfw/base/team_color.h",
      from: `      team_color_detail::team_info(std::u16string(team_enum::kIndependent)), u"txt_color");`,
      to: `      team_color_detail::team_info(team), u"txt_color");`,
    },
    {
      note: "text 色：fallback 优先于查表结果（`||` 的短路顺序反了）",
      file: "native/lfw/base/team_color.h",
      from: `  if (color != nullptr && !color->empty()) return *color;
  if (fallback != nullptr) return *fallback;`,
      to: `  if (fallback != nullptr) return *fallback;
  if (color != nullptr && !color->empty()) return *color;`,
    },
    {
      note: "描边色：丢掉「回落到 Independent」这一步",
      file: "native/lfw/base/team_color.h",
      from: `
  if (info == nullptr) info = team_color_detail::team_info(std::u16string(team_enum::kIndependent));`,
      to: ``,
    },
    {
      note: "描边色：字段名取成文字色",
      file: "native/lfw/base/team_color.h",
      from: `  const std::u16string* color = team_color_detail::team_field(info, u"txt_outline_color");`,
      to: `  const std::u16string* color = team_color_detail::team_field(info, u"txt_color");`,
    },
    {
      note: "描边色：team 的查表退化成恒查 Independent",
      file: "native/lfw/base/team_color.h",
      from: `  const Object* info = team_color_detail::team_info(team);`,
      to: `  const Object* info = team_color_detail::team_info(std::u16string(team_enum::kIndependent));`,
    },
    {
      note: "两队色：查的是别的 Defines 表",
      file: "native/lfw/base/team_color.h",
      from: `  const Value* map = defines::find(u"Defines.TeamInfoMap");`,
      to: `  const Value* map = defines::find(u"Defines.TeamEnum");`,
    },

    // ---- 2g：base/get_short_file_size_txt.h --------------------------------------
    {
      note: "文件大小：B 支的 < 写成 <=",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: `  if (bytes < 1024) return number_to_string(bytes) + u"B";`,
      to: `  if (bytes <= 1024) return number_to_string(bytes) + u"B";`,
    },
    {
      note: "文件大小：KB 支的 < 写成 <=",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: `  if (bytes < 1024) return strip_dot_zero(number_to_fixed_1(bytes)) + u"KB";`,
      to: `  if (bytes <= 1024) return strip_dot_zero(number_to_fixed_1(bytes)) + u"KB";`,
    },
    {
      note: "文件大小：MB 支的 < 写成 <=",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: `  if (bytes < 1024) return strip_dot_zero(number_to_fixed_1(bytes)) + u"MB";`,
      to: `  if (bytes <= 1024) return strip_dot_zero(number_to_fixed_1(bytes)) + u"MB";`,
    },
    {
      note: "文件大小：B 支用 toFixed(1) 而不是 ToString",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: `  if (bytes < 1024) return number_to_string(bytes) + u"B";`,
      to: `  if (bytes < 1024) return number_to_fixed_1(bytes) + u"B";`,
    },
    {
      note: "文件大小：第一段除的是 1000",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: `  if (bytes < 1024) return number_to_string(bytes) + u"B";
  bytes /= 1024;`,
      to: `  if (bytes < 1024) return number_to_string(bytes) + u"B";
  bytes /= 1000;`,
    },
    {
      note: "文件大小：第二段除的是 1000",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: `  if (bytes < 1024) return strip_dot_zero(number_to_fixed_1(bytes)) + u"KB";
  bytes /= 1024;`,
      to: `  if (bytes < 1024) return strip_dot_zero(number_to_fixed_1(bytes)) + u"KB";
  bytes /= 1000;`,
    },
    {
      note: "文件大小：第三段除的是 1000",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: `  if (bytes < 1024) return strip_dot_zero(number_to_fixed_1(bytes)) + u"MB";
  bytes /= 1024;`,
      to: `  if (bytes < 1024) return strip_dot_zero(number_to_fixed_1(bytes)) + u"MB";
  bytes /= 1000;`,
    },
    {
      note: "文件大小：KB 支的尾巴写成 B",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: ` + u"KB";`,
      to: ` + u"B";`,
    },
    {
      note: "文件大小：MB 支的尾巴写成 KB",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: ` + u"MB";`,
      to: ` + u"KB";`,
    },
    {
      note: "文件大小：GB 支的尾巴写成 MB",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: ` + u"GB";`,
      to: ` + u"MB";`,
    },
    {
      note: "文件大小：不做 `.replace(\".0\", \"\")`",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: `    if (at == std::u16string::npos) return s;
    return s.substr(0, at) + s.substr(at + 2);`,
      to: `    if (at == std::u16string::npos) return s;
    return s;`,
    },
    {
      note: "文件大小：replace 多删一位（`at + 2` → `at + 3`）",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: `    return s.substr(0, at) + s.substr(at + 2);`,
      to: `    return s.substr(0, at) + s.substr(at + 3);`,
    },
    {
      note: "文件大小：GB 支忘了 strip",
      file: "native/lfw/base/get_short_file_size_txt.h",
      from: `  return strip_dot_zero(number_to_fixed_1(bytes)) + u"GB";`,
      to: `  return number_to_fixed_1(bytes) + u"GB";`,
    },
  ],
};
