// `LFW` 门面第一批（4AB）的变异档：类核心（静态/实例表/ID 族/回调/广播/进度/难度）
// + 缝接线（实例注册、点设备输入、随机实体）。
//
// 用例：`cases/lfw/{basic,keys,misc,welds}.txt`（任一锁住即可）。
export default {
  subject: "lfw",
  mutations: [
    {
      note: "实例表：构造后不登记自己",
      file: "native/lfw/lfw.cpp",
      from: `  instances_ref().push_back(this);
  host_->pointings_add_ui_input(*this);`,
      to: `  host_->pointings_add_ui_input(*this);`,
    },
    {
      note: "reset_new_id：重置到 99 而不是 100",
      file: "native/lfw/lfw.cpp",
      from: `void LFW::reset_new_id() { __id = 100.0; }`,
      to: `void LFW::reset_new_id() { __id = 99.0; }`,
    },
    {
      note: "random_entity_info：facing 判定式改成恒 -1",
      file: "native/lfw/lfw.cpp",
      from: `  e.facing = std::fmod(_mt.range(0.0, 100.0), 2.0) != 0.0 ? -1.0 : 1.0;`,
      to: `  e.facing = std::fmod(_mt.range(1.0, 100.0), 2.0) != 0.0 ? -1.0 : 1.0;`,
    },
    {
      note: "random_entity_info：x 轴边界取反",
      file: "native/lfw/lfw.cpp",
      from: `  const double x = _mt.range(l, r);
  const double z = _mt.range(f, n);`,
      to: `  const double x = _mt.range(r, l);
  const double z = _mt.range(f, n);`,
    },
    {
      note: "random_entity_info：z 轴边界取反",
      file: "native/lfw/lfw.cpp",
      from: `  const double x = _mt.range(l, r);
  const double z = _mt.range(f, n);`,
      to: `  const double x = _mt.range(l, r);
  const double z = _mt.range(n, f);`,
    },
    {
      note: "dispose：不发 on_dispose 回调",
      file: "native/lfw/lfw.cpp",
      from: `  callbacks.call(u"on_dispose", {LfwCallbackArgs{this}});`,
      to: `  (void)0;`,
    },
    {
      note: "emit_progress：进度 +1",
      file: "native/lfw/lfw.cpp",
      from: `void LFW::emit_progress(const std::u16string& content, double progress) {
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = content;
  args.num = progress;
  callbacks.call(u"on_progress", {args});
}`,
      to: `void LFW::emit_progress(const std::u16string& content, double progress) {
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = content;
  args.num = progress + 1.0;
  callbacks.call(u"on_progress", {args});
}`,
    },
    {
      note: "emit_progress：文本加尾巴",
      file: "native/lfw/lfw.cpp",
      from: `void LFW::emit_progress(const std::u16string& content, double progress) {
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = content;
  args.num = progress;
  callbacks.call(u"on_progress", {args});
}`,
      to: `void LFW::emit_progress(const std::u16string& content, double progress) {
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = content + u"!";
  args.num = progress;
  callbacks.call(u"on_progress", {args});
}`,
    },
    {
      note: "emit_progress_size：大小 +1",
      file: "native/lfw/lfw.cpp",
      from: `  args.num2 = to_number(size);
  args.has_num2 = true;`,
      to: `  args.num2 = to_number(size) + 1.0;
  args.has_num2 = true;`,
    },
    {
      note: "broadcast：回调文本加尾巴",
      file: "native/lfw/lfw.cpp",
      from: `void LFW::broadcast(const Value& m) {
  broadcasts.push_back(to_string(m));
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = to_string(m);
  callbacks.call(u"on_broadcast", {args});
}`,
      to: `void LFW::broadcast(const Value& m) {
  broadcasts.push_back(to_string(m));
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = to_string(m) + u"!";
  callbacks.call(u"on_broadcast", {args});
}`,
    },
    {
      note: "loop_offset：偏移先加到列表长度上",
      file: "native/lfw/lfw.cpp",
      from: `  double off = std::fmod(offset, static_cast<double>(len));`,
      to: `  double off = std::fmod(offset + 1.0, static_cast<double>(len));`,
    },
  ],
};
