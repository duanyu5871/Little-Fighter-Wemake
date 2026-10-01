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
    utils/math/                      ← 镜像 src/LFW/utils/math/
      base.h                         ★ libm 单一收口（镜像 base.ts）
      floor_float.h  round_float.h
      clamp.h  clamp_add.h  float_equal.h  normalize.h
      normalize_plane.{h,cpp}  calc_plane.{h,cpp}
      line_plane_intersection.{h,cpp}  range.{h,cpp}
      probability.{h,cpp}  project_to_line.{h,cpp}
      mersenne_twister.{h,cpp}       ← utils/math/MersenneTwister.ts
    (待搬) utils/{container_help,schema,easing,...} defines/ base/ loader/
           collision/ entity/ state/ stage/ bot/ buff/ bg/ cmds/ controller/
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

---

## 移植进度

| 步骤 | 范围 | 状态 |
|---|---|---|
| 0 | `core/` + 差分测试台 | ✅ 通过（MT19937 逐位一致，7564 行） |
| 0.5 | 差分台子泛化为可插拔 subject | ✅ 通过 |
| 1 | `utils/math/` | ✅ 通过（`math/scalar` 90 行、`math/plane` 52 行） |
| 1.5 | `utils/{container_help,schema,easing}` | 待做 |
| 2 | `collision/` | 待做 |
| 3 | `defines/` + `base/Expression` + `loader/get_val_*`（103 条 getter 表） | 待做 |
| 4 | `entity/` + `World.step` | 待做（高价值但最耦合） |

**注意修正过的顺序**：`loader/preprocess_*.ts` 在运行时依赖 `dat_translator`
（`CondMaker` / `set_hit_flag` / `make_entity_special` / `xml_x_entity_data` /
`cook_ball_frame_state_*`），所以 `dat_translator` 里那块**必须和 `loader/` 一起搬**，
不能跳过。`ui/` 永远不要先搬。
