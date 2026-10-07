// `lfw/dat_translator/xml/*`（xml 方言读写层）的变异档。
//
// 用例：`cases/xml/layer.txt`。
//
// 有意不覆盖（不可观察 / 走不到 / 台面造不出）：
//   * `xml_2_cpoint` 的 `reorder_fields`：`cpoint_new()` 是空对象，且赋值顺序本来就与
//     `cpoint_info_fields` 的顺序一致 ⇒ 重排结果与插入顺序相同，不可观察。
//   * `xml_2_qube` 里 `delete_undefined` 的「六键内」效果：六键随后被整体重写，只有
//     「六键之外、值为 undefined 的键」能观察（用例 `qout2.q` 锁住）。
//   * `xml_x_next_frame` 的 `blink_time`/`mp*`/`ctrl` 等属性：layer 用例没脚本化到每一项
//     （同形的 `dv` 已锁）；后续帧族用例再补。
//   * `xml_2_bpoint` 与 `xml_x_bpoint` 的重叠面：`bpoint_info_new()` 的默认值在写口不参与
//     （写口只读给到的字段）。
export default {
  subject: "xml",
  cases: ["layer"],
  mutations: [
    // ---------------------------------------------------------------- 助手
    {
      note: "one_or_arr(): 单元素数组不再拆包",
      file: "native/lfw/dat_translator/xml/one_or_arr.cpp",
      from: `  if (a->size() == 1) return a->at(0);`,
      to: `  if (false) return a->at(0);`,
    },
    {
      note: "non_empty(): 空数组也算非空",
      file: "native/lfw/dat_translator/xml/one_or_arr.cpp",
      from: `  if (a != nullptr && a->size() != 0) return mess;`,
      to: `  if (a != nullptr) return mess;`,
    },
    {
      note: "merge_by_tag(): 不合并后续同名子元素",
      file: "native/lfw/dat_translator/xml/merge_by_tag.cpp",
      from: `  for (size_t i = 1; i < children.size(); ++i) {
    assign_into(ret, parser(*children[i]));
  }`,
      to: `  for (size_t i = 1; i < children.size(); ++i) {
  }`,
    },
    {
      note: "merge_by_tag(): 不给 target 合并",
      file: "native/lfw/dat_translator/xml/merge_by_tag.cpp",
      from: `  if (truthy(target)) assign_into(target, ret);`,
      to: `  if (false) assign_into(target, ret);`,
    },
    {
      note: "merge_by_tag(): 没有同名子元素也去解析（越界）",
      file: "native/lfw/dat_translator/xml/merge_by_tag.cpp",
      from: `  if (children.empty()) return Value();`,
      to: `  if (false) return Value();`,
    },
    {
      note: "parse_rect_qube(): rect 有效时也去看 qube",
      file: "native/lfw/dat_translator/xml/parse_rect_qube.cpp",
      from: `  if (rect && rect->size() >= 4) return from_nums(*rect);`,
      to: `  if (false && rect && rect->size() >= 4) return from_nums(*rect);`,
    },
    {
      note: "parse_rect_qube(): z 取了 h 的分量",
      file: "native/lfw/dat_translator/xml/parse_rect_qube.cpp",
      from: `  o->set(u"z", v.size() > 4 ? Value(v[4]) : Value());`,
      to: `  o->set(u"z", v.size() > 4 ? Value(v[3]) : Value());`,
    },

    // ---------------------------------------------------------------- qube
    {
      note: "xml_2_qube(): rect/qube 优先级反了",
      file: "native/lfw/dat_translator/xml/xml_x_qube.cpp",
      from: `    Value fallback = field_or(*o, kKeys[i]);
    if (const std::optional<double> vb = at(b, i)) fallback = Value(*vb);
    if (const std::optional<double> va = at(a, i)) fallback = Value(*va);`,
      to: `    Value fallback = field_or(*o, kKeys[i]);
    if (const std::optional<double> va = at(a, i)) fallback = Value(*va);
    if (const std::optional<double> vb = at(b, i)) fallback = Value(*vb);`,
    },
    {
      note: "xml_2_qube(): 不删旧值里的 undefined 键",
      file: "native/lfw/dat_translator/xml/xml_x_qube.cpp",
      from: `  delete_undefined(out);`,
      to: `  ;`,
    },
    {
      note: "xml_2_qube(): 只写回四个分量（丢 z/l）",
      file: "native/lfw/dat_translator/xml/xml_x_qube.cpp",
      from: `  for (size_t i = 0; i < 6; ++i) o->set(std::u16string(kKeys[i]), temp[i]);`,
      to: `  for (size_t i = 0; i < 4; ++i) o->set(std::u16string(kKeys[i]), temp[i]);`,
    },

    // ---------------------------------------------------------------- velocity
    {
      note: "xml_to_velocity_info(): 软属性不再覆盖数字分量",
      file: "native/lfw/dat_translator/xml/xml_to_velocity_info.cpp",
      from: `    if (const std::optional<double> d = (*v)[i]) o.set(std::u16string(keys[i]), Value(*d));`,
      to: `    if (const std::optional<double> d = (*v)[i]; false) o.set(std::u16string(keys[i]), Value(*d));`,
    },
    {
      note: "xml_to_velocity_info(): 不删值为 undefined 的键",
      file: "native/lfw/dat_translator/xml/xml_to_velocity_info.cpp",
      from: `    if (v != nullptr && std::holds_alternative<std::monostate>(*v)) o->remove(std::u16string(key));`,
      to: `    if (false) o->remove(std::u16string(key));`,
    },
    {
      note: "xml_to_velocity_info(): 不用 out 里的现值做回落",
      file: "native/lfw/dat_translator/xml/xml_to_velocity_info.cpp",
      from: `    o->set(std::u16string(key),
           from_opt(get_num_or(el, std::u16string(key), field_or(*o, key))));`,
      to: `    o->set(std::u16string(key), from_opt(el.get_num(std::u16string(key))));`,
    },

    // ---------------------------------------------------------------- bdy
    {
      note: "xml_x_bdy(): qube 的 h/z 反了",
      file: "native/lfw/dat_translator/xml/xml_x_bdy.cpp",
      from: `    qube->push_back(field_or(b, u"h"));
    qube->push_back(field_or(b, u"z"));`,
      to: `    qube->push_back(field_or(b, u"z"));
    qube->push_back(field_or(b, u"h"));`,
    },
    {
      note: "xml_x_bdy(): action 子元素的 tag 写错",
      file: "native/lfw/dat_translator/xml/xml_x_bdy.cpp",
      from: `  xml_x_non_empty(xml, field_or(b, u"actions"), u"action", xml_x_colli_action, ret);`,
      to: `  xml_x_non_empty(xml, field_or(b, u"actions"), u"act", xml_x_colli_action, ret);`,
    },
    {
      note: "xml_x_bdy(): 不写 id 属性",
      file: "native/lfw/dat_translator/xml/xml_x_bdy.cpp",
      from: `  ret->set_attr(u"id", field_or(b, u"id"));`,
      to: `  ;`,
    },
    {
      note: "xml_2_bdy(): ref 不再回落 prefab_id",
      file: "native/lfw/dat_translator/xml/xml_x_bdy.cpp",
      from: `    if (!v) v = el.get_str(u"prefab_id");`,
      to: `    ;`,
    },
    {
      note: "xml_2_bdy(): 不读 hit_flag",
      file: "native/lfw/dat_translator/xml/xml_x_bdy.cpp",
      from: `  o->set(u"hit_flag", from_opt(get_num_or(el, u"hit_flag", field_or(*o, u"hit_flag"))));`,
      to: `  ;`,
    },
    {
      note: "xml_2_bdy(): 不读 test",
      file: "native/lfw/dat_translator/xml/xml_x_bdy.cpp",
      from: `  o->set(u"test", from_opt(get_str_or(el, u"test", field_or(*o, u"test"))));
  o->set(u"code", from_opt(get_num_or(el, u"code", field_or(*o, u"code"))));`,
      to: `  o->set(u"code", from_opt(get_num_or(el, u"code", field_or(*o, u"code"))));`,
    },

    // ---------------------------------------------------------------- itr
    {
      note: "xml_x_itr(): caughtact 用错 tag",
      file: "native/lfw/dat_translator/xml/xml_x_itr.cpp",
      from: `  push_attr_t_next(xml, ret, i, u"caughtact");`,
      to: `  push_attr_t_next(xml, ret, i, u"caughtact2");`,
    },
    {
      note: "xml_x_itr(): dv 三元组顺序反了",
      file: "native/lfw/dat_translator/xml/xml_x_itr.cpp",
      from: `  ret->set_arr_attr_soft(u"dv", arr3(field_or(i, u"dvx"), field_or(i, u"dvy"),
                                     field_or(i, u"dvz")));`,
      to: `  ret->set_arr_attr_soft(u"dv", arr3(field_or(i, u"dvy"), field_or(i, u"dvx"),
                                     field_or(i, u"dvz")));`,
    },
    {
      note: "xml_x_itr(): arest 写成了 vrest",
      file: "native/lfw/dat_translator/xml/xml_x_itr.cpp",
      from: `  ret->set_attr(u"arest", field_or(i, u"arest"));`,
      to: `  ret->set_attr(u"arest", field_or(i, u"vrest"));`,
    },
    {
      note: "xml_2_itr(): catchingact 读成 caughtact",
      file: "native/lfw/dat_translator/xml/xml_x_itr.cpp",
      from: `  o->set(u"catchingact", xml_2_t_next_frame(el.children_by_tag(u"catchingact")));`,
      to: `  o->set(u"catchingact", xml_2_t_next_frame(el.children_by_tag(u"caughtact")));`,
    },
    {
      note: "xml_2_itr(): 不读速度信息",
      file: "native/lfw/dat_translator/xml/xml_x_itr.cpp",
      from: `  xml_to_velocity_info(el, ret);
  xml_2_qube(el, ret);`,
      to: `  xml_2_qube(el, ret);`,
    },

    // ---------------------------------------------------------------- next_frame
    {
      note: "xml_x_next_frame(): 不写 expression 子元素",
      file: "native/lfw/dat_translator/xml/xml_x_next_frame.cpp",
      from: `  const Value expression = field_or(i, u"expression");
  if (truthy(expression)) {
    std::shared_ptr<IXMLElement> el = xml.create(u"expression");
    el->set_attr(u"value", expression);
    ret->insert(el);
  }`,
      to: `  const Value expression = field_or(i, u"expression");
  (void)expression;`,
    },
    {
      note: "xml_x_next_frame(): dv 的 y/z 反了",
      file: "native/lfw/dat_translator/xml/xml_x_next_frame.cpp",
      from: `    arr->push_back(field_or(i, u"dvx"));
    arr->push_back(field_or(i, u"dvy"));
    arr->push_back(field_or(i, u"dvz"));`,
      to: `    arr->push_back(field_or(i, u"dvx"));
    arr->push_back(field_or(i, u"dvz"));
    arr->push_back(field_or(i, u"dvy"));`,
    },
    {
      note: "xml_2_next_frame(): id 不再拆单元素数组",
      file: "native/lfw/dat_translator/xml/xml_x_next_frame.cpp",
      from: `  o->set(u"id", one_or_arr(el.get_str_arr(u"id")));`,
      to: `  o->set(u"id", non_empty(el.get_str_arr(u"id")));`,
    },
    {
      note: "xml_2_next_frame(): wait 优先取字符串",
      file: "native/lfw/dat_translator/xml/xml_x_next_frame.cpp",
      from: `    const std::optional<double> n = el.get_num(u"wait");
    o->set(u"wait", n ? Value(*n) : from_opt(el.get_str(u"wait")));`,
      to: `    o->set(u"wait", from_opt(el.get_str(u"wait")));`,
    },
    {
      note: "xml_x_t_next_frame(): 不挂到父上",
      file: "native/lfw/dat_translator/xml/xml_x_next_frame.cpp",
      from: `    if (parent) parent->insert(el);`,
      to: `    if (parent && false) parent->insert(el);`,
    },
    {
      note: "xml_x_t_next_frame(): 插入顺序反了",
      file: "native/lfw/dat_translator/xml/xml_x_next_frame.cpp",
      from: `    if (parent) parent->insert(el);`,
      to: `    if (parent) parent->insert(el, 0);`,
    },
    {
      note: "xml_2_t_next_frame(): 单项也回数组",
      file: "native/lfw/dat_translator/xml/xml_x_next_frame.cpp",
      from: `  if (arr->size() > 1) return Value(std::move(arr));`,
      to: `  if (arr->size() >= 1) return Value(std::move(arr));`,
    },

    // ---------------------------------------------------------------- wpoint
    {
      note: "xml_x_wpoint(): pos 的 z 分量写成了 x",
      file: "native/lfw/dat_translator/xml/xml_x_wpoint.cpp",
      from: `    pos->push_back(field_or(i, u"x"));
    pos->push_back(field_or(i, u"y"));
    pos->push_back(field_or(i, u"z"));`,
      to: `    pos->push_back(field_or(i, u"x"));
    pos->push_back(field_or(i, u"y"));
    pos->push_back(field_or(i, u"x"));`,
    },
    {
      note: "xml_x_wpoint(): v 的 y/z 反了",
      file: "native/lfw/dat_translator/xml/xml_x_wpoint.cpp",
      from: `    v->push_back(field_or(i, u"dvx"));
    v->push_back(field_or(i, u"dvy"));
    v->push_back(field_or(i, u"dvz"));`,
      to: `    v->push_back(field_or(i, u"dvx"));
    v->push_back(field_or(i, u"dvz"));
    v->push_back(field_or(i, u"dvy"));`,
    },
    {
      note: "xml_2_wpoint(): pos[0] 不再当 x 的回落",
      file: "native/lfw/dat_translator/xml/xml_x_wpoint.cpp",
      from: `    std::optional<double> fallback = at(pos, 0);
    if (!fallback) fallback = opt_num(field_or(*o, u"x"));
    o->set(u"x", from_opt(get_num_or(el, u"x", from_opt(fallback))));`,
      to: `    o->set(u"x", from_opt(get_num_or(el, u"x", field_or(*o, u"x"))));`,
    },

    // ---------------------------------------------------------------- cpoint
    {
      note: "xml_x_cpoint(): 不写 throwv",
      file: "native/lfw/dat_translator/xml/xml_x_cpoint.cpp",
      from: `    ret->set_arr_attr_soft(u"throwv", Value(std::move(throwv)));`,
      to: `    ;`,
    },
    {
      note: "xml_x_cpoint(): vaction 的 tag 写错",
      file: "native/lfw/dat_translator/xml/xml_x_cpoint.cpp",
      from: `       xml_x_t_next_frame(xml, field_or(i, u"vaction"), u"vaction")) {`,
      to: `       xml_x_t_next_frame(xml, field_or(i, u"vaction"), u"vact")) {`,
    },
    {
      note: "xml_2_cpoint(): 不读 vaction 子元素",
      file: "native/lfw/dat_translator/xml/xml_x_cpoint.cpp",
      from: `  o->set(u"vaction", xml_2_t_next_frame(el.children_by_tag(u"vaction")));`,
      to: `  o->set(u"vaction", xml_2_t_next_frame(el.children_by_tag(u"vact")));`,
    },
    {
      note: "xml_2_cpoint(): pos[0] 不再当 x 的回落",
      file: "native/lfw/dat_translator/xml/xml_x_cpoint.cpp",
      from: `    std::optional<double> fallback = at(pos, 0);
    if (!fallback) fallback = opt_num(field_or(*o, u"x"));
    o->set(u"x", from_opt(get_num_or(el, u"x", from_opt(fallback))));`,
      to: `    o->set(u"x", from_opt(get_num_or(el, u"x", field_or(*o, u"x"))));`,
    },

    // ---------------------------------------------------------------- armor / chase
    {
      note: "xml_x_armor_info(): 假值也去造元素",
      file: "native/lfw/dat_translator/xml/xml_x_armor_info.cpp",
      from: `  if (!truthy(a)) return nullptr;`,
      to: `  if (false) return nullptr;`,
    },
    {
      note: "xml_x_armor_info(): 不写 toughness",
      file: "native/lfw/dat_translator/xml/xml_x_armor_info.cpp",
      from: `  ret->set_attr(u"toughness", field_or(a, u"toughness"));`,
      to: `  ;`,
    },
    {
      note: "xml_2_armor_info(): 空元素也照样读（解引用）",
      file: "native/lfw/dat_translator/xml/xml_x_armor_info.cpp",
      from: `  if (el == nullptr) return Value();`,
      to: `  if (false) return Value();`,
    },
    {
      note: "xml_x_chase(): 三轴相等也走逗号串（多余的分支继续执行）",
      file: "native/lfw/dat_translator/xml/xml_x_chase.cpp",
      from: `    el->set_attr(u"overshoot", Value(*x));
    return;
  }`,
      to: `    el->set_attr(u"overshoot", Value(*x));
  }`,
    },
    {
      note: "xml_x_chase(): 三轴全 undefined 也写属性",
      file: "native/lfw/dat_translator/xml/xml_x_chase.cpp",
      from: `  if (!x && !y && !z) return;`,
      to: `  if (false) return;`,
    },
    {
      note: "xml_2_chase(): overshoot 的 y/z 反了",
      file: "native/lfw/dat_translator/xml/xml_x_chase.cpp",
      from: `  const char16_t* const keys[3] = {u"x", u"y", u"z"};`,
      to: `  const char16_t* const keys[3] = {u"x", u"z", u"y"};`,
    },
  ],
};
