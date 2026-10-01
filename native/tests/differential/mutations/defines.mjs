export default {
  subject: "defines",
  mutations: [
    {
      note: "生成: BdyKind::Defend 值改错",
      file: "native/lfw/defines/bdy_kind.h",
      from: `  Defend = 2000,`,
      to: `  Defend = 2001,`,
    },
    {
      note: "生成: name_of 少一个 case",
      file: "native/lfw/defines/bdy_kind.h",
      from: `    case static_cast<int>(BdyKind::Ignore): return u"Ignore";
`,
      to: ``,
    },
    {
      note: "生成: 字符串枚举成员值改错",
      file: "native/lfw/defines/team_enum.h",
      from: `inline constexpr const char16_t* kTeam_8 = u"8";`,
      to: `inline constexpr const char16_t* kTeam_8 = u"9";`,
    },
    {
      note: "手写: 位标志 OR 组合写错",
      file: "native/lfw/defines/hit_flag.h",
      from: `  Both = Ally | Enemy,`,
      to: `  Both = Ally | Enemy | Ball,`,
    },
    {
      note: "手写: 别名反向映射没取「后者胜」",
      file: "native/lfw/defines/facing_flag.h",
      from: `      return u"SameAsBearer";`,
      to: `      return u"SameAsCatcher";`,
    },
    {
      note: "手写: 跨枚举引用取错成员",
      file: "native/lfw/defines/entity_enum.h",
      from: `  Ball = static_cast<int>(HitFlag::Ball),`,
      to: `  Ball = 31,`,
    },
  ],
};
