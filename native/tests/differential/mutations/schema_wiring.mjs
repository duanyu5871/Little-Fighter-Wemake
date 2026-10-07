// `loader/preprocess_bg_data` 接 schema 校验（4W 接线）的变异档。
//
// 用例：`cases/loader_more/all.txt`（`bg` op：两侧把 `Ditto.warn/error` 捕获成 `bgw`/`bge` 行）。
//
// 有意不覆盖：`if (!sv.warnings().empty() && warnings != nullptr)` 的 `empty()` 守卫去掉后
// 插空区间是 no-op ⇒ 不可观察。
export default {
  subject: "loader_more",
  cases: ["all"],
  mutations: [
    {
      note: "terrain 项校验换成 stage phase 的 schema（错误全变）",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `      sv.validate(t, schema_i_terrain_info());`,
      to: `      sv.validate(t, schema_i_stage_phase_info());`,
    },
    {
      note: "terrain 项干脆不校验（warn/error 都没了）",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `      sv.validate(t, schema_i_terrain_info());`,
      to: `      (void)0;`,
    },
    {
      note: "warnings 的 sink 填成了 errors",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `        warnings->insert(warnings->end(), sv.warnings().begin(), sv.warnings().end());`,
      to: `        warnings->insert(warnings->end(), sv.errors().begin(), sv.errors().end());`,
    },
    {
      note: "errors 的 sink 填成了 warnings",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `        errors->insert(errors->end(), sv.errors().begin(), sv.errors().end());`,
      to: `        errors->insert(errors->end(), sv.warnings().begin(), sv.warnings().end());`,
    },
    {
      note: "errors 的回填换成看 warnings 是否非空",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `      if (!sv.errors().empty() && errors != nullptr) {`,
      to: `      if (!sv.warnings().empty() && errors != nullptr) {`,
    },
    {
      note: "校验后不 reset（错误会带进下一项）",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `      sv.reset();`,
      to: `      (void)0;`,
    },
  ],
};
