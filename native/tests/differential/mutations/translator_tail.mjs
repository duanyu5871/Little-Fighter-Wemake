export default {
  subject: "translator_tail",
  mutations: [
    {
      note: "edit_info 漏掉数组源",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `    if (const Array* from = as_array(edit)) {
      for (size_t i = 0; i < from->size(); ++i) {
        src.set(number_to_string(static_cast<double>(i)), from->at(i));
      }
    }`,
      to: `    if (const Array* from = as_array(edit)) {
      for (size_t i = 0; i < from->size(); ++i) {
        src.set(number_to_string(static_cast<double>(i + 1)), from->at(i));
      }
    }`,
    },
    {
      note: "edit_info 跳过值为 undefined 的键",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `        const Value* v = from->get(k);
        if (v != nullptr) src.set(k, *v);`,
      to: `        const Value* v = from->get(k);
        if (v != nullptr && truthy(*v)) src.set(k, *v);`,
    },
    {
      note: "edit_info 写入的是 undefined 而不是原值",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `        const Value* v = from->get(k);
        if (v != nullptr) src.set(k, *v);`,
      to: `        const Value* v = from->get(k);
        if (v != nullptr) src.set(k, Value());`,
    },
    {
      note: "edit_info 后一个源不覆盖前一个",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `        const Value* v = from->get(k);
        if (v != nullptr) src.set(k, *v);`,
      to: `        const Value* v = from->get(k);
        if (v != nullptr && src.get(k) == nullptr) src.set(k, *v);`,
    },
    {
      note: "edit_info 忽略对象源",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `    if (const Object* from = as_object(edit)) {`,
      to: `    if (const Object* from = nullptr) {
      (void)from;`,
    },
    {
      note: "scale_num_field 用 round 而不是 floor",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `  o.set(std::u16string(key), Value(floor(10000.0 * to_number(*cur))));`,
      to: `  o.set(std::u16string(key), Value(round(10000.0 * to_number(*cur))));`,
    },
    {
      note: "scale_num_field 的乘数写错",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `  o.set(std::u16string(key), Value(floor(10000.0 * to_number(*cur))));`,
      to: `  o.set(std::u16string(key), Value(floor(1000.0 * to_number(*cur))));`,
    },
    {
      note: "scale_num_field 判定写成 truthy",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `  if (cur == nullptr || !is_num(*cur)) return;
  o.set(std::u16string(key), Value(floor(10000.0 * to_number(*cur))));`,
      to: `  if (cur == nullptr || !truthy(*cur)) return;
  o.set(std::u16string(key), Value(floor(10000.0 * to_number(*cur))));`,
    },
    {
      note: "帧上的 round_float 判定写成 is_num",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `  const Value* cur = o.get(std::u16string(key));
  if (cur == nullptr || !truthy(*cur)) return;
  o.set(std::u16string(key), Value(round_float(to_number(*cur))));`,
      to: `  const Value* cur = o.get(std::u16string(key));
  if (cur == nullptr || !is_num(*cur)) return;
  o.set(std::u16string(key), Value(round_float(to_number(*cur))));`,
    },
    {
      note: "帧上的 round_float 乘数写错",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `  o.set(std::u16string(key), Value(round_float(to_number(*cur))));`,
      to: `  o.set(std::u16string(key), Value(round_float(to_number(*cur), 100.0)));`,
    },
    {
      note: "帧上漏掉 ctrl_z",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `  static const char16_t* const kRounded[] = {u"dvx", u"dvy", u"dvz", u"acc_x", u"acc_y",
                                             u"acc_z", u"ctrl_x", u"ctrl_y", u"ctrl_z"};`,
      to: `  static const char16_t* const kRounded[] = {u"dvx", u"dvy", u"dvz", u"acc_x", u"acc_y",
                                             u"acc_z", u"ctrl_x", u"ctrl_y"};`,
    },
    {

      note: "dataset 漏掉 gravity",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `    round_truthy_field(*dataset, u"friction_x");
    round_truthy_field(*dataset, u"friction_z");
    round_truthy_field(*dataset, u"gravity");`,
      to: `    round_truthy_field(*dataset, u"friction_x");
    round_truthy_field(*dataset, u"friction_z");`,
    },
    {
      note: "cpoint 的键表写错",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `  if (Object* cp = member_object(frame, u"cpoint")) {
    static const char16_t* const kKeys[] = {u"throwvx", u"throwvy", u"throwvz"};
    scale_fields(*cp, kKeys, 3);
  }`,
      to: `  if (Object* cp = member_object(frame, u"cpoint")) {
    static const char16_t* const kKeys[] = {u"throwvx", u"throwvy", u"throwvz1"};
    scale_fields(*cp, kKeys, 3);
  }`,
    },
    {
      note: "cpoint 用 wpoint 的键",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `    static const char16_t* const kKeys[] = {u"throwvx", u"throwvy", u"throwvz"};
    scale_fields(*cp, kKeys, 3);`,
      to: `    static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz"};
    scale_fields(*cp, kKeys, 3);`,
    },
    {
      note: "wpoint 漏掉 dvz",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `    static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz"};
    scale_fields(*wp, kKeys, 3);`,
      to: `    static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz1"};
    scale_fields(*wp, kKeys, 3);`,
    },
    {
      note: "opoint 漏掉 speedz",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `    static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz", u"speedz"};
    scale_elements(*opoint, kKeys, 4);`,
      to: `    static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz", u"speedz1"};
    scale_elements(*opoint, kKeys, 4);`,
    },
    {
      note: "base 的 9 个字段漏掉 weight",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `        u"dash_distance", u"dash_distancez", u"rowing_height", u"rowing_distance",
        u"weight"};
    scale_fields(*base, kBaseKeys, 9);`,
      to: `        u"dash_distance", u"dash_distancez", u"rowing_height", u"rowing_distance",
        u"weight"};
    scale_fields(*base, kBaseKeys, 8);`,
    },
    {
      note: "base 的字段名写错",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `        u"weight"};
    scale_fields(*base, kBaseKeys, 9);`,
      to: `        u"weights"};
    scale_fields(*base, kBaseKeys, 9);`,
    },
    {
      note: "base 的 jump_height 键名写错",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `        u"jump_height",   u"jump_distance", u"jump_distancez", u"dash_height",`,
      to: `        u"jump_height1",  u"jump_distance", u"jump_distancez", u"dash_height",`,
    },
    {
      note: "brokens 的 speedz 键名写错",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `      static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz", u"speedz"};
      scale_elements(*brokens, kKeys, 4);`,
      to: `      static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz", u"speedz1"};
      scale_elements(*brokens, kKeys, 4);`,
    },
    {
      note: "bot 的键名写错",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `    Value* bot = member_value(*base_value, u"bot");`,
      to: `    Value* bot = member_value(*base_value, u"bots");`,
    },
    {
      note: "bot 的 actions 键名写错",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `      Value* actions = member_value(*bot, u"actions");`,
      to: `      Value* actions = member_value(*bot, u"action");`,
    },
    {
      note: "e_ray 的 max_d 键名写错",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `    static const char16_t* const kKeys[] = {u"x",     u"z",     u"min_x", u"max_x",
                                            u"min_z", u"max_z", u"max_d"};`,
      to: `    static const char16_t* const kKeys[] = {u"x",     u"z",     u"min_x", u"max_x",
                                            u"min_z", u"max_z", u"max_d1"};`,
    },
    {
      note: "e_ray 的 min_z 键名写错",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `    static const char16_t* const kKeys[] = {u"x",     u"z",     u"min_x", u"max_x",
                                            u"min_z", u"max_z", u"max_d"};`,
      to: `    static const char16_t* const kKeys[] = {u"x",     u"z",     u"min_x", u"max_x",
                                            u"min_z1", u"max_z", u"max_d"};`,
    },
    {
      note: "整个 frames 段被跳过",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `  Value* frames = member_value(ret, u"frames");
  if (frames != nullptr && truthy(*frames)) {`,
      to: `  Value* frames = member_value(ret, u"frames");
  if (false) {`,
    },
    {
      note: "整个 base 段被跳过",
      file: "native/lfw/dat_translator/float_scaling_entity.cpp",
      from: `  Value* base_value = member_value(ret, u"base");
  Object* base = base_value != nullptr ? as_object(*base_value) : nullptr;
  if (base != nullptr) {`,
      to: `  Value* base_value = member_value(ret, u"base");
  Object* base = base_value != nullptr ? as_object(*base_value) : nullptr;
  if (false) {`,
    },
    {
      note: "头部占位长度 123 写成 124",
      file: "native/lfw/dat_translator/decode_lf2_dat.cpp",
      from: `constexpr size_t kHeadPlaceholderLength = 123;`,
      to: `constexpr size_t kHeadPlaceholderLength = 124;`,
    },
    {
      note: "头部占位长度 123 写成 122",
      file: "native/lfw/dat_translator/decode_lf2_dat.cpp",
      from: `constexpr size_t kHeadPlaceholderLength = 123;`,
      to: `constexpr size_t kHeadPlaceholderLength = 122;`,
    },
    {
      note: "密码串写错一个字符",
      file: "native/lfw/dat_translator/decode_lf2_dat.cpp",
      from: `const char16_t kPwd[] = u"SiuHungIsAGoodBearBecauseHeIsVeryGood";`,
      to: `const char16_t kPwd[] = u"SiuHungIsAGoodBearBecauseHeIsVeryGood1";`,
    },
    {
      note: "解密用加而不是减",
      file: "native/lfw/dat_translator/decode_lf2_dat.cpp",
      from: `    buf[i] = static_cast<uint8_t>(buf[i] - static_cast<uint8_t>(kPwd[i % pwd_len]));`,
      to: `    buf[i] = static_cast<uint8_t>(buf[i] + static_cast<uint8_t>(kPwd[i % pwd_len]));`,
    },
    {
      note: "密码下标错位",
      file: "native/lfw/dat_translator/decode_lf2_dat.cpp",
      from: `  for (size_t i = 0; i < len; ++i) {
    buf[i] = static_cast<uint8_t>(buf[i] - static_cast<uint8_t>(kPwd[i % pwd_len]));
  }`,
      to: `  for (size_t i = 0; i < len; ++i) {
    buf[i] = static_cast<uint8_t>(buf[i] - static_cast<uint8_t>(kPwd[(i + 1) % pwd_len]));
  }`,
    },
    {
      note: "密码长度算进了结尾 NUL",
      file: "native/lfw/dat_translator/decode_lf2_dat.cpp",
      from: `  const size_t pwd_len = sizeof(kPwd) / sizeof(kPwd[0]) - 1;`,
      to: `  const size_t pwd_len = sizeof(kPwd) / sizeof(kPwd[0]);`,
    },
    {
      note: "解密循环少解一个字节",
      file: "native/lfw/dat_translator/decode_lf2_dat.cpp",
      from: `  for (size_t i = 0; i < len; ++i) {`,
      to: `  for (size_t i = 0; i + 1 < len; ++i) {`,
    },
    {
      note: "输出字符整体偏移 1",
      file: "native/lfw/dat_translator/decode_lf2_dat.cpp",
      from: `    out.push_back(static_cast<char16_t>(buf[i]));`,
      to: `    out.push_back(static_cast<char16_t>(buf[i] + 1));`,
    },
    {
      note: "输出从第 0 个字节开始",
      file: "native/lfw/dat_translator/decode_lf2_dat.cpp",
      from: `  for (size_t i = kHeadPlaceholderLength; i < buf.size(); ++i) {`,
      to: `  for (size_t i = 0; i < buf.size(); ++i) {`,
    },
  ],
};
