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

具体来说，`native/lfw/` 里禁止：

| 类别 | 禁止 | 应该用 |
|---|---|---|
| 文件 | `<filesystem>` `<fstream>` `<cstdio>` | 注入的 `IImporter` |
| 流 | `<iostream>` `<sstream>` | 什么都不用（核心不打印） |
| 线程 | `<thread>` `<mutex>` `<atomic>` `<future>` | 无（核心是单线程的） |
| 时间 | `<chrono>` `<ctime>` | 注入的 `IClock` |
| 随机 | `<random>` | `core/mersenne_twister.h` |
| locale | `<locale>` `<regex>` | `core/js_string.h`（待创建） |
| 异常 | `throw` / `try` / `catch` | 返回 `std::optional` 或错误收集结构 |
| RTTI | `typeid` / `dynamic_cast` | 标签 + `switch`（对应 TS 的 `is_fighter()` 那套） |
| 舍入 | `std::round` `std::rint` `std::nearbyint` | `lfw::js_round()` |
| 格式化 | `std::to_string` | `lfw::js_number_to_string()`（待实现） |
| 链接 | `target_link_libraries(lfw_core ...)` 非空 | **保持为空** |

跑一遍检查：

```
node native/tools/native.mjs lint
```

---

## ★ 确定性：这是移植门槛，不是优化选项

`src/LFW` 是纯逻辑 + 定点化物理，两侧必须**逐位**一致，否则联机/回放会分叉。
已核到行的地雷清单：

| # | 坑 | 现有证据 | C++ 侧规矩 |
|---|---|---|---|
| 1 | **FMA 收缩** | — | MSVC 刻意不用 `/arch:AVX*`；Clang/GCC 必须 `-ffp-contract=off`。已在 `CMakeLists.txt` 里强制 |
| 2 | **`Math.round`** | `utils/math/base.ts` 里 `round = Math['round']` | JS 是 half-up **朝 +∞**（`Math.round(-0.5) === -0`）；`std::round` 是 half-away-from-zero，**必错**。用 `js_round()` |
| 3 | **NaN 比较器** | `World.ts:50` `x_sorter`：`const d = a.aabb_min_x - b.aabb_min_x; if (d !== 0) return d;` | NaN 时 JS sort 规范把结果当 `+0`；`std::sort` 收到 NaN 返回 = 违反严格弱序 = **UB**。C++ 比较器必须把 NaN 归一成 0 |
| 4 | **`std::sort` 不稳定** | `BotController.chasings.sort` / `CameraCtrl.fighters.sort` / `NearestTargets.targets.sort` / `ReachableGroup.reachables.sort` / `MoveTableLogic._steps.sort` 都没有 tie-breaker | V8 `Array#sort` 自 7.0 起稳定 → 必须 `std::stable_sort` |
| 5 | **`String(number)`** | 字段拼接、XML 输出 | 最短往返（`to_chars`），**不是** `to_string`；且 JS 的指数阈值（`1e21` / `1e-7`）与 `to_chars` 不同 |
| 6 | **`Object.keys()` 顺序** | `loader/DatMgr.ts:135/157`：`JSON.stringify(xml_roundtrip) === JSON.stringify(this)` | JS = 整数键升序 + 字符串键插入序 → `OrderedMap` |
| 7 | **字符串比较 = UTF-16 code unit** | — | `std::string` 是 UTF-8 字节序：ASCII 一致，**中文不一致**。参与排序的键要么证明全 ASCII，要么用 `utf16_compare` |
| 8 | **`localeCompare`** | `ui/component/DanmuGameLogic.ts:272` | locale 相关，C++ 无等价物 —— 需要定死 collation |

对应的自检工具：**差分测试台**（见下）。

---

## 目录

```
native/
  CMakeLists.txt
  CMakePresets.json
  cmake/toolchains/emscripten.cmake   # WASM（⚠ 未验证，需要 emsdk）
  lfw/                                # ★ 纯核心
    core/                             #   JS 语义底座（全新，TS 侧没有对应物）
      js_num.{h,cpp}                  #   js_round / floor_float / round_float / 位模式
      state_hash.{h,cpp}              #   跨实现一致的状态哈希（差分测试地基）
      mersenne_twister.{h,cpp}        #   ← utils/math/MersenneTwister.ts
    (待搬) utils/ defines/ base/ loader/ collision/ entity/ state/ stage/ bot/ ...
  tests/differential/                 # ★ 两侧对拍
    PROTOCOL.md                       #   用例 DSL + trace 格式 + 状态哈希字段顺序
    cases/*.txt
    mt_trace_main.cpp                 #   C++ 侧执行器
    mt_trace.ts                       #   TS 侧执行器（导入 src/LFW 的**真实实现**）
    run.mjs                           #   调度 + 逐行比对
  tools/
    native.mjs                        # configure / build / lint / test
    check_lfw_cpp_includes.mjs        # 分层规则检查
  host/                               # 宿主实现，镜像 src/DittoImpl/（尚未创建）
  ffi/lfw_c.h                         # 纯 C ABI 导出（尚未创建）
```

**命名约定**：C++ 侧文件名一律 `snake_case`，TS 侧保持 `PascalCase`。
这样 `World.ts ↔ world.h` 的映射是机械的，也天然避开 NTFS 大小写不敏感带来的冲突。
头文件统一 `.h`（因为 `ffi/lfw_c.h` 要能被 C 消费）。

---

## 怎么跑

```
node native/tools/native.mjs all
```

或者分开：

```
node native/tools/native.mjs configure   # 需要 VS 2022（脚本会自动找 vswhere）
node native/tools/native.mjs build
node native/tools/native.mjs lint
node native/tools/native.mjs test        # 差分测试
node native/tools/native.mjs test mt_basic   # 只跑一个用例
```

产物：

```
native/build/msvc-x64/bin/lfw_mt_trace.exe
native/build/gen/trace.mt_basic.{cpp,ts}.txt    ← diff 失败时看这两个
```

---

## 移植顺序

**不要按目录顺序搬。** 按「信息量 / 成本」排：

```
第 0 步  core/ + 差分测试台                                   ← 现在这里
第 1 步  utils/ + collision/          纯数学、零宿主、确定性最敏感
第 2 步  defines/ + base/Expression + loader/get_val_*（103 条 getter 表）
第 3 步  entity/ + World.step         高价值但最耦合，排最后

dat_translator/ 和 ui/                永远不要先搬（耦合高、确定性敏感度低）
```

**第 0 步为什么先测 MT19937：** 它是 `src/LFW` 里唯一一个两侧都已存在、
且输出可以逐位对比的东西。它同时覆盖 32 位整数运算（`>>>` / `<<` / 隐式
int32 截断）、一段非标准的双精度种子初始化、以及 `floor_float` 那层浮点量化。
如果这 1000 个数对不上，说明地基就有问题 —— **那比在 `World.step` 里发现
第 300 帧漂移便宜一万倍。**
