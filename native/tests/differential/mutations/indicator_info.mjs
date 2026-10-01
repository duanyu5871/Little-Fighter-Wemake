export default {
  subject: "indicator_info",
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
      from: `  if (!truthy(w) || !truthy(h)) return;`,
      to: `  if (!truthy(w) && !truthy(h)) return;`,
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

  for_each_object(field_at(frame, u"itr"), [&](Value& o) {`,
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

  for_each_object(field_at(frame, u"itr"), [&](Value& o) {`,
    },
    {
      note: "bdy 的 y 不再减 bh",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `                                   {u"y", sub(sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y"))), bh)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });

  for_each_object(field_at(frame, u"itr"), [&](Value& o) {`,
      to: `                                   {u"y", sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y")))}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f1w), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });

  for_each_object(field_at(frame, u"itr"), [&](Value& o) {`,
    },
    {
      note: "itr 的 z 用 falsy 兜底",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  for_each_object(field_at(frame, u"itr"), [&](Value& o) {
    const Value bw = or_zero_if_undefined(field_at(o, u"w"));
    const Value bh = or_zero_if_undefined(field_at(o, u"h"));
    const Value rect1 = make_qube({{u"w", bw},
                                   {u"h", bh},
                                   {u"z", or_zero_if_undefined(field_at(o, u"z"))},`,
      to: `  for_each_object(field_at(frame, u"itr"), [&](Value& o) {
    const Value bw = or_zero_if_undefined(field_at(o, u"w"));
    const Value bh = or_zero_if_undefined(field_at(o, u"h"));
    const Value rect1 = make_qube({{u"w", bw},
                                   {u"h", bh},
                                   {u"z", or_zero_if_falsy(field_at(o, u"z"))},`,
    },
    {
      note: "itr 的 w/h 互换",
      file: "native/lfw/dat_translator/cook_frame_indicator_info.cpp",
      from: `  for_each_object(field_at(frame, u"itr"), [&](Value& o) {
    const Value bw = or_zero_if_undefined(field_at(o, u"w"));
    const Value bh = or_zero_if_undefined(field_at(o, u"h"));`,
      to: `  for_each_object(field_at(frame, u"itr"), [&](Value& o) {
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
}`,
      to: `                                   {u"y", sub(sub(add(f1y, f1h), or_zero_if_undefined(field_at(o, u"y"))), bh)}});
    const Value rect2 =
        copy_with_x(rect1, sub(sub(add(f2x, f2x), bw), or_zero_if_undefined(field_at(o, u"x"))));
    set_indicator(o, rect1, rect2);
  });
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
  ],
};
