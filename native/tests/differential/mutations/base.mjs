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
  ],
};
