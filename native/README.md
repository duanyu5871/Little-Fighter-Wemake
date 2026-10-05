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
| 时间 | `<chrono>` `<ctime>` | 注入的 `IClock`（`base/clock.h`） |
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
      clock.h                        注入式 IClock（禁止 <chrono>）
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
    helper/
      manhattan_xz.{h,cpp}           ← helper/manhattan_xz.ts
      randoming.{h,cpp}              ← helper/Randoming.ts
    state/
      spawn_ice_piece.{h,cpp}        ← state/spawn_ice_piece.ts
    (待搬) utils/{container_help/traversal,get_keys,set_obj_field,take_number,assign,list_fn,
           schema,string_parser,type_check,type_cast}
           loader/ entity/ stage/ bot/ buff/ bg/ cmds/ controller/
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

### 增量构建为什么快（以及别把它弄慢）

- `native.mjs build [<subject>]`：带 subject 时只构建 `lfw_core` + `lfw_trace_<subject>`
  （不带 `--target` 时 `lfw_core.lib` 一变就要重链**全部**测试 exe，实测 46 个）。
- **VS 环境只捕获一次**：`call vcvars64.bat` 实测 **5400 ms/次**，之前每次 `build` 都重跑一遍。
  现在缓存到 `native/build/gen/vsenv.json`（带 `vcvars` 路径 + mtime 校验），
  之后 `spawnSync(cmake, …)` 直接用缓存好的 env，连 `cmd.exe` 都省了。
  删掉 `native/build/` 后会重新捕获，无需干预。
- `native.mjs test <subject> --reuse-ts`（变异测试用）：变异只改 `native/lfw/**`
  ⇒ TS 侧产物不可能变，直接复用 `native/build/gen/trace.<subject>.<case>.ts.txt`。
  缓存比用例文件或 subject `.ts` 旧时会**自动回退**去真跑 TS，所以手工改用例不会读到陈旧结果。
- `node native/tools/mutate.mjs <spec>` 现在会打印
  `总耗时 / ms per mutation / 最慢的一条`，一慢就看得见。
  ⚠️ 跑之前先确认 `native/build/mutate-backup/` **不存在** —— 它存在说明上一次跑被中断，
  源文件可能还是变异体（下次启动会自动恢复）。

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
| 2c | `defines/` 的**运行时数据**（`Defines` 命名空间 67 条 + 7 个顶层对象 + 5 个函数） | ✅ 通过（`defines_runtime/all` 149 行；9 条变异全杀）。`I*.ts` 结构体与 33 个 `*_new()` 只服务 `dat_translator`，运行时不用，暂缓 |
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
   `MersenneTwister.ts → ../../cases_instances`（`mt_cases` **调试探针**；9j 已把 `Cases` +
   `mt_cases` + `mark`/`debugging`/`case`/`pure`/`load` 一起搬进 `native/lfw/cases.{h,cpp}` 与
   `utils/math/mersenne_twister.{h,cpp}`，见 DESIGN §53）；
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
| `math/mersenne_twister` | `pure()` 的 `mt` 是 `uint32_t`，TS 里 `twist()` 之后是 signed int32 的**同一批位模式** | 位模式全等；差分里直接打印 `mt[i]` 的地方先 `>>> 0`（`state` 哈希一直如此） |
| `math/mersenne_twister` | 不保留 `range(min, max, debugging)` 的**第三参** | TS 里是死参数（函数体只看 `this.debugging`） |
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
| `Entity::_team` 是 `std::u16string` | TS 的 `team` 保持原值（可能是数字）⇒ 数字队伍在端口里被字符串化（TS 存 `3`，端口存 `"3"`） | 队伍在真实数据里一直是字符串（`lfw.new_team` / 帧数据），端口内部也全程按字符串比较；只有「碰撞攻击者带数字队伍」这种人造输入会撞上，用例只喂字符串（9l） |

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

| 4g | `dat_translator/bots` 角色数据第二批（davis/jack/justin/louis/mark/sorcerer，累计 11/22） | ✅ 通过（`bots_data/all` 42 行；变异 **87/87 全杀**） |

| 4h | `dat_translator/bots` 角色数据第三批（firen/firzen/henry/julian/louisex/woody，累计 17/22） | ✅ 通过（`bots_data/all` 64 行；变异 **130/130 全杀**） |

| 4i | `dat_translator/bots` 角色数据第四批（deep/dennis/freeze/john/rudolf，**22/22 完成**） | ✅ 通过（`bots_data/all` 82 行；变异 **174/174 全杀**） |

| 4j | `dat_translator/fighters` 角色装配（23 个 + `make_fighter_special`） | ✅ 通过（`fighters_special/all` 93 行；变异 **133/133 全杀**） |

| 4k | `dat_translator` 收尾（`float_scaling_entity` / `decode_lf2_dat` / `edit_info`） | ✅ 通过（`translator_tail/all` 63 行；变异 **35/35 全杀**） |

| 4l | 步骤 4 第一块 `entity/` 纯助手（`calc_v`/`find_direction`/`face_helper`/`type_check`/`EntitySnapshot`） | ✅ 通过（`entity_helpers/all` 153 行；变异 **62/62 全杀**） |

| 4m | 步骤 4 第二块 `controller/` 纯助手（`DoubleClick`/`SeqKeys`/`KeyStatus`/`ControllerDoubleClicks`） | ✅ 通过（`controller_helpers/all` 102 行；变异 **55/55 全杀**） |

| 4n | 步骤 4 第三块 `bot/` 纯助手（`closest`/`is_ray_hit`/`DummyEnum`/`NearestTargets`）+ 补 `helper/manhattan_xz` | ✅ 通过（`bot_helpers/all` 201 行；变异 **99/99 全杀**） |

| 4o | 步骤 4 第四块 `collision/` 纯助手（`is_fall`/`is_armor_work`/`calc_itr_velocity`）+ 补 `Entity.dataset` | ✅ 通过（`collision_helpers/all` 243 行；变异 **99/99 全杀**） |

| 4p | 步骤 4 第五块 `entity/Summary` + `SummaryMgr`（+ `js_add` / `is_independent`） | ✅ 通过（`summary_helpers/all` 146 行；变异 **57/57 全杀**） |

| 4q | 步骤 4 第六块 `entity/DrinkInfo` + `collision/handle_stiffness`（`calc_stiffness`） | ✅ 通过（`drink_stiffness/all` 63 行；变异 **44/44 全杀**） |

| 4r | 步骤 4 第七块 `helper/Randoming` + `state/spawn_ice_piece`（+ 注入式 `IClock`） | ✅ 通过（`mt_random/all` 143 行；变异 **55/55 全杀**） |

| 4s | 覆盖审计 + `base/Expression` 加固（用例 187→928 行；+ 构建链提速） | ✅ 通过（`expression` 4 个用例 928 行；变异 **69/69 全杀**） |

| 4t | 覆盖审计（二）：`utils/math/MersenneTwister` 加固（用例 42→71 行；`pick`/`take` 增打剩余数组） | ✅ 通过（`mt_basic` 7614 行；变异 **49/49 全杀**） |

| 4u | 覆盖审计（三）：`utils/math/` 全家族加固（`clamp`/`clamp_add`/`normalize`/`float_equal`/`range`/`probability`/平面几何） | ✅ 通过（`math/scalar` 95 行 + `math/plane` 59 行；变异 **61/61 全杀**） |

| 4v | 覆盖审计（四）：`base/Value` + `Object`/`Array` 加固（用例 187→489 行；`odel` 增打删除结果） | ✅ 通过（`value` 5 个用例 489 行；变异 **71/71 全杀**） |
| 4w | 覆盖审计（五）：`utils/` 全家族加固（`type_check`/`type_cast` 首次纳入差分；`ease_*`/`Times` 改为按实参个数分派以暴露默认实参） | ✅ 通过（`utils` 5 个用例 321 行；变异 **108/108 全杀**） |
| 4x | 覆盖审计（六）：`core/json` 加固（`parse` 94→161 行、`stringify` 69→89 行） | ✅ 通过（`json` 2 个用例 250 行；变异 **76/76 全杀**） |

| 4y | 覆盖审计（七）：`collections` 全家族加固（`graves` / `array/*` / `container_help/*`；1 实参 `fisrt`/`last` 首次纳入、`map_arr`/`loop_arr` 回调补下标与数组实参、新增 `ensure` 的 `Value` 重载覆盖） | ✅ 通过（`collections` 2 个用例 194 行；变异 **68/68 全杀**） |

| 4z | 覆盖审计（八）：`core` 加固（`js_num` / `js_string`；空白常量表补 `\v`/`\f`、`Math.round` 大奇数修正、`parse_radix` 的 >64 位舍入路径） | ✅ 通过（`core` 4 个用例 10969 行；变异 **112/112 全杀**） |

| 5a | 步骤 5 第一片 `Transform`（位置/缩放/旋转补间；新增 `native/tools/ts_scope.mjs` 用于定切片顺序） | ✅ 通过（`transform` 4 个用例 339 行；变异 **45/45 全杀**） |

| 5b | 步骤 5 第二片 `Ground`（地形高度/阻挡/地面碰撞几何；顺带落地 `ITerrainInfo`） | ✅ 通过（`ground` 3 个用例 310 行；变异 **58/58 全杀**） |

| 5c | 步骤 5 第三片之一：`ControllerKeyStatus` / `ControllerResult` + `GameKey` 三张表（`GKLabels`/`AGK`/`CONFLICTS_KEY_MAP`；`fire` 守卫改为 JS 真值判定） | ✅ 通过（`controller_input` 104 行；变异 **72/72 全杀**） |
| 5d | 步骤 5 第三片之二：`BaseController` 主控全链（相对方向 / 单击 / 双击 / 顺序与同拍 / 世界动作；`CtrlEnv` 输入缝；`RL/DU/dj` 保留负零） | ✅ 通过（`base_controller` 866 行；变异 **133/133 全杀**） |

| 6a | 步骤 6 之叶子：`collision` 处理器（`stiffness` / `body_goto` / `super_punch_me` / `weapon_picked` / `rest` / `itr_kind_magic_flute`；`HandlersEnv` 行为缝 + 调用序列差分；保住 `??` 与 `\|\|` 的惰性） | ✅ 通过（`collision_handlers` 60 行；变异 **30/30 全杀**） |

| 4a | 步骤 4 主体：`Buff` 基类 + `grant_buff`（`Times` 三重计时 / 受害者表压缩 / 特效实体跟随 / 快照往返；`IBuffEntity`+`BuffEnv` 行为缝；修 `show_effect` 同键覆盖 bug） | ✅ 通过（`buff` 139 行；变异 **61/61 全杀**） |

| 6b | 切片 6 核心：`Collision`（碰撞对象装配 / 判定链 / 快照往返 / 副本；`CollisionActor`+`CollisionCoreEnv` 缝；保留 `from_snapshot` 取 `aframe.bdy` 怪癖；修 `index_by` 不支持数组下标、`dataset` 误写两个真 bug） | ✅ 通过（`collision_core` 258 行；变异 **72/72 全杀**） |

| 6c | 切片 6 核心：动作表 `collision_action_handlers`（21 种动作分发 + `VALUE_STEAL` 全分支 + `FUSION` 合体 + `apply_buff` 两级判定；`IActionEntity`+`ActionEnv` 缝；复刻 `play_sound` 默认位置与赋值链右到左求值） | ✅ 通过（`action_handlers` 110 行；变异 **70/70 全杀**） |

| 6d | 切片 6 核心：`handlers2`（受伤/抓取/冰冻(含 effect 版)/护盾 5 个 handler；`IHandlerEntity`+模块级 `Handlers2Env` 缝；复刻 JS 参数默认值、`is_fighter` 按实体、`calc_itr_velocity` 注入） | ✅ 通过（`collision_handlers2` 63 行；变异 **49/49 全杀**） |

| 6e | 切片 6 核心：`handlers3`（旋风 / 球受击 a·b / 护甲；`IH3Entity` + 模块级 `Handlers3Env` 缝；新增 `collision_defaults.h`；区分 `>` 与 `>=` 两种临界值比较、`hp_r` 先于 `hp` 赋值、`missing()` 只认 `undefined`） | ✅ 通过（`collision_handlers3` 267 行；变异 **78/78 全杀**） |

| 6f | 切片 6 核心：`handlers4`（球击中他人 / 武器击中他人；`IH4Entity` + 模块级 `Handlers4Env` 缝；保住 `hp_r` 先于 `hp`、`-0.3 * vx` 的负零、`arest` 先快照后回写，并记录 `truthy(bdefend)` 前缀与 `Weapon_OnHand` 前置守卫两处不可观测冗余） | ✅ 通过（`collision_handlers4` 84 行；变异 **81/81 全杀**） |

| 6g | 切片 6 核心：`weapon_is_hit`（武器被命中；`IWeaponIsHitEntity : IHandlerEntity` + 模块级 `WeaponIsHitEnv` 缝；`calc_itr_velocity` 走注入缝并复刻 `x_direction = facing`；保住 `set_velocity` 的分量跳过与 y>0 自动离地、`indexes?.throwings?.[0]` 的数组/对象/缺失三分支；修正临界值 100 不是 140） | ✅ 通过（`collision_weapon_is_hit` 115 行；变异 **85/85 全杀**） |

| 6h | 切片 6 核心：`fall`（倒地；`IFallEntity : IHandlerEntity` + 模块级 `FallEnv` 缝；`turn_face` 与直传 `x_direction` 两条朝向路径、`velocity.x / facing` 的 `NaN` 与 `±Infinity` 语义、`critical_hit` 对象/数组双形态与 `index_0` 三态取值；记录 `critical_hit` 缺键时 TS 抛 `TypeError` 这一有意分歧） | ✅ 通过（`collision_fall` 113 行；变异 **84/84 全杀**） |

| 6i | 切片 6 核心：`n_bdy_normal`（普通 bdy 受击；`INbdyNormalEntity : IFallEntity` + 模块级 `NbdyNormalEnv` 缝；子处理器直接调真函数；保住 `==` 与 `===` 之别、三分组各自的调用顺序、`switch` 的 `case void 0`、`id.length` 三态、`a(r)` 的 `floor+fmod`，并记录「非 Fighter 到不了 SilentHit」「字符串长度为 0 不可达」两处不可观测） | ✅ 通过（`collision_n_bdy_normal` 175 行；变异 **100/100 全杀**） |

| 6j | 切片 6 核心：`n_bdy_defend`（bdy 防御；`INdbdyDefendEntity : INbdyNormalEntity` + 模块级 `NbdDefendEnv`，action 分发走 `dispatch` 缝；保住德摩根三段守卫、爆炸类无视朝向、`bdefend` 默认 32、`defend_ratio` 的 `??` 取源、`if (vx)` 的 0/-0/NaN、破防与未破防两条分支对 `bdy.actions` 的不同白名单） | ✅ 通过（`collision_n_bdy_defend` 110 行；变异 **65/65 全杀**） |

| 6k | 切片 6 核心：`ball_frozen`（冻结飞球；`IFrozenEntity` + `BallFrozenEnv{is_ball,is_fighter}`，模块级可变 opoint 复刻；保住宽松比较的 kind 白名单、分组交换与双向守卫、三条 break 的短路或、`cx1`/`cx2` 的四个分支、`x` 独享 facing 符号与 `round` 位置、`spawn` falsy 时跳过 `enter_frame`） | ✅ 通过（`collision_ball_frozen` 151 行；变异 **89/89 全杀**） |

| 6l | 切片 6 核心：`healing`（治疗 buff；`IHealingEntity{id,dataset,buff_entity}` + `HealingEnv`，`grant_buff` 用真实现；保住 `!itr.injury` 早退、`Math.max` 的 NaN 传播、`ceil(injury / max(1,value)) * max(1,ticks)` 的算子位置、`kind + "_" + victim.id` 的 buff id） | ✅ 通过（`collision_healing` 55 行；变异 **22/22 全杀**） |

| 6m | 切片 6 核心：`keeper`（碰撞处理表注册半边；`CollisionKeeper::add/register/load_handlers` + `HANDLER_CONFIGS` 19 条 + `collisions_keeper` 单例；保住 `pack_a/b` 组合键、`ALL_STATES` 身份哨兵、四个 `is_u*` 守卫、`handlers` 先清空、表格顺序即处理顺序、`handle_body_goto` 的声明名） | ✅ 通过（`collision_keeper` 108 行；变异 **45/45 全杀**）｜`handle()` 待后续单元 |

| 6n | 切片 6 核心：`keeper` 的 `handle()`（调度半边；`CollisionKeeper::handle` + `KeeperEnv` 六个新缝 `call_handler`/`ball_frozen`/`run_action`/两处 push/`victim_play_sound`；保住 `handle_ball_frozen(v,a,itr)` 的实参顺序、`itr_tests/bdy_tests` 的**预计算时机**、`pretest` 真假两条取值路径、严格 `=== false` 才跳过、六个静音 kind 的严格比较与 `data.base.hit_sounds` 两层取值；`ball_hit` 照抄死代码） | ✅ 通过（`collision_keeper_handle` 77 行；变异 **48/48 全杀**） |

| 6o | 切片 4 首个：`Buff_GroupAttack` / `Buff_Electrify`（`buff` 基类可继承化：`place_effect`/`mount`/`unmount`/`init` 转虚，`IBuffEntity` 增补能力一律用**带默认实现的虚函数**以免动 10 个已门禁 harness；保住 `KIND` 作标记键、`effect_oid/frame_id`、`place_effect`→居中、`mount` 转调基类、`unmount` 用 `del_mark` 的**条件删除**） | ✅ 通过（`buff_marks` 41 行；变异 **30/30 全杀**） |

| 6p | 切片 4 第二组：`Buff_Healing` / `Buff_MpHealing`（静态 `duration_of` 的两个 `max(1,·)` 位置、`mount` 打标记并逐受害者取 tick 间隔、`has_on_tick()` 必须置真、`on_tick` 的回血/回蓝与 `hp_r`/`mp_max` 钳制、`MpHealing` **没有**上限守卫（TS 原文即注释掉）、`unmount` 的条件删除） | ✅ 通过（`buff_healing` 52 行；变异 **42/42 全杀**） |

| 6q | 切片 4 第三组：`Buff_Electroshock`（`init` 设 `ticks=3`、`effect_oid="fx"` 而 `effect_frame_id` **不覆写**（承基类 `"0"`）、`place_effect` 转调居中版、`mount` 用**宽松** `==` 跳过 Injured/Falling 并把时长 `round_float(duration()/2)`、`on_tick` 用**严格** `===` 跳过 Falling/Lying 后把 `wait` 加一、非战士直接返回） | ✅ 通过（`buff_electroshock` 60 行；变异 **22/22 全杀**） |

| 6r | 切片 4 第四组：`Buff_MagicFlute` / `Buff_MagicFlute2`（`init` 设 `ticks=duration=3`、`on_tick` 先取 `prev_hp` 再扣 `hp_r -= 0.5` / `hp -= injury(2/1)`、写 `fallinjury=20`、`toughness=0` 并报 `summary_mgr.apply_damage`、`on_update` 用 `calc_v(y, acc, AccTo, acc, 1)`（`3` / `1.5`）加速度、`set_velocity(null, vy)`、`handle_velocity_decay(0.25)`，随后按**严格** type 分支：Fighter 非 Falling 进 `falling[-1][0]`、Weapon 非两个天空态则继队友阵营并进 `in_the_skys[0]`） | ✅ 通过（`buff_magic_flute` 82 行；变异 **58/58 全杀**） |

| 6s | 切片 5 起点：`State_Base`（状态基类；`IStateEntity : IBuffEntity` 只加 `velocity_x/z` + 模块级 `StateEnv{buff_env}`；可选钩子 `pre_update`/`enter`/`on_dead`/`on_landing`/`get_gravity`/`get_sudden_death_frame`/`get_caught_end_frame`/`get_auto_frame`/`find_frame_by_id`/`on_leave_ground` 用**空 `std::function` 成员**复刻 `?.` 的存在性判断；保住 `leave` 的严格 `HealSelf` 与 `grant_buff`+`duration_of(104)`、`on_restrict` 的 `MIN_V=0.5` / 移动 y 同时刷 x 与 z / **`!== null`（`undefined` 仍写回）**/ `clamp` 对非数原样返回 / 位置按字段写回） | ✅ 通过（`state_base` 46 行；变异 **37/37 全杀**） |

| 6t | 切片 5：`CharacterState_Base`（`IStateEntity` 补 11 个带默认实现的虚函数；**五个可选钩子在构造函数里赋值**而非覆写虚函数；`update` → `handle_ground_velocity_decay`；`on_landing` 优先帧自带 `on_landing` 否则退 `indexes.landing_2`；`get_auto_frame` 的 Heavy/on_ground/hp>0 三段优先级；`get_sudden_death_frame` 先 `set_velocity(2*facing, 2)` 再取 `falling[1][1]`；`get_caught_end_frame` 用 `-cvx_d*facing` 且 y 直接取 `cvy_d` 并取 `falling[-1][1]`；`on_leave_ground` 四个状态的严格比较 + Heavy 掉落 + `NEXT_FRAME_AUTO`） | ✅ 通过（`character_state_base` 68 行；变异 **50/50 全杀**） |

| 6u | 切片 5：`CharacterState_Standing` / `_Running` / `_Injured`（`IStateEntity` 补 `ground_y` / 实体侧 `get_sudden_death_frame()` / `holding_set_team`；`Standing` 的 `hp<=0` 早退与 `position.y > ground_y` 进天空帧；`Running` 的 `if(vz)` + `dz=abs(vz/4)` 双向拖拽 + 只写 x + 无 else 的骤死帧；`Injured` 的**严格** Heavy 判定、先 `drop_holding()` 再 `holding.team = e.team`；`super.enter?.()` 是死代码照抄为空） | ✅ 通过（`character_state_basic` 82 行；变异 **37/37 全杀**） |

| 6v | 切片 5：`CharacterState_Walking`（`IStateEntity` 补 `frame_info` / `ctrl_ud` / `ctrl_lr` / `holding_is_weapon` / `handle_wait_flag` / `enter_frame_by_id_fallback`（**新开带默认实现的虚函数转发给一参数版本，绝不动已有签名**）；保住空闲判定的三条 `&&` 与 `wait` 真值判定、`is_weapon(holding)` 与 `base_type === Heavy` 两个独立条件、`handle_wait_flag(void 0, e.frame)` 实参顺序与写回、`enter_frame_by_id(key, **true**)`、`hp<=0` 的 `return` 与 `position.y > ground_y` 的 else-if 结构） | ✅ 通过（`character_state_walking` 52 行；变异 **34/34 全杀**） |

| 6w | 切片 5：`CharacterState_Caught` / `_Rowing`（`Caught.update` **不调** `super.update` 因而没有地面速度衰减；`Caught.enter` 的 `fall_value = fall_value_max` + 清零速度 + 「掉落看有无持有物、改阵营看是否重型」两个不同守卫；`Rowing.on_landing` 覆盖基类钩子并回落 `landing_1`；`Rowing.enter` 的严格 `prev_frame.state !== Falling`、四个 `rowing_*` dataset 相乘、`velocity.x >= 0` 决定翻滚方向、`calc_v(…, Default, 0)` 四个实参） | ✅ 通过（`character_state_caught_rowing` 59 行；变异 **39/39 全杀**） |

| 6x | 切片 5：五个小状态 `state_misc`（`State_WeaponBroken` 的裸 `enter_frame(GONE_FRAME_INFO)`；`State_TransformToCatching` / `CharacterState_TransformToLouisEX` 都不调 `super`；`State_TransformTo8XXX.leave` 的 `typeof state !== "number"` 守卫、`"" + (state - 8000)` 取数据 id、`old_data` 需在 transform 前取、两处严格类型比较与 `on_fighter_add` 回调；`BallState_Base` 的四个球状态严格判否后清 `shaking`/`motionless`/速度） | ✅ 通过（`state_misc` 58 行；变异 **42/42 全杀**） |

| 6y | 切片 5：`CharacterState_Dash`（`ctrl_ud`/`ctrl_lr` 由 `bool` 改 `double`（`Walking` 文本零改动、门禁复跑 34/34）；`enter` 的 `position.y > ground_y && velocity.y !== 0` 双条件早退；六个 `dash_*` dataset 按序拆读再相乘；`next_vz` 保留初值而 `next_vx` 的初值是死代码；x 方向五路选择（Running→`facing*dx`、LR→`LR*dx`、`vx>0`→`dx`、`vx<0`→`-dx`、兜底→`facing*dx`）） | ✅ 通过（`character_state_dash` 63 行；变异 **42/42 全杀**） |

| 6z | 切片 5：`CharacterState_Burning`（**`enter` 里调的是 `super.update`**（TS 原文怪异之处，端口用捕获 `this` 的 lambda 复刻）；`update` 按 x 速度**反向**转身且 `0` 不转；`leave` 清 `bounced`；`on_landing` 的 `!bounced && (vy <= 阈值y || abs(vx) > 阈值x)` 短路（决定了 `dataset` 读几条）、`bouncing[-1][1]` 与 `lying[-1]` 两种索引形状、`set_velocity(null, spd)` 只写 y） | ✅ 通过（`character_state_burning` 51 行；变异 **37/37 全杀**） |

| 7a | 切片 5：`CharacterState_Teleport2NearestEnemy` / `_Teleport2FarthestAlly`（两个 TS 类镜像，端口合并成 `pick_target` + `teleport_to` 两个自由函数；`IStateEntity` 补 9 个缝，其中 `is_*_ref` / `ref_*` 对应 TS 属性读故**静默**，`ground_segment` / `ground_y` 是方法调用故**打日志**；原样保留 `abs(o.position.z - o.position.z)` 这条恒 0 的 TS bug；不对称谓词 `nearest ? (best < 0 \| dis < best) : (dis > best)`；`x` 带 `facing * 120` 偏移而 `z` 不带；无目标时保留自身 `x`/`z` 但 `y` 仍取自 `ground.y`） | ✅ 通过（`character_state_teleport` 106 行；变异 **61/61 全杀**） |

| 7b | 切片 5：`WeaponState_Base` + `defines/weapon_bounce.h`（新增手写常量表对应 `Defines.WT_*` 九张表，用「下标越界即缺失」复刻 JS 对象取键；三个钩子进构造函数、`update` **不调** `super.update`；`get_auto_frame` 的 `if (!indexes)` 是真守卫（靠 `has_data_indexes()` 缝）；`hit_ground_rebouncing` 的三级 `??` 兜底、`drop_hurted` 两层守卫、`hp`/`hp_r` 属性写故静默、`e.state == SE.Weapon_Throwing` 用宽松 `equals`、`const nf` 遮蔽参数、以及 **`dvx >= bounce_min_z \|\| dvx < -bounce_min_z` 这条原文笔误**） | ✅ 通过（`weapon_state_base` 163 行；变异 **73/73 全杀**） |

| 7c | 切片 5：`StateBase_Proxy` + `State_15` + `State_Frozen`（**`leave`/`update`/`on_restrict` 是虚函数而非可选钩子**（首次编译即撞 `C3867`）；四个代理按值持有且基类初始化列表**不能 move**（成员会拿到被掏空的值）；枚举判定用 `is_*_data`（`is_fighter(v)` 读的是 `v.data`）；`State_Frozen.enter` 包装父钩子（`has_catcher` / 宽松 `== Heavy` / 065 音效），`leave` 覆写并**调父两次**（⇒ HealSelf 授予两次 buff）、`on_landing` **不调 super** | ✅ 通过（`state_base_proxy` 113 行；变异 **55/55 全杀**） |

| 7d | 切片 5：四个武器状态 `WeaponState_OnGround` / `_OnHand` / `_Throwing` / `_InTheSky`（`weapon_state_misc` 一个单元；`IStateEntity` 补 9 个缝；保住 `e.lfw.new_team` 全量赋值、`Math.max` 的 NaN/±0、`== Boomerang` 宽松比较、`velocity.x * 0.6` 单实参且不取整、`\|\|` 链取第一真值、`hit_ground_rebouncing` 与 `InTheSky.update` **两处相反的实参顺序**、`base ?? 表 ?? 99` 三层兜底与 Heavy 守卫、六比较的排他边界；记录 `fast_y`/`fast_z` 中间表项不可观测、`enter_frame_by_id(undefined)` 渲染差异为有意不覆盖） | ✅ 通过（`weapon_state_misc` 203 行；变异 **61/61 全杀**） |

| 7e | 切片 5：`CharacterState_Drink` + `State_Burning`（`StateBase_Proxy` **补 character 代理注入构造**（成员改 `unique_ptr` 以保多态）；`IStateEntity` 补 `hp_max` / `holding_drink` / `holding_*` 五缝；Drink 的三段 tick 门控、`min(hp_max, …)` 钳制（`hp_r` 也用 `hp_max`，照抄）、三空才掉罐子 + `mt.range(-6,6)/2`；Burning 走代理分派：Fighter→`CharacterState_Burning`、Weapon/Ball→对应基类、其它→裸 `State_Base`；记录 `holding.hp = hp_r = 1` 的静默赋值链与 `mt.mark` 调试探针不移植） | ✅ 通过（`burning_drink` 149 行；变异 **38/38 全杀**） |

| 7f | 切片 5：`CharacterState_Jump`（`IStateEntity` 补 12 缝：`ctrl_is_bot` / `ctrl_is_end` / `jumping_*` 读+写 8 缝 / `prev_frame` / `update_velocity`；`enter` 只清四个 `jumping` 字段、`on_landing` 帧优先否则 `landing_1` + `update_velocity({dvz:4,ctrl_z:Control})`；`update` 的「无条件衰减 → `float_equal(position.y, ground_y)` 早退 → 机器人/人类两条路计步（`round_float` 每次都用）→ `jump_flag` 宽松真值 → 六条 dataset 逐行读取 → `min=4` 插值」；**记录原文脆弱点：`atom_time > 0` 时插值处的 `jumping.t` 恒为真**，`else` 只有 `atom_time = 0` 才可达，故用例专门保留该档） | ✅ 通过（`character_state_jump` 103 行；变异 **48/48 全杀**） |

| 7g | 切片 5：`CharacterState_Falling`（`IStateEntity` 补 13 缝：`ctrl_reset_key_list` / `data_id` / `shaking` / **`handle_ground_velocity_decay(factor)` 重载** / `fuse_bys` / `ref_set_velocity` / `dismiss_fusion` / `defend_value_max` + `set_defend_value` / `resting_max` + `set_resting` / `set_throwinjury` / `data_indexes_critical_hit`；**首个带实例状态的单元**：`_bouncing_frames_map`（dataId→Set\<frameId\>）跨实体缓存，`enter` 只在「没缓存过」且「`indexes.bouncing` 为真」时建表（短路顺序保真）；融合 `next_vx *= -1` 逐个写回（`vx = 0` 时与原文一样得 `-0`）；落地 `frame.on_landing` 优先，否则 `find_direction` 三级 `\|\|` 兜底 + `!bounced && (vy <= y阈值 \|\| abs(vx) > x阈值)`；**记录「Set 展开顺序不可观测」「`x/facing`、`x*facing`、`facing/x` 符号恒同故原理上不可杀」「`super.leave` 的 HealSelf 分支归 `state_base`」**） | ✅ 通过（`character_state_falling` 173 行；变异 **77/77 全杀**） |

| 7h | 切片 5：`CharacterState_Lying`（**最后一个 character state**；`IStateEntity` 补 22 缝：三个 `lying_*_count` 读+写、`toughness_max` + `set_toughness_resting`、`set_hp_max`、`dead_join` 读+写 + `dead_gone`、`reserve` 读+写、`wakeup_invuln` 读+写、`set_invulnerable`、`set_blinking`、`blink_and_respawn` / `blink_and_gone`、`world_puppets`、`world_etc`；`on_dead` / `find_frame_by_id` 是 `std::function` 钩子（**不是虚函数，写 `override` 会 C3668 编译失败**）；`enter` 的持有物三段、`on_dead` 的傀儡队伍 `Set` 判定 + `reserve` 递减 + 三段排他分支、`update` 的「攻击分支先 `return`」双段计数器（`count_a + 1` 用 `js_add` 保字符串拼接、`% 2` 用 `std::fmod` 保负奇数/小数为真）、`leave` 的 `hp = hp_r = hp_max = dead_join.hp ?? hp_max` 赋值链与 `??`（`0` 会胜出）、**记录「`set_invulnerable(30)` 必被 wakeup 块覆盖故不可观测」与「`pressing_d` 场景必须让另一键计数为偶数，否则被提前 `return` 短路」的实测教训**） | ✅ 通过（`character_state_lying` 235 行；变异 **84/84 全杀**） |

| 8a | 切片 8：状态注册表 `States` + `ENTITY_STATES` 装配（`states.{h,cpp}` + `entity_states.{h,cpp}` + **`state_names.h`**：RTTI 关掉（`/GR-`）故 `typeid` 不可用，用主模板不定义的 `state_name_of<T>` 显式表 + `state_name_u16` 逐字节加宽来复刻 `constructor.name`；`add<T>` 用 `value.state` 当键、`make<T>(key,…)` 显式键（`fallback` 的缓存键是**字符串** `` `${type}_${code}` `` 而非数值）；**保住 JS `Map` 的键语义**「数字键 ≠ 字符串键」与「重设已有键只换值不挪位置」；闭区间 `set_in_range`、`switch` 严格比较的四路 `fallback`（命中返回**同一实例**）；装配结果 999 + 7 + 34 = **1040** 条） | ✅ 通过（`entity_states` 1127 行；变异 **49/49 全杀**） |

| 8b | 切片 8：`WorldDataset` 全局调参表（`world_dataset.{h,cpp}`；**建模成有序键表**而不是 103 个 C++ 字段，因为已移植调用点全部按键取值；103 条默认值按 TS 声明顺序抄写，`screen_w/screen_h/sync_render/difficulty` 用 `Defines`/枚举实值 794/450/3/3，三个 `[CheatEnum.*]` 键是 `GIM_INK`/`HERO_FT`/`LF2_NET`；复刻 `make_private_properties` 语义：托管键 = `!_pure && 键 ∈ world_dataset_fields`，`strict_equals` 同值**完全不动作**（`0` 与 `-0` 等价），否则先 `on_<键>_change` 再 `on_dataset_change`，两条都拿 `(curr, prev)`；`pure` 分支整体跳过安装 → 字段可枚举、无通知、**没有** `__is_world_dataset__`；标记键在 own 属性顺序**最后**且**不在**表里；`set` 区分「键不存在」（追加、不发通知）与「存在但未托管」（存值、不发通知）；`dump_dataset` 按 UTF-16 码元排序遍历表；`DEFAULT` 懒加载单例跨调用可写；**记录「`_$_` 备份属性与 `on_<键>_change` 槽属于 `make_private_properties` 实现细节，harness 在 TS 侧归一化 `Object.keys`」「因为 103 个默认值全在表里，省略表归属判断的变异原理上不可杀」**） | ✅ 通过（`world_dataset` 65 行；变异 **55/55 全杀**） |

| 9a | 切片 9：`Entity` 的构造 / `reset` + 数值通知层（**强连通块的第一刀**；`entity.{h,cpp}`，宿主服务注入成 `IEntityHost`（`world.dataset` / `bg.data.dataset` / `mark_players_alive` / `lfw.new_id` / `new_team` / `factory.acquire_ctrl|release_ctrl` / `enter_frame` / `apply_opoints` / `play_sound`，每个虚函数注明镜像的 TS 表达式）；`_data`/`frame`/`armor`/`dead_join` 等仍是 `Value`；`_state` 先退化成两个可选钩子；**分清 `??` 与 `||`**（`_x ?? dataset.x` 只有 nullish 回退，`armor \|\| null`、`player.name \|\| \`Player ${id}\``、`variant = Number(team) \|\| 0` 是真值语义）；严格相等才静默（`0`/`-0` 等价）；**`fall_value`/`defend_value` 用未取整入参比较、取整后存储**（`20.00049` 会「值不变但仍通知」）；下降时先抬 `resting`/`toughness_resting` 再发自己的通知；`hp` 死亡分支的短路守卫（`state !== Gone` / `frame.id !== "gone"` / `brokens?.length`）+ `frame.on_dead ?? data.on_dead`；`mp` 耗竭分支 + `summary_mgr` 的 `hp_lost`/`mp_usage` 累加与队伍一份；控制器「值 + 真对象」双层（`is_human_ctrl` 读标记、`gravity` 调 `ctrl.is_end`）；`name` 返回 `Value`（区分 `undefined`/`null`/`""`）；`reset_armor` 的 `toughness = toughness_max = …` 赋值链顺序；新增 `stat_slots()` 窥视口给 harness/宿主读私有槽位） | ✅ 通过（`entity` 319 行；变异 **115/115 全杀**） |
| 9b | 切片 9：`Entity` 的速度 / 摩擦 / 重力层（**强连通块的第二刀**；`get dvx/dvy/dvz`、`set_velocity`、`leave_ground`、`handle_ground_velocity_decay`、`handle_velocity_decay`、`handle_gravity`、`update_velocity`；`set_velocity` 收 `Value` 才能同时表达「`null`/`undefined` 跳过」与「`NaN` 写入」；`dvx/dvy/dvz` 的「假值原样返回、真值乘 `fv*_f`」（数据集缺失 → `NaN`）；速度衰减的**双钳位**（`x = dvx` 会把 `null` 写回该轴不动、`x = -dvx` 写数值 `-0`）；`ctrl_x && !LR` 才清零目标速度；落地区分靠**对象同一性** `_landing_frame === frame`；`gravity_enabled = true` 只吃 `undefined`（`null` 算假）；`acc == void 0` 的宽松默认（`null` 也吃、`0` 不吃）与 `vxm == AccTo` 的宽松相等（`"4"` 同样命中）；`LR|UD|jd` 四路分派 + `Enable`/`Disable` 一律方向 1；`update_velocity` 只写 `velocity`（不碰 `prev_velocity`、不 `leave_ground`）；`atom_time` 缩放重力 / `pow(f, atom_time)` / `acc_*` 三处） | ✅ 通过（`entity` 763 行；变异 **207/207 全杀**） |
| 9c | 切片 9：`Entity` 的帧查找 / 标志处理（**强连通块的第三刀**；`find_frame_by_id` / `find_auto_frame` / `find_align_frame` / `get_prev_frame` / `get_sudden_death_frame` / `get_caught_end_frame` / `handle_facing_flag` / `handle_wait_flag` / `get_frame_wait`；`switch (id)` 是**严格比较**，所以 `null` 不落 `case void 0` 而是查 `frames["null"]`（`Ditto.warn` 无副作用，端口不调用）；状态钩子返回值走 `truthy`（`0`/`""` 穿透），`find_auto_frame` 是 `??` 链而 `get_sudden_death_frame`/`get_caught_end_frame` 是 `\|\|`（假值也回退 `NEXT_FRAME_AUTO`）；`find_align_frame` 的 `(indexOf + 1) % len`（未命中 → `dst[0]`）；`get_caught_end_frame` 就地抬 `position.y = ground_y + 1` **不取整**；`handle_facing_flag` 的 12 支 + default，`FacingFlag` 里 `SameAsBearer == SameAsCatcher == 4`、`OpposingBearer == OpposingCatcher == 5` 所以 **bearer 两支是不可达死代码**（catcher 支在前）；`Trend` 的 `const { LR } = this.ctrl` 无可选链；`handle_wait_flag` 四级判定（`wait == void 0 && frame` 宽松比较 / `is_positive` 只认正数 / `"i" \|\| !frame` / `"d"` 的差值式），「存在但假值」的帧算没有帧；`get_frame_wait` = `frame.wait + wait_offset`，`_from_wait_block` 时再减 `_atom_time`；差分补齐「`echo` 钩子」锁钩子入参） | ✅ 通过（`entity` 977 行；变异 **265/265 全杀**） |
| 9d | 切片 9：`Entity` 的快照读写层（**强连通块的第四刀**；`to_snapshot` / `read_snapshot` + `copies` + 两个新宿主缝 `find_data`（`lfw.datas.find`）/ `find_entity`（`world.entity_map.get`）；端口 `nums` 收 `std::vector<Value>` 而不是 `double`，因为 `MP_MAX` / `HP_MAX` 这类槽位可以是 `null` 而 `null` 与 `NaN` 在回读后**可观测不同**（`Times` 的 5 槽块走 `double` 暂存，`utils/times.*` 未改）；`?? NaN` 槽位（`RESTING_MAX` / `FALL_VALUE_MAX` / `DEFEND_VALUE_MAX` / `DEFEND_RATIO` / `CATCH_TIME_MAX` / `DISMISS_TIME`）与裸槽位分两类，`num_or_null` 只吃真 NaN；布尔四件套是 `!== 0`（`null` 算 **true**）而不是 `truthy`；`copies` 用插入序 + 去重的向量复刻 `Set`（尾逗号 `slice(0,-1)`、空集合写 `''`、`x,` 会多出一个空成员）；`transforms` 要两个 id 都非空且两份数据都查得到，写侧 `size() > 0` / `> 1`；`dead_join` 走 `json_stringify` / `json_parse`（TS 在非法 JSON 上抛异常，属端口模型外）；`read_snapshot` 全静态赋值、**不触发任何通知**；帧查找的 `?? this.frame` 是死代码，`LANDING_FRAME_ID` 的空串才是真分支） | ✅ 通过（`entity` 1387 行；变异 **407/407 全杀**） |
| 9e | 切片 9：`Entity` 的每 tick 恢复层（**强连通块的第五刀**；`stat_recovering` / `hp_recovering` / `mp_recovering` / `toughness_recovering` / `fall_value_recovering` / `defend_value_recovering`；两段式分支（`*_resting > 0` 时按 `_atom_time` 排空并夹到各自的 max，否则走 `Times.add` 门控的恢复支），排空支由 `frame.toughness_recover` / `frame.stat_recover` 开关；`hp` / `mp` 每次调用先用 `dataset(hp_r_ticks)` / `dataset(mp_r_ticks)` 覆盖 tick 上限，`toughness` / `fall` / `defend` 用 `reset()` / `reset_armor()` 装好的区间；`fall_value_max` / `defend_value_max` 是**带数据集兜底的 getter**（缺键 → NaN，`clamp_add` 的 `value > NaN` 恒假 → 原值保留）；`hp_recovering` 夹的是私有 `_hp_r` 而不是 `hp_max`（所以要用快照窥视口把 `HP_R` 设到 `hp_max` 以下才看得见）；`mp_recovering` 的比率：`a = hp_max()` / `b = _hp` 先各自夹到 500，再 `1 + round_float((a - min(r_ratio * b, a)) / 100)`（三位小数取整可位级观测）；六个写入全走 setter，通知日志是判别力来源） | ✅ 通过（`entity` 1653 行；变异 **464/464 全杀**） |
| 9f | 切片 9：`Entity` 的标记 / 发射者 / 出弹点速度（**强连通块的第六刀**；`set_mark` / `del_mark` / `is_ally` / `get_emitter` / `get_opoint_speed_z`；`marks` 是 `Map<string,string>` → 端口 `std::map`（插入序 vs 字典序，harness 两侧都把转储**排序**后打印）；`prev == void 0` 与 `marks.get(key) == prev` 都是**宽松比较**（`null` 算「没有期望值」、`"3" == 3` 命中），所以这两处用 `equals` 而不是 `strict_equals`；`del_mark` 返回的是 `Map.delete` 的布尔（条件成立但键不存在 → `false`）；`is_ally` 是严格 `===`（字符串团队上宽松/严格同义，只能靠比较对象/取反类变异检验）；`get_emitter(idx)` 的 JS 数组下标（分数 / 负数 / 越界 → `undefined`）+ `if (!id) return;` 把**空串**也拦掉 + `entity_map.get` 查不到是 `undefined`；`get_opoint_speed_z` 的 `speedz !== void 0` 先胜出（`null` 原样返回）、`is_fighter(emitter)` 只读 emitter 的数据（端口签名收 `const Entity*`，`v?.data` 让缺失 emitter 也走非 fighter）、`switch (this.state)` 是**严格数字开关**（字符串 `"1002"` / 小数 `1002.5` / `null` / `undefined` 都不命中），只有 Ball_Flying 3000 / Ball_3006 3006 / Weapon_Throwing 1002 / HeavyWeapon_InTheSky 2000 给 `Defines.DEFAULT_OPOINT_SPEED_Z`；差分用快照窥视口把实体 id 改成 `""` 来锁「空串 emitter id 不解析」） | ✅ 通过（`entity` 1749 行；变异 **502/502 全杀**） |
| 9g | 切片 9：`Entity` 的状态接线（**强连通块的第七刀**，也是 Entity 块第一刀结构接线；新增 `set_state` + `_state` + `EntityStateView` 适配器，把此前注入的六个状态钩子（`on_dead` / `get_gravity` / `find_frame_by_id` / `get_auto_frame` / `get_sudden_death_frame` / `get_caught_end_frame`）改成从**活动状态对象**上取；`states.get(code) \|\| states.fallback(_data.type, code)`（数字键 vs 字符串键 `\"${type}_${code}\"`，端口用 `n:` / `s:` 前缀区分），`fallback` 按**严格**类型分派 Fighter/Weapon/Ball/其余，命中即缓存；`if (this._state === v) return` 是对象同一性早退；`leave(this, this.frame)` 拿当前帧而 `enter(this, this.get_prev_frame())` 拿上一帧；`reset` 清 `_state` 并重设 `_states`（这一行 9a 就抄了，9g 才第一次有观察点）；`EntityStateView` 把 `IStateEntity` 的 `Value` 签名与 `Entity` 的 `double` getter 对上（18 个转发成员全部被差分覆盖，其余留成接口默认值并在 DESIGN §50.3 列明）；TS `Map` 覆盖键时旧状态仍被 `_state` 引用而端口会析构——真实注册表只在启动时建一次，故端口用裸指针安全，差分也只在“覆盖非活动键”时替换） | ✅ 通过（`entity` 1852 行；变异 **541/541 全杀**） |
| 9h | 切片 9：`Entity` 的 v_rest / 关系清理 / 闪烁标记（**强连通块的第八刀**；`add_v_rest` / `get_v_rest` / `del_v_rest`（三张表 `vrests`/`blockers`/`superpunchs`，kind 严格比较、覆盖只写不删旧镜像）、`get_flag`（Ally/Enemy + Dead + `| type` 的 ToInt32 位组合）、`clean_holding` / `clean_catching`（只清「对方回指自己」）、`drop_catching`（`set_catching(null)` + `enter_frame(auto)` 宿主缝 + 恒真）、`blink_and_gone` / `blink_and_respawn`（**直接写 `_blinking`**，不走 `max(0,·)` 的 setter）+ `update_itr_bdy_hit_ground`（真值门、`{y=0,h=0}` 默认、严格 `>`、`continue` 走完全表）；harness 新增 `vrest`/`vrestget`/`vrestdel`/`vrestdump`/`flag`/`cleanhold`/`cleancatch`/`dropcatch`/`blinkgone`/`blinkrespawn`/`itrground`/`linkb` 与八位关系探针） | ✅ 通过（`entity` 2009 行；变异 **594/594 全杀**，其中 9h 新增 53 条） |
| 9i | 切片 9：`Entity` 的帧进入链 / 位置层 / 跟随关系 / transform（**强连通块的第九刀**，也是 Entity 块**第一次把宿主缝换成真链路**；`set_frame`（gone 清 opoint、`interval_mode` + `interval_id` 的稳定压实、状态钩子、四个标志、cpoint / broadcast / holding 收尾）、`enter_frame` / `enter_frame_by_id` / `handle_next_frame_result` / `get_next_frame`（`Gone/NotFound/Entered/Fallback` 四态、`infinity_mp` 门后的 mp/hp 消耗、`this.frame.next === which` 的**对象同一性**分支 + `hit.d ?? NEXT_FRAME_AUTO`、数组分支的 `pick` + 拆开的 `has_next_frame_judge`/`next_frame_judge` 双缝）、`set_position`（轴跳过 + `round_float` + MIN_SAFE 的 `prev_position` 复制 + x→z→y 三条单轴请求 + 整体 `on_restrict` 与状态钩子 + `Ground::y`）、`update_position`（四个门、blockers 朝阻挡者方向清零并写 `prev_velocity`、梯形半步积分）、`follow_bearer` / `follow_catcher`（**bearer 是快照**：TS `const { bearer } = this`，端口原先读成员导致投掷分支的 `vz` 恒 0，新用例杀出并修掉）、`drop_holding`（对齐帧 `{id:"auto"}` 回退、`vrests` 克隆）、`pick`、`transform` / `transfrom_to_another`（`fmod` 环、帧 245 请求、`copies` 的 transform 与掉队清理）；新增 `entity/enter_frame_result.h`、`MersenneTwister::pick_value`、`IStateEntity::assign_position`（否则 `on_restrict` 与 `set_position` 互递归爆栈）、`EntityStateView::set_velocity` 转发（否则状态钩子的速度钳制静默失效）、`States::fallback(Value, Value)`；harness 新增 `setpos`/`updatepos`/`setframe`/`enter`/`enternext`/`enterid`/`followbearer`/`followcatcher`/`drop`/`pick`/`transform`/`transnext`/`terrain`/`vratt`/`copyself`/`frameb`/`bkeys`/`hook viewpos|viewframe|viewenter` 与 `create_ctrl`/`judge`/`broadcast` 日志（`itrground`/`dropcatch`/`drop`/`pick`/`follow*`/`summaries` 的观察点也一并扩到帧 / 被放下侧 / 挑选计数），并删掉 TS 侧的 `enter_frame` spy） | ✅ 通过（`entity` 2326 行；可执行变异 **750/750 全杀**，另有 67 条经逐条查证在本主题不可观测、已记录在 `mutations/entity.mjs` 头部与 DESIGN §52.5；9i 新增 226 条、其中 159 条转为全杀） |
| 9j | 切片 9：`MersenneTwister` 的调试面 + `Cases`（**强连通块外的补齐刀**，把「已知偏差」表里最后一条 mt 省略搬完；新增 `native/lfw/cases.{h,cpp}`：`Cases` 的 `name`/`separator`=`\uffe5`/`cases`/`times`/`reset`/`push`/`submit`（`push` 的文本格式与 `Array.join()` 语义逐字节对齐）+ `mt_cases()` 单例（`sus_cases` 无调用点、不建）+ `Array::remove_at`（`take<T>` 的 `splice` 要按位删）；`MersenneTwister` 补 `mark` / `debugging` / `log_case`（TS 的 `case`）/ `log_entry`（`int`/`float`/`range`/`pick`/`take` 的调试推送 —— `float` 内部调 `int`、`pick`/`take` 内部调 `range`，所以各产生**两条**条目）/ `pure` / `load` / `reset(seed, debuging)` 的第二参与链式返回 / `take_value`；harness 新增 `mark`/`debug`/`case`/`cases`/`cinfo`/`creset`/`pure`/`load`/`pickv`/`takev` 与 `seed <n> d`，新用例 `mt_debug.txt`） | ✅ 通过（`mersenne_twister` 7747 行；变异 **115/115 全杀**，9j 新增 66 条；另有 6 条经逐条查证在本主题不可观测、已记录在 `mutations/mersenne_twister.mjs` 头部与 DESIGN §53.4） |
| 9k | 切片 9：`MersenneTwister` 的 `mark` 探针回填（**把 9j 建好的落点接上消费者**；`src/` 里 20 处 `mt.mark = …` 中，已移植代码里的 8 处逐处补回：`Entity::drop_holding`（`dh_1`）、`Entity::follow_bearer`（`dh_v`）、`Entity::get_next_frame`（`gnf_0`/`gnf_1`）、`spawn_ice_piece::{ice_piece_x,ice_piece_y}`、`Randoming::random_in`（`mark = name`）、`CharacterState_Drink` 掉落分支（`drink_drop`，新增 `IStateEntity::holding_mt_mark` 缝）；`weapon_is_hit`/`action_handlers` 的旧缝本就不缺、不动；`Entity::spawn`/`update`/`spark_point`、`ValExpression`、`bot/*`、`stage/*`、`ui/*` 的调用点随各自的刀；harness 新增 `entity` 的 `run mtdebug`/`run mtmark`/`run mtcases`、`mt_random` 的 `mt dbg`/`mt mark`/`cases`、`burning_drink` 的 `run mtmark`（TS holder 双件把 `lfw.mt.mark` 换成带日志的 getter/setter），新用例 `entity/mt_probe.txt`） | ✅ 通过（`entity` 2363 行 / `mt_random` 158 行 / `burning_drink` 157 行；变异 **entity 759/759、mt_random 64/64、burning_drink 41/41 全杀**，其中 9k 新增 21 条；`dh_1` 一处不可观察、已记录在 `mutations/entity.mjs` 头部与 DESIGN §54.3） |
| 9l | 切片 9：`Entity` 的 opoint 生成簇（`spawn` / `on_spawn` / `attach`；**opoint 链条第一次落地**：`spawn(opoint)` / `spawn(opoint, offset, facing)` 的 `unimportant && entity_count() > 355` 门、`mt.mark = se_1`、`pick(oid)`、`find_data`、`create_entity_with_bot`、`.on_spawn().attach(opoint.ghost)`、同 id 进 `copies`、`vrests` 复制；`on_spawn` 的 emitters/team/facing 传播（含 `Ball_Rebounding` 分支读 `lastest_collided.attacker`）、`pos_type` 两套位置公式、`__gen_{x,y,z,facing,dvx,dvy,dvz}`（宿主缝 `gen_field`：`nullopt` = 没有这个字段 ⇒ 回落，回 `undefined` ⇒ 走 `?? 默认`）、`get_opoint_speed_z`、hp/mp 四档覆盖、`z_disabled`（`Normal`/`Burning`）、`ud = ctrl.UD()` 只在发射者是 fighter 时生效、Fixed/Extra 速度模式、`OpointKind.Pick` 的 `drop_holding` + bearer/holding 双向链接、`motionless ?? 2`；`attach` 的 `_mounted` 守卫 / `_spawn_time` / `_ghosted`+`motionless` 清零 / 落地判定 / `FrameId.None → auto`；`IEntityHost` 新增 5 缝（`entity_count` / `add_entities` / `game_time` / `create_entity_with_bot` / `gen_field`，`apply_opoints` 占位缝保留给下一刀），harness 新增 `env ecount|gtime|gen|genclear` + `run spawn|spawnv|spawndump|attach|lastcollided`，新用例 `entity/spawn.txt`） | ✅ 通过（`entity` 2479 行 = main 2326 + mt_probe 37 + spawn 116；变异 **entity 820/820 全杀**，其中 9l 新增 61 条） |
