# native —— LFW 核心的 C++ 实现

目标：**单一实现 / 平台解耦**。让 `src/LFW` 的规则层能在浏览器（WASM）、
Electron 桌面壳、以及将来的原生客户端上跑同一份逻辑。

对应关系：

| | TS（现状，唯一可运行实现） | C++（本目录） |
|---|---|---|
| 纯核心 | `src/LFW/` | `native/lfw/` |
| 宿主实现 | `src/DittoImpl/` | `native/host/`（**尚未创建**） |
| 注入边界 | `src/LFW/ditto/Instance.ts` 的 `IDittoPack` | 镜像到 `native/lfw/ditto/`（**尚未创建**） |

---

## 硬规则（`native/lfw/` 下）

这份规则和 `src/LFW/README.md` 是同一个意图的两面：
**让核心不可撤销地保持平台无关。**

> TS 侧：只能写标准 ECMAScript（由 `scripts/check-lfw-esonly.mjs` 保证）
> C++ 侧：只能写纯语言设施（由 `native/tools/check_lfw_cpp_includes.mjs` 保证）

**零宿主 · 零 IO · 零线程 · 零异常 · 零 RTTI · 零第三方库**

| 类别 | 禁止 | 应该用 |
|---|---|---|
| 文件 | `<filesystem>` `<fstream>` `<cstdio>` | 注入的 `IImporter` |
| 流 | `<iostream>` `<sstream>` | 什么都不用（核心不打印） |
| 线程 | `<thread>` `<mutex>` `<atomic>` `<future>` | 无（核心是单线程的） |
| 时间 | `<chrono>` `<ctime>` | 注入的 `IClock` |
| 随机 | `<random>` | `utils/math/mersenne_twister.h` |
| locale | `<locale>` `<regex>` | 待建的 `utils/math/string.h` |
| 异常 | `throw` / `try` / `catch` | `std::optional` / 指针返回 `nullptr` |
| RTTI | `typeid` / `dynamic_cast` | 标签 + `switch`（对应 TS 的 `is_fighter()` 那套） |
| 舍入 | `std::round` `std::rint` `std::nearbyint` | `utils/math/base.h` 里的 `round` |
| 格式化 | `std::to_string` | 待建的 `js_number_to_string` |
| 链接 | `target_link_libraries(lfw_core ...)` 非空 | **保持为空** |

```
npm run native lint
```

---

## ★ 确定性：这是移植门槛，不是优化选项

`src/LFW` 是纯逻辑 + 定点化物理，两侧必须一致，否则联机/回放会分叉。
已核到行的地雷清单：

| # | 坑 | 现有证据 | C++ 侧规矩 |
|---|---|---|---|
| 1 | **FMA 收缩** | — | MSVC 刻意不用 `/arch:AVX*`；Clang/GCC 必须 `-ffp-contract=off`。已在 `CMakeLists.txt` 里强制 |
| 2 | **`Math.round`** | `utils/math/base.ts` 里 `round = Math['round']` | JS 是 half-up **朝 +∞**（`Math.round(-0.5) === -0`）；`std::round` 是 half-away-from-zero，**必错**。用 `core/js_num.h` 的 `js_round` |
| 3 | **NaN 比较器** | `World.ts:50` `x_sorter`：`const d = a.aabb_min_x - b.aabb_min_x; if (d !== 0) return d;` | NaN 时 JS sort 规范把结果当 `+0`；`std::sort` 收到 NaN 返回 = 违反严格弱序 = **UB**。C++ 比较器必须把 NaN 归一成 0 |
| 4 | **`std::sort` 不稳定** | `BotController.chasings.sort` / `CameraCtrl.fighters.sort` / `NearestTargets.targets.sort` / `ReachableGroup.reachables.sort` / `MoveTableLogic._steps.sort` 都没有 tie-breaker | V8 `Array#sort` 自 7.0 起稳定 → 必须 `std::stable_sort` |
| 5 | **`String(number)`** | 字段拼接、XML 输出 | 最短往返（`to_chars`），**不是** `to_string`；且 JS 的指数阈值（`1e21` / `1e-7`）与 `to_chars` 不同 |
| 6 | **`Object.keys()` 顺序** | `loader/DatMgr.ts:135/157`：`JSON.stringify(xml_roundtrip) === JSON.stringify(this)` | JS = 整数键升序 + 字符串键插入序 → `OrderedMap` |
| 7 | **字符串比较 = UTF-16 code unit** | — | `std::string` 是 UTF-8 字节序：ASCII 一致，**中文不一致**。参与排序的键要么证明全 ASCII，要么用 `utf16_compare` |
| 8 | **`localeCompare`** | `ui/component/DanmuGameLogic.ts:272` | locale 相关，C++ 无等价物 —— 需要定死 collation |

### libm：唯一无法用编译选项消除的源

`round_float` 本身是**纯 IEEE 运算**（比较 + `+0.5` + `std::floor` + 除法），
所以它跨实现逐位一致 —— 差异只可能从它**上游**漏进来。上游只有两个来源：

- FMA 收缩 → 已用编译选项消除
- **libm 函数**（`pow` / `exp` / `log` / `sin` / `cos` / `tan` / `atan2` / `acos` / `asin` / `hypot`）
  → V8 用自己的 fdlibm 移植，MSVC 用 UCRT，**可以差 1 ULP 甚至更多**
  （`sqrt` 安全：IEEE 要求正确舍入）

**收口**：`src/LFW` 与 `native/lfw` 各有一个 `utils/math/base.h` 镜像同一组名字。
将来要让两侧数值一致，只改这两个文件。`src/LFW` 里已经 **0 处**直调 `Math.pow/cos/…`，
也 **0 处** `**` 运算符（`**` 与 `Math.pow` 同算法，也算泄漏）。

自检命令：

```powershell
Select-String -Path "src\LFW\*.ts","src\LFW\**\*.ts" -Pattern 'Math\s*\.\s*(pow|cos|sin|tan|atan2|hypot|acos|asin|exp|log)\s*\(|[\w\)\]]\s\*\*\s[\w\(\-\.]'
```

### 实体状态永远落在 1/1000 格点上

`Entity.set_position` / `set_velocity` **每次写入都过 `round_float`**
（`Entity.ts:2348` / `:2340`），`handle_gravity` / `update_velocity` 的中间量也是。

实测：60 万个格点样本，`round_float` 的不动点性质 **0 例外**。
⇒ 两侧状态**要么逐位相同，要么至少差 0.001**，**不存在"缓慢漂移放大"**。
⇒ 从相同状态出发、且该帧运算无 libm，则下一帧状态**逐位相同**（可证明）。

---

## ★ 共享可变单例：TS 里三个几何函数的隐藏语义

`normalize_plane` / `calc_plane` / `line_plane_intersection` 在 TS 里都返回
**模块级共享的可变对象**（`const result = {...}` 然后 `return result`）。

C++ 侧必须镜像 —— 返回指向文件级 static 的 `const Plane*` / `const Vec3*`，
**不能**按值返回。否则"先拿引用、再调用一次、回头读第一次的引用"会静默出错。

差分测试里的 `alias_*` 用例专门锁这个语义（见 `tests/differential/PROTOCOL.md` §1.3）。
另外这三个函数在 TS 里会 `throw`（`range` / `project_to_line`），
C++ 侧映射成 `std::optional` 的 `nullopt`。

---

## 目录

```
native/
  CMakeLists.txt                     确定性开关（FMA / 异常 / RTTI）
  CMakePresets.json                  msvc-x64 / wasm
  cmake/toolchains/emscripten.cmake  WASM（⚠ 未验证，需要 emsdk）
  lfw/                               ★ 纯核心
    core/                            JS 语义地基（TS 侧无对应物）
      js_num.{h,cpp}                 js_round / js_to_uint32 / f64_bits
      state_hash.{h,cpp}             FNV-1a 64（差分测试用）
    defines/
      i_bounding.h                   ← defines/IBounding
    base/
      graves.h                       ← base/Graves.ts（对象池）
    utils/
      math/                          ← 镜像 src/LFW/utils/math/
        base.h                       ★ libm 单一收口（镜像 base.ts）
        floor_float.h  round_float.h
        clamp.h  clamp_add.h  float_equal.h  normalize.h
        normalize_plane.{h,cpp}  calc_plane.{h,cpp}
        line_plane_intersection.{h,cpp}  range.{h,cpp}
        probability.{h,cpp}  project_to_line.{h,cpp}
        mersenne_twister.{h,cpp}     ← utils/math/MersenneTwister.ts
      array/
        loop_arr.h  make_arr.h  map_arr.h
      container_help/
        filter.h  find.h  fisrt.h  ensure.h  loop_offset.h
        map_no_void.h  nested_map.h  nested_multi_map.h
      easing/
        ease_linearity.h  ease_in_out_sine.h  ease_in_out_quint.h
      cross_bounding.h  utf8.{h,cpp}  times.{h,cpp}
    (待搬) utils/{container_help/traversal,get_keys,set_obj_field,take_number,assign,list_fn,
           schema,string_parser,type_check,type_cast}
           loader/ entity/ state/ stage/ bot/ buff/ bg/ cmds/ controller/
           collision/（非叶子：靠 entity/buff/World）
  tests/differential/                ★ 两侧对拍（详见 PROTOCOL.md）
  tools/
    native.mjs                       configure / build / lint / test
    check_lfw_cpp_includes.mjs       分层规则检查
  host/                              宿主实现，镜像 src/DittoImpl/（尚未创建）
  ffi/lfw_c.h                        纯 C ABI 导出（尚未创建）
```

**命名约定**：C++ 侧文件名一律 `snake_case`，TS 侧保持 `PascalCase`。
这样 `World.ts ↔ world.h` 的映射是机械的，也天然避开 NTFS 大小写不敏感带来的冲突。
头文件统一 `.h`（因为 `ffi/lfw_c.h` 要能被 C 消费）。

小函数（`clamp` / `float_equal` / `normalize` / `base`）写成 header-only 的 `inline`，
避免为 6 行代码造 `.cpp`。

---

## 怎么跑

```
npm run native all                      # configure → build → lint → test
npm run native configure
npm run native build
npm run native lint
npm run native test                     # 或 test <subject> [<case>]
```

不需要配 PATH —— 脚本自己用 `vswhere` 找 VS 2022、自己调 `vcvars64.bat`、
自己把 VS 自带的 `cmake.exe` / `ninja.exe` 加进 PATH。
生成的实际命令落在 `native/build/gen/native-cmd.cmd`（可以直接打开看）。

VS Code 里也已经指好（`.vscode/settings.json`）：

```jsonc
"cmake.sourceDirectory": "${workspaceFolder}/native",
"cmake.buildDirectory": "${sourceDirectory}/build/vscode"
```

第二行是必须的 —— `cmake.buildDirectory` 的默认值 `${workspaceFolder}/build`
会撞上仓库根的 **Vite 网页构建输出目录**。

### IntelliSense

`.vscode/c_cpp_properties.json` 把 C++ 扩展指向 **CMake 导出的编译数据库**：

```jsonc
"compileCommands": "${workspaceFolder}/native/build/msvc-x64/compile_commands.json"
```

⇒ **先跑一次 `npm run native configure`**（或 `all`）。没这一步时编辑器会报
`cannot open source file "memory"` / `"vector"` 这类标准库缺失的错（不是代码问题）。

- `native/CMakePresets.json` 的 `msvc-x64` / `wasm` 预设都带 `CMAKE_EXPORT_COMPILE_COMMANDS=ON`；
- `native/tools/native.mjs configure` 在 `compile_commands.json` 缺失时会**重跑** configure
  （只有 `build/` 存在但缺这个文件时才补，不会每次白跑）；
- `compileCommands` 不存在时才退到配置里的 `includePath` / `compilerPath` 兜底；
  `compilerPath` 跟着 VS 版本走，升级 VS 后可能要改（当前 14.41.34120）。

---

## 移植进度

| 步骤 | 范围 | 状态 |
|---|---|---|
| 0 | `core/` + 差分测试台 | ✅ 通过（MT19937 逐位一致，7564 行） |
| 0.5 | 差分台子泛化为可插拔 subject | ✅ 通过 |
| 1 | `utils/math/` | ✅ 通过（`math/scalar` 90 行、`math/plane` 52 行） |
| 1.5a | `utils/{easing,cross_bounding,utf8,times}` + `defines/i_bounding.h` | ✅ 通过（52 / 8 / 41 / 74 行） |
| 1.5b | `base/graves.h` + `utils/array/` + `utils/container_help/` | ✅ 通过（`collections/basic` 81 行、`collections/nested` 55 行） |
| 1.6 | `base/Expression`（V7）+ `Object`（V6）+ `JSON`/`JSON5` | ✅ 通过（`expression` 303+216、`value/object` 109、`json` 62+157、`json5` 302+111+6454 行；共 48 条变异全杀） |
| 1.7 | `fields.ts`（字段描述 DSL + `fields()` + `reorder_fields` + `validate_fields`） | ✅ 通过（`fields` 77+155 行；19 条变异全杀） |
| 2a | `defines/` 的**枚举**（52 个：47 生成 + 5 手写；覆盖已验） | ✅ 通过（`defines/all` 851 行；6 条变异全杀） |
| 2b | `defines/` 的**字段表**（34 张，生成） | ✅ 通过（`defines_fields/all` 34 行；5 条变异全杀） |
| 2c | `defines/` 的**运行时数据**（`Defines` 命名空间 66 条 + 7 个顶层对象 + 5 个函数） | ✅ 通过（`defines_runtime/all` 149 行；9 条变异全杀）。`I*.ts` 结构体与 33 个 `*_new()` 只服务 `dat_translator`，运行时不用，暂缓 |
| 2d | `base/{FSM,Callbacks,NoEmitCallbacks}` | ✅ 通过（`base/core` 3127 行；21 条变异全杀） |
| 3a | `dat_translator/CondMaker`（条件字符串构造器，所有默认条件的共同前置） | ✅ 通过（`cond_maker/all` 55 行；17 条变异全杀） |
| 3b | `defines/` 名称/标签函数（`labels`，`set_hit_flag`/`set_bdy_kind` 的前置） | ✅ 通过（`labels/all` 107 行；15 条变异全杀） |
| 3c | `dat_translator` 小助手（`take*` / `set_*` / `fixed_float` / `find_float` / `copy_*`） | ✅ 通过（`dat_helpers/all` 100 行；19 条变异全杀） |
| 3d | `dat_translator` 帧跳转（`get_next_frame_by_raw_id` / `cook_next_frame_cost` / `add_next_frame`） | ✅ 通过（`next_frame/all` 83 行；17 条变异全杀） |
| 3e | `dat_translator/ColonValueReader`（手写匹配，替代被禁的 `RegExp`） | ✅ 通过（`colon_reader/all` 52 行；14 条变异全杀） |
| 3f | `dat_translator` 的 `cook_*` / `float_scaling_itr` | ✅ 通过（`cookers/all` 68 行；32 条变异全杀） |
| 3g | `dat_translator/make_frame_state`（+ `foreach` / `ensure` 的 `Value` 重载） | ✅ 通过（`cookers/mfstate` 20 行；累计 49 条变异全杀） |
| 3h | `dat_translator/frame_behavior/*`（14 个模块）+ `make_frame_behavior` 分发器 | ✅ 通过（`cookers/fbehavior` 52 行一次全对；累计 71 条变异全杀） |
| 3i | `dat_translator` 字符串匹配器（`match_colon_value` / `match_block_once` / `take_blocks` / `delete_undefined`） | ✅ 通过（`string_matchers/all` 35 行；13 条变异全杀） |
| 3j | `dat_translator` 的 `make_frames_special` / `make_entity_data`（+ `traversal`） | ✅ 通过（`entity_data/all` 22 行；11 条变异全杀） |
| 3k | `dat_translator/make_itr_prefabs`（+ 手写 `entry:` 匹配器 / `to_num` / `is_non_empty_str`） | ✅ 通过（`itr_prefabs/all` 14 行；11 条变异全杀） |
| 3l | `dat_translator` 的 `make_ball_data` / `make_weapon_data` | ✅ 通过（`entity_kinds/all` 17 行；19 条变异全杀） |
| 3m | `dat_translator/make_bg_data`（+ `make_bg_layer` / `bg_color_translate`） | ✅ 通过（`bg_data/all` 11 行；22 条变异全杀） |
| 3n | `dat_translator/post_process_obj_data`（装配层入口） | ✅ 通过（`entity_data/all` 29 行；累计 16 条变异全杀） |
| 3o | `dat_translator/EditBdy` + `cook_ball_bdy_get_hit_to_frame_20/30` | ✅ 通过（`ball_bdy/all` 11 行；14 条变异全杀） |
| 3 | `loader/get_val_*`（103 条 getter 表） | 待做 |
| 4 | `entity` + `collision` + `buff` + `state` + `controller` + `bot` + `World` | 待做（**必须整块搬**，见下） |

**步骤 2–4 是修正过的。** 把 `import type` 排除后算运行时依赖图，得到两个关键事实：

1. `utils/` 到根目录只有两条边：`cross_bounding.ts → ../defines`（仅类型，无害）和
   `MersenneTwister.ts → ../../cases_instances`（`mt_cases` **调试探针**，C++ 侧本来就不移植）；
   到 `base/` 只有一条边：`container_help/nested_map.ts → base/Graves`。
   ⇒ **补上 `Graves` 之后，`utils/` 就是一个完整、自洽的叶子。**
2. `defines/` 到根目录的 36 条边里 35 条都是 `→ ../fields`。
3. **`entity` / `collision` / `buff` / `state` / `controller` / `bot` / `(root)` 互相依赖，
   是一个约 1.3 万行的强连通块 —— 无法逐个搬，必须整块搬。**
   （之前把 `collision/` 当成步骤 2 是错的：它依赖 `buff` / `entity` / `World` / `LFW`。）

`utils/{schema,string_parser}` 属于数据装载路径，跟步骤 2/3 一起搬。

### 已知偏差（C++ 侧**没有**照搬的地方）

| 位置 | 偏差 | 原因 |
|---|---|---|
| `container_help/find.h` | 只实现了“按值找”。TS 版在可迭代对象未命中后还会退到 `for...in`，把 `[key, value]` 元组喂给谓词 | 对数值数组 + 数值谓词而言两条路径等价（元组参与数值比较恒为 false），但这个差异要记住 |
| `container_help/{traversal,get_keys,set_obj_field,take_number,assign}` + `foreach` 的对象分支 | 未移植 | 都依赖“JS 对象”这个动态类型，等 `Value`/`JsObject` 建好再搬 |
| `container_help/list_fn.h` | 不移植 | 整个 `src/LFW` 里唯一的真·反射，只有 `NoEmitCallbacks.add()` 一个调用点 ⇒ 改成显式注册表 |
| `math/mersenne_twister` | 少了 `mark` / `debugging` / `mt_cases` | 只是写 `Cases` 的调试探针，不影响输出与状态 |
| `ensure.h` | 接受了 `std::vector` 而不是变参 | 语义等价；**但两边都不能传空 items**（TS 的 `item` 是必填项） |
| `NestedMap` | 用 `std::map`（按键序），不是 JS `Map` 的插入序 | 它的公开 API 不暴露迭代 ⇒ **序不可观测**。若将来给它加迭代方法，必须先补插入序 |
| `NestedMap::ref()` | C++ 独有的新增方法（返回内层值的引用） | 让 `NestedMultiMap::add` 能 O(1)；不影响已有行为 |
| `NestedMultiMap` | 内部统一存 `std::vector<V>`（TS 是 `V \| V[]` 两态） | 公开 API（`add`/`first`/`has`/`collect`/`remove`/`clear`）上两者行为一致 |
| `Graves` / `NestedMap` | TS 的 `delete` → C++ 的 `remove` | `delete` 是 C++ 保留字 |
| `fields.cpp` 的 `to_array` | TS 里叫 `as_array` | 避开与 `core/value.h` 的 `as_array`（返回 `const Array*`）重名 |
| `fields.cpp` 用 `Object` 顶替 TS 的 `Map` | 字段描述表在 C++ 侧是 `Object` | 两边等价（Map 插入序 = 对象的 `Object.keys` 序，已由差分验证）；C++ 的 `Value` 没有 Map 种类 |
| `fields.cpp` 对三类“TS 会抛异常”的输入 | 直接返回 / 当空字段表 | 无异常可用；且（`field_map` 为 `null`、`object` 字段缺 `fields`、`field` 为 `undefined`）在 `defines/` 数据里不可达 |
| `lfw/defines/*.h`（43 个） | **由 `native/tools/gen_defines_enums.mjs` 生成，不要手改** | 枚举是纯数据。改完 TS 重跑生成器；差分 `defines` 会验 |
| `FacingFlag` / `HitFlag` / `EntityEnum` / `CMD` / `BinOp` | **手写**，不在生成范围内 | 前 4 个的成员引用了同文件常量 / 同枚举成员 / 别的枚举（生成器不求值）；`BinOp` 是早期手写的 `bin_op.h` |
| 生成的 `bdy_kind_name_of` 等 | 命名 `xxx_name_of`（TS 里叫 `bdy_kind_name`） | 避开将来手写同名函数 |
| `lfw/defines/fields_gen.h` | **由 `native/tools/gen_defines_fields.mjs` 生成，不要手改** | 它跑真实的 TS 字段表、内嵌成 JSON5，运行时用 `json5_parse` 还原 |
| `JSON.stringify` 会把 `-0` 写成 `0` | 生成字段表时用 | 当前 34 张表里没有 `-0`；若有，差分会在位模式上暴露 |

> `native.mjs all` 里的 `coverage` 步骤会跑 `tools/check_defines_coverage.mjs`：
> 它用「运行时枚举 TS 导出」这条**独立于生成器**的路径，检查 TS 里的枚举/字段表有没有漏搬。

**注意修正过的顺序**：`loader/preprocess_*.ts` 在运行时依赖 `dat_translator`
（`CondMaker` / `set_hit_flag` / `make_entity_special` / `xml_x_entity_data` /
`cook_ball_frame_state_*`），所以 `dat_translator` 里那块**必须和 `loader/` 一起搬**，
不能跳过。`ui/` 永远不要先搬。

| 3p | `dat_translator/cook_ball_frame_state_15/3000/3001/3005/3006`（+ 内部 `cook_shrink_and_bounce`） | ✅ 通过（`ball_frame_state/all` 21 行；变异 **22/22 全杀**） |
| 3q | `dat_translator/hit_next_frame_*`（8 个）+ `utils/container_help/assign` | ✅ 通过（`hit_next_frame/all` 15 行；变异 **26/26 全杀**） |
| 3r | `dat_translator/parase_indexes` + `match_hash_end` + `utils/string_help` | ✅ 通过（`parase_indexes/all` 18 行、`string_matchers/all` 48 行；变异 **19/19** + **15/15**） |
| 3s | `dat_translator/cook_frames` + `take_sections` | ✅ 通过（`cook_frames/all` 20 行；变异 **27/27 全杀**） |
| 3t | `dat_translator/make_ball_special` + `find_value_index` | ✅ 通过（`make_ball_special/all` 17 行；变异 **24/24 全杀**） |
| 3u | `dat_translator/make_weapon_special` + `broken_piece_frames` | ✅ 通过（`make_weapon_special/all` 42 行；变异 **29/29 全杀**） |
| 3v | `dat_translator/make_stage_info_list` | ✅ 通过（`make_stage_info_list/all` 22 行；变异 **25/25 全杀**） |
| 3w | `dat_translator/cook_file_variants` | ✅ 通过（`cook_file_variants/all` 14 行；变异 **13/13 全杀**） |
| 3x | `dat_translator/frame_editing` | ✅ 通过（`frame_editing/all` 24 行；变异 **13/13 全杀**） |
| 3y | `dat_translator/make_fighter_data` + `take_number` + `bots/frames` | ✅ 通过（`make_fighter_data/all` 38 行；变异 **37/37 全杀**） |
| 3z | `dat_translator/obj_dat_to_json` + `set_obj_field` | ✅ 通过（`obj_dat_to_json/all` 14 行；变异 **27/27 全杀**） |
| 4a | `dat_translator/make_buring_smoke` + `loader/preprocess_pic` + `loader/preprocess_stage` | ✅ 通过（`loader_helpers/all` 22 行；变异 **23/23 全杀**） |
| 4b | `loader/preprocess_ball_frame` + `loader/preprocess_bg_data` + `loader/resolve_prefab` | ✅ 通过（`loader_more/all` 25 行；变异 **29/29 全杀**） |
| 4c | `dat_translator/cook_frame_indicator_info` | ✅ 通过（`indicator_info/all` 16 行；变异 **22/22 全杀**） |
| 4d | `loader/preprocess_action` + `loader/preprocess_bot_data` + `loader/preprocess_next_frame` | ✅ 通过（`loader_actions/all` 68 行；变异 **33/33 全杀**） |
| 4e | `dat_translator/bots` 动作构建层（`constants` / `frames` / `bot_actions` 14 个构建器） | ✅ 通过（`bots_build/all` 53 行；变异 **50/50 全杀**） |

| 4f | `dat_translator/bots/BotMaker` + 前 5 个 `make_bot_data_*`（bat/hunter/jan/knight/monk） | ✅ 通过（`bots_data/all` 20 行；变异 **37/37 全杀**） |
