export default {
  subject: "defines_fields",
  mutations: [
    {
      note: "字段表的 order 值改错",
      file: "native/lfw/defines/fields_gen.h",
      from: `"toughness_r_tick","order":14`,
      to: `"toughness_r_tick","order":15`,
    },
    {
      note: "options 的 value 改错",
      file: "native/lfw/defines/fields_gen.h",
      from: `{"value":4,"label":"Injury"`,
      to: `{"value":5,"label":"Injury"`,
    },
    {
      note: "非 ASCII 文本改错",
      file: "native/lfw/defines/fields_gen.h",
      from: `"??比例"`,
      to: `"XX比例"`,
    },
    {
      note: "整条字段被删掉",
      file: "native/lfw/defines/fields_gen.h",
      from: `"fireproof":{"type":"int","title":"防火","options":[{"value":0,"label":"NO"},{"value":1,"label":"YES"}],"key":"fireproof","order":4},`,
      to: ``,
    },
    {
      note: "字段类型改错",
      file: "native/lfw/defines/fields_gen.h",
      from: `"motionless_ratio":{"type":"float"`,
      to: `"motionless_ratio":{"type":"int"`,
    },
  ],
};
