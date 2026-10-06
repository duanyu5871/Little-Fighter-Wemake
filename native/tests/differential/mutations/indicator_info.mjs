export default {
  subject: "indicator_info",
  // 有意不覆盖：`bdy` 列表之后的 `if (!ok) return false;` —— 它和末尾那次检查等价
  // （中间只有 `itr` 的遍历，而它会因为 `ok == false` 立刻返回，两边都在返回前停住）。
  mutations: [
    {
      note: "不再检查 pic 是否有 w 键",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  const bool use_pic = pic_o != nullptr && has_key(pic_o, u"w");`,
      to: `  const bool use_pic = pic_o != nullptr;`,
    },
    {
      note: "h 的来源也照着 w 判定改",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  const Value h = use_pic ? field_at(pic, u"h") : field_at(frame, u"height");`,
      to: `  const Value h = use_pic ? field_at(pic, u"w") : field_at(frame, u"height");`,
    },
    {
      note: "w/h 的提前返回改成与",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  if (!truthy(w) || !truthy(h)) return true;`,
      to: `  if (!truthy(w) && !truthy(h)) return true;`,
    },
    {
      note: "f_qube_1 的 y 用加号",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `                              {u"y", sub(field_at(frame, u"centery"), h)},`,
      to: `                              {u"y", add(field_at(frame, u"centery"), h)},`,
    },
    {
      note: "f_qube_1 的 z 写成 1",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `                              {u"z", Value(0.0)},
                              {u"l", Value(0.0)}});
  const Value f2 = copy_with_x(f1, sub(field_at(frame, u"centerx"), field_at(f1, u"w")));`,
      to: `                              {u"z", Value(1.0)},
                              {u"l", Value(0.0)}});
  const Value f2 = copy_with_x(f1, sub(field_at(frame, u"centerx"), field_at(f1, u"w")));`,
    },
    {
      note: "f_qube_2 的 x 用 h",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  const Value f2 = copy_with_x(f1, sub(field_at(frame, u"centerx"), field_at(f1, u"w")));`,
      to: `  const Value f2 = copy_with_x(f1, sub(field_at(frame, u"centerx"), field_at(f1, u"h")));`,
    },
    {
      note: "indicator 的两个朝向键互换",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  Object o;
  o.set(u"1", q1);
  o.set(u"-1", q2);`,
      to: `  Object o;
  o.set(u"1", q2);
  o.set(u"-1", q1);`,
    },
    {
      note: "opoint 的 rect 键序变化",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    const Value rect1 = make_qube({{u"w", Value(2.0)},
                                   {u"h", Value(2.0)},
                                   {u"x", sub(add(f1x, field_at(o, u"x")), Value(1.0))},
                                   {u"y", sub(sub(add(f1y, f1h), field_at(o, u"y")), Value(1.0))},
                                   {u"z", or_zero_if_falsy(field_at(o, u"z"))},
                                   {u"l", Value(0.0)}});`,
      to: `    const Value rect1 = make_qube({{u"w", Value(2.0)},
                                   {u"h", Value(2.0)},
                                   {u"y", sub(sub(add(f1y, f1h), field_at(o, u"y")), Value(1.0))},
                                   {u"x", sub(add(f1x, field_at(o, u"x")), Value(1.0))},
                                   {u"z", or_zero_if_falsy(field_at(o, u"z"))},
                                   {u"l", Value(0.0)}});`,
    },
    {
      note: "opoint 的 x 偏移符号反了",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `                                   {u"x", sub(add(f1x, field_at(o, u"x")), Value(1.0))},
                                   {u"y", sub(sub(add(f1y, f1h), field_at(o, u"y")), Value(1.0))},
                                   {u"z", or_zero_if_falsy(field_at(o, u"z"))},`,
      to: `                                   {u"x", add(add(f1x, field_at(o, u"x")), Value(1.0))},
                                   {u"y", sub(sub(add(f1y, f1h), field_at(o, u"y")), Value(1.0))},
                                   {u"z", or_zero_if_falsy(field_at(o, u"z"))},`,
    },
    {
      note: "opoint 的 z 不再兜底",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `                                   {u"z", or_zero_if_falsy(field_at(o, u"z"))},
                                   {u"l", Value(0.0)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), Value(2.0)), field_at(o, u"x")));`,
      to: `                                   {u"z", field_at(o, u"z")},
                                   {u"l", Value(0.0)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), Value(2.0)), field_at(o, u"x")));`,
    },
    {
      note: "opoint 的 rect2 x 用加号",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), Value(2.0)), field_at(o, u"x")));`,
      to: `    const Value rect2 =
        copy_with_x(rect1, sub(add(add(f2x, f1w), Value(2.0)), field_at(o, u"x")));`,
    },
    {
      note: "cpoint 的 oz 不再用 falsy 兜底",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    const Value oz = or_zero_if_falsy(or_zero_if_undefined(field_at(cpoint, u"z")));`,
      to: `    const Value oz = or_zero_if_undefined(field_at(cpoint, u"z"));`,
    },
    {
      note: "bpoint 的 z 用 undefined 兜底",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `                                   {u"z", or_zero_if_falsy(field_at(bpoint, u"z"))},
                                   {u"l", Value(0.0)}});`,
      to: `                                   {u"z", or_zero_if_undefined(field_at(bpoint, u"z"))},
                                   {u"l", Value(0.0)}});`,
    },
    {
      note: "wpoint 的 y 用加号",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `                                   {u"y",
                                    sub(sub(add(f1y, f1h), field_at(wpoint, u"y")), Value(1.0))},`,
      to: `                                   {u"y",
                                    sub(add(add(f1y, f1h), field_at(wpoint, u"y")), Value(1.0))},`,
    },
    {
      note: "bdy 的 rect 键序变化",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    const Value rect1 = make_qube({{u"w", bw},
                                   {u"h", bh},
                                   {u"z", or_zero_if_undefined(field_at(o, u"z"))},
                                   {u"l", or_zero_if_undefined(field_at(o, u"l"))},
                                   {u"x", add(f1x, or_zero_if_undefined(field_at(o, u"x")))},
                                   {u"y", sub(sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y"))), bh)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });
  if (!ok) return false;

  for_each_object(field_at(frame, u"itr"), ok, [&](Value& o) {`,
      to: `    const Value rect1 = make_qube({{u"w", bw},
                                   {u"h", bh},
                                   {u"l", or_zero_if_undefined(field_at(o, u"l"))},
                                   {u"z", or_zero_if_undefined(field_at(o, u"z"))},
                                   {u"x", add(f1x, or_zero_if_undefined(field_at(o, u"x")))},
                                   {u"y", sub(sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y"))), bh)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });
  if (!ok) return false;

  for_each_object(field_at(frame, u"itr"), ok, [&](Value& o) {`,
    },
    {
      note: "bdy 的 y 不再减 bh",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `                                   {u"y", sub(sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y"))), bh)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });
  if (!ok) return false;

  for_each_object(field_at(frame, u"itr"), ok, [&](Value& o) {`,
      to: `                                   {u"y", sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y")))}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });
  if (!ok) return false;

  for_each_object(field_at(frame, u"itr"), ok, [&](Value& o) {`,
    },
    {
      note: "itr 的 z 用 falsy 兜底",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  for_each_object(field_at(frame, u"itr"), ok, [&](Value& o) {
    const Value bw = or_zero_if_undefined(field_at(o, u"w"));
    const Value bh = or_zero_if_undefined(field_at(o, u"h"));
    const Value rect1 = make_qube({{u"w", bw},
                                   {u"h", bh},
                                   {u"z", or_zero_if_undefined(field_at(o, u"z"))},`,
      to: `  for_each_object(field_at(frame, u"itr"), ok, [&](Value& o) {
    const Value bw = or_zero_if_undefined(field_at(o, u"w"));
    const Value bh = or_zero_if_undefined(field_at(o, u"h"));
    const Value rect1 = make_qube({{u"w", bw},
                                   {u"h", bh},
                                   {u"z", or_zero_if_falsy(field_at(o, u"z"))},`,
    },
    {
      note: "itr 的 w/h 互换",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  for_each_object(field_at(frame, u"itr"), ok, [&](Value& o) {
    const Value bw = or_zero_if_undefined(field_at(o, u"w"));
    const Value bh = or_zero_if_undefined(field_at(o, u"h"));`,
      to: `  for_each_object(field_at(frame, u"itr"), ok, [&](Value& o) {
    const Value bw = or_zero_if_undefined(field_at(o, u"h"));
    const Value bh = or_zero_if_undefined(field_at(o, u"w"));`,
    },
    {
      note: "itr 的 rect2 x 用 f2x",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `                                   {u"y", sub(sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y"))), bh)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });
  if (!ok) return false;

  return true;
}`,
      to: `                                   {u"y", sub(sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y"))), bh)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f2x), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });
  if (!ok) return false;

  return true;
}`,
    },
    {
      note: "or_zero_if_falsy 改成 undefined 版",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `Value or_zero_if_falsy(const Value& v) { return truthy(v) ? v : Value(0.0); }`,
      to: `Value or_zero_if_falsy(const Value& v) { return or_zero_if_undefined(v); }`,
    },
    {
      note: "or_zero_if_undefined 改成 falsy 版",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `Value or_zero_if_undefined(const Value& v) {
  return std::holds_alternative<std::monostate>(v) ? Value(0.0) : v;
}`,
      to: `Value or_zero_if_undefined(const Value& v) { return truthy(v) ? v : Value(0.0); }`,
    },
    {
      note: "copy_with_x 改成直接赋值",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `      out.set(k, k == u"x" ? x : *o->get(k));`,
      to: `      out.set(k, *o->get(k));`,
    },
    // ---------------------------------------------------------------- 本刀新增的失败路径
    {
      note: "pic 是原始值时不抛",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  if (truthy(pic) && !is_object_like(pic)) return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "pic 的对象判定反过来",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  if (truthy(pic) && !is_object_like(pic)) return false;`,
      to: `  if (truthy(pic) && is_object_like(pic)) return false;`,
    },
    {
      note: "pic 的存在判定改成 is_nullish",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  if (truthy(pic) && !is_object_like(pic)) return false;`,
      to: `  if (!std::holds_alternative<std::monostate>(pic) && !is_object_like(pic)) return false;`,
    },
    {
      note: "is_object_like 只认对象",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  return as_object(v) != nullptr || as_array(v) != nullptr;`,
      to: `  return as_object(v) != nullptr;`,
    },
    {
      note: "is_object_like 只认数组",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  return as_object(v) != nullptr || as_array(v) != nullptr;`,
      to: `  return as_array(v) != nullptr;`,
    },
    {
      note: "frame 不是对象时不抛",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  if (f == nullptr) return true;`,
      to: `  if (f == nullptr) return false;`,
    },
    {
      note: "列表是假值也要遍历（⇒ 非数组失败）",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  if (!truthy(list)) return;
  Array* a = const_cast<Array*>(as_array(list));`,
      to: `  if (false) return;
  Array* a = const_cast<Array*>(as_array(list));`,
    },
    {
      note: "列表是真值非数组也当成功",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  if (a == nullptr) {
    ok = false;
    return;
  }`,
      to: `  if (a == nullptr) {
    return;
  }`,
    },
    {
      note: "列表项是标量也当成功",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    if (!is_object_like(item)) {
      ok = false;
      return;
    }`,
      to: `    if (!is_object_like(item)) {
      return;
    }`,
    },
    {
      note: "列表项是标量时只跳过（不结束遍历）",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    if (!is_object_like(item)) {
      ok = false;
      return;
    }`,
      to: `    if (!is_object_like(item)) {
      ok = false;
      continue;
    }`,
    },
    {
      note: "opoint 之后不检查 ok",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    set_indicator(o, rect1, rect2);
  });
  if (!ok) return false;

  const Value cpoint = field_at(frame, u"cpoint");`,
      to: `    set_indicator(o, rect1, rect2);
  });
  if (false) return false;

  const Value cpoint = field_at(frame, u"cpoint");`,
    },
    {
      note: "itr 之后不检查 ok",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  if (!ok) return false;

  return true;
}`,
      to: `  if (false) return false;

  return true;
}`,
    },
    {
      note: "set_indicator 给标量也当成功",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    return as_array(target) != nullptr;`,
      to: `    return true;`,
    },
    {
      note: "set_indicator 给数组也当失败",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    return as_array(target) != nullptr;`,
      to: `    return false;`,
    },
    {
      note: "indicator 的键写成 __indicator_info_",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  t->set(u"__indicator_info", indicator_pair(q1, q2));`,
      to: `  t->set(u"__indicator_info_", indicator_pair(q1, q2));`,
    },
    {
      note: "cpoint 的失败不上报",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    if (!set_indicator(cpoint, rect1, rect2)) return false;`,
      to: `    if (false) return false;`,
    },
    {
      note: "bpoint 的失败不上报",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    if (!set_indicator(bpoint, rect1, rect2)) return false;`,
      to: `    if (false) return false;`,
    },
    {
      note: "wpoint 的失败不上报",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `    if (!set_indicator(wpoint, rect1, rect2)) return false;`,
      to: `    if (false) return false;`,
    },
  ],
};
