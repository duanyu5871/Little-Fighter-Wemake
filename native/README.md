# native —— LFW 核心的 C++ 实现

目标：**单一实现 / 平台解耦**。让 `src/LFW` 的规则层能在浏览器（WASM）、
Electron 桌面壳、以及将来的原生客户端上跑同一份逻辑。

对应关系：

| | TS（现状，唯一可运行实现） | C++（本目录） |
|---|---|---|
| 纯核心 | `src/LFW/` | `native/lfw/` |
| 宿主实现 | `src/DittoImpl/` | `native/host/`（`common/lfw_boot` 共享装配 + headless 宿主 `FsZip`/`HeadlessHost`/`lfw_run` + `wasm/` 的 C ABI 宿主） |
| 前端宿主（JS 侧） | `src/DittoImpl/`（浏览器） | `native/web/`（JS 宿主 / 冒烟 / 打包 / 预演页） |
| 注入边界 | `src/LFW/ditto/Instance.ts` 的 `IDittoPack` | 镜像到 `native/lfw/ditto/`（`IXML` / `IXMLElement` / `IZip` / `IZipObject` 等宿主服务接口） |

---

## 硬规则（`native/lfw/` 下）

- **平台实现一律不许落在 `native/lfw/**`**：渲染、音频、图片、输入、时钟、wasm/ABI 这些**具体实现**
  只放 `native/host/**`（C++ 侧）与 `native/web/**`（JS 侧）；核心只留**接口**与**逻辑**。
  `lfw.h` 里与平台相关的东西只允许是"**宿主工厂**"（`create_world_renderer` / `create_ui_node_renderer`
  这种"向宿主索要实现"的钩子）。

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
  cmake/toolchains/emscripten.cmake  WASM（已验证：编出 liblfw_core.a；见「WASM 构建」节）
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
    loader/ entity/ stage/ bot/ buff/ bg/ cmds/ controller/ ui/ animation/
      dat_translator/ ditto/                 ← 以上均已搬
      collision/（非叶子：靠 entity/buff/World）
    未搬（按需）：utils/container_help/{get_keys,list_fn}、utils/string_parser
  tests/differential/                ★ 两侧对拍（详见 PROTOCOL.md）
  tools/
    native.mjs                       configure / build / lint / test
    check_lfw_cpp_includes.mjs       分层规则检查
  host/                              ★ 宿主实现（M1：headless 可运行入口）
    fs_zip.{h,cpp}                   目录当 zip 用（`IZip` / `IZipObject` 的读盘实现）
    headless_host.{h,cpp}            `ILfwHost` 的最小可用实现 + 手动时钟/定时器 + 空渲染器
    lfw_run.cpp                      `lfw_run`：装数据包 → 起世界 → 推帧（每 N 帧打摘要）
    common/lfw_boot.{h,cpp}          宿主无关的装配（选 stage/bg → 造战士）+ 一帧摘要：两宿主共用
  ffi/lfw_c.h                        纯 C ABI 导出（尚未创建）
  DESIGN.md                          现行设计；WASM 对接方案见其中 `## ★ 宿主层与 WASM 对接设计（方案）` 一节
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
  spec 可以带 `cases: ["<case>", …]` 只跑相关用例（一份变异通常只有一两份用例看得见，
  测试那一段从 ~2.0s 降到 ~0.2s，整条 `build` + `test` 从 ~4.5s 到 ~2.8s；
  不写就照旧跑该 subject 的全部用例）。
  ⚠️ 跑之前先确认 `native/build/mutate-backup/` **不存在** —— 它存在说明上一次跑被中断，
  源文件可能还是变异体（下次启动会自动恢复）。

### 跑起来（headless，`host/`）

`lfw_run` 把宿主缝接到真数据上、**手动推帧**（不依赖平台的 raf / setInterval），跑完打
每帧摘要与实体表快照。M1 只支持「已解包的包」：

```
# 一次性：把包解开（目录 = 包内容）
mkdir native\build\host-data\prel -Force; Expand-Archive dist\prel.zip -DestinationPath native\build\host-data\prel -Force
mkdir native\build\host-data\data -Force; Expand-Archive dist\data.zip  -DestinationPath native\build\host-data\data  -Force

# 跑（`--root` 给 `builtin_data/…` 的根；`--zip` 按给定顺序 load）
native\build\msvc-x64\bin\lfw_run.exe --root dist ^
  --zip native\build\host-data\prel --frames 300 --every 100
```

输出形如：

```
[lfw_run] loaded: fighters=1 weapons=0 balls=0 entities=1 stages=2 bg=3
[lfw_run] spawn p1 gim @(-80.0, 0.0, 0.0) team=1
[lfw_run] stage=tutorial bg=ash_forest entities=2
[frame    299] entities=2 ghosts=0 hp=1000.0 digest=7335aa25f4e1ce86
[lfw_run] done: frames=300 ups=60 digest=7335aa25f4e1ce86 warns=0 errors=0 …
```

`digest` 是「实体表（按 id 排序）的位置/血量/队/状态」的 FNV-1a —— 同参数**逐位可复现**
（`--time` 给时钟初值 = `MersenneTwister` 的种子）。渲染 / 音频 / 指针 / UI 图片在
headless 宿主里是空实现（会计数并在结尾报一行）。

⚠️ 完整数据包（`--zip ...\data`）现在**能加载**了：包里的 12 个 `.xml`（7 个 bg、5 把武器）走
`host.xml_parse → ToolXML::parse`（已搬，见 DESIGN §141）。两个包一起装：

```
native\build\msvc-x64\bin\lfw_run.exe --root dist ^
  --zip native\build\host-data\prel --zip native\build\host-data\data ^
  --fighter 1 --fighter 4 --stage 1 --frames 900 --every 300
# loaded: fighters=26 weapons=19 balls=24 entities=5 stages=28 bg=16
```

（`--dump` 会打印各表前两条与全部 id，挑 `--fighter` / `--stage` 用。）

---

## WASM 构建（已验证）

端口核心能编成 wasm：`lfw_core` 编过 **0 错误**（62 s / `-j 16`），产物 `native/build/wasm/lfw/liblfw_core.a`
**19.3 MiB**（357 个成员）。需要 emsdk（本机装在仓库外 `%USERPROFILE%\emsdk`，SDK 6.0.12 / clang 24）。

```
cd native
# 先让 emcc 在 PATH 上（emsdk_env.bat，或手写 PATH 加上 emsdk 与 upstream\emscripten）
cmake --preset wasm
cmake --build build\wasm --target lfw_core -j 16     # ⚠ 必须限定 target
```

⚠ **一定要带 `--target lfw_core`**：这个预设的 `all` 里有 **117 个可执行文件**（`lfw_run` + 116 个
`lfw_trace_*`），不限定就会去链 117 个 wasm 可执行文件（未验证，且不是这一阶段要的东西）。

两条已知前提（都已落进 `cmake/toolchains/emscripten.cmake`）：VS 捆绑的 CMake **没有**
`Modules/Platform/Emscripten.cmake` ⇒ 工具链文件改为优先 `include` emsdk 自带的那份（拿不到才退回手写设置）；
`-msimd128=off` clang 不认 ⇒ 改成 `-mno-simd128`。**链接期开关仍是空白**：`-sMODULARIZE` /
`-sALLOW_MEMORY_GROWTH` / `-sEXPORTED_FUNCTIONS` 这些要等真链接（写 JS 宿主那一步）再定，见 DESIGN
「宿主层与 WASM 对接设计」W12。

## WASM 宿主（端口已在 wasm 里真跑）

骨架已通：**整个端口在 wasm 里跑起来**，JS 只提供**时钟 + 日志**并逐帧驱动；渲染/音频/图片仍用
headless 那套顶着（下一步才接）。

```
node native/web/build-wasm.mjs          # 编 wasm（emsdk 须在 PATH 上；~61 s，0 错误）
node native/web/smoke.mjs --profile 2   # Node 冒烟：装数据 → boot → 推 900 帧 → 对拍摘要
node native/web/smoke.mjs --profile 1 --live
```

产物 `native/build/wasm/bin/lfw_wasm.{mjs,wasm}`（67 KB + 2.8 MB）。冒烟结论是硬的：**三条摘要与 MSVC 的
`lfw_run` 逐位一致**（`7335aa25f4e1ce86` / `e4a9437292047044` / `f29d3bb1ea330c32`）；
`fighters=26 weapons=19 balls=24 stages=28 bg=21 bots=22 moves=24` 全装进来；`--profile 1` 连跑两遍输出逐字节相同。

⚠ **一条必须记住的坑**：`LFW` 构造里会 `world.start_update()` ⇒ **端口自己的 Ticker 也在推帧**。
宿主要么**只**实现时钟槽、让 Ticker 自己走（`--live`），要么**只**手动调 `lfw_frame`（`--profile *`）；
**两边都做 ⇒ 一帧走两步**（`lifetime` 翻倍）。手动模式才是与 `lfw_run` 对拍的那条路。

数据怎么进 wasm：JS 先把解包好的包**写进 emscripten 的 MEMFS**（浏览器走 `bundle.json` + `bundle.bin`
一次拉取），`FsZip`/`HeadlessHost` 照旧按目录读 —— **端口零改动**，`.wasm` 里不塞数据。

（台面那条 WASM 构建节是 `lfw_core` 静态库；这一节是能跑起来的目标，两者并存。）

## 验收：宿主三条摘要（改完必跑）

同一份数据、同一命令**逐位可复现**；三条都过才算没打破 headless 行为：

```
native\build\msvc-x64\bin\lfw_run.exe --root dist ^
  --zip native\build\host-data\prel --frames 300 --every 300
  ⇒ digest=7335aa25f4e1ce86（warns=0）

… --zip native\build\host-data\prel --zip native\build\host-data\data ^
  --fighter 1 --fighter 4 --stage 1 --frames 900 --every 900
  ⇒ digest=e4a9437292047044（warns=1）

… --zip native\build\host-data\prel --zip native\build\host-data\data ^
  --stage 0 --fighter 1 --fighter 4 --vs --frames 600 --every 600
  ⇒ digest=f29d3bb1ea330c32（warns=1）
```

`warns=1` 是既有的 `DatMgr::_add_obj id duplicated…`（不是新告警）。差分/变异/覆盖的跑法与判据见
`tests/differential/PROTOCOL.md`；**装载偶发乱码/NaN 那一类不确定失败已修**（根因与护栏见 DESIGN
「雷区」一节）⇒ 现在偶发失败**就是**回归，别再"重跑一次凑数"。

## 移植进度

| 步骤 | 范围 | 状态 |
|---|---|---|
| 0 | `core/` + 差分测试台 | ✅ 通过（MT19937 逐位一致，7564 行） |
| 0.5 | 差分台子泛化为可插拔 subject | ✅ 通过 |
| 1 | `utils/math/` | ✅ 通过（`math/scalar` 90 行、`math/plane` 52 行） |
| 1.5a | `utils/{easing,cross_bounding,utf8,times}` + `defines/i_bounding.h` | ✅ 通过（52 / 8 / 41 / 74 行） |
| 1.5b | `base/graves.h` + `utils/array/` + `utils/container_help/` | ✅ 通过（`collections/basic` 81 行、`collections/nested` 55 行） |
| 1.6 | `base/Expression`（V7）+ `Object`（V6）+ `JSON`/`JSON5` | ✅ 通过（`expression` 303+216、`value/object` 109、`json` 62+157、`json5` 302+111+6454 行） |
| 1.7 | `fields.ts`（字段描述 DSL + `fields()` + `reorder_fields` + `validate_fields`） | ✅ 通过（`fields` 77+155 行） |
| 2a | `defines/` 的**枚举**（52 个：47 生成 + 5 手写；覆盖已验） | ✅ 通过（`defines/all` 851 行） |
| 2b | `defines/` 的**字段表**（34 张，生成） | ✅ 通过（`defines_fields/all` 34 行） |
| 2c | `defines/` 的**运行时数据**（`Defines` 命名空间 67 条 + 7 个顶层对象 + 5 个函数） | ✅ 通过（`defines_runtime/all` 149 行）。`I*.ts` 结构体与 33 个 `*_new()` 只服务 `dat_translator`，运行时不用，暂缓 |
| 2d | `base/{FSM,Callbacks,NoEmitCallbacks}` | ✅ 通过（`base/core` 3127 行） |
| 3a | `dat_translator/CondMaker`（条件字符串构造器，所有默认条件的共同前置） | ✅ 通过（`cond_maker/all` 55 行） |
| 3b | `defines/` 名称/标签函数（`labels`，`set_hit_flag`/`set_bdy_kind` 的前置） | ✅ 通过（`labels/all` 107 行） |
| 3c | `dat_translator` 小助手（`take*` / `set_*` / `fixed_float` / `find_float` / `copy_*`） | ✅ 通过（`dat_helpers/all` 100 行） |
| 3d | `dat_translator` 帧跳转（`get_next_frame_by_raw_id` / `cook_next_frame_cost` / `add_next_frame`） | ✅ 通过（`next_frame/all` 83 行） |
| 3e | `dat_translator/ColonValueReader`（手写匹配，替代被禁的 `RegExp`） | ✅ 通过（`colon_reader/all` 52 行） |
| 3f | `dat_translator` 的 `cook_*` / `float_scaling_itr` | ✅ 通过（`cookers/all` 68 行） |
| 3g | `dat_translator/make_frame_state`（+ `foreach` / `ensure` 的 `Value` 重载） | ✅ 通过（`cookers/mfstate` 20 行） |
| 3h | `dat_translator/frame_behavior/*`（14 个模块）+ `make_frame_behavior` 分发器 | ✅ 通过（`cookers/fbehavior` 52 行一次全对） |
| 3i | `dat_translator` 字符串匹配器（`match_colon_value` / `match_block_once` / `take_blocks` / `delete_undefined`） | ✅ 通过（`string_matchers/all` 35 行） |
| 3j | `dat_translator` 的 `make_frames_special` / `make_entity_data`（+ `traversal`） | ✅ 通过（`entity_data/all` 22 行） |
| 3k | `dat_translator/make_itr_prefabs`（+ 手写 `entry:` 匹配器 / `to_num` / `is_non_empty_str`） | ✅ 通过（`itr_prefabs/all` 14 行） |
| 3l | `dat_translator` 的 `make_ball_data` / `make_weapon_data` | ✅ 通过（`entity_kinds/all` 17 行） |
| 3m | `dat_translator/make_bg_data`（+ `make_bg_layer` / `bg_color_translate`） | ✅ 通过（`bg_data/all` 11 行） |
| 3n | `dat_translator/post_process_obj_data`（装配层入口） | ✅ 通过（`entity_data/all` 29 行） |
| 3o | `dat_translator/EditBdy` + `cook_ball_bdy_get_hit_to_frame_20/30` | ✅ 通过（`ball_bdy/all` 11 行） |
| 3aa | `loader/get_val_from_entity`（39 条 getter 表）+ 惰性的 `get_val_from_lf2` / `get_val_from_world` | ✅ 通过（`entity/get_val` 319 行） |
| 3ab | `loader/get_val_from_collision`（86 条 getter 表；`attacker` / `victim` 是活实体 ⇒ 宿主缝） | ✅ 通过（`entity/collision_val` 672 行） |
| 3ac | `loader/preprocess_bdy` + `loader/preprocess_itr`（+ `preprocess_action` 的 `action.tester` 落地；14 个 `itr.kind` 分支的默认条件） | ✅ 通过（`loader_frames/all` 222 行） |
| 3ad | `loader/preprocess_frame` + `utils/read_nums`（帧装配：prefab 合并 / weapon 与 fighter 的状态默认值 / `cook_frame_indicator_info` / `fold_aabb` / `__hit_ground_*` / `center` / `pics`） | ✅ 通过（`loader_frames/all` 544 行；另有 `indicator_info` 那一路的失败通道） |
| 3ae | `loader/preprocess_entity_data`（实体数据的总装入口：四路 special / `itr_prefabs`·`bdy_prefabs` 逐条 / `base` 的图·音效 / `pre_*`·`post_hitkeys` 的 `__*_map` / `on_dead`·`on_exhaustion` / `portraits`·`frames` / `__pics` / `base.bot`） | ✅ 通过（`loader_entity` 新 subject，387 行） |
| 3 | `loader/get_val_*` 余下部分（`get_val_from_bot_ctrl` / `get_val_getter_from_stage`） | ✅ 通过（`entity/ball_ctrl`、`stage` 等 subject 均已搬） |
| 4r | `controller/BallController` + `helper/closer_one` | ✅ 通过（`entity/ball_ctrl` 174 行） |
| 4 | `entity` + `collision` + `buff` + `state` + `controller` + `bot` + `World` | ✅ 通过（整块搬完） |
| 4宿主 | 步骤 4 **宿主接入（M1：headless 可运行）**：`host/` 的 `FsZip`（目录当 zip）+ `HeadlessHost`（`ILfwHost` 最小实现：读盘 import / `ToolXML` / 手动时钟 / 空渲染）+ `lfw_run`（装包 → 起世界 → 手动推帧 → 摘要） | ✅ 通过（`prel` 包：2 个 gim 起 `tutorial` 舞台，300 帧逐位可复现；lint 9776/0）。`Entity.ts` 底部那四句模块级 `Factory.register_entity` 与 `IWorldLfw::acquire_entity` 缝 |
| 4XML | 步骤 4 **`ToolXML::parse`（XML 文本 → `IXMLElement`）**：fast-xml-parser 那套选项的形状口径（直接文本保留空白并按序拼、属性走 `String(value)`、无值属性 `"true"`、`ns:` 前缀去掉、CDATA 原样、只解五个命名实体），注释先剥、声明/PI/DOCTYPE 跳过 | ✅ 通过（完整 `data.zip` 现在能装：`fighters=26 weapons=19 balls=24 entities=5 stages=28 bg=16`；`--fighter 1 --fighter 4 --stage 1` 900 帧逐位可复现；lint 9776/0）。细节见 DESIGN §141 |
| 4主干 | 步骤 4 **主干**（宿主层：`stage/` 与 `World` / `LFW` / `Factory` / `Resources` / `Camera` / `Keys` **之前**能单独搬的部分，按依赖序：`I18N` → `loader/get_import_fallbacks` → `PlayerInfo` → `Camera` → `ZipMgr` → `Resources` → `Factory` → `stage/*` → `World` → `LFW`） | 进行中（`I18N` + `loader/get_import_fallbacks` ✅ 262 行；`PlayerInfo` ✅ 270 行；`ZipMgr` ✅ 114 行；`Camera` ✅ 369 行；`Resources` ✅ 136 行；`Factory` ✅ 92 行；`stage/Expressions` + `stage/Status` + `bg/{Background,Layer}` ✅ 70 + 83 行；`stage/Item` ✅ 622 行；`stage/Stage` ✅ 1045 行；`World`（状态与查询那半）✅ 638 行；`World::step` + `update_once` + `catch_up` + `start_update` + `Entity::release`（**非碰撞配对**那半；配对与 `collision_get` / `collisions_keeper.handle` 留给 4L）✅ 用例 315 行） |

**步骤 2–4 是修正过的。** 把 `import type` 排除后算运行时依赖图，得到两个关键事实：

1. `utils/` 到根目录只有两条边：`cross_bounding.ts → ../defines`（仅类型，无害）和
   `MersenneTwister.ts → ../../cases_instances`（`mt_cases` **调试探针**；已把 `Cases` +
   `mt_cases` + `mark`/`debugging`/`case`/`pure`/`load` 一起搬进 `native/lfw/cases.{h,cpp}` 与
   `utils/math/mersenne_twister.{h,cpp}`，见 DESIGN §57）；
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
| `container_help/get_keys` + `foreach` 的对象分支 | 未移植 | `get_keys` 在 `src/LFW` 里没有调用点（只有 `index.ts` 的 re-export）⇒ **按需不搬**；`foreach` 的对象分支（`typeof any === 'object'` ⇒ 转 `traversal`）在端口里由调用点直接改用 `traversal`（见 `dat_translator/*`）⇒ 本函数只留数组分支 |
| `container_help/list_fn.h` | 不移植 | 整个 `src/LFW` 里唯一的真·反射，只有 `NoEmitCallbacks.add()` 一个调用点 ⇒ 改成显式注册表 |
| `math/mersenne_twister` | `pure()` 的 `mt` 是 `uint32_t`，TS 里 `twist()` 之后是 signed int32 的**同一批位模式** | 位模式全等；差分里直接打印 `mt[i]` 的地方先 `>>> 0`（`state` 哈希一直如此） |
| `math/mersenne_twister` | 不保留 `range(min, max, debugging)` 的**第三参** | TS 里是死参数（函数体只看 `this.debugging`） |
| `ensure.h` | 接受了 `std::vector` 而不是变参 | 语义等价；**但两边都不能传空 items**（TS 的 `item` 是必填项） |
| `BallController.chasing` | 存的是**引用快照**（建引用那一刻的字段），TS 的 `this.chasing` 是活实体 | 控制器层拿不到 `Entity`（走 `Value` 引用）。`update_lookup` 开头用 `reidentify` 按 `id` 从名单认领新引用，让「目标还在世界里」这条路径与 TS 逐字段一致；目标已离开名单时端口读旧快照（残差，见 DESIGN §5 第 1 条） |
| `BallController.update_lookup` 的 `self` | 由 `CtrlEnv` 的位置快照现搭（`self_ref`），不是 `entities[me]` | TS 读 `this.entity.position`；端口按分层约定不 include `entity.h`。`me` 只是调用方给的下标，两者只在「`me` 恰好指向自己」时等价 |
| `BallController.chasing` 的归属 | 放在 `BaseController` 上（TS 是 `BallController` 的字段） | `spawn` 的 `OpointMultiEnum` 分支只有 `BaseController*`（`is_ball_ctrl()` 门之后才写）；语义不变 |
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
| `lfw/defines/schemas_gen.h` | **由 `native/tools/gen_defines_schemas.mjs` 生成，不要手改** | 11 份 `Schema_*`（纯数据：无函数/无类实例）dump 成 JSON5；扫描根 = `src/LFW/defines` 全量 + `src/LFW/ui` 里导出 `Schema_*` 的模块（4AH 起） |
| `SchemaValidator` 的类类型分支 | TS 的 `typeof type === 'function'`（自定义类）、“值必须是字符串”、`Object.defineProperty` 惰性属性、`instance_getter/setter` 钩子不建形 | `Value` 装不下函数；已移植 10 份 schema 无此形态（DESIGN §88） |
| `preprocess_bg_data` 的 `Ditto.warn/error` | 端口加 `warnings` / `errors` **sink 参数**（宿主缝） | TS 是宿主包全局；台面两侧分别用 sink / 替换桩捕获成 `bgw`/`bge`（DESIGN §88） |
| `JSON.stringify` 会把 `-0` 写成 `0` | 生成字段表时用 | 当前 34 张表里没有 `-0`；若有，差分会在位模式上暴露 |
| `Entity::_team` 是 `std::u16string` | TS 的 `team` 保持原值（可能是数字）⇒ 数字队伍在端口里被字符串化（TS 存 `3`，端口存 `"3"`） | 队伍在真实数据里一直是字符串（`lfw.new_team` / 帧数据），端口内部也全程按字符串比较；只有「碰撞攻击者带数字队伍」这种人造输入会撞上，用例只喂字符串（9l） |
| `State_Frozen::leave` 里 `StateBase_Proxy::leave` 被调了两次 | TS 只调一次 `super.leave(e, next_frame)`，端口在 `e.apply_opoints(ice_piece_opoints())` 之后又多调了一次 | 9m 复核 `apply_opoints` 调用点时发现；`apply_opoints` 的相对顺序与 TS 一致，所以不影响 9m，但这多出来的一次是**真的多**（`get_proxy(e).leave`）⇒ 归状态层复核/修（现有 `entity_states` / `character_state_*` 用例观察不到） |
| `base/ValExpression` 的宿主面 | TS 的取数器是 `(e: Entity) => number`（`DEFAULT_VARS` 直接读 `e.frame.width` 等、`rand`/`pick`/`bag`/`flip` 读 `e.lfw.mt`）；端口把它收成 **Ctx 概念**：`frame_var(name)` + `mt()` | 端口没有能被 `base/` 依赖的 `Entity`（循环包含）⇒ 照 `Expression<Ctx>` 的既有约定模板化。取数器给 `double`：帧**缺字段**时 TS 拿到 `undefined`，那一支（`range(undefined, undefined)` 提前返回、`pick` 的 `v !== undefined` 过滤）端口不复现，用例只喂有值的帧（见 PROTOCOL §6.105） |
| `preprocess_opoint` 不写回 `__gen_*` | TS 把编译出来的 `ValExpression` **挂到记录上**；端口把九组编译结果**交还调用方**（`std::map<__gen_* 名, ValExpression>`），编译失败的字段不在表里 | `Value` 只有 7 种类型装不下函数（DESIGN §9.42 的既有结论）。差分时两侧都按「把 `__gen_*` 删掉」对齐（TS 侧由 harness 删），`?? opoint.__gen_x` 的「失败保持原值」语义照旧成立 |
| `host/`（宿主实现） | 渲染 / 音频 / 指针 / UI 图片是**空实现**（图片给零尺寸占位并计数） | 都是平台侧的东西；M1 只要「能起世界、能推帧」，它们不参与世界推进（DESIGN §140） |
| `host/HeadlessHost::md5` | 占位哈希（FNV-1a ×2），**不是真 MD5** | 只给 `ui_load_img` 拼 key 用；与 `Ditto.MD5` 逐位一致留到 zip 那刀 |
| `host/FsZip` | 只吃「已解包」的目录；压缩包本体（中央目录 / DEFLATE / md5 校验）未做 | M1 先用 `Expand-Archive` 出来的目录跑通（DESIGN §5） |
| `host/` 的 `zip_*` | 下载 / 读 blob / zip 缓存一律「没有」⇒ 只走 `ZipItem{path, IZip*}` 这条**对象**加载路 | 宿主没有 blob / 下载层（DESIGN §5）；`load_zip_from_url` 那条路等 zip 那刀 |
| `Entity.ts` 底部的模块级 `Factory.register_entity` | 端口用 `entity.cpp` 里一次**静态初始化**顶「模块副作用」 | C++ 没有模块初始化；时间点必须早于任何用户代码，差分台面靠「后注册覆盖」与 TS 对齐 |
| `ToolXML::parse` 的实体解码 | 只解 `&amp; &lt; &gt; &quot; &apos;`；十进制/十六进制引用（`&#65;` / `&#x42;`）**照抄** | 实测 fast-xml-parser 在这套选项下就不解数字引用（DESIGN §141.2 的探针表） |
| `ToolXML::parse` 的命名空间 | 只去 `ns:` 前缀，没有额外的 `xmlns` 处理 | 真实数据里 12 个 `.xml` 都不用命名空间（DESIGN §141.2） |
| `ToolXML::parse` 的宽容度 | 闭合标签与栈顶不同名时「忽略这次闭合」；`<!DOCTYPE …>` 跳到下一个 `>` | fast-xml-parser 本身很宽容；真实数据格式良好，这条只影响畸形输入（DESIGN §141.2） |
| `base/Ticker` / `base/FPS` 的三处省略 | ① `Ticker.TAG`（`static readonly`）不移植；② `ITimeout.add` 的 `...args` 不移植；③ `Ticker._tick` 的 `try { this._step_once() } finally { this._schedule() }` 写成顺序两句 | ① 全仓库没有读取点（同其它槽的约定）；② TS 源码里自己注明「无用的预留参数」；③ lint 禁 `try`/`catch`，且端口的 `on_step` 不抛 ⇒ 「抛异常也重排」这条路径不可表示（见 DESIGN §5） |
| `Number.prototype.toFixed` 只移植了 **f = 1** | 端口叫 `number_to_fixed_1`（`core/js_string.h`） | 唯一的调用点是 `get_short_file_size_txt`；一般化要 `10^f·m` 的大整数尾数（f ≤ 22 才进得了 128 位）。**别拿 `dat_translator/fixed_float` 顶替**：它返回数值、走浮点路径，只在 `\|x\|·10^f < 2^53` 时与 JS 等价（DESIGN §5 第 1 条） |
| `base/clock.h` 的空槽回落 | TS 里 `Ditto.Clock` / `Ditto.Timeout` 永远装着宿主；端口的 `clock()` / `timeout()` 可能是空槽 ⇒ `clock_now()` 回 `0.0`、`clock_hidden()` 回 `false`、`clock_add`/`timeout_add` 回 `0`（=「没有句柄」）、`clock_del`/`timeout_del` 变 no-op | 不是防御性编程：`Randoming::default_mt()` 在**静态初始化期**取时钟取种子，那时宿主还没装槽（与 `callbacks_warn()` 的 `if (warn)` 同款约定） |
| `get_val_from_entity` 的词是查表，不是属性查找 | TS 是 `entity_val_getters[word]`，`word` 撞上 `Object.prototype`（`constructor` / `toString` / `__proto__` …）会拿到一个**函数**而不是 `undefined`；端口是向量查表，查不到给 `nullptr` | 原型污染的产物、数据文件写不出这种词，复现了也只能崩在同一处 ⇒ 不复现（DESIGN §5 第 1 条）。用例只用普通拼错的词 |
| `get_val_from_world` 的回落投影不移植 | TS 是 `(e, ...arg) => fallback(e.world, ...arg)`；端口两个惰性文件都恒 `nullptr`，没有那层投影 | `ValGetter` 是裸函数指针（`base/expression.h`），捕获不了 `world → world.lfw` ⇒ 等 `LF2Val` 真有实现时按 getter 生成静态适配器（DESIGN §65.1） |
| `entity_val_getters` 里的碰撞类型判定读 `data_type` | TS 读活实体的 `c.attacker.data.type`；端口的 `Collision` 里是 `CollisionActor` 快照，只有 `data_type` | 成立的前提是「造碰撞的一方把 `data.type` 写进 `data_type`」（`collision_core` 的差分台子就是这个约定）；`length_of` 的非数组/非字符串分支、`Holding_W_Type` 的 `?? 0` 也一并不可达（DESIGN §5 第 3/4 条） |
| `collision_val_getters` 的活实体走宿主缝 | TS 的 `Collision.attacker` / `.victim` **就是**那两个活实体（读的时候才取字段）；端口的 `Collision` 只有 `CollisionActor` 快照（`collision/` 层看不见 `Entity`）⇒ 新增 `CollisionValEnv { find_entity(id) }`，按 `id` 把活实体交回来 | `collision/` 是「实体不知道我」的那一层；缝的先例是 `set_nbdy_normal_env`（DESIGN §67.2） |
| 找不到那个 `id` 时读实体的项给 `undefined` | TS 那边不存在「读了才知道对象没了」——`Collision` 里存的是活对象引用，只能靠 `undefined.x` 抛 `TypeError` 表达；端口 `Value` 的链式取字段一路给 `undefined` | 只有「实体已经不在世界里」才走得到，真实世界不会；差分里只比较两边都给 `undefined` 的那几个词（DESIGN §5 第 1 条） |
| `victim_is_chasing` 比的是 `id` 不是对象 | TS 是 `c.victim === c.attacker.ctrl.chasing`（同一个对象）；端口 `chasing` 与 `victim` 都是快照 ⇒ 比 `id` | 实体的 `id` 唯一，等价；同 `4r` 的 `reidentify` 是同一类问题（DESIGN §5） |
| `armor_work` 现搭一个 JS 视角 | TS 把整个 `Collision` 递给 `is_armor_work`；端口按它读的四项（`victim.armor` / `bframe` / `itr` / `aframe`）现搭一个 `Value` | 端口 `is_armor_work(const Value&)` 本来就是 JS 对象视角（`collision/is_armor_work.{h,cpp}`），`collision/` 层不需要认识 `Entity`（DESIGN §67.2） |
| `bdy.__tester` / `itr.__tester` / `action.tester` 存**源串** | TS 存 `new Expression(test, get_val_geter_from_collision)` 的编译结果（带函数字段的对象）；端口 `Value` 装不下 ⇒ 存源串，由消费侧（`collision.cpp` / `collision/keeper.cpp` 的 `CollisionCoreEnv::tester_run` 宿主缝）按需编译 | `Value` 只有 7 种类型（DESIGN §9.42 起的老结论）；消费侧本来就把 `tester` 当不透明值转交，签名不用动（DESIGN §68.2） |
| 加载期不编译判定器 | TS 在这三处会 `new Expression(...)` 解析源串（两端词都不认识时还会 `Ditto.warn`），端口只把串挂上去、不解析 | 那是副作用不是数据；`Expression` 的解析结果带函数字段、两端不可比（同 `__judger`，DESIGN §68.2 第 2 条） |
| `bdy` / `itr` 是**数组**时的行为 | TS 能往数组实例上挂 `__tester`；端口的 `Array` 只有元素、没有键位 ⇒ `bdy` 侧按失败处理，`itr` 侧两边都是「成功、无副作用」（实例是标量同理：`itr` 成功、`bdy` 失败） | 这类数据没有意义（`bdy` / `itr` 是对象字面量）；用例里 `itr` 侧有 `a()` / 标量，`bdy` 侧不写（DESIGN §8 第 1 条、DESIGN §68.3 第 6 条） |
| `preprocess_bdy` / `preprocess_itr` 的失败用 `bool` + `error` 出参 | TS 抛 `Error`（`prefab_error(...)`）；端口返回 `false`，只有 prefab 错误的文本能对上，TypeError 类的失败（`bdy` 是标量、`actions` 非数组、动作缺 `type` …）只有 `throw` 这一位可比 | 端口一律不用异常（既有约定）；harness 只打印以 `[` 开头的文本 |
| `preprocess_frame` 的 `__seq_map` | TS 是 `Map`，端口用 `Object` 顶替（渲染里都是 `{k:v}`） | `core` 没有 `Map`；键序不同（JS `Map` 保插入序，`Object` 把整数键前置升序）⇒ 差分用例里的 `seqs` 键只能升序 |
| `preprocess_frame` 不落地 `__gen_*` | `frame.opoint?.forEach(preprocess_opoint)` 的编译结果（九个 `__gen_*`）端口不写 | 同 `preprocess_opoint` 的既有约定（现解析走 `Entity::gen_or` → `World::gen_field` 宿主缝）；差分两侧都剥掉 |
| `preprocess_frame` 不落地 `frame.sound` 的加载任务 | TS 往 `jobs` 里 `push(lfw.sounds.load(...))`，端口不写 | 加载副作用不是数据（同 `A_SOUND` 的既有约定） |
| `preprocess_frame` 的 `frame` 是标量 | TS 读标量属性不抛、第一次写（`frame.width ??= 0`）才抛；端口在入口直接 `false` | 不给标量模型挂属性；差分只比「失败」这一位，`frame` 为标量时两边都是「无副作用地失败」 |
| `cook_frame_indicator_info` 的失败用 `bool` 返回 | V41 版是 `void`：`"w" in pic` 对原始值会抛、`?.forEach` 对非数组会抛、给标量挂 `__indicator_info` 会抛 —— 端口当时静默跳过 | 该函数只有 `preprocess_frame` 一个调用点；改成 `bool` 后两边对齐（数组目标仍算「成功」：TS 能挂属性，而渲染里看不见） |
| `read_nums` 的失败用 `bool` + `error` 出参 | TS 抛 `Error`（`[read_nums] failed, …`）；端口返回 `false`，文本逐字对齐 | 端口不用异常；`preprocess_frame` 的 `center` 会把这段文本透给 `msg=` |
| `traversal` 的**值版**补上字符串分支 | TS `Object.keys("ab")` 是 `["0","1"]` ⇒ 字符串也会走回调（`base.files` 就是这种输入）；端口原来对字符串直接返回 ⇒ 少走回调 | 这一刀按 `traversal_write` 的写法补齐（值版不写回、给的是那一个字符）；值本身在本 subject 里不可观察 |
| `preprocess_entity_data` 的 `__pre_hitkeys_map` / `__post_hitkeys_map` 用 `Object` 顶替 `Map` | TS 用 `new Map()`（键序 = 首次插入序、重复键不增 size） | `core` 没有 `Map`；`renderValue` 对两者输出同形（`{k:v}`），用例里的键都是短下标（不含整数键前置的那种排序差异） |
| `preprocess_entity_data` 不落地加载任务与 `data.xml` | TS 往 `jobs` 里 `push(images.load_img(...))` / 调 `sounds.load(...)`、末尾把 `xml` 挂成函数 | 副作用不是数据、函数装不进 `Value`（DESIGN §9.42）；端口只保留这些调用点的**失败面**（`jobs.push` / `jobs.length` / `Promise.all` 的可迭代要求 / `errors.length`） |
| `preprocess_entity_data` 的 `errors` 只做「缺失就失败」 | TS 有 `errors` 数组、末尾 `if (errors.length) Ditto.warn(errors)`；端口用 `error` 出参（单条）且不落地 `Ditto` | 与 `preprocess_bdy` / `preprocess_itr` / `preprocess_frame` 的既有约定一致（前述）；差分不比较 `errors` |
| `make_ball_special` 对稀疏输入不抛（**3t 遗留**） | TS 在 `id "220"` 这类分支上读 `data.frames[50]` 抛；端口的 `make_ball_special(Value&)` 是 `void`（没有失败通道）⇒ `frames` 缺失时跳过 | 会在 `preprocess_entity_data` 的 ball 分支暴露（`ed` 用例挑表里能跑通的 id，见 `mutations/preprocess_entity_data.mjs` 头部）|
| `make_fighter_special` 的 `ensure(data.base.group, …)`（**3y 遗留**） | TS 的 `ensure(output, item)` 对**非数组真值**（如 `group: 1`）会 `1.push(...)` 抛；端口 `ensure.h` 把非数组一律换成新数组 | 同上：`make_fighter_special(Value&)` 返回 `Value`、没有失败通道 ⇒ `preprocess_entity_data` 的 fighter 分支只能照端口语义走；`ed` 用例的 `group` 一律给数组 |
| `I18N` 的三张表用 `std::map` 顶 `Map` | TS 是 `new Map()`；端口 `_words` / `_lists` / `_alias_map` 都是 `std::map` | `core` 没有 `Map`；三张表都只做「按键取」、从不迭代 ⇒ 序不可观察（同 `NestedMap` / `preprocess_entity_data` 的既有约定）。`Map` **不做** ToString（数字键 ≠ 字符串键）这条语义靠「语言必须是字符串」保住：`words_of` / `lists_of` / `alias_of` 对非字符串语言一律当查不到；词键那一边照 JS 做 `to_string`（`key_at`） |
| `I18N` 不实现 `base_words` / `base_lists` 两个 getter | TS 里是 `this._words.get('')!` / `this._lists.get('')!` | `src/LFW` 里没有使用者（只有 `I18N.ts` 自己的声明），差分用例也不观察；同其它未移植槽的约定 |
| `PlayerInfo` 的字段用 `Value` | TS 的 `_info.name` 是 `string`、`local` / `mine` 是 `boolean`、`ctrl` 是 `CtrlDevice`、`keys` 是 `Record<GameKey,string>`，运行时都可能是别的 | 端口保留原值（同 `Entity::_team` 的既有约定）：构造参数 / `load` 的 JSON5 / 台面都可能给非类型值，用例专门喂 `null` / 数字 / 数组 |
| `PlayerInfo.loaded` 是**挂起**的 | TS 构造里 `this.loaded = this.load()`：那次 load 到第一个 `await` 之后才真正生效，所以构造后立刻注册的监听者也能收到 `load` 期间的回调；端口把那次 load 挂起，第一次 `loaded()` / `load()` 才落地 | 端口没有 promise / 微任务；挂起模型与 TS 的时序等价（台面里 `new` 之后都会读一次 `loaded()`） |
| `on_ctrl_changed` 只传两个参数 | TS 是 `(value, prev, player)`，第 3 个是 `PlayerInfo` 实例 | `Value` 装不下对象实例（DESIGN §9.42）；台面只比前两个参数 |
| `Ditto.warn` 只保留**第一个**参数 | TS 是 `warn(tag, ...)`；端口 `IPlayerInfoHost::warn(text)` | 差分只比第一条文本（`[PlayerInfo::load] …`），其余是日志上下文 |
| `Ditto.Cache.put` 没有失败面 | TS 的 `put(...)` 没 `await`（rejection 无人接）；端口 `cache_put` 返回 `void` | 与 TS 一样「写了就算」；`del` 仍有 `bool` 失败面（TS 那边是 `await` + try） |
| `Ditto` 没装好时不建模 | TS 在 `Ditto.Cache` 是 `undefined` 时会于 `get` / `del` 上抛 `TypeError`；端口假定宿主已装好 `cache_get` / `cache_del` | 台面一定装好；真实调用点（`DatMgr` / `stage`）也在装好 `Ditto` 之后才 `new PlayerInfo` |
| JS 属性读写语义的少数档没建模 | 端口的 `Array`（`std::vector`）装不下「挂在数组上的字符串键」与超长稀疏数组（`length` / 下标上限 1e6）；数组的「洞」在端口里是显式 `undefined`；字符串只认 `length` 与下标两种自有属性 | 这些档要 `_info.keys` 不是普通对象才碰得到（`PlayerInfo` 里造不出来）；`prop_get` / `prop_set` 按 JS 语义保留，台面不喂这些输入 |
| `to_lower_case` 只覆盖部分 Unicode | 覆盖 ASCII / Latin-1（除 `×`）/ Latin Extended-A / 希腊 / 西里尔 + `U+0130` 两码点特例；表外码点原样返回 | 键名实际只出现在这几段；`İ` 一档照抄 JS（`i` + `U+0307`），用例覆盖到了 |
| `ditto/zip/IZip` 只建模 `name` / `file(string)` / `file(RegExp)` / `md5` | TS 还有 `files` / `set` / `blob` | 缺的三条没有调用点 ⇒ 按需不搬；`file(RegExp)` 只认移植触及的固定模式表（`std::regex` 被 lint 禁），表外模式按「全串相等」处理 |
| `IZip.file` 不给抛错面 | 真实 zip 是纯查表、不会抛；TS 里宿主实现若抛会往外传 | 台面不脚本化「宿主自己抛」这种情形 |
| `IZipObject` 不做 `blob` / `uint8_array` | TS 是八个成员（`name` / `text` / `json` / `blob` / `blob_url` / `array_buffer` / `uint8_array` / `image_bitmap`，读法全 async） | 端口有其余六个；缺的两条没有调用点 ⇒ 按需不搬（值形状偏差另见本表的 `array_buffer` / `image_bitmap` 两行） |
| `ILoadedZip` 按**值**存进列表 | TS 的 `add(zip)` 存的是同一个对象引用（`unshift`）⇒ 宿主事后改 `{zip, info}` 在 TS 里看得见、端口看不见 | 真实调用点 `LFW.ts` 是现造现加；台面不造这种情形 |
| `ZipMgr.md5s()` 给 `Value[]` | TS 声明 `string[]`，但实际原样返回 `info.md5`（`?? '` 只吞 `null` / `undefined`） | 端口给 `Value` 不丢信息 |
| `IDataInfo` 只建模自有 8 个字段、且 `info` 必须非空 | TS 是普通对象（可挂别的键），`info` 为 nullish 时读 `info.md5` 会抛 `TypeError` | 台面只喂 TS 声明里的字段 |
| `ditto::vec2` 只收数字 | TS 的 `new Vector2(x, y)` 默认参只吃 `undefined`；显式 `null` 会原样写进 `x` / `y` | `Camera` 只用无参与数字形式；台面只喂数字 |
| `_dested` / `_locked` 的分量只能是数字 | TS 里 `_dested.x` 是 nullish 时 `?? this.destination.x` 会走后半；`jump_*` 也能把 `undefined` 写进 `position.x` | 记在偏差表；台面只喂数字 |
| `Camera` 读 `World::world_stage()` / `world_bg()` / `world_dataset()` 必须回对象 | TS 里 `world.stage` / `bg` / `dataset` 是 nullish 时读属性会抛 `TypeError`；端口的 `field_or` 对非对象给 `undefined` | 台面只喂对象（`undefined` 字段走的是 `to_number` ⇒ NaN）；这三条原来是 `ICameraWorld` 缝，相机直接持真 `World*` |
| `modern_screen_height()` 找不到键时回 0 | TS 的 `Defines.MODERN_SCREEN_HEIGHT` 是模块常量（450），不可能缺 | 端口走运行时表，缺键时 `defines::num` 给 0（`to_number(undefined)` 会给 NaN） |
| `IVector2` 只实现 `x` / `y` / `set` | TS 的 three.js `Vector2` 还有 `add` / `sub` / `length` / `clone` / `normalize` / `equals` | `Camera` 只读写两项；随用到的物理刀再补 |
| `base/dedup.h` 是直通 | TS 的 `deduped` 用模块级 `Map<string, Promise>` 做**并发**去重（同 key 的调用共享同一次执行，settle 后删条目） | 同步端口没有交错 ⇒ 共享观察不到；连带 `dedup_key` 拼出的键字符串在端口里不参与逻辑（台面只做顺序调用） |
| `IZipObject` 的 `array_buffer()` 给字节数组 | TS 是 `ArrayBuffer`（宿主对象） | 端口的 `Value` 装不下宿主二进制类型；台面把两边都渲染成字节列表 |
| `IZipObject` 的 `image_bitmap()` 给 `Value` 标记 | TS 是 `ImageBitmap`（GPU 资源） | 同上；真实数据由后续渲染刀决定怎么接 |
| `Ditto.XML.parse` 的结果是 `Value` 标记 | TS 是 `IXMLElement`（`tag` / `children` / `attrs` / `text` / `parent` + 一批方法） | 只判它的真值；`IXMLElement` 的成员随用到的刀再补 |
| `Resources` 的 `ImportResult.file` / `origin` 是 `Value` | TS 里是 `file?: string` / `origin?: string` | 回退那条路**不写** `origin`、`import_image_bitmap` 回退时 `file` 是 `paths[0]` ⇒ 用 `Value` 才能把「缺席」与空串分开 |
| `Resources` 的五个 `import_*` 是同步 + `bool` 失败出参 | TS 是 `async`，宿主失败靠 promise reject 往外传 | 端口无异常/无 promise；失败面用 `bool` + `error`，文案与 TS 的 `Error.message` 逐字对齐 |
| `FactoryKey` 是 `Value`，表与池用 `std::vector<pair<…>>` 而不是 `Map` / `Set` | TS 的 `Key = string \| number \| symbol`，`Map` / `Set` 按 SameValueZero 判键（`NaN` 也是合法键） | `Value` 装不下 `symbol` 与「`NaN` 当键」；用 `std::vector` 是因为 JS 的 `Map` / `Set` 迭代是**插入序**且覆盖时位置不变（`std::map` / `std::set` 会按键序重排） |
| `ICtrlCreator` 用**指针身份**当键 | TS 直接拿 `ctrl.constructor`（类本身）当 `Map` 键 | `Value` 装不下类；注册点（`register_ctrl`）与归池点（`BaseController::creator()`）用的是同一个指针 ⇒ 等价 |
| `BaseController` 新增 `creator()` / `set_creator()` | TS 靠 `ctrl.constructor` 反查（语言自带反射） | C++ 无反射：`acquire_ctrl` 新建时记下 creator，`release_ctrl` 从实例取回 |
| `acquire_ctrl` 复用时不带参 `reset()`（`player_id` 单独写） | TS 是 `ret.reset(player_id, entity)` | 端口 `BaseController::reset()` 无参（既有约定）；顺序仍是先写 `player_id` 再 `reset()` |
| `Factory::set_warn` 是进程级 sink | TS 是全局 `Ditto.warn` | 端口没有单例容器；机制同 `IPlayerInfoHost::warn` 等既有缝 |
| `IEntityCreators` 是 `std::function<Entity*(World*, const Value&, state::States*)>`，`World` / `LFW` 只前置声明；`IBuffCreator::create` 返回 `buff::Buff*` | TS 是 `(world, data, states) => Entity` 与带 `static KIND` / `static GROUPS` 的类 | 不构造世界；creator / 注册信息由宿主提供，端口只透传 |
| `Factory._usedALIAS` 不移植 | TS 声明了 `protected static readonly _usedALIAS = new Set<string[]>()` | 全仓库没有调用点（同其它只声明不用的槽）；同族的 `create_entity_with_player` / `create_components` / `register_component` / `components` 都已搬 |
| `stage/Expressions` 存的是**副本**（`std::vector`），不是 TS 那个数组本身 | TS 的 `_list` 就是内部数组、`reset(list)` 传回 `list()` 时靠**同一性**早退 | 端口的同一性判定改成「传进来的正是 `list()` 返回的那一份」（不能比地址：临时量可能落在同一格栈上）；对外可观察的行为一致 |
| `stage/Expressions::index()` 是端口新增的只读口 | TS 的 `_index` 是 `protected` | 台面要观测游标；`is_first` / `is_last` 只给两位信息 |
| `bg` 的 `info` / `data` 是 `Value` | TS 是 `IBgLayerInfo` / `IBgData`（普通对象） | 与既有约定一致；`{...info, x}` 的浅拷贝在端口要**显式复制** `Object`（`Value` 共享 `shared_ptr`） |
| `bg/Background` 的 `middle` 是小结构 `Middle` | TS 是对象字面量 `{ x, z }` | 同 `ditto::vec2` 的先例；台面只读两项 |
| `bg/Background` 的 `near` / `far` 两个访问器叫 `near_plane()` / `far_plane()` | TS 字段名就是 `near` / `far` | 那两个名字是 `<windows.h>` 的宏，端口不能直接用 |
| `bg/Background` 读 `data.base` / `data.layers` 走 `field_or` | TS 里 `data.base` 是 nullish 时读 `info.name` 会抛 `TypeError`；`data.layers` 非数组时 `for...of` 会抛 | 端口给 `undefined` / 跳过（台面只喂「对象 base + 数组或 nullish layers」） |
| `helper/Randoming` 模板化成 `RandomingT<T>`（`Randoming` 仍是 `RandomingT<Value>` 的别名） | TS 的 `Randoming<T>` 本来就泛型，`Item.randoming` 是 `Randoming<Randoming<IEntityData>>` | `Value` 装不下「`Randoming` 实例」⇒ 加一个 `RandomingItem<T>` 特化（元素是 `std::shared_ptr<Randoming>` 时 `taken` 的缺省与 `v != taken` 都用指针） |
| `helper/Randoming` 的 `default_mt()` 放在 `randoming_default_mt()` 里 | TS 只有一个静态 `Randoming.mt` 实例（不随元素类型分家） | 模板静态成员会每个 `T` 一份 ⇒ 实例必须放在非模板函数里 |
| `stage/Item` 的 `entity_callback` 按实体各建一份 | TS 是**一个**共享对象，`e.callbacks.del(this.entity_callback)` 传的是对象本身 | 端口的 `Callbacks::on()` 返回 `Remover`、监听体由容器持有 ⇒ 用「两个 `Remover` + 实体指针」记账（效果等价：`objects.delete(e)` + 不再监听 e） |
| `stage/Item` 的析构会摘掉所有监听 | TS 里那个回调会**一直挂在实体上**（JS 对象不会被析构，实体还强引用着 `Item`） | 端口必须摘，否则实体会拿着悬空的 `this` |
| `stage/Item` 的 `x` / `y` / `z` / `enemy_*` 传给 `mt.range` 前会 `to_number` | TS 的 `mt.range(min, max)` 里 `max - min` 是数值强转、`floor(...) + min` 是 JS 的 `+`（`min` 是字符串时会拼接） | 端口 `range(double, double)` 只吃数字 ⇒ 台面不喂非数字（`x z` 这种 `null` 档两边一致，已覆盖） |
| `stage/Stage` 的 `callbacks` 用 `CallbacksT<StageCallbackArgs>` | TS 是 `Callbacks<IStageCallbacks>`，回调签名里有 `Stage` 本身 | `Value` 装不下 C++ 对象 ⇒ 给一个专用 payload（`stage` + 两个值位），同 `FSM` 用 `CallbacksT<FSM*>` 的先例 |
| `stage/Stage` 的 `phase.__end_testers` / `dialog.__end_testers` 由宿主 `IStageLfw::end_testers(owner)` 提供 | TS 里 dat 层已经把 `end_test` 编成了 `IExpression<Stage>` **实例**数组 | `Value` 装不下表达式实例 ⇒ 宿主按「那份数据」交出列表，`Stage` 只负责 `reset` / `flow`（台面把 `__test` 就地转成对象上的 `__end_testers` 序号、实例存在宿主表里 —— 与 TS 的引用语义一致，也不能拿对象**指针**当缓存键：地址会被复用） |
| `stage/Stage` 的 `IDialogState` 用值拷贝的 `DialogState` | TS 是 `{...prev, list}` 对象字面量（每次换新对象，旧的那份留给回调） | 端口按值拷贝 ⇒ `prev` / `curr` 天然是两份；`list` 也是拷贝（TS 里 `[...prev.list, ...more]`） |
| `stage/Stage::warn` 是进程级 sink | TS 是全局 `Ditto.warn` | 同 `Factory::set_warn` 的既有写法 |
| `stage/Stage::set_phase` 的同一性早退用 `same_ref` | TS 的 `phase === this.phase` 是**引用**比较 | `strict_equals` 对对象是**深比较** ⇒ 端口单独写 `same_ref`（对象比指针） |
| `stage/Stage` 的 `_disposers` 永远是空的 | TS 里声明了 `_disposers` 却**没有任何 push 点** | 照抄成员与 `dispose` 里的调用；这条路径不可达（记在 `mutations/stage_main.mjs` 头部） |
| `stage/Stage::change_bg` 在 `world.bg` 为 null 时不抛 | TS 里 `prev_bg.dispose()` 没有 null 守卫（`world.bg` 为 undefined 时抛 `TypeError`） | `World` 构造里先建了 `bg` 再建 `Stage` ⇒ 该路径不可达；端口给守卫（台面只喂「有 bg」的世界） |
| `World::render_worker_id_` 用 `0` 表示 TS 的 `undefined` | `Ditto.Render.add` 的句柄在端口里走 `clock_add`（槽空时回 0），TS 那边没装 Worker 就是 `undefined` | 台面把 0 打成 `u` 对齐；`stop_render` 的「有没有句柄」判断也因此按 `!= 0` 写 |
| `World::fps_value()` 落空回 `NaN` | TS 的 `get FPS()` 在 `sync_render` 不是五个枚举值时返回 `undefined` | 端口的分量是 `double` ⇒ 用 `NaN` 顶替，台面把两者都打 `u` |
| `World::on_step_error` 的 `Date.now()` 与 `e.errors` | TS 读全局 `Date.now()`、参数是异常对象（`Ditto.warn(e)` + `e?.errors`） | 端口把时间收进时钟槽（`clock_now()`，与 `render_once` 同一套）、把「有没有 `errors`」收成 `bool`，第二句告警文本固定 |
| `World::mark_players_alive(Entity&, bool)` | TS 是 `world.mark_players_alive(this, alive)`（实体自报） | 端口原来只有 `mark_players_alive(bool)`，宿主那面少一个实体参数 ⇒ 补上（`entity.h` 与台面假宿主同步改） |
| `World` 的 `Defines.RANDOM_BG.id` 走运行时表 | TS 里是模块常量（`"?"`） | 端口 `defines_value` 缺键时给 `undefined` ⇒ 台面把 `Defines.RANDOM_BG` 当普通数据喂，`change_bg` 的随机分支才能按同一个值判定 |
| `ditto/xml` 的两份 TS 实现选 **tool** 那份当默认 | 浏览器版（`src/DittoImpl/xml/XMLElement.ts`）是 DOM + `XMLSerializer` + `xml-formatter`；tool 版（`tool/src/xml/*`）是纯内存 + fast-xml-parser | 缝的本意是 宿主装服务（DESIGN §9.68）；tool 版无平台依赖、Node 侧能直接跑差分；两边语义差记在下面几条 |
| `ToolXMLElement.as_object` 保留 `type` 属性 | 浏览器版 `if (attr.name == 'type') continue` 跳过它 | tool 语义；`dat_translator/xml` 的读写都按 tool 走 |
| `get_str_arr` / `get_num_arr` **忽略 `or`** | tool 版 `return ret`（缺省就是 `undefined`）；浏览器版是 `ret ?? or` | 端口接口因此只给 optional 版（没有 `or` 重载）；调用点自己 `??` 兜 |
| `as_number()` / `as_boolean()` 恒走缺省（**tool 的死路径**） | tool 版先调 `as_string()`，而它按 `type == 'string'` 挡一道 ⇒ number / boolean 元素永不算值（探针实测 `<number type="number">7</number>` 的 `as_number()` 是 `undefined`）；浏览器版直接读 `value ?? text` 能算 | 照抄 tool；连带 `get_num` / `get_bool` / `get_num_arr` 的子元素分支只有属性回落半边可观察（记在 `mutations/xml.mjs` 头部） |
| `text` 是**自身**文本 | 浏览器版是 `textContent`（含后代文本） | tool 语义；`stringify` 的「有子元素时文本被丢」也照 tool |
| `set_text` 写自身文本 | 浏览器版 `textContent = …` 会替换子节点内容（DOM 行为） | tool 语义（同上一行的 §「自身文本」口径） |
| `insert` 不先摘旧父；`remove` 只认直接子 | 浏览器版 `insert` 先 `child._parent?.remove(child)`、`remove` 用 DOM `contains`（祖先/后代都算） | tool 语义；端口用 `shared_ptr` 装「同一元素挂两个父」 |
| `from_number` 写**文本**（`<number type="number">7</number>`） | 浏览器版写 `value` 属性（`from_string` 两边都写 `value`） | tool 语义；读回时 `attr('value') ?? text` 两边都能接 |
| `stringify` 是 fast-xml-parser 的 `XMLBuilder` | 浏览器版是 `XMLSerializer` + `xml-formatter`（形状差远了） | 端口复刻 `format:true` 的全部形状：2 空格缩进、行尾 `\n`、同名子元素按标签分组、空子元素整块丢、`<x></x>` 不压成 `<x/>`、转义只有 `& < > " '` 五个（见 DESIGN §84） |
| `ToolXMLElement::insert` / `remove` 的 `static_cast<ToolXMLElement*>` | —— | 缝上只有这一种实现；宿主换实现要另写 |
| `CMDS` 是**惰性注册**（首次查表时一次性登记全部 36 条） | TS 在模块加载期按模块逐个 `CMDS.register(...)`（也是 36 条） | 端口没有加载期钩子 ⇒ 表建在 `cmds.cpp`（`register_defaults`）、次序照 `index.ts`；两边的命令表逐条对齐 |
| `CMDS::handle` 对全空白命令直接跳过 | TS 的 `this.inst.words[0].toLowerCase()` 会抛 `TypeError` | 端口无异常；「共同可观测量不存在」不算覆盖缺口（记在 DESIGN §89 与变异档头部） |
| `CMDS::args` 用 `std::map` | TS 是对象字面量：撞原型键（`constructor` 等）会读到函数 | 数据不可达不建形；`eq > 0` / 末次覆盖 / 缺席给 `undefined` 三条语义照抄 |
| `CMDS` 的默认表 + 台面 `__probe__` 都走 `register_cmd` | TS 的注册是 `CMDS.register` 静态方法 | 形状一致（键统一降格、helps 另存）；台面私货用大写键注册以暴露注册侧降格 |

> `native.mjs all` 里的 `coverage` 步骤会跑 `tools/check_defines_coverage.mjs`：
> 它用「运行时枚举 TS 导出」这条**独立于生成器**的路径，检查 TS 里的枚举/字段表/schema 有没有漏搬
> （4AH 起 `src/LFW/ui` 里导出 `Schema_*` 的模块也在扫描范围内）。

**注意修正过的顺序**：`loader/preprocess_*.ts` 在运行时依赖 `dat_translator`
（`CondMaker` / `set_hit_flag` / `make_entity_special` / `xml_x_entity_data` /
`cook_ball_frame_state_*`），所以 `dat_translator` 里那块**必须和 `loader/` 一起搬**，
不能跳过。`ui/` 永远不要先搬。

### `--vs`：按 TS 的对战模式构造场景

`lfw_run --vs` 走的是 TS `GamePrepareLogic.start_game` 的**对战模式分支**（`is_stage_mode === false`）
加 `VsModeLogic.on_start`，与默认的闯关口径区分开：

| 项 | 闯关（默认） | `--vs` |
| --- | --- | --- |
| 队伍 | 宿主硬编 `"1"` / `"2"` | `lfw.new_team()`（自增 `team_N`） |
| 朝向 | 按序号 `i%2` | `mt.pick([Left, Right])` |
| 站位 | `x = -80 + 160*i`、`z = 0` | `x = mt.range(screen_w/3, 2*screen_w/3)`、`z = mt.range(bg.far, bg.near)`（`cam_x` 宿主恒 0：没有准备页相机） |
| hp/mp | 覆盖成 500/500 | **不覆盖**（保持 fighter 数据）＋ `mp = mp_max * 2 / 5` |
| 世界开关 | —— | `paused = false`、`dataset.playrate = 1`、`dataset.infinity_mp = 0` |

```
native\build\msvc-x64\bin\lfw_run.exe --root dist ^
  --zip native\build\host-data\prel --zip native\build\host-data\data ^
  --vs --stage 0 --fighter 1 --fighter 4 --frames 900 --every 300
```

⚠️ 闯关模式的**站位/朝向/队伍**仍是宿主自己的口径（TS 是 `mt.range(40, 80) + cam_x`、
`FacingFlag.Right`、`slot_info.team || new_team`），没有改 —— 一是保持 DESIGN §140/DESIGN §141 那两条摘要基线
可比（`d9b00db799724a2b` / `eeecc812075e8b74`），二是这块要按 TS 补齐属于独立的切片。
