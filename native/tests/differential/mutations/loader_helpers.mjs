export default {
  subject: "loader_helpers",
  mutations: [
    {
      note: "buring_smoke 的 foo 判定反向",
      file: "native/lfw/dat_translator/make_buring_smoke.cpp",
      from: `  if (foo == 1) {`,
      to: `  if (foo == 2) {`,
    },
    {
      note: "buring_smoke 的 oid 写错",
      file: "native/lfw/dat_translator/make_buring_smoke.cpp",
      from: `Value oid_broken_weapon() { return Value(std::u16string(oid::kBrokenWeapon)); }`,
      to: `Value oid_broken_weapon() { return Value(std::u16string(u"998")); }`,
    },
    {
      note: "buring_smoke 的 interval_id 前缀写错",
      file: "native/lfw/dat_translator/make_buring_smoke.cpp",
      from: `  ret.set(u"interval_id", Value(std::u16string(u"buring_smoke_") + number_to_string(static_cast<double>(foo))));`,
      to: `  ret.set(u"interval_id", Value(std::u16string(u"burning_smoke_") + number_to_string(static_cast<double>(foo))));`,
    },
    {
      note: "buring_smoke 的 interval 写成 2",
      file: "native/lfw/dat_translator/make_buring_smoke.cpp",
      from: `  ret.set(u"interval", n(3));`,
      to: `  ret.set(u"interval", n(2));`,
    },
    {
      note: "buring_smoke 的 unimportant 写成 0",
      file: "native/lfw/dat_translator/make_buring_smoke.cpp",
      from: `  ret.set(u"unimportant", n(1));`,
      to: `  ret.set(u"unimportant", n(0));`,
    },
    {
      note: "buring_smoke 的 interval_mode 写成 0",
      file: "native/lfw/dat_translator/make_buring_smoke.cpp",
      from: `  ret.set(u"interval_mode", n(1));`,
      to: `  ret.set(u"interval_mode", n(0));`,
    },
    {
      note: "buring_smoke 的 kind 写成 1",
      file: "native/lfw/dat_translator/make_buring_smoke.cpp",
      from: `  ret.set(u"kind", n(0));`,
      to: `  ret.set(u"kind", n(1));`,
    },
    {
      note: "buring_smoke 的 speedz/ghost 顺序互换",
      file: "native/lfw/dat_translator/make_buring_smoke.cpp",
      from: `  ret.set(u"speedz", n(0));
  ret.set(u"ghost", n(1));`,
      to: `  ret.set(u"ghost", n(1));
  ret.set(u"speedz", n(0));`,
    },
    {
      note: "buring_smoke 的 gen_x 公式改 w/3",
      file: "native/lfw/dat_translator/make_buring_smoke.cpp",
      from: `    ret.set(u"gen_x", s(u"round(rand(round(w/4), round(3*w/4)))"));`,
      to: `    ret.set(u"gen_x", s(u"round(rand(round(w/3), round(3*w/4)))"));`,
    },
    {
      note: "preprocess_pic 优先看 deg",
      file: "native/lfw/loader/preprocess_pic.cpp",
      from: `  if (is_number(rad)) {
    p->set(u"deg", Value(std::get<double>(rad) * 180.0 / PI));`,
      to: `  if (is_number(deg)) {
    p->set(u"deg", Value(std::get<double>(rad) * 180.0 / PI));`,
    },
    {
      note: "preprocess_pic 的 rad->deg 公式反了",
      file: "native/lfw/loader/preprocess_pic.cpp",
      from: `    p->set(u"deg", Value(std::get<double>(rad) * 180.0 / PI));`,
      to: `    p->set(u"deg", Value(std::get<double>(rad) * PI / 180.0));`,
    },
    {
      note: "preprocess_pic 的 cos/sin 互换",
      file: "native/lfw/loader/preprocess_pic.cpp",
      from: `    const double r = std::get<double>(rad);
    p->set(u"__cos_r", Value(cos(r)));
    p->set(u"__sin_r", Value(sin(r)));`,
      to: `    const double r = std::get<double>(rad);
    p->set(u"__cos_r", Value(sin(r)));
    p->set(u"__sin_r", Value(cos(r)));`,
    },
    {
      note: "preprocess_pic 的 deg 分支 cos/sin 互换",
      file: "native/lfw/loader/preprocess_pic.cpp",
      from: `    const double d = std::get<double>(deg) * PI / 180.0;
    p->set(u"rad", Value(d));
    p->set(u"__cos_r", Value(cos(d)));
    p->set(u"__sin_r", Value(sin(d)));`,
      to: `    const double d = std::get<double>(deg) * PI / 180.0;
    p->set(u"rad", Value(d));
    p->set(u"__cos_r", Value(sin(d)));
    p->set(u"__sin_r", Value(cos(d)));`,
    },
    {
      note: "preprocess_pic 的 deg 分支公式反了",
      file: "native/lfw/loader/preprocess_pic.cpp",
      from: `    const double d = std::get<double>(deg) * PI / 180.0;`,
      to: `    const double d = std::get<double>(deg) * 180.0 / PI;`,
    },
    {
      note: "preprocess_pic 用 truthy 判数字",
      file: "native/lfw/loader/preprocess_pic.cpp",
      from: `bool is_number(const Value& v) { return std::holds_alternative<double>(v); }`,
      to: `bool is_number(const Value& v) { return truthy(v); }`,
    },
    {
      note: "preprocess_frame_pic 丢掉原 pic",
      file: "native/lfw/loader/preprocess_pic.cpp",
      from: `  if (pic == nullptr || !truthy(*pic)) return pic != nullptr ? *pic : Value();`,
      to: `  if (pic == nullptr || !truthy(*pic)) return Value();`,
    },
    {
      note: "preprocess_wpoint 不返回原对象",
      file: "native/lfw/loader/preprocess_pic.cpp",
      from: `Value preprocess_wpoint(Value& wpoint) { return wpoint; }`,
      to: `Value preprocess_wpoint(Value& wpoint) { return Value(); }`,
    },
    {
      note: "preprocess_stage_phase 不做 delete_undefined",
      file: "native/lfw/loader/preprocess_stage.cpp",
      from: `  dat_translator::delete_undefined(v);
  reorder_fields(v, stage_phase_info_fields());`,
      to: `  reorder_fields(v, stage_phase_info_fields());`,
    },
    {
      note: "preprocess_stage_phase 用错了键表",
      file: "native/lfw/loader/preprocess_stage.cpp",
      from: `  reorder_fields(v, stage_phase_info_fields());`,
      to: `  reorder_fields(v, stage_info_fields());`,
    },
    {
      note: "preprocess_stage 不遍历 phases",
      file: "native/lfw/loader/preprocess_stage.cpp",
      from: `      const size_t n = a->size();
      for (size_t i = 0; i < n; ++i) {
        Value item = a->at(i);
        preprocess_stage_phase(item);
      }`,
      to: `      const size_t n = 0;
      for (size_t i = 0; i < n; ++i) {
        Value item = a->at(i);
        preprocess_stage_phase(item);
      }`,
    },
    {
      note: "preprocess_stage 不做 delete_undefined",
      file: "native/lfw/loader/preprocess_stage.cpp",
      from: `  dat_translator::delete_undefined(v);
  reorder_fields(v, stage_info_fields());
  return v;
}`,
      to: `  reorder_fields(v, stage_info_fields());
  return v;
}`,
    },
    {
      note: "preprocess_stage 读错 phases 键名",
      file: "native/lfw/loader/preprocess_stage.cpp",
      from: `  const Value* phases = o->get(u"phases");
  if (phases != nullptr) {`,
      to: `  const Value* phases = o->get(u"phase");
  if (phases != nullptr) {`,
    },
    {
      note: "preprocess_stage 用错了键表",
      file: "native/lfw/loader/preprocess_stage.cpp",
      from: `  reorder_fields(v, stage_info_fields());
  return v;
}`,
      to: `  reorder_fields(v, stage_phase_info_fields());
  return v;
}`,
    },
  ],
};
