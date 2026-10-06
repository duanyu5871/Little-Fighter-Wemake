// `stage/Expressions` + `stage/Status` + `bg/Background` + `bg/Layer`。
//
// 用例：`cases/stage/expr.txt`、`cases/stage/bg.txt`（本文件把两个都跑）。
//
// 有意不覆盖（不可观察或按构造等价）：
//   * `Status` 三个常量与 `status_entries()`：只是字符串表，用例里逐个对过值；
//   * `Expressions::index()`：TS 的 `_index` 是 `protected`（台面只能从 `is_first` / `is_last`
//     与 `run` 的日志反推），端口这个只读口只是给台面观测用；
//   * `flow` 里 `is_last` 取在 `run` 之前还是之后：`run` 既不碰 `_index` 也不碰 `_items` ⇒ 等价；
//   * `Background` 的 `world` 字段：TS 只存不读，端口同样只存。
export default {
  subject: "stage",
  cases: ["expr", "bg"],
  mutations: [
    // ---------------------------------------------------------------- stage/Expressions
    {
      note: "Expressions: is_first 把 0 排在外面",
      file: "native/lfw/stage/expressions.h",
      from: `  bool is_first() const { return _index <= 0.0; }`,
      to: `  bool is_first() const { return _index < 0.0; }`,
    },
    {
      note: "Expressions: is_last 用严格大于（最后一项不算最后）",
      file: "native/lfw/stage/expressions.h",
      from: `  bool is_last() const { return _index >= static_cast<double>(_items.size()) - 1.0; }`,
      to: `  bool is_last() const { return _index > static_cast<double>(_items.size()) - 1.0; }`,
    },
    {
      note: "Expressions: reset 不复位游标",
      file: "native/lfw/stage/expressions.h",
      from: `  void reset(const Items& list) {
    _index = 0.0;`,
      to: `  void reset(const Items& list) {
    _index = -1.0;`,
    },
    {
      note: "Expressions: reset 去掉同一性早退（传回自己的 list 会把列表清空）",
      file: "native/lfw/stage/expressions.h",
      from: `    if (&list == &_items) return;`,
      to: `    if (false) return;`,
    },
    {
      note: "Expressions: reset 清空后不再灌入",
      file: "native/lfw/stage/expressions.h",
      from: `    if (!list.empty()) _items.insert(_items.end(), list.begin(), list.end());`,
      to: `    if (false) _items.insert(_items.end(), list.begin(), list.end());`,
    },
    {
      note: "Expressions: next 用 max（停在原地还往回走）",
      file: "native/lfw/stage/expressions.h",
      from: `  void next() { _index = min(_index + 1.0, static_cast<double>(_items.size()) - 1.0); }`,
      to: `  void next() { _index = max(_index + 1.0, static_cast<double>(_items.size()) - 1.0); }`,
    },
    {
      note: "Expressions: run 的返回值取反",
      file: "native/lfw/stage/expressions.h",
      from: `    return _items[static_cast<size_t>(i)]->run(arg);`,
      to: `    return !_items[static_cast<size_t>(i)]->run(arg);`,
    },
    {
      note: "Expressions: run 不把参数转给表达式",
      file: "native/lfw/stage/expressions.h",
      from: `    return _items[static_cast<size_t>(i)]->run(arg);`,
      to: `    return _items[static_cast<size_t>(i)]->run(T());`,
    },
    {
      note: "Expressions: flow 不因 is_last 收手（只看真假）",
      file: "native/lfw/stage/expressions.h",
      from: `      if (!pass || is_last) break;`,
      to: `      if (is_last) break;`,
    },

    // ---------------------------------------------------------------- bg/Layer
    {
      note: "Layer: is_static 不看 cc 是否 undefined",
      file: "native/lfw/bg/layer.cpp",
      from: `  const bool no_cycle = is_undefined(field_or(_info, u"cc")) ||`,
      to: `  const bool no_cycle = false ||`,
    },
    {
      note: "Layer: is_static 不看 c1 是否 undefined",
      file: "native/lfw/bg/layer.cpp",
      from: `                        is_undefined(field_or(_info, u"c1")) ||`,
      to: `                        false ||`,
    },
    {
      note: "Layer: is_static 不看 c2 是否 undefined",
      file: "native/lfw/bg/layer.cpp",
      from: `                        is_undefined(field_or(_info, u"c2"));`,
      to: `                        false;`,
    },
    {
      note: "Layer: is_static 不看 offsetAnimX",
      file: "native/lfw/bg/layer.cpp",
      from: `  return no_cycle && !truthy(field_or(_info, u"offsetAnimX")) &&`,
      to: `  return no_cycle && true &&`,
    },
    {
      note: "Layer: is_static 不要求 absolute",
      file: "native/lfw/bg/layer.cpp",
      from: `         !truthy(field_or(_info, u"offsetAnimY")) && truthy(field_or(_info, u"absolute"));`,
      to: `         !truthy(field_or(_info, u"offsetAnimY")) && true;`,
    },
    {
      note: "Layer: update 不做取模",
      file: "native/lfw/bg/layer.cpp",
      from: `    const double now = std::fmod(count, to_number(cc));`,
      to: `    const double now = count;`,
    },
    {
      note: "Layer: update 的三件套判定漏掉 c2",
      file: "native/lfw/bg/layer.cpp",
      from: `  if (!is_undefined(cc) && !is_undefined(c1) && !is_undefined(c2)) {`,
      to: `  if (!is_undefined(cc) && !is_undefined(c1)) {`,
    },
    {
      note: "Layer: 可见区间的两端用 || 连",
      file: "native/lfw/bg/layer.cpp",
      from: `    _visible = now >= to_number(c1) && now <= to_number(c2);`,
      to: `    _visible = now >= to_number(c1) || now <= to_number(c2);`,
    },
    {
      note: "Layer: 左端写成严格大于（边界算不可见）",
      file: "native/lfw/bg/layer.cpp",
      from: `    _visible = now >= to_number(c1) && now <= to_number(c2);`,
      to: `    _visible = now > to_number(c1) && now <= to_number(c2);`,
    },
    {
      note: "Layer: 右端写成严格小于（边界算不可见）",
      file: "native/lfw/bg/layer.cpp",
      from: `    _visible = now >= to_number(c1) && now <= to_number(c2);`,
      to: `    _visible = now >= to_number(c1) && now < to_number(c2);`,
    },
    {
      note: "Layer: 没有循环信息时反而不可见",
      file: "native/lfw/bg/layer.cpp",
      from: `  } else {
    _visible = true;
  }`,
      to: `  } else {
    _visible = false;
  }`,
    },

    // ---------------------------------------------------------------- bg/Background
    {
      note: "Background: id 读 alias_id",
      file: "native/lfw/bg/background.cpp",
      from: `  _id = field_or(_data, u"id");`,
      to: `  _id = field_or(_data, u"alias_id");`,
    },
    {
      note: "Background: info 取整个 data（而不是 data.base）",
      file: "native/lfw/bg/background.cpp",
      from: `  const Value info = field_or(_data, u"base");`,
      to: `  const Value info = _data;`,
    },
    {
      note: "Background: name 用真值判断（空串会回退 id）",
      file: "native/lfw/bg/background.cpp",
      from: `  _name = is_nullish(name_v) ? _id : name_v;`,
      to: `  _name = truthy(name_v) ? name_v : _id;`,
    },
    {
      note: "Background: left 读 right",
      file: "native/lfw/bg/background.cpp",
      from: `  _left = num_or(field_or(info, u"left"), 0.0);`,
      to: `  _left = num_or(field_or(info, u"right"), 0.0);`,
    },
    {
      note: "Background: right 读 left",
      file: "native/lfw/bg/background.cpp",
      from: `  _right = num_or(field_or(info, u"right"), 0.0);`,
      to: `  _right = num_or(field_or(info, u"left"), 0.0);`,
    },
    {
      note: "Background: width 的反了",
      file: "native/lfw/bg/background.cpp",
      from: `  _width = _right - _left;`,
      to: `  _width = _left - _right;`,
    },
    {
      note: "Background: depth 的反了",
      file: "native/lfw/bg/background.cpp",
      from: `  _depth = _near - _far;`,
      to: `  _depth = _far - _near;`,
    },
    {
      note: "Background: height 的缺省值写成 1",
      file: "native/lfw/bg/background.cpp",
      from: `  _height = num_or(field_or(info, u"height"), 0.0);`,
      to: `  _height = num_or(field_or(info, u"height"), 1.0);`,
    },
    {
      note: "Background: middle.x 用差而不是和的一半",
      file: "native/lfw/bg/background.cpp",
      from: `  _middle.x = (_right + _left) / 2.0;`,
      to: `  _middle.x = (_right - _left) / 2.0;`,
    },
    {
      note: "Background: middle.z 混进 right",
      file: "native/lfw/bg/background.cpp",
      from: `  _middle.z = (_far + _near) / 2.0;`,
      to: `  _middle.z = (_far + _right) / 2.0;`,
    },
    {
      note: "Background: zoom_x 不做 nullish 回退",
      file: "native/lfw/bg/background.cpp",
      from: `  _zoom_x = num_or(field_or(info, u"zoom_x"), 1.0);`,
      to: `  _zoom_x = to_number(field_or(info, u"zoom_x"));`,
    },
    {
      note: "Background: zoom_y 读成 zoom_z",
      file: "native/lfw/bg/background.cpp",
      from: `  _zoom_y = num_or(field_or(info, u"zoom_y"), 1.0);`,
      to: `  _zoom_y = num_or(field_or(info, u"zoom_z"), 1.0);`,
    },
    {
      note: "Background: layers 为真值也不加层",
      file: "native/lfw/bg/background.cpp",
      from: `  if (truthy(layers)) {`,
      to: `  if (false) {`,
    },
    {
      note: "Background: 每层都拿第 0 个 info",
      file: "native/lfw/bg/background.cpp",
      from: `      for (size_t i = 0; i < arr->size(); ++i) add_layer(arr->at(i));`,
      to: `      add_layer(arr->at(0));`,
    },
    {
      note: "Background: 浅拷贝时丢掉原有字段",
      file: "native/lfw/bg/background.cpp",
      from: `      out->set(key, v != nullptr ? *v : Value());`,
      to: `      (void)key;
      (void)v;`,
    },
    {
      note: "Background: 浅拷贝不写 x",
      file: "native/lfw/bg/background.cpp",
      from: `  out->set(u"x", Value(x));`,
      to: `  (void)x;`,
    },
    {
      note: "Background: data_index 用前置自增",
      file: "native/lfw/bg/background.cpp",
      from: `  const double data_index = _layer_data_index++;`,
      to: `  const double data_index = ++_layer_data_index;`,
    },
    {
      note: "Background: loop 的步长偏 1",
      file: "native/lfw/bg/background.cpp",
      from: `  const double loop = is_undefined(loop_v) ? 0.0 : to_number(loop_v);`,
      to: `  const double loop = is_undefined(loop_v) ? 0.0 : to_number(loop_v) + 1.0;`,
    },
    {
      note: "Background: 循环的右界写成 width - loop",
      file: "native/lfw/bg/background.cpp",
      from: `  const double right = _width + loop;`,
      to: `  const double right = _width - loop;`,
    },
    {
      note: "Background: 循环条件用 <= （多铺一层）",
      file: "native/lfw/bg/background.cpp",
      from: `  for (x -= loop; x < right; x += loop) {`,
      to: `  for (x -= loop; x <= right; x += loop) {`,
    },
    {
      note: "Background: 循环副本不换 x（直接用原 info）",
      file: "native/lfw/bg/background.cpp",
      from: `    _layers.push_back(std::make_shared<Layer>(this, with_x(info, x), data_index, loop_index));`,
      to: `    _layers.push_back(std::make_shared<Layer>(this, info, data_index, loop_index));`,
    },
    {
      note: "Background: loop_index 不递增",
      file: "native/lfw/bg/background.cpp",
      from: `    loop_index++;`,
      to: `    loop_index += 0.0;`,
    },
    {
      note: "Background: update 不推进计数",
      file: "native/lfw/bg/background.cpp",
      from: `void Background::update() {
  _update_times++;`,
      to: `void Background::update() {
  _update_times += 0.0;`,
    },
    {
      note: "Background: update 不往下传（层永远停在初始 visible）",
      file: "native/lfw/bg/background.cpp",
      from: `  for (const std::shared_ptr<Layer>& layer : _layers) layer->update(_update_times);`,
      to: `  for (const std::shared_ptr<Layer>& layer : _layers) (void)layer;`,
    },
    {
      note: "Background: dispose 不清层",
      file: "native/lfw/bg/background.cpp",
      from: `  _layers.clear();`,
      to: `  (void)0;`,
    },
    {
      note: "Background: dispose 不复位 data_index",
      file: "native/lfw/bg/background.cpp",
      from: `  _layer_data_index = 0.0;`,
      to: `  (void)0;`,
    },
  ],
};
