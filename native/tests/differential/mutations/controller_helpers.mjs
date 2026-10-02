export default {
  subject: "controller_helpers",
  mutations: [
    {
      note: "DoubleClick.press 的间隔判定用严格小于",
      file: "native/lfw/controller/double_click.cpp",
      from: `  if (_time + time <= interval) {`,
      to: `  if (_time + time < interval) {`,
    },
    {
      note: "DoubleClick.press 首按下时的取负写错",
      file: "native/lfw/controller/double_click.cpp",
      from: `  _time = -time;
  _data[0] = data;
  _data[1] = Value();`,
      to: `  _time = time;
  _data[0] = data;
  _data[1] = Value();`,
    },
    {
      note: "DoubleClick.press 双击时写进 data[0]",
      file: "native/lfw/controller/double_click.cpp",
      from: `    _time = time;
    _data[1] = data;
    _fired = Value(true);`,
      to: `    _time = time;
    _data[0] = data;
    _fired = Value(true);`,
    },
    {
      note: "DoubleClick.press 双击时不置 fired",
      file: "native/lfw/controller/double_click.cpp",
      from: `    _data[1] = data;
    _fired = Value(true);
    return;`,
      to: `    _data[1] = data;
    _fired = Value(false);
    return;`,
    },
    {
      note: "DoubleClick.press 首按下时不清 data[1]",
      file: "native/lfw/controller/double_click.cpp",
      from: `  _time = -time;
  _data[0] = data;
  _data[1] = Value();`,
      to: `  _time = -time;
  _data[0] = data;`,
    },
    {
      note: "DoubleClick.step 漏掉取负",
      file: "native/lfw/controller/double_click.cpp",
      from: `void DoubleClick::step() {
  _time = -_time;`,
      to: `void DoubleClick::step() {
  _time = _time;`,
    },
    {
      note: "DoubleClick.step 的 data 搬移方向反了",
      file: "native/lfw/controller/double_click.cpp",
      from: `  _data[0] = _data[1];
  _data[1] = Value();
  _fired = Value(false);`,
      to: `  _data[1] = _data[0];
  _data[0] = Value();
  _fired = Value(false);`,
    },
    {
      note: "DoubleClick.step 不清 fired",
      file: "native/lfw/controller/double_click.cpp",
      from: `  _data[0] = _data[1];
  _data[1] = Value();
  _fired = Value(false);`,
      to: `  _data[0] = _data[1];
  _data[1] = Value();`,
    },
    {
      note: "DoubleClick.reset 漏掉 used",
      file: "native/lfw/controller/double_click.cpp",
      from: `  _fired = Value(false);
  _used = Value(false);
}`,
      to: `  _fired = Value(false);
}`,
    },
    {
      note: "DoubleClick.reset 漏掉 data[1]",
      file: "native/lfw/controller/double_click.cpp",
      from: `  _data[0] = Value();
  _data[1] = Value();
  _fired = Value(false);
  _used = Value(false);`,
      to: `  _data[0] = Value();
  _fired = Value(false);
  _used = Value(false);`,
    },
    {
      note: "DoubleClick.to_snapshot 的键序调换",
      file: "native/lfw/controller/double_click.cpp",
      from: `  o.set(u"time", Value(_time));
  o.set(u"used", _used);
  o.set(u"fired", _fired);`,
      to: `  o.set(u"used", _used);
  o.set(u"time", Value(_time));
  o.set(u"fired", _fired);`,
    },
    {
      note: "DoubleClick.to_snapshot 的 used 写成 fired",
      file: "native/lfw/controller/double_click.cpp",
      from: `  o.set(u"used", _used);`,
      to: `  o.set(u"used", _fired);`,
    },
    {
      note: "DoubleClick.to_snapshot 的 data 顺序互换",
      file: "native/lfw/controller/double_click.cpp",
      from: `  pair.push_back(_data[0]);
  pair.push_back(_data[1]);`,
      to: `  pair.push_back(_data[1]);
  pair.push_back(_data[0]);`,
    },
    {
      note: "DoubleClick.from_snapshot 的 data 取值互换",
      file: "native/lfw/controller/double_click.cpp",
      from: `    _data[0] = a->size() > 0 ? a->at(0) : Value();
    _data[1] = a->size() > 1 ? a->at(1) : Value();`,
      to: `    _data[1] = a->size() > 0 ? a->at(0) : Value();
    _data[0] = a->size() > 1 ? a->at(1) : Value();`,
    },
    {
      note: "DoubleClick.from_snapshot 漏掉 name",
      file: "native/lfw/controller/double_click.cpp",
      from: `  _fired = field_or(s, u"fired");
  _name = field_or(s, u"name");`,
      to: `  _fired = field_or(s, u"fired");`,
    },
    {
      note: "DoubleClick.from_snapshot 的 time 键名写错",
      file: "native/lfw/controller/double_click.cpp",
      from: `  _time = to_number(field_or(s, u"time"));`,
      to: `  _time = to_number(field_or(s, u"Time"));`,
    },
    {
      note: "SeqKeys.press 的完成下标判定写错",
      file: "native/lfw/controller/seq_keys.cpp",
      from: `    if (_idx == static_cast<double>(len) - 1.0) {`,
      to: `    if (_idx == static_cast<double>(len)) {`,
    },
    {
      note: "SeqKeys.press 失配时不清 hit",
      file: "native/lfw/controller/seq_keys.cpp",
      from: `    if (j == arr.size()) {
      _idx = 0;
      _hit = 0;
      return;
    }`,
      to: `    if (j == arr.size()) {
      _idx = 0;
      return;
    }`,
    },
    {
      note: "SeqKeys.press 失配时不清 idx",
      file: "native/lfw/controller/seq_keys.cpp",
      from: `    if (j == arr.size()) {
      _idx = 0;
      _hit = 0;
      return;
    }`,
      to: `    if (j == arr.size()) {
      _hit = 0;
      return;
    }`,
    },
    {
      note: "SeqKeys.press 完成时 hit 置 0",
      file: "native/lfw/controller/seq_keys.cpp",
      from: `      _idx = 0;
      _hit = 1;
      return;`,
      to: `      _idx = 0;
      _hit = 0;
      return;`,
    },
    {
      note: "SeqKeys.press 不消耗匹配到的字符",
      file: "native/lfw/controller/seq_keys.cpp",
      from: `    arr.erase(arr.begin() + static_cast<std::ptrdiff_t>(j));
    if (_idx == static_cast<double>(len) - 1.0) {`,
      to: `    if (_idx == static_cast<double>(len) - 1.0) {`,
    },
    {
      note: "SeqKeys.press 不推进 idx",
      file: "native/lfw/controller/seq_keys.cpp",
      from: `      return;
    }
    _idx += 1;
  }`,
      to: `      return;
    }
  }`,
    },
    {
      note: "SeqKeys.press 的循环条件用了 arr.empty()",
      file: "native/lfw/controller/seq_keys.cpp",
      from: `  while (_idx < static_cast<double>(len) && !arr.empty()) {`,
      to: `  while (_idx < static_cast<double>(len) && arr.empty()) {`,
    },
    {
      note: "SeqKeys.reset 只清 idx",
      file: "native/lfw/controller/seq_keys.cpp",
      from: `void SeqKeys::reset() {
  _idx = 0;
  _hit = 0;
}`,
      to: `void SeqKeys::reset() {
  _idx = 0;
}`,
    },
    {
      note: "SeqKeys.to_snapshot 的 idx/hit 互换",
      file: "native/lfw/controller/seq_keys.cpp",
      from: `  o.set(u"idx", Value(_idx));
  o.set(u"hit", Value(_hit));`,
      to: `  o.set(u"idx", Value(_hit));
  o.set(u"hit", Value(_idx));`,
    },
    {
      note: "SeqKeys.from_snapshot 的 keys 写成 data",
      file: "native/lfw/controller/seq_keys.cpp",
      from: `  _keys = to_string(field_or(s, u"keys"));
  _data = field_or(s, u"data");`,
      to: `  _keys = to_string(field_or(s, u"data"));
  _data = field_or(s, u"data");`,
    },
    {
      note: "KeyStatus 的真值判定恒为真",
      file: "native/lfw/controller/key_status.cpp",
      from: `bool truthy_num(double n) { return n != 0.0 && !std::isnan(n); }`,
      to: `bool truthy_num(double n) { return true; }`,
    },
    {
      note: "KeyStatus.is_start 的比较符取反",
      file: "native/lfw/controller/key_status.cpp",
      from: `  return truthy_num(_d_time) && _d_time == time;`,
      to: `  return truthy_num(_d_time) && _d_time != time;`,
    },
    {
      note: "KeyStatus.is_start 漏掉真值守卫",
      file: "native/lfw/controller/key_status.cpp",
      from: `  return truthy_num(_d_time) && _d_time == time;`,
      to: `  return _d_time == time;`,
    },
    {
      note: "KeyStatus.is_hit 漏掉按下时间守卫",
      file: "native/lfw/controller/key_status.cpp",
      from: `  if (!truthy_num(_d_time)) return false;
  const double dt = time - _d_time;`,
      to: `  const double dt = time - _d_time;`,
    },
    {
      note: "KeyStatus.is_hit 的时长判定用闭区间",
      file: "native/lfw/controller/key_status.cpp",
      from: `  return dt < key_hit_duration;`,
      to: `  return dt <= key_hit_duration;`,
    },
    {
      note: "KeyStatus.is_hit 的时长算反",
      file: "native/lfw/controller/key_status.cpp",
      from: `  const double dt = time - _d_time;`,
      to: `  const double dt = _d_time - time;`,
    },
    {
      note: "KeyStatus.is_hld 的时序比较用闭区间",
      file: "native/lfw/controller/key_status.cpp",
      from: `  return !is_hit(time, key_hit_duration) && _d_time > _u_time;`,
      to: `  return !is_hit(time, key_hit_duration) && _d_time >= _u_time;`,
    },
    {
      note: "KeyStatus.is_hld 的 is_hit 取反",
      file: "native/lfw/controller/key_status.cpp",
      from: `  return !is_hit(time, key_hit_duration) && _d_time > _u_time;`,
      to: `  return is_hit(time, key_hit_duration) && _d_time > _u_time;`,
    },
    {
      note: "KeyStatus.is_end 用严格小于",
      file: "native/lfw/controller/key_status.cpp",
      from: `bool KeyStatus::is_end() const { return _d_time <= _u_time; }`,
      to: `bool KeyStatus::is_end() const { return _d_time < _u_time; }`,
    },
    {
      note: "KeyStatus.use 不置 used",
      file: "native/lfw/controller/key_status.cpp",
      from: `double KeyStatus::use() {
  _used = 1;
  return _d_time;
}`,
      to: `double KeyStatus::use() {
  _used = 0;
  return _d_time;
}`,
    },
    {
      note: "KeyStatus.use 返回抬起时间",
      file: "native/lfw/controller/key_status.cpp",
      from: `  _used = 1;
  return _d_time;`,
      to: `  _used = 1;
  return _u_time;`,
    },
    {
      note: "KeyStatus.hit 不重置 used",
      file: "native/lfw/controller/key_status.cpp",
      from: `  _d_time = std::holds_alternative<std::monostate>(t) ? time : to_number(t);
  _used = 0;`,
      to: `  _d_time = std::holds_alternative<std::monostate>(t) ? time : to_number(t);`,
    },
    {
      note: "KeyStatus.hit 的默认值判定写成 null",
      file: "native/lfw/controller/key_status.cpp",
      from: `  _d_time = std::holds_alternative<std::monostate>(t) ? time : to_number(t);`,
      to: `  _d_time = std::holds_alternative<NullTag>(t) ? time : to_number(t);`,
    },
    {
      note: "KeyStatus.end 写进了按下时间",
      file: "native/lfw/controller/key_status.cpp",
      from: `void KeyStatus::end(double time) { _u_time = time; }`,
      to: `void KeyStatus::end(double time) { _d_time = time; }`,
    },
    {
      note: "KeyStatus.reset 漏掉 used",
      file: "native/lfw/controller/key_status.cpp",
      from: `  _u_time = 0;
  _used = 0;
}`,
      to: `  _u_time = 0;
}`,
    },
    {
      note: "KeyStatus.to_snapshot 的按下/抬起互换",
      file: "native/lfw/controller/key_status.cpp",
      from: `  a.push_back(Value(_d_time));
  a.push_back(Value(_u_time));`,
      to: `  a.push_back(Value(_u_time));
  a.push_back(Value(_d_time));`,
    },
    {
      note: "KeyStatus.to_snapshot 的 used 写成按下时间",
      file: "native/lfw/controller/key_status.cpp",
      from: `  a.push_back(Value(_used));`,
      to: `  a.push_back(Value(_d_time));`,
    },
    {
      note: "KeyStatus.from_snapshot 的 used 不做三态归一",
      file: "native/lfw/controller/key_status.cpp",
      from: `  _used = truthy(used) ? 1.0 : 0.0;`,
      to: `  _used = to_number(used);`,
    },
    {
      note: "KeyStatus.from_snapshot 的抬起时间取错下标",
      file: "native/lfw/controller/key_status.cpp",
      from: `  _u_time = a != nullptr && a->size() > 1 ? to_number(a->at(1)) : 0.0;`,
      to: `  _u_time = a != nullptr && a->size() > 0 ? to_number(a->at(0)) : 0.0;`,
    },
    {
      note: "ControllerDoubleClicks 的 L 槽名映射写错",
      file: "native/lfw/controller/controller_double_clicks.cpp",
      from: `    : L(Value(std::u16string(u"d"))),`,
      to: `    : L(Value(std::u16string(u"L"))),`,
    },
    {
      note: "ControllerDoubleClicks 的 D 槽名映射写错",
      file: "native/lfw/controller/controller_double_clicks.cpp",
      from: `      D(Value(std::u16string(u"L"))),`,
      to: `      D(Value(std::u16string(u"R"))),`,
    },
    {
      note: "ControllerDoubleClicks 的 d 槽名映射写错",
      file: "native/lfw/controller/controller_double_clicks.cpp",
      from: `      d(Value(std::u16string(u"R"))),`,
      to: `      d(Value(std::u16string(u"d"))),`,
    },
    {
      note: "ControllerDoubleClicks 的 slot 把 L 指到 R",
      file: "native/lfw/controller/controller_double_clicks.cpp",
      from: `  if (key == u"L") return &L;`,
      to: `  if (key == u"L") return &R;`,
    },
    {
      note: "ControllerDoubleClicks 的 slot 把 j 指到 U",
      file: "native/lfw/controller/controller_double_clicks.cpp",
      from: `  if (key == u"j") return &j;`,
      to: `  if (key == u"j") return &U;`,
    },
    {
      note: "ControllerDoubleClicks.to_snapshot 的键序调换",
      file: "native/lfw/controller/controller_double_clicks.cpp",
      from: `  o.set(u"U", U.to_snapshot());
  o.set(u"D", D.to_snapshot());`,
      to: `  o.set(u"D", D.to_snapshot());
  o.set(u"U", U.to_snapshot());`,
    },
    {
      note: "ControllerDoubleClicks.to_snapshot 的 U 写成 D",
      file: "native/lfw/controller/controller_double_clicks.cpp",
      from: `  o.set(u"U", U.to_snapshot());`,
      to: `  o.set(u"U", D.to_snapshot());`,
    },
    {
      note: "ControllerDoubleClicks.from_snapshot 的键序错配",
      file: "native/lfw/controller/controller_double_clicks.cpp",
      from: `  const char16_t* const kKeys[] = {u"L", u"R", u"U", u"D", u"d", u"j", u"a"};`,
      to: `  const char16_t* const kKeys[] = {u"R", u"L", u"U", u"D", u"d", u"j", u"a"};`,
    },
    {
      note: "ControllerDoubleClicks.from_snapshot 漏掉最后一个槽",
      file: "native/lfw/controller/controller_double_clicks.cpp",
      from: `  for (size_t i = 0; i < 7; ++i) {`,
      to: `  for (size_t i = 0; i < 6; ++i) {`,
    },
    {
      note: "ControllerDoubleClicks.reset 漏掉一个槽",
      file: "native/lfw/controller/controller_double_clicks.cpp",
      from: `  DoubleClick* const all[] = {&L, &R, &U, &D, &d, &j, &a};`,
      to: `  DoubleClick* const all[] = {&L, &R, &U, &D, &d, &j, &j};`,
    },
  ],
};
