# 差分测试协议

目标：**同一份用例，TS 侧与 C++ 侧输出一致。**

TS 侧必须导入 `src/LFW` 下的**真实实现**（不是副本），C++ 侧是移植版。
任何一处语义偏差都会在文本 diff 里暴露出来。

---

## 0. 结构

```
tests/differential/
  run.mjs                      调度：自动发现 subject，两侧跑同一 case，逐行比对
  subjects/
    trace_util.h               公共工具（Line / hex16 / bits_hex / q_bits / split_ws）
    trace_util.ts
    <subject>.cpp              C++ 侧执行器 → CMake 自动生成 lfw_trace_<subject>.exe
    <subject>.ts               TS 侧执行器（esbuild 现场打包）
  cases/
    <subject>/<case>.txt       一个 subject 一个目录，可放多个 case 文件
```

`run.mjs` 用 `subjects/*.ts` 发现 subject（排除 `trace_util`），
再到 `cases/<subject>/` 找 case。所以**加一个 subject 不需要改 `run.mjs`，也不用改 CMake**。

---

## 1. case 文件（DSL）

### 1.1 通用规则

- `#` 到行尾是注释；空行忽略
- token 之间用任意空白分隔
- 缺省参数两边必须一致（见各 subject 的说明）

### 1.2 subject: `mersenne_twister`

```
seed  <number>                          # mt.reset(seed)；同时输出一行 run
int   <count>                           # 取 count 个原始 32 位整数
float <count>                           # 取 count 个 [0,1) 浮点（已量化到 1/1000）
range <min> <max> <count>               # 取 count 个 [min,max) 内的值
pick  <item> <item> ...                 # 用 items 建数组，pick 一次（不改数组）
take  <item> <item> ...                 # 用 items 建数组，take 一次（会移除元素）
state                                   # 输出当前 MT 状态哈希
```

### 1.3 subject: `math`

镜像 `src/LFW/utils/math/*`。**参数缺省值必须与 TS 完全一致**，否则会假报差异。

```
clamp            <value> <lo> <hi>
clamp_add        <value> <offset> <lo> <hi>
normalize        <n> [<p>]                  # p 缺省 = 1000
float_equal      <x> <y>
equal            <x> <y>
eqgt             <x> <y>
eqlt             <x> <y>
range            <from> <to> [<gap>]        # gap 缺省 = 1
probability      <times> <p>
normalize_plane  <a> <b> <c> <d>
calc_plane       <x1> <y1> <z1> <x2> <y2> <z2> <x3> <y3> <z3>
line_plane       <a> <b> <c> <d> <x1> <y1> <z1> <x2> <y2> <z2> [<is_direction>] [<is_segment>]
project_to_line  <x> <y> <m> <n>
alias_normalize_plane  <a1> <b1> <c1> <d1> <a2> <b2> <c2> <d2>
alias_calc_plane       <18 个数>
alias_line_plane       <20 个数>
```

**`throw` 的映射**：TS 侧 `range` / `project_to_line` 会 `throw`，
C++ 侧返回 `std::optional` 的 `nullopt`。执行器把两者都渲染成 `null`。

**`alias_*` 是干什么的**：`normalize_plane` / `calc_plane` / `line_plane_intersection`
在 TS 里都返回**模块级共享的可变单例对象**。C++ 侧必须镜像这一点（返回 `const Plane*` /
`const Vec3*` 指向文件级 static），否则"先拿引用、再调用一次、回头读第一次的引用"这种
用法会静默出错。`alias_*` 连续调用两次并**同时输出两次捕获到的引用**：

- 若两侧都是共享单例 → 两次输出都等于**第二次**的结果
- 若 C++ 改成按值返回 → 第一次输出会是**第一次**的结果 → 差异暴露

### 1.4 subject: `utils`

镜像 `src/LFW/utils/`（`math/` 之外的部分）。

```
ease_linearity            <factor> [<from>] [<to>]
ease_linearity_backward   <v> [<from>] [<to>]
ease_in_out_sine          <factor> [<from>] [<to>]
ease_in_out_sine_backward <v> [<from>] [<to>]
ease_in_out_quint         <factor> [<from>] [<to>]
ease_in_out_quint_backward <v> [<from>] [<to>]
cross_bounding            <l0> <r0> <t0> <b0> <n0> <f0> <l1> <r1> <t1> <b1> <n1> <f1>
utf8_encode               <u16hex>...
utf8_decode               <bytehex>...
times_new                 [<min>] [<max>]
times_set_range           <a> <b>
times_set_lifes           [<v>]
times_set_min             <v>
times_set_max             <v>
times_set_value           <v>
times_reset
times_reborn
times_state
times_add                 <d>
times_write_nums
times_read_nums           <v> <min> <max> <lifes> <remains>
times_snapshot
times_read_snapshot       <v> <min> <max> <lifes> <remains>
```

缺省值：`<from>` = 0，`<to>` = 1；`times_new` 的 `<min>` = 0、`<max>` = `Number.MAX_SAFE_INTEGER`；
`times_set_lifes` = `-1`。

**名字映射**：TS 把 `backward` 挂在函数对象上（`ease_linearity.backward`），
C++ 侧是独立函数 `ease_linearity_backward`。TS 里 `times.min` / `times.max` /
`times.value` 是 getter+setter，C++ 里是 `min()` / `set_min()`。

**`utf8_encode` / `utf8_decode` 用十六进制 token 传参**，因为需要能表达
孤立代理项（`d83d`）、非法前导字节（`f8` / `ff`）这类 UTF-16 / UTF-8 的边角。
输出是**十进制**的字节值 / UTF-16 code unit 值。

---

## 2. 输出 trace

每行一个 token 序列，token 之间**单个空格**，行末**无空格**。
浮点一律输出 **IEEE-754 位模式的 16 位小写十六进制**（不做十进制格式化 ——
格式化本身就会引入差异）。

`null` 表示"无结果"（TS 的 `null` / `undefined` 或 `throw`，C++ 的 `nullopt` / `nullptr`）。

| subject | op | 输出 |
|---|---|---|
| mersenne_twister | `run` | `run <bits16>` |
| | `state` | `state <hex16>` |
| | `int` | `int <decimal>` |
| | `float` | `float <bits16>` |
| | `range` | `range <bits16min> <bits16max> <bits16result>` |
| | `pick` / `take` | `<op> <bits16\|-> <remaining>` |
| math | `clamp` `clamp_add` `normalize` | `<op> <bits16>` |
| | `float_equal` `equal` `eqgt` `eqlt` | `<op> <true\|false>` |
| | `range` | `range <count> <bits16>...` 或 `range null` |
| | `probability` | `probability <bits16>` |
| | `normalize_plane` | `normalize_plane <bits16> <bits16> <bits16> <bits16>` |
| | `calc_plane` | `calc_plane <bits16>×4` 或 `calc_plane null` |
| | `line_plane` | `line_plane <bits16>×3` 或 `line_plane null` |
| | `project_to_line` | `project_to_line <bits16> <bits16>` 或 `project_to_line null` |
| | `alias_*` | `<op> <两次捕获到的值依次输出>` |
| utils | `ease_linearity` / `_backward` | `<op> <bits16>` |
| | `ease_in_out_sine` / `_backward` | `<op> <bits16>`（量化） |
| | `ease_in_out_quint` / `_backward` | `<op> <bits16>`（量化） |
| | `cross_bounding` | `cross_bounding <bits16>×6` |
| | `utf8_encode` | `utf8_encode <byteCount> <byteDecimal>...` |
| | `utf8_decode` | `utf8_decode <u16Count> <codeUnitDecimal>...` |
| | `times_*` | `<op> <bits16>×5`（`times_add` 前面多一个 `<true\|false>`） |

### 2.1 判据：量化后再比（**不是**原始位模式）

**跨语言不要求原始浮点位级相同。** 判据是：

> 经过 `round_float(v)` 之后一致 —— 也就是 `Math.round(v * 1000) / 1000`。

两侧各自调用**自己那份真实实现**：

| | 实现 |
|---|---|
| TS | `src/LFW/utils/math/round_float.ts` |
| C++ | `lfw::round_float()`（`native/lfw/utils/math/round_float.h`） |

理由：`round_float` 是**游戏自己的可观测精度**。两个值量化到同一个格点，
对游戏而言就是同一个值；量化后不同，才是真的不同。
另外 `Math.round` 的半值规则（平局朝 +∞，且 `|x| < 0.5` 返回带符号 0）
本来就与 `std::round` 不同，所以这一步本身也在校验 `js_round`。

**量化只用在需要它的地方**，不是无差别套用：

| 类别 | 判据 | 为什么 |
|---|---|---|
| `mersenne_twister` 的 `float` / `range` 结果 / `pick` / `take` | 量化 | 浮点通道 |
| `mersenne_twister` 的 `int` / `state` / 剩余长度 | **逐位精确** | 整数与内部状态，是最后一道防线 |
| `math` 的 `probability` | 量化 | 内部走 `pow`（libm），跨实现不可保证 |
| `math` 其余全部 | **逐位精确** | 纯 IEEE 算术（`+ - * /`、比较、`std::floor`），精确是可达且是应有的标准 |
| `utils` 的 `ease_in_out_sine` / `_backward` | 量化 | `cos` / `acos` |
| `utils` 的 `ease_in_out_quint` / `_backward` | 量化 | `pow` |
| `utils` 其余全部（含 `ease_linearity` / `cross_bounding` / `utf8_*` / `times_*`） | **逐位精确** | 纯 IEEE 算术或整数 |

`-0` 与 `+0` 位模式不同（`8000000000000000` vs `0000000000000000`），
所以位模式输出顺带锁住了符号零的行为。
`math/scalar` 里刻意放了一批 `-0` 用例。

---

## 3. 状态哈希的字段顺序（**这是协议**）

FNV-1a 64（offset basis `0xcbf29ce484222325`，prime `0x00000100000001b3`）。
每个字段按**小端字节**喂入。

```
u32 matrix
u32 upper_mask
u32 lower_mask
u32 index
f64 seed          ← 注意是 f64，不是 u32（TS 存的是原始 number）
u64 times
u32 mt[0]
...
u32 mt[623]
```

改这个顺序 = **两侧同时改**，否则差分测试会报"状态不同"。
实现：C++ 见 `native/lfw/core/state_hash.{h,cpp}` + `mersenne_twister.cpp`
的 `state_hash()`；TS 见 `subjects/mersenne_twister.ts`。

---

## 4. 怎么跑

```
node native/tools/native.mjs test                 # 全部 subject、全部 case
node native/tools/native.mjs test mersenne_twister
node native/tools/native.mjs test math plane      # subject + case 名
```

产物落在 `native/build/gen/`：

```
trace.<subject>.<case>.cpp.txt    C++ 侧输出
trace.<subject>.<case>.ts.txt     TS 侧输出
```

diff 失败时脚本会打印**第一处不同的行号**与两侧内容 —— 那就是漂移点。

---

## 5. 加一个新的 subject

1. 写 `subjects/<name>.cpp`（`#include "trace_util.h"`）+ `subjects/<name>.ts`
   （`import ... from "./trace_util"`）
2. 建 `cases/<name>/*.txt`
3. `node native/tools/native.mjs build` —— CMake 的 glob 会自动生成 `lfw_trace_<name>.exe`

不用改 `run.mjs`，不用改 `CMakeLists.txt`。

---

## 6. 变异测试（每次加 subject 都该做）

差分测试"通过"本身不证明它有效。**必须故意写错一处，确认能被抓到。**

已验证过的：

| 变异 | 结果 |
|---|---|
| MT `twist()` 的 `x >> 1` 写成 int32 右移 | FAIL，第 7 行（前 5 个输出完全一样 —— 只有 `mt[i]` 最高位为 1 时才显形） |
| MT `next_float()` 加 `+1e-15`（亚量化） | PASS —— 容忍度确实生效 |
| MT 量化格点从 1/1000 改成 1/100 | FAIL，第 10 行（第一个 `float`） |
| `line_plane_intersection` 的 `is_segment` 去掉 eps 容差 | FAIL，第 36 行（`t = -1e-20`，落在 `[-eps, 0)` 内） |
| `utf8.cpp` decode 的 4 字节前导判定 `(b0 & 0xf8) == 0xf0` 放宽成 `b0 >= 0xf0` | FAIL，第 32 行（`f8 80 80 80 80` 本该被跳过） |
| `times.cpp` `set_range` 的 `_value = a` 改成 `_value = _min` | FAIL，第 7 行（`times_set_range 10 5`） |

**最后一条值得记一笔**：最初写的边界用例是错的（算出来 `t = 0.5`，离边界很远），
变异**没被抓住**。原因是 `t > 1+eps` 与 `t > 1` 只在 `1 < t <= 1+eps` 时才有区别，
而在 1 附近 double 的间距恰好等于 eps —— **只有 `t = 1.0000000000000002` 一个值可分**。
负侧宽松得多（0 附近是次正规数，`[-eps, 0)` 里有很多 double）。
所以边界用例是**搜出来的，不是猜出来的**。
