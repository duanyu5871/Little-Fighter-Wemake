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

### 1.5 subject: `collections`

镜像 `base/Graves` + `utils/array/*` + `utils/container_help/*`。

```
graves_new
graves_add            <v>
graves_take
graves_l

filter_gt             <t> <x>...
find_gt               <t> <x>...
find_last_gt          <t> <x>...
fisrt_gt              <t> <x>...
last_gt               <t> <x>...
map_no_void_gt        <t> <x>...
intersection          <x>... | <x>...
ensure                [<x>...] | <x>...
loop_offset           <current> <offset> <x>...
make_arr              <size>
map_arr_mul           <k> <x>...
map_arr_scalar        <k> <x>
loop_arr_idx          <x>...
loop_arr_scalar       <x>

nested_set            <k1> <k2> <v>
nested_get            <k1> <k2>
nested_has            <k1> <k2>
nested_del            <k1> <k2>
nested_clear
nested_multi_add      <k1> <k2> <v>
nested_multi_first    <k1> <k2>
nested_multi_has      <k1> <k2>
nested_multi_collect  <k1> <k2>
nested_multi_del      <k1> <k2>
nested_multi_clear
```

**谓词词汇表**：为了避免在 case 里写函数，谓词统一是“大于阈值”：

| 后缀 | 实际谓词 |
|---|---|
| `filter_gt` / `find_gt` / `find_last_gt` | `v => v > t` |
| `fisrt_gt` / `last_gt` | `v => v > t ? v : undefined` |
| `map_no_void_gt` | `v => v > t ? v * 2 : undefined` |

`|` 是两个数组的分隔符。`ensure` 的左侧为空 = output 是 `undefined`；
**右侧不能为空**（TS 的 `item` 是必填项）。

`make_arr` / `map_arr_*` / `loop_arr_*` 用的是恒等 / 乘常数这样的小函数 ——
测的是**辅助函数的机制**（含 `map_arr` / `loop_arr` 的标量分支），不是那个 fn。

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
| collections | `graves_add` | `graves_add <l.length>` |
| | `graves_take` / `graves_l` | `<op> <bits16\|->...`（`graves_l` 前面多一个长度） |
| | `filter_gt` `map_no_void_gt` `intersection` `ensure` `make_arr` `map_arr_*` `nested_multi_collect` | `<op> <count> <bits16>...` |
| | `find_gt` `find_last_gt` `fisrt_gt` `last_gt` `loop_offset` `nested_get` `nested_multi_first` | `<op> <bits16\|->` |
| | `nested_has` `nested_del` `nested_multi_has` `nested_multi_del` | `<op> <true\|false>` |
| | `loop_arr_idx` `loop_arr_scalar` | `<op> <count> <idxDecimal>...` |

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
| `collections` 全部 | **逐位精确** | 下标 / 计数 / 整数，没有浮点运算 |

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
| `graves.h` `add` 的 `_l[--_i]` 改成 `_l[_i--]` | FAIL（**崩溃**，不是漂移 —— 索引回绕后越界） |
| `nested_map.h` `clear()` 去掉 `kv.second.clear()` | FAIL，第 29 行（回收的内层 map 带着旧键） |

### 6.1 `native/tools/mutate.mjs`

每次手改编译再改回来太容易出错（忘了还原、锚点匹配到多处），所以有了运行器：

```
node native/tools/mutate.mjs native/tests/differential/mutations/<subject>.mjs
```

规格文件 `default export { subject, mutations: [{ note, file, from, to }] }`。
运行器会：先确认每条 `from` 在目标文件里**恰好出现一次**（否则直接报锚点数并退出，不猜），
跑一次基线确认本来是绿的，然后**逐条**：改 → `build` → `test` → 还原。
编译失败也算 `killed`（说明变异打到了不可编译的地方，不是有效变异，要换）。
结尾统一还原并重建，最后输出 `N/总数 killed`，有存活就 exit 1。

存活（`SURVIVED`）意味着**用例没有鉴别力**，要补边界用例，而不是放过。

### 6.2 `json5`（`JSON5.parse`，12 条全杀）

| 变异 | 抓它的用例 |
|---|---|
| `default` 丢 `case '/'`（不进注释） | `p5 "//c\n1"`、`/*c*/1` |
| `peek()` 不合并代理对 | `p5 "{\uD835\uDC00:1}"` |
| `read()` 代理对只前进 1 | `p5 "{\uD835\uDC00:1}"`（键被截断成孤立低代理） |
| `fail_identifier()` 丢 `column -= 5` | `s5 "{\\u0067:1}"`（错误列号 6→1） |
| `escape()` 的 `\0` 后不检查数字 | `p5 "\"\\01\""` |
| `zero` 丢掉 `sign` | `p5 "-0"`（dump 里数字打**位模式**，`-0` 才可分） |
| `value` 把 `+` 当负号 | `p5 "+123"` |
| `string` 引号闭合判断反转 | `p5 "{\"a\":1}"` |
| `afterPropertyValue` 漏 `}` | `p5 "{a:1}"` |
| `hexEscape` 第二位不校验 | `p5 "\"\\x4\""`（补的用例） |
| `unicodeEscape` 少读一位 | `p5 "\"\\u123\""`（补的用例） |
| `identifierNameStartEscape` 不校验码点类别 | `s5 "{\\u0067:1}"`（`g` 不是 ID_Start） |

### 6.3 `json5`（`JSON5.stringify`，12 条全杀）

| 变异 | 抓它的用例 |
|---|---|
| 引号选择反转（`'` ↔ `"`） | `q5 "a"` |
| 不转义选中的引号 | `q5 "a'b\"c"`（1 个 `'`、1 个 `"` → 选 `'`，必须转义） |
| `\0` 后跟数字不特判 | `q5 "a\u00001b"`（该输出 `'a\x001b'`） |
| 空格也被 `\x` 化（`c < ' '` → `c < '!'`） | `q5 "\u0020"` |
| 键的后续字符用 `id_start` 而非 `id_continue` | `w5 "{a1:1}"` |
| 键首字符长度恒为 1（不识别代理对） | `w5 "{\uD835\uDC00:1}"` |
| `cp_at` 不合并代理对 | `w5 "{\uD835\uDC00:1}"` |
| 对象成员漏逗号 | `w5 "{a:1,b:2}"` |
| 冒号后多一个空格 | `w5 "{a:1}"` |
| 字符串值不加引号 | `w5 "{a:'x'}"` |
| 布尔值互换 | `w5 "true"` |
| `null` 序列化成空串 | `w5 "null"` |

`q5` 是"把一段字符串直接 stringify"，`w5` 是"解析再序列化"，`c5` 验循环引用。
**`w5` 顺带覆盖了 `number_to_string`**（`-0` → `0`、`1e21` → `1e+21`、`0x10` → `16`）。

### 6.4 切词器曾经在静默地截断参数（重要）

`split_ws` 原来按空白切，**不认引号**。后果：

- `p5 "[1, 2]"` 被切成 `p5` / `"[1,` / `2]"` —— 两侧拿到的是**同一个被截断的**
  参数，所以对拍照样"通过"，只是**根本没测到想测的东西**。
- 改成引号感知后，`json/parse` 的输出行数从 **150 → 157**，说明原来确实有 7 行
  在测残料。（`json5/parse` 也从 304 → 302。）
- 同一处还有 `#` 注释剥离：原来是 `replace(/#.*$/, "")`，引号里的 `#` 也会被砍。现在只在引号外识别。

**教训**：对拍两侧用同一个有缺陷的解析器时，"通过"是假的。
**测试基础设施本身的缺陷不会被对拍发现** —— 只能靠"输出行数/覆盖范围符合预期"来察觉。
所以每次加 subject 时顺手核对一下行数是否合理，是有价值的。

（另外：用例文件里的**真实换行**仍然不可用 —— `readCaseLines` / `std::getline`
先按行切。控制字符一律写成 `\uXXXX` 转义，由 `parse_js_string_literal` 还原。）

`json5` 的 TS 侧参考实现就是 `node_modules/json5` 本身 —— **它是要对齐的真相源，不是待测对象**。
被验的是 `native/lfw/core/json5.cpp` 与它的逐字节一致。

顺带一条：移植 `lib/parse.js` 的 `string` 状态时我"记得"上游允许字符串里出现裸换行，
差点把 `fail_char` 改成放行。查源码才发现**上游就是报错的**（只有 `\u2028`/`\u2029` 放行并 warn）。
**差分测试的价值之一就是阻止你"修"一个不是 bug 的地方。**

倒数第二条值得记：`Graves::add` 的 `_l[_i--]` 会让 `_i` 回绕成 `SIZE_MAX`，
下一次访问直接越界崩掉。**这是我自己写的 bug**，`/W4` 没报，差分测试第 6 行就抓到了。
顺带说明：差分的失败形式有两种 —— “漂移”（能逐行定位）和“崩溃”（只能知道哪一侧挂了），
两者都是有效的失败信号。

**另外两条的教训**：

- `nested_map.h` 的那条最初**没被抓住** —— 因为当时用例里 `clear()` 之后设的键
  正好是之前设过的键，回收的脏 map 被覆写了。补上“`clear()` 后设一个**不同**的键，
  再查旧键应当不存在”才变可区分。**回收池（`Graves`）的 bug 必须用“回收后访问旧键”来抓。**
- 两个变异一起打上去时，二者都报 “C++ failed” 而分不清是谁 —— **变异测试要一个一个来。**

P.S. TS 侧那个 `Times.lifes` 的无限递归（`return this.lifes`）就是在写 `times` subject 时
被执行器直接撞出来的 —— `RangeError: Maximum call stack size exceeded`。

**最后一条值得记一笔**：最初写的边界用例是错的（算出来 `t = 0.5`，离边界很远），
变异**没被抓住**。原因是 `t > 1+eps` 与 `t > 1` 只在 `1 < t <= 1+eps` 时才有区别，
而在 1 附近 double 的间距恰好等于 eps —— **只有 `t = 1.0000000000000002` 一个值可分**。
负侧宽松得多（0 附近是次正规数，`[-eps, 0)` 里有很多 double）。
所以边界用例是**搜出来的，不是猜出来的**。
