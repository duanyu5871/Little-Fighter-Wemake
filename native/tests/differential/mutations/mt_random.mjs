export default {
  subject: "mt_random",
  mutations: [
    {
      note: "duplicate 为 undefined 时不回落 false",
      file: "native/lfw/helper/randoming.cpp",
      from: `  if (std::holds_alternative<std::monostate>(v)) return Value(false);
  return v;`,
      to: `  if (std::holds_alternative<std::monostate>(v)) return v;
  return v;`,
    },
    {
      note: "duplicate 回落条件写成 falsy",
      file: "native/lfw/helper/randoming.cpp",
      from: `Value duplicate_or_default(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return Value(false);
  return v;
}`,
      to: `Value duplicate_or_default(const Value& v) {
  if (!truthy(v)) return Value(false);
  return v;
}`,
    },
    {
      note: "默认 mt 的种子固定为 0（不读时钟）",
      file: "native/lfw/helper/randoming.cpp",
      from: `  static MersenneTwister instance(clock_now());`,
      to: `  static MersenneTwister instance(0.0);`,
    },
    {
      note: "默认 mt 不是同一个实例",
      file: "native/lfw/helper/randoming.cpp",
      from: `  static MersenneTwister instance(clock_now());
  return instance;`,
      to: `  MersenneTwister instance(clock_now());
  return instance;`,
    },
    {
      note: "构造不收传入的 mt",
      file: "native/lfw/helper/randoming.cpp",
      from: `      _mt(mt != nullptr ? mt : &default_mt()),`,
      to: `      _mt(&default_mt()),`,
    },
    {
      note: "构造不拷 src 到 cur",
      file: "native/lfw/helper/randoming.cpp",
      from: `      _src(std::move(src)),
      _cur(_src),`,
      to: `      _src(std::move(src)),
      _cur(),`,
    },
    {
      note: "create 丢掉 duplicate 实参",
      file: "native/lfw/helper/randoming.cpp",
      from: `  return std::make_shared<RandomingT<T>>(std::move(name), std::move(src), mt,
                                         duplicate_or_default(duplicate));`,
      to: `  return std::make_shared<RandomingT<T>>(std::move(name), std::move(src), mt,
                                         Value(false));`,
    },
    {
      note: "set_src 顺手清空 cur",
      file: "native/lfw/helper/randoming.cpp",
      from: `RandomingT<T>& RandomingT<T>::set_src(std::vector<T> src) {
  _src = std::move(src);`,
      to: `RandomingT<T>& RandomingT<T>::set_src(std::vector<T> src) {
  _src = std::move(src);
  _cur.clear();`,
    },
    {
      note: "get 的两个分支互换",
      file: "native/lfw/helper/randoming.cpp",
      from: `  _taken = truthy(_duplicate) ? random_get() : random_take();`,
      to: `  _taken = truthy(_duplicate) ? random_take() : random_get();`,
    },
    {
      note: "get 的 duplicate 判定用严相等 true",
      file: "native/lfw/helper/randoming.cpp",
      from: `  _taken = truthy(_duplicate) ? random_get() : random_take();`,
      to: `  _taken = strict_equals(_duplicate, Value(true)) ? random_get() : random_take();`,
    },
    {
      note: "random_get 的取值下界从 0 变 1",
      file: "native/lfw/helper/randoming.cpp",
      from: `T RandomingT<T>::random_get() {
  const size_t idx = static_cast<size_t>(random_in(0.0, static_cast<double>(_src.size())));`,
      to: `T RandomingT<T>::random_get() {
  const size_t idx = static_cast<size_t>(random_in(1.0, static_cast<double>(_src.size())));`,
    },
    {
      note: "random_get 越界时返回 null",
      file: "native/lfw/helper/randoming.cpp",
      from: `  const size_t idx = static_cast<size_t>(random_in(0.0, static_cast<double>(_src.size())));
  if (idx >= _src.size()) return RandomingItem<T>::out_of_range();
  return _src[idx];`,
      to: `  const size_t idx = static_cast<size_t>(random_in(0.0, static_cast<double>(_src.size())));
  if (idx >= _src.size()) return RandomingItem<T>::null_taken();
  return _src[idx];`,
    },
    {
      note: "random_take 每次都重填 cur",
      file: "native/lfw/helper/randoming.cpp",
      from: `T RandomingT<T>::random_take() {
  if (_cur.empty()) {`,
      to: `T RandomingT<T>::random_take() {
  if (true) {`,
    },
    {
      note: "random_take 单元素分支直接清空",
      file: "native/lfw/helper/randoming.cpp",
      from: `      _cur = std::move(kept);
    } else {
      _cur = _src;
    }`,
      to: `      _cur = std::move(kept);
    } else {
      _cur.clear();
    }`,
    },
    {
      note: "random_take 的 > 写成 >=",
      file: "native/lfw/helper/randoming.cpp",
      from: `    if (_src.size() > 1) {`,
      to: `    if (_src.size() >= 1) {`,
    },
    {
      note: "random_take 的过滤条件取反",
      file: "native/lfw/helper/randoming.cpp",
      from: `        if (RandomingItem<T>::loose_ne(item, _taken)) kept.push_back(item);`,
      to: `        if (!RandomingItem<T>::loose_ne(item, _taken)) kept.push_back(item);`,
    },
    {
      note: "random_take 的过滤用严相等",
      file: "native/lfw/helper/randoming.h",
      from: `  static bool loose_ne(const Value& a, const Value& b) { return !equals(a, b); }`,
      to: `  static bool loose_ne(const Value& a, const Value& b) { return !strict_equals(a, b); }`,
    },
    {
      note: "random_take 取值下标固定 0",
      file: "native/lfw/helper/randoming.cpp",
      from: `  const T taken = _cur[idx];`,
      to: `  const T taken = _cur[0];`,
    },
    {
      note: "random_take 删除首元素而不是抽中元素",
      file: "native/lfw/helper/randoming.cpp",
      from: `  _cur.erase(_cur.begin() + static_cast<std::ptrdiff_t>(idx));`,
      to: `  _cur.erase(_cur.begin());`,
    },
    {
      note: "random_take 空 cur 时返回 null",
      file: "native/lfw/helper/randoming.cpp",
      from: `  if (idx >= _cur.size()) return RandomingItem<T>::out_of_range();
  const T taken = _cur[idx];`,
      to: `  if (idx >= _cur.size()) return RandomingItem<T>::null_taken();
  const T taken = _cur[idx];`,
    },
    {
      note: "random_in 的上界写成 0",
      file: "native/lfw/helper/randoming.cpp",
      from: `double RandomingT<T>::random_in(double l, double r) {
  _mt->mark = _name;
  return _mt->range(l, r);
}`,
      to: `double RandomingT<T>::random_in(double l, double r) {
  _mt->mark = _name;
  return _mt->range(l, 0.0);
}`,
    },
    {
      note: "初始 taken 是 false 而不是 null",
      file: "native/lfw/helper/randoming.h",
      from: `  static Value null_taken() { return Value(NullTag{}); }`,
      to: `  static Value null_taken() { return Value(false); }`,
    },

    {
      note: "spawn_ice_piece 的 kind 写成 1",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  opoint.set(u"kind", Value(0.0));
  opoint.set(u"x", Value(0.0));`,
      to: `  opoint.set(u"kind", Value(1.0));
  opoint.set(u"x", Value(0.0));`,
    },
    {
      note: "spawn_ice_piece 的 x/y 插入序互换",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  opoint.set(u"x", Value(0.0));
  opoint.set(u"y", Value(0.0));`,
      to: `  opoint.set(u"y", Value(0.0));
  opoint.set(u"x", Value(0.0));`,
    },
    {
      note: "spawn_ice_piece 的 oid 写错",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  opoint.set(u"oid", Value(std::u16string(oid::kBrokenWeapon)));`,
      to: `  opoint.set(u"oid", Value(std::u16string(u"998")));`,
    },
    {
      note: "spawn_ice_piece 的 action 键写成 act",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  opoint.set(u"action", action_of(id));`,
      to: `  opoint.set(u"act", action_of(id));`,
    },
    {
      note: "spawn_ice_piece 的 dvx 写成 1",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  opoint.set(u"dvx", Value(0.0));
  opoint.set(u"dvy", Value(0.0));`,
      to: `  opoint.set(u"dvx", Value(1.0));
  opoint.set(u"dvy", Value(0.0));`,
    },
    {
      note: "spawn_ice_piece 的 ghost 写成 0",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  opoint.set(u"ghost", Value(1.0));`,
      to: `  opoint.set(u"ghost", Value(0.0));`,
    },
    {
      note: "spawn_ice_piece 丢掉 speedz",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  opoint.set(u"speedz", Value(0.0));
  opoint.set(u"unimportant", Value(1.0));`,
      to: `  opoint.set(u"unimportant", Value(1.0));`,
    },
    {
      note: "spawn_ice_piece 的 unimportant 写成 0",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  opoint.set(u"speedz", Value(0.0));
  opoint.set(u"unimportant", Value(1.0));`,
      to: `  opoint.set(u"speedz", Value(0.0));
  opoint.set(u"unimportant", Value(0.0));`,
    },
    {
      note: "action 的 id 用固定字符串",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  action.set(u"id", Value(id));`,
      to: `  action.set(u"id", Value(std::u16string(u"0")));`,
    },

    {
      note: "ice_piece_opoints 的第 4 个 id 写成 121",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `      spawn_ice_piece(u"130"), spawn_ice_piece(u"130"), spawn_ice_piece(u"130"),
      spawn_ice_piece(u"120"), spawn_ice_piece(u"120"), spawn_ice_piece(u"125"),`,
      to: `      spawn_ice_piece(u"130"), spawn_ice_piece(u"130"), spawn_ice_piece(u"130"),
      spawn_ice_piece(u"121"), spawn_ice_piece(u"120"), spawn_ice_piece(u"125"),`,
    },
    {
      note: "ice_piece_opoints 的第 3 个 id 写成 131",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `      spawn_ice_piece(u"130"), spawn_ice_piece(u"130"), spawn_ice_piece(u"130"),
      spawn_ice_piece(u"120"), spawn_ice_piece(u"120"), spawn_ice_piece(u"125"),`,
      to: `      spawn_ice_piece(u"130"), spawn_ice_piece(u"130"), spawn_ice_piece(u"131"),
      spawn_ice_piece(u"120"), spawn_ice_piece(u"120"), spawn_ice_piece(u"125"),`,
    },
    {
      note: "ice_piece_opoints 少一个 135 条目",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `      spawn_ice_piece(u"135"), spawn_ice_piece(u"135"), spawn_ice_piece(u"135"),
      spawn_ice_piece(u"135"), spawn_ice_piece(u"135"), spawn_ice_piece(u"135"),
      spawn_ice_piece(u"135")};`,
      to: `      spawn_ice_piece(u"135"), spawn_ice_piece(u"135"), spawn_ice_piece(u"135"),
      spawn_ice_piece(u"135"), spawn_ice_piece(u"135"), spawn_ice_piece(u"135")};`,
    },
    {
      note: "ice_piece_opoints 的一条 135 写成 136",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `      spawn_ice_piece(u"135"), spawn_ice_piece(u"135"), spawn_ice_piece(u"135"),
      spawn_ice_piece(u"135")};`,
      to: `      spawn_ice_piece(u"135"), spawn_ice_piece(u"135"), spawn_ice_piece(u"135"),
      spawn_ice_piece(u"136")};`,
    },

    {
      note: "ice_piece_dvx 去掉实体判定",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `Value ice_piece_dvx(const Value& e, MersenneTwister& mt) {
  if (!entity::is_object(e)) return Value(0.0);`,
      to: `Value ice_piece_dvx(const Value& e, MersenneTwister& mt) {
  if (false) return Value(0.0);`,
    },
    {
      note: "ice_piece_dvx 的兜底值写成 1",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `Value ice_piece_dvx(const Value& e, MersenneTwister& mt) {
  if (!entity::is_object(e)) return Value(0.0);`,
      to: `Value ice_piece_dvx(const Value& e, MersenneTwister& mt) {
  if (!entity::is_object(e)) return Value(1.0);`,
    },
    {
      note: "ice_piece_dvx 用 dvy 的取值表",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `    dvx_randoming() = std::make_shared<Randoming>(u"ice_piece_vx", dvx_values(), &mt);`,
      to: `    dvx_randoming() = std::make_shared<Randoming>(u"ice_piece_vx", dvy_values(), &mt);`,
    },
    {
      note: "ice_piece_dvx 的缓存身份判定取反",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  if (dvx_randoming() == nullptr || dvx_randoming()->mt() != &mt) {`,
      to: `  if (dvx_randoming() == nullptr || dvx_randoming()->mt() == &mt) {`,
    },
    {
      note: "ice_piece_dvx 不看缓存直接新建",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  if (dvx_randoming() == nullptr || dvx_randoming()->mt() != &mt) {`,
      to: `  if (true) {`,
    },
    {
      note: "ice_piece_dvy 去掉实体判定",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `Value ice_piece_dvy(const Value& e, MersenneTwister& mt) {
  if (!entity::is_object(e)) return Value(0.0);`,
      to: `Value ice_piece_dvy(const Value& e, MersenneTwister& mt) {
  if (false) return Value(0.0);`,
    },
    {
      note: "ice_piece_dvy 用 dvx 的取值表",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `    dvy_randoming() = std::make_shared<Randoming>(u"ice_piece_vy", dvy_values(), &mt);`,
      to: `    dvy_randoming() = std::make_shared<Randoming>(u"ice_piece_vy", dvx_values(), &mt);`,
    },
    {
      note: "ice_piece_dvy 的缓存身份判定取反",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  if (dvy_randoming() == nullptr || dvy_randoming()->mt() != &mt) {`,
      to: `  if (dvy_randoming() == nullptr || dvy_randoming()->mt() == &mt) {`,
    },
    {
      note: "ice_piece_dvy 复用 dvx 的缓存槽",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  if (dvy_randoming() == nullptr || dvy_randoming()->mt() != &mt) {
    dvy_randoming() = std::make_shared<Randoming>(u"ice_piece_vy", dvy_values(), &mt);
  }
  return dvy_randoming()->get();`,
      to: `  if (dvx_randoming() == nullptr || dvx_randoming()->mt() != &mt) {
    dvx_randoming() = std::make_shared<Randoming>(u"ice_piece_vy", dvy_values(), &mt);
  }
  return dvx_randoming()->get();`,
    },

    {
      note: "ice_piece_x 去掉实体判定",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `Value ice_piece_x(const Value& e, MersenneTwister& mt) {
  if (!entity::is_object(e)) return Value(0.0);`,
      to: `Value ice_piece_x(const Value& e, MersenneTwister& mt) {
  if (false) return Value(0.0);`,
    },
    {
      note: "ice_piece_x 读 height",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  const double width = to_number(field_or(field_or(e, u"frame"), u"width"));
  const double r = width / 4.0;
  mt.mark = u"ice_piece_x";
  return Value(round(width / 2.0 + mt.range(-r, r)));
}

Value ice_piece_y`,
      to: `  const double width = to_number(field_or(field_or(e, u"frame"), u"height"));
  const double r = width / 4.0;
  mt.mark = u"ice_piece_x";
  return Value(round(width / 2.0 + mt.range(-r, r)));
}

Value ice_piece_y`,
    },
    {
      note: "ice_piece_x 的范围用 width/2",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  const double width = to_number(field_or(field_or(e, u"frame"), u"width"));
  const double r = width / 4.0;
  mt.mark = u"ice_piece_x";
  return Value(round(width / 2.0 + mt.range(-r, r)));`,
      to: `  const double width = to_number(field_or(field_or(e, u"frame"), u"width"));
  const double r = width / 2.0;
  mt.mark = u"ice_piece_x";
  return Value(round(width / 2.0 + mt.range(-r, r)));`,
    },
    {
      note: "ice_piece_x 的中心写成 width/4",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  mt.mark = u"ice_piece_x";
  return Value(round(width / 2.0 + mt.range(-r, r)));`,
      to: `  mt.mark = u"ice_piece_x";
  return Value(round(width / 4.0 + mt.range(-r, r)));`,
    },
    {
      note: "ice_piece_x 的范围退化成单点",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  return Value(round(width / 2.0 + mt.range(-r, r)));`,
      to: `  return Value(round(width / 2.0 + mt.range(r, r)));`,
    },
    {
      note: "ice_piece_x 用 floor 而不是 round",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  const double width = to_number(field_or(field_or(e, u"frame"), u"width"));
  const double r = width / 4.0;
  mt.mark = u"ice_piece_x";
  return Value(round(width / 2.0 + mt.range(-r, r)));`,
      to: `  const double width = to_number(field_or(field_or(e, u"frame"), u"width"));
  const double r = width / 4.0;
  mt.mark = u"ice_piece_x";
  return Value(floor(width / 2.0 + mt.range(-r, r)));`,
    },
    {
      note: "ice_piece_y 去掉实体判定",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `Value ice_piece_y(const Value& e, MersenneTwister& mt) {
  if (!entity::is_object(e)) return Value(0.0);`,
      to: `Value ice_piece_y(const Value& e, MersenneTwister& mt) {
  if (false) return Value(0.0);`,
    },
    {
      note: "ice_piece_y 读 width",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  const double height = to_number(field_or(field_or(e, u"frame"), u"height"));`,
      to: `  const double height = to_number(field_or(field_or(e, u"frame"), u"width"));`,
    },
    {
      note: "ice_piece_y 的范围用 height/3",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  const double height = to_number(field_or(field_or(e, u"frame"), u"height"));
  const double r = height / 4.0;
  mt.mark = u"ice_piece_y";
  return Value(round(height / 2.0 + mt.range(-r, r)));`,
      to: `  const double height = to_number(field_or(field_or(e, u"frame"), u"height"));
  const double r = height / 3.0;
  mt.mark = u"ice_piece_y";
  return Value(round(height / 2.0 + mt.range(-r, r)));`,
    },
    {
      note: "ice_piece_y 用 floor 而不是 round",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  const double height = to_number(field_or(field_or(e, u"frame"), u"height"));
  const double r = height / 4.0;
  mt.mark = u"ice_piece_y";
  return Value(round(height / 2.0 + mt.range(-r, r)));`,
      to: `  const double height = to_number(field_or(field_or(e, u"frame"), u"height"));
  const double r = height / 4.0;
  mt.mark = u"ice_piece_y";
  return Value(floor(height / 2.0 + mt.range(-r, r)));`,
    },
    {
      note: "ice_piece_y 的中心写成 height/4",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  mt.mark = u"ice_piece_y";
  return Value(round(height / 2.0 + mt.range(-r, r)));`,
      to: `  mt.mark = u"ice_piece_y";
  return Value(round(height / 4.0 + mt.range(-r, r)));`,
    },
    {
      note: "random_in：mark 写成常量",
      file: "native/lfw/helper/randoming.cpp",
      from: `  _mt->mark = _name;`,
      to: `  _mt->mark = u\"x\";`,
    },
    {
      note: "random_in：不写 mark",
      file: "native/lfw/helper/randoming.cpp",
      from: `  _mt->mark = _name;\n  return _mt->range(l, r);`,
      to: `  return _mt->range(l, r);`,
    },
    {
      note: "random_in：mark 写在抽取之后",
      file: "native/lfw/helper/randoming.cpp",
      from: `  _mt->mark = _name;\n  return _mt->range(l, r);`,
      to: `  const double v = _mt->range(l, r);\n  _mt->mark = _name;\n  return v;`,
    },
    {
      note: "ice_piece_x：mark 写成 ice_piece_y",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  mt.mark = u\"ice_piece_x\";`,
      to: `  mt.mark = u\"ice_piece_y\";`,
    },
    {
      note: "ice_piece_x：不写 mark",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  const double r = width / 4.0;\n  mt.mark = u\"ice_piece_x\";\n  return`,
      to: `  const double r = width / 4.0;\n  (void)0;\n  return`,
    },
    {
      note: "ice_piece_x：mark 写在抽取之后",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  mt.mark = u\"ice_piece_x\";\n  return Value(round(width / 2.0 + mt.range(-r, r)));`,
      to: `  const Value out = Value(round(width / 2.0 + mt.range(-r, r)));\n  mt.mark = u\"ice_piece_x\";\n  return out;`,
    },
    {
      note: "ice_piece_y：mark 写成 ice_piece_x",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  mt.mark = u\"ice_piece_y\";`,
      to: `  mt.mark = u\"ice_piece_x\";`,
    },
    {
      note: "ice_piece_y：不写 mark",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  const double r = height / 4.0;\n  mt.mark = u\"ice_piece_y\";\n  return`,
      to: `  const double r = height / 4.0;\n  (void)0;\n  return`,
    },
    {
      note: "ice_piece_y：mark 写在抽取之后",
      file: "native/lfw/state/spawn_ice_piece.cpp",
      from: `  mt.mark = u\"ice_piece_y\";\n  return Value(round(height / 2.0 + mt.range(-r, r)));`,
      to: `  const Value out = Value(round(height / 2.0 + mt.range(-r, r)));\n  mt.mark = u\"ice_piece_y\";\n  return out;`,
    },
  ],
};
