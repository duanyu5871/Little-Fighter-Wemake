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

`w5` / `p5` 的第一个 token 之后可以插一个**可选标签**（`w5 <label> <text>`），
两侧都会把它原样打进输出行。真实数据用例（`real_data*.txt`，114 个文件）就靠它定位是哪个文件出问题。

### 6.3.1 真实数据用例

`cases/json5/real_data.txt`（114 个文件，全部 `w5`）与 `real_data_tree.txt`
（<4KB 的 86 个文件，`p5` 逐节点）由 `native/tools/gen_json5_real_data_cases.mjs`
从 `lf2s/**/*.json5` 生成。

⚠ **这类用例要断言"确实解析成功了"**：如果 114 个文件全部报同一个错，两侧当然"一致"，
但测试什么也没验。核对办法：

```powershell
Select-String -Path native\build\gen\trace.json5.real_data.cpp.txt -Pattern ' perr ' -AllMatches
```

应当为 0，且 `ok=` 等于文件数。**任何"批量喂数据"的对拍都要做这一步自检。**

### 6.4 `fields`（19 条全杀）

`fields.ts` 是“字段描述 DSL + 字段表 + 校验”，C++ 侧直接用 `Value`/`Object` 实现
（TS 的 `Map` 用 `Object` 顶替），所以共用同一套值字面量 DSL。
TS 侧用**仓库里真正的** `src/LFW/fields.ts` 当参照。

| 变异 | 抓它的用例 |
|---|---|
| `w()` 的第二个字符串不进 `desc` 分支 | `fw "int" 2 s "T" s "D"` |
| `w()` 的 `desc` 追加改成覆盖 | `fw "int" 3 s "T" s "D" s "E3"` |
| `fields()` 的 `order` 从 1 开始 | 任意 `ff` |
| `fields()` 先补 `key`/`order` 再合并源对象 | `ff o 1 a o 2 key s "old" order n 9` |
| `reorder_fields` 排序方向反向 | `fr o 3 c n 1 a n 2 b n 3 o 3 a…2 b…0 c…1` |
| `order` 为 `undefined` 也算已知字段 | `fr o 2 a n 1 b n 2 o 2 a o 1 order u b o 1 order n 1` |
| 重排时不先清空（`set` 原地更新 ⇒ 序不变） | 任意 `fr` |
| `known` 初始顺序反转（暴露稳定排序） | `fr … order n 1 … order n 1`（相等） |
| `to_array` 只看 `null` 不看 `undefined` | `fa u` |
| `to_array` 对数组返回拷贝 | `fas a 2 n 1 n 2` |
| `Object.assign` 的字符串索引键全取第 0 个 | `ff o 1 a s "x"` |
| `nullable` 不看真值 | `fv o 1 a z o 1 a o 2 type s "int" nullable b 0` |
| `array === 'auto'` 失效 | `fv o 1 a n 1 o 1 a o 2 type s "int" array s "auto"` |
| `array === true` 用真值判定 | `fv o 1 a n 1 o 1 a o 2 type s "int" array s "no"` |
| int 丢掉整数判定 | `fv o 1 a n 1.5 o 1 a o 1 type s "int"` |
| int 的 `min` 用 `<=` | `fv o 1 a n 2 o 1 a o 2 type s "int" min n 2` |
| options 用宽松相等 | `fv o 1 a n 1 o 1 a o 2 type s "int" options a 1 o 1 value s "1"` |
| options 列表里 `undefined` 用 `json_text` | `fv o 1 a n 9 … options a 2 o 1 value n 1 o 2 label s "x" desc s "d"` |
| 未知字段不告警 | 任意带多余键的 `fv` |

**两个真 bug 是用例抓出来的，值得记**：

1. **`Array.prototype.join` 把 `undefined` 变成空串，模板字符串变成 `"undefined"`**。
   `field.options.map(o => JSON.stringify(o.value)).join(', ')` 在某个 option 缺 `value` 时
   产出 `"1, "`，而我第一版复用了同一个 helper 得到 `"1, undefined"`。
   ⇒ **错误消息的文本也在对拍范围内，不是"无关紧要的输出"**。
2. **TS 里 `typeof null === 'object'`**，所以 `w()` 的 `Object.assign` 分支会吃 `null` 与数组
   （数组会把下标变成键 `"0"`/`"1"`），而**字符串走另一个分支**（当 title/desc）。

**三条“TS 会抛异常”的输入不能入对拍**（C++ 无异常）：`reorder_fields(obj, null)`、
`object` 字段缺 `fields` 且值为非空对象、`validate_value` 的 `field` 为 `undefined`。
都记进 `README.md` 的已知偏差表。

### 6.5 `defines`（46 个枚举，6 条全杀）

这个 subject **不需要输入**（`cases/defines/all.txt` 只是占位）：它把全部枚举 dump 出来与 TS 侧对拍。
输出格式：

```
E <EnumName>
F <成员名> <值|"...">        成员（按成员名排序）
R <值> <名字>                数值枚举的反向映射（按值升序，去重）
```

C++ 侧完全由 `lfw/defines/all_enums.h` 的注册表驱动、TS 侧由生成的
`subjects/gen/defines_enums.ts` 清单驱动 —— **两边都没有手抄的枚举名单**。

| 变异 | 结果 |
|---|---|
| 生成的 `BdyKind::Defend = 2000` → `2001` | killed（`F` 行） |
| 生成的 `name_of` 少一个 case | killed（`R` 行少一条） |
| 生成的字符串枚举 `kTeam_8 = u"8"` → `u"9"` | killed（`F` 行） |
| 手写 `HitFlag::Both = Ally \| Enemy` → 再加 `\| Ball` | killed（位组合值错） |
| 手写 `FacingFlag` 的反向映射把 `SameAsBearer` 写成 `SameAsCatcher` | killed（别名「后者胜」语义） |
| 手写 `EntityEnum::Ball` 从 `HitFlag::Ball` 改常量 `31` | killed（跨枚举引用） |

**⚠ 教训**：第一版生成的头里**同时**写了枚举初始值和表里的字面量 ⇒
前 3 条变异**全部存活**（比对的只是那张表）。改成表引用枚举成员后全杀。
⇒ 打变异时如果要用"改值"来验，先确认被改的那处**就是**被比对的那处。

### 6.6 `defines_fields`（34 张字段表，5 条全杀）

不需要输入（`cases/defines_fields/all.txt` 只是占位）：每张表打一行 `T <name> <render>`。
C++ 侧读生成的内嵌 JSON5（`json5_parse`），TS 侧读真正的 `src/LFW/defines/*.ts`。

| 变异 | 结果 |
|---|---|
| 字段表里的 `order` 值改错 | killed |
| `options` 的某个 `value` 改错 | killed |
| 非 ASCII 标题改错（`??比例` → `XX比例`） | killed |
| 整条字段被删掉 | killed |
| 字段 `type` 改错（`float` → `int`） | killed |

这个 subject 的价值有两层：① 验生成器内嵌的数据与 TS 一致；
② 验 `json5_parse` 能原样还原这些真实数据（**非 ASCII + 键序 + 嵌套表**）。

### 6.8 覆盖检查是差分的必需补充

**差分只能比"两边都有"的东西。** 当 C++ 的数据是**生成**出来的、TS 侧的清单也出自同一个生成器时，
生成器漏掉的东西会被两边同时漏掉 —— 差分测试**看不见**。

所以凡是生成的数据，都要配一条**从权威实现出发的独立覆盖检查**：

```
node native/tools/check_defines_coverage.mjs      # 已接进 native.mjs all 的 coverage 步
```

它不走正则，而是**运行时 import 全部 `defines/**/*.ts`**、扫导出、按形状分类，再与 C++ 注册表比对。
这条路径立刻查出一个真漏洞（`export const enum TerrainEnum` 从未被生成，796 → 851 行）。

判断"形状"的规则（避免误报）：

| 形状 | 判据 |
|---|---|
| 字段表 | 名字以 `_fields` 结尾 |
| 数值枚举 | **同时**有 `名→数` 与 `数→名` 两类条目 |
| 文本枚举（待人工确认） | 所有值都是字符串 |
| 标签表 | `*Descriptions` / `*Labels` / `*Names` / `*_LABEL_MAP` / `*_DESC_MAP`，或**键全是数字串且值全是字符串** |
| 别名 | 与前面某个枚举**是同一个对象**（按同一性归组，任一名字被注册即算覆盖） |
| 忽略 | 数组 |

**别名必须按对象同一性归组**，否则 `TE`/`E_Val`/`WT` 这类 `export const X = Enum` 会被误报成缺失。

### 6.9.1 `base`（`NoEmitCallbacks` / `Callbacks` / `FSM`，21 条全杀）

- **自由文本必须用 `esc()` 而不是 `to_ascii()` 渲染。** 溢出告警里有 `U+2014`；
  `to_ascii` 是 `static_cast<char>(c)` ⇒ `0x2014` 被截成 `0x14`，C++ 侧打印成空、TS 侧打印 `—`，
  diff 就停在这一行。这是本轮**唯一**的漂移，改用两侧共用的 `esc()` 后 3127 行全对。
- **等价变异体**：`pending_count()` 的 `_pendings.size() - _head` 改成 `_pendings.size()` **存活**。
  不是用例没鉴别力，而是 `compact()` 在每次 flush 收尾 / 溢出返回前都把 `_head` 归零
  ⇒ flush 之外 `_head` 恒为 0，两种写法等价。换成「`compact()` 后不把 `_head` 归零」就能杀
  （20/21 → 21/21）。
  教训：变异存活先判断**是否等价**，再决定是补用例还是换变异点。
- 时序用例必须显式包含（少一个就会漏）：派发期间的 `add`（新 listener 当次**不得**触发）、
  `once` 在一次 flush 内只触发一次、`del` 后重加排到末尾、溢出告警的复位（第二次溢出要再报一次）、
  `enter` / `leave` 的先后。

### 6.9.2 `defines_runtime`（`Defines` 运行时数据，9 条全杀）

- **序列化保真度也是被测对象**：生成器最初用 `JSON.stringify`，`-0` 变 `0`、`NaN` 变 `null`，
  差分停在第 51 行（`VOID_BG.base.near`）。改成自家 JSON5 序列化（`-0`/`NaN`/`Infinity` 原样）后全对。
- **两侧的「条目顺序」是测试基建，不是数据**：C++ 侧按 `Defines` 名排序、TS 侧把 TOP_LEVEL 放在前，
  造成「第 2 行就漂移」的假象。两侧统一 `sort()` 后，差分才反映真实内容。
- **`parse_value` 的 `s` 是独立 token**：`s"1"` 会报 `bad value literal`，正确写法 `s "1"`。
  这条坑使「空字符串」很久没被真测到——`isind ""` 传的是 2 个引号字符 ⇒ `!== 1` 与 `> 1` 等价，变异存活。
- **「缺失的成员」用存在性表达**，不要让访问器伪造默认值：`num()` 对未知名字回 0 是刻意的
  （真实调用点都传已知名），差分里用 `has`（C++ `find() != nullptr`）表达 `Defines[k] === undefined`。

### 6.9.3 `cond_maker`（17 条全杀；4 个存活者的分析）

- **`_HAS_EXCEPTIONS=0` + 空 `std::function` = 无输出崩溃**：`CondMaker` 的默认 `_term`
  初始为空，调用它抛 `bad_function_call`，在 `_HAS_EXCEPTIONS=0` 下变成 fastfail（退出码
  `0xC0000409`）；而且管道下 stdout 是**块缓冲** ⇒ 连 `printf` 的内容都看不到。
  定位靠直接跑 exe 看退出码。修法：构造时就填入默认格式化器（不能依赖“用之前会设”）。
- **变异存活先分三类**：这次 4 个存活里 3 个是**用例缺口**、1 个是等价变异。
  1. `opok` 的 mask 收窄是**等价**的（`"!"` 只在中途出现，外部观察不到）
     ⇒ 换成「`add` 的允许集合漏掉 `&&`」，立刻可杀。
  2. 「`done` 不再对片段 trim」等价于末尾 trim —— 除非**中段**片段两端有空白。
     补 `add n 1 == n 1` + `and s " a" == s "b"` 后杀死。
  3. `js_trim` 的 U+3000 / U+FEFF 分支没被覆盖 ⇒ 补用例。
  4. 「子错误不再向上传播」没被杀，是因为用例用 `empty` 当“坏运算符”，
     但 `not`/`wrap`/`add` 的运算符是**裸 token** ⇒ `empty` 被当成合法运算符 `"empty"`。
     改用 nullish 操作数才能真正触发子错误。
- 教训：**用例里“构造非法输入”的手段本身也要验证**（先确认它真的非法）。

### 6.9.4 `labels`（15 条全杀）

- **`falsy` 与 `nullish` 回退会给相反结果**：`bdy_kind_name` 用 `if (!ret)`，
  `wpoint_kind_name` 用 `??` ⇒ 前者把 `BdyKind["Normal"]`（=0）当成未命中。
  这是差分第一轮就抳到的真差异（第 88 行）。
- **记忆化是可观察行为**：`get_hit_flag_name` 会把组合名写回源表，而且**键用原始值**。
  只用“多次调用结果一致”无法验证它 ⇒ 在用例里 `dump` 整张表（开头一次、中间一次）
  才能看到表长变化与键的形状。
- **枚举对象的正/反向键都是字符串**：数字枚举 `obj[0]` 与 `obj["0"]` 命中同一键，
  所以 `bd_kind_name("0")` 会得到 `"Normal"`、而 `bdy_kind_name("Normal")` 得到
  `unknown_Normal`（falsy）。用例两边都要写。

### 6.9.5 `dat_helpers`（19 条全杀；两个“假存活”的原因）

- **声明顺序 = 无限递归**：`is_num(const Value&)` 写成 `is_num(*d)` 委托给
  `is_num(double)`，但后者声明在**后面** ⇒ 普通名字查找看不到它 ⇒ `double` 隐式转成
  `Value` ⇒ **自己调自己** ⇒ 爆栈（表现为 `C++ failed` 且无输出）。修法：把 `is_num(double)`
  提到前面。教训：**C++ 重载不要“先写调用方后写被调方”**。
- **“存活”先看是不是等价变异**：
  1. 「`is_positive`/`not_zero_num` 允许 0」存活 —— 因为我在 `take_num_impl` 里把
     `> 0`/`== 0` 又**内联了一遍**，那两个谓词根本没被调。改成调用谓词后立刻可杀。
  2. 「`is_num` 不再要求有限」存活 —— 因为 `is_num(const Value&)` 把检查也**内联了一遍**，
     `is_num(double)` 不可达。改成委托后立刻可杀。
  ⇒ 规律：**同一规则只能写一处**，否则变异打在死代码上，永远杀不掉。
- `toFixed` 不能用 `x * 10^f` 近似：`2.675 * 100` 会被舍成 `267.5`，而精确值是
  `267.4999…` ⇒ 平局判错。用 `std::fma` 的精确残差定平局（见 `DESIGN.md` §4.17）。

### 6.9.6 `next_frame`（17 条全杀）

- **共享常量对象的身份也要对拍**：`Defines.NEXT_FRAME_*` 在 TS 里是**同一个对象**，
  调用方改它会“污染”后续调用。C++ 若返回副本，普通对比看不出差异 ⇒ 用例里先用 `nfmut`
  改写，再重新取一次，看改动是否仍在（两侧一致才过）。
  同理：`NEXT_FRAME_*` 路径**不走** cook（提前 return），所以要有一条
  `nfc n 1000 ...` 证明 costs 对它无效。
- **变异可能写成“编译错误”**：把 `costs->get(id)` 改成 `costs->get(raw)` 时，`raw` 并不在
  那个函数作用域里 ⇒ 报 compile-error。这类变异**不算有效度量**（不是测试抓到的），
  要改成同作用域内可编译的等价改写（如 `js_string_of(*idv)`）。
- **幸存变异先看“用例是不是真的走到了那条分支”**：`edit_next_frame` 只对单对象生效的变异
  幸存，因为我用例把**对象**传了进去，数组分支从未进入。补 `newarrobj`（池里直接放对象数组）后即可杀。

### 6.9.7 `colon_reader`（14 条全杀；一条“转写错”抳出来的真差异）

- **转写/记忆不可靠，差异用权威源码裁决**：我先把 `read()` 的删除语句记成了
  `slice(0, index) + slice(index + match.length)`，于是 C++ 写了“正常”的删除，
  差分报出 `rem` 不同（TS 那边反而**变长**）。没继续猜——直接重读 `ColonValueReader.ts`，
  发现原文是 `slice(0, index) + slice(match.length)`（从头部等长删）⇒ 照抄后 49 行全对。
  ⇒ **不要用“更合理”的行为代替原行为**；“删掉错误”必须删到与原代码逐字一致。
- **等价变异换成可观察的**：`名字后的 \s* 只跳一个空白` 与“跳过整段”不可区分
  （空白数≠1 时两者都失败，=1 时也相同）⇒ 换成“完全不跳空白”，立刻可杀。
- **测试自己的断言也要能区分**：`edit_next_frame`、`\s*` 两次都因为“用例没真正走到那条分支/那个差异”
  而出现假存活。

### 6.9.8 `cookers`（16 条全杀；又一例「后续步骤把差异规整掉」）

- **等价变异的一种形态：后置步骤抹平了差异**。变异把 `cook_bdy` 的 `take` 换成 `get`
  （理论上键的插入位置不同）——但 `cook_bdy` 最后一定会 `reorder_fields` 把键序整体规整，
  于是差异不可见。换成“kind 归一化失效”后即可杀。
  ⇒ 判等价变异时，要往后看**后续步骤是不是会把差异擦掉**。
- **另一种：用例值区分不出两个算子**。`float_scaling_itr` 的 `floor` vs `round` 变异幸存，
  因为 `1.2345 * 10000` 略大于整数（两者同结果）。换成 `0.1235`（乘积略**小于**整数）
  和 `-0.1235` 后立刻可杀。⇒ 选边界值要瞄准**两个算子行为分岔的那个方向**。
- 补一个 `nf n 999` 检查点，专门验证 cook 没有污染 `Defines.NEXT_FRAME_*` 共享常量。

### 6.9.9 `cook_opoint`（追加 16 条，32/32 全杀）

- **后置用例会掩盖前序污染**：`cook_itr` 的“克隆 `caughtact`”变异一开始幸存——
  因为 `p2`（`action:999` + `facing:3`）会经由 `cook_opoint` 往**同一共享常量**
  `Defines.NEXT_FRAME_AUTO` 写 `facing`，恰好把前面的污染覆盖掉。
  ⇒ 共享状态检查点必须放在**可能写入它的第一处用例之前**。
- **“死代码路径”也是等价变异源**：硬编码帧列表 `['50','54','109']` 只能被
  `action` 是数字的路径走到（`action` 先被 `take` 删掉、只在数字分支重建）。
  传字符串 `"109"` 永远触发不到 ⇒ 必须用 `action: -109`（经 `get_next_frame_by_raw_id`
  变成 `id:"109"`）。**写用例前先确认“这条分支真的能到达”**。

### 6.9.10 `make_frame_state`（追加 17 条，49/49 全杀）

- **机制性等价要能说清“破口在哪”**：`foreach` 的对象分支在本仓库里观测等价（见
  DESIGN §4.21），所以 C++ 只写数组路径。但等价是有条件的 ⇒ 必须补一条**反例用例**
  把条件钉住（`bdy` 写成普通对象 `{a:1}` ⇒ 两侧都退化成“取到键字符串、`kind` 为
  `undefined`、跳过”），否则“等价”只是“我没测出来”。
- **强转写的值要快照下来**：`OLD_LouisCastOff` 的 5 个 opoint 是手抄的，差分一次过
  并不代表抄对了——所以变异里同时钉 `x` 的符号、`oid` 的 A/B、以及**字段顺序**
  （把 `x`/`y` 两行换位）。顺序类差异只在 `render` 按插入序输出时可见。

### 6.9.11 `frame_behavior/*`（追加 22 条）

- **链式赋值 `a = b = c = 1` 的副作用顺序是从右往左**（先 `c`）⇒ 对象键的**插入序**由此
  确定，而 `render` 按插入序打印 ⇒ 这是可观测语义，不能“顺手”按书写顺序写。
  （这是第二次栽在“键序可观测”上；第一次是 `Object::set` 对已有键原地更新。）
- **“看着像写反了”的映射不要改**：分发器把 `AngelBlessingStart` 映到 `jan_chaseh_start`、
  `DevilJudgementStart` 映到 `jan_chase_start`。差分一次就确认它是原样行为，
  再补一条“互换即被杀”的变异把这个事实钉住。
- **对象展开（spread）会在字面量中间插键** ⇒ 被展开的函数不能在目标对象上直接调用
  （只能“读回”它的结果再造字面量），否则键序变了。
- DSL 补充：`setarr <id> <key> <v1> <v2> …` 的**每个元素各自**写成 `o <n> <k> <v> …`，
  不要把两个元素合并成 `o 2 …`（那样第二个 `o` 会被当成下一个 key）。

### 6.9.12 `string_matchers`（13 条；先 12/13，补用例后全杀）

- **等价 / 用例缺口要分清，并且要能说出「差异在哪个方向上显形」**：`end_pos` 收在冒号处
  一开始幸存，我先拿 `a:b:c` 当反例——是错的：键回溯后剩下的部分不再构成匹配，两版输出相同。
  真正能区分的是**值里还含 `:`**（`a: b:c` ⇒ 正确版 1 条、变异版 2 条）⇒ 补 `a: b:c` /
  `k: v:w u: 1` 后立刻被杀。与 §6.9.8 的 `floor`/`round` 同源：**反例要瞄准分岔方向**，
  不能只是「看起来更刁钻」。
- **已知等价（没写成变异，写进文档）**：
  - `find_block` 的 `scan = s + 1` 重扫循环：非空 `start` 时，后面若存在 `end`，
    第一个 `start` 一定也能看到它 ⇒ 循环不可观察（写成这样是为了与正则语义逐字对应）；
  - `match_colon_value` 的前导/尾随 `trim()`：匹配器本就会从任意下标起扫，
    trim 与否对「最左匹配 + 全局迭代」的结果无影响 ⇒ 保留只为与源实现一致。
- **「同一规则只写一处」又救了一次**：`trim(start)` / `trim(end)` 原本在 `match_block_once`
  与 `take_blocks` 各写一遍，收敛成 `find_block_trimmed` 后变异点才只有一个。

### 6.9.13 `entity_data`（11 条；先 10/11，补用例后全杀）

- **「样本里放了目标字符」≠「目标字符被测试到了」**：`|` 保留规则的变异幸存，原因不是用例不够
  刁钻，而是含 `|` 的那个样本带了 `hash:""` ⇒ `??` 不回落 ⇒ **根本没走到文件名字符过滤**。
  补一个「无 hash + 文件名含 `|`」的样本后立刻被杀。
  教训：**先确认这条分支真的被走到**（同 §6.9.9 的「死代码路径」，只是这次死在「前置条件把
  分支短路了」）。
- **共享对象的原地修改要两边都能观察**：`make_entity_data` 改的是 ctx 里的 base ⇒ 用例除了打印
  返回值，还要 `dump` 原 ctx；否则「改成改副本」这种变异不会显形。

### 6.9.14 `itr_prefabs`（12 条；先 10/12，定性后全杀）

- **幸存 1：等价变异（后置步骤 erase 差异，第 3 次）**。`remain` 从“名字结尾”开始取、还是从
  “跳过空白后”开始取，在输出上没有差别——因为 `match_colon_value` 自己会 `trim`，而 `remain`
  本身不进输出。⇒ 判等价依旧要往后看：**这个中间值会不会被后续步骤规整掉**。
- **幸存 2：用例缺口**。“块存在且非空、但一个 entry 都没有”走的是 `list.length === 0` 的
  `undefined` 分支；而我的用例里这条被“块 trim 后为空”的**更早**分支挡住了 ⇒ 补 p14 后被杀。
  ⇒ 与 §6.9.13 同一条：**先确认目标分支真的能到达**。

### 6.9.15 `entity_kinds`（19 条全杀；但过程里踩了两个工具坑）

- **用例 DSL 的个数写错会伪装成“崩溃”**：`index o 4` 却只给 3 对键值 ⇒ 解析器错位，
  后续行被当成“第 4 个值”接着吃 ⇒ 症状是 `exit=2`（truncated）与**访问违例**混着出现，
  看起来像代码 bug。我用前缀二分 + 单键探测绕了很久才回到“先验证用例本身”。
  ⇒ **看到“崩了”先跑最小用例**；`o <n>` 要逐个数（这次一口气写错 9 行）。
- **变异锚点必须逐字照抄磁盘文本**：那个文件经过“创建失败 + 定点修补”⇒ 保留了**旧格式**
  （多行 `make_obj`），而我在变异里按**我想写的格式**写锚点 ⇒ 连续两次 `anchor occurs 0 times`。
  ⇒ 已把 `mutate.mjs` 改成**一次性列出所有坏锚点**，不再遇到第一个就退出。

### 6.9.16 `bg_data`（22 条；先 19/22 后全杀）

- **幸存变体必须“对已覆盖的输入真的不同”**：`bmp` 后缀的变异把条件改成“任意位置出现 bmp”
  ⇒ 我的用例里 `MidbmpX.bmp` 末尾**也是** `bmp` ⇒ 两版输出相同 ⇒ 看起来像“用例缺口”，
  实际是**变异方向没被覆盖**。补 `has_bmp_inside.gif`（中间有、结尾不是）后被杀；
  同理“至少 5 字符”的变异需要**恰好 4 字符**的样本（`abmp`）。
  ⇒ 定性时先问：**这个变异在我的输入集合上到底哪一步不同？**
- **前置的 `.trim()` 会把“前提”吃掉**：块内空行的变异幸存，因为 `make_bg_layer` 先对整个块
  `.trim()` ⇒ 块首的空行根本进不了 `split`。改成块**中间**的空行后立刻被杀。

### 6.9.17 `post_process_obj_data`（追加 5 条，16/16 全杀）

- **真值判断里的“空容器”是陷阱**：`ctx.index.groups` 用 `if (… && ctx.index.groups)` ⇒
  空数组**通过**（把 group 设成 `[]`）、`null` / `false` 不通过。用例要两边都写，
  否则“只判 null”的变异会活下来。
- **`as_object` 的 const 重载会咬人**：`Object* p = as_object(v);` 只有当 `v` 是**非 const**
  `Value` 时才给可变指针；声明成 `const Value v` 会拿到 `const Object*` 并编译失败。

### 6.9.18 `ball_bdy`（14 条全杀；含一次“前提条件”事故）

- **又一次“前提条件”**：`ctx.data.id` 被无条件访问 ⇒ TS 在 `data` 缺失时**抛错**，
  而 C++ 把它当成 `undefined` ⇒ 差分直接报 `TS failed`。这类输入要从用例里**剔除**，
  并在 DESIGN 里记成“有意保留的差异”。
- **C++ 端的 API 细节**：`CondMaker` 只有 **`add`**（没有 `add_`），`not_` / `and_` / `or_` 才带后缀。
  照 TS 的 `add(` 写就不会错；我按“成对”习惯写了 `add_`，编译一次报 10 处。

### 6.9 用例 DSL 的两个坑（我踩了 6 次）

值字面量是**前缀记法带个数**：`o <n> <k1> <v1> …`、`a <n> <v1> …`。
个数写错时症状分三种，都要会认：

| 症状 | 含义 |
|---|---|
| `value literal truncated at token i of n` | 个数**写多了**（或整行少了 token） |
| `bad value literal 'x'` | 个数写多了，多出来的位置被当成下一项的 kind |
| `line N: K trailing token(s) after op 'x'` | 个数**写少了**（`fields` subject 专有这个自检） |

三种都会**把整行的 token 逐行打印出来**，所以定位是直接的。
写用例时宁可用 `Select-String -Pattern '^fr '` 把同类行列出来逐个核对个数。

### 1.6 subject: `fields`（`fields.ts`）

TS 侧用**仓库里真正的 `src/LFW/fields.ts`**，C++ 侧是 `lfw/fields.cpp`。
因为描述符本身就是普通对象，所以直接复用值字面量；字段表是 TS 的 `Map`，
C++ 侧用 `Object` 顶替，TS 侧用 `new Map(Object.entries(o))` 还原。

```
fw "<type>" <n> <v1>..<vn>      构造描述符（type 取 string/float/int/boolean/object/map/空串）
ff <obj>                         fields(obj)         → 字段表
fm <map>                         fields_map_2_fields_obj
fr <obj> <map>                   reorder_fields（就地重排）后打印 obj
fa <v>                           to_array
fas <v>                          to_array 是否与原数组同一个引用
fv <data> <map>                  validate_fields → 1 行 ok + 每行 `e <err>` / `w <warn>`
```

`fields` subject 额外有“残留 token”自检：任何 op 解析完后若还有 token 未消费，直接报错
（挡的是“个数写少了”这类静默错误）。

输出统一走 `trace_util` 的 `render_value` / `renderValue`：
`u` / `z` / `b0|b1` / `n<number_to_string>:<16位十六进制位模式>` / `s"..."`（`esc` 转义）/
`[...]` / `{"key":...}`（键按 `Object.keys` 序，TS 的 `Map` 也渲染成 `{}`）。
**同一个 `render*` 两边必须产出完全相同的文本**，所以 `Map` 与 `Object` 的差异在这里被抹平。

### 1.7 切词器曾经在静默地截断参数（重要）

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

### 6.9.19 `ball_frame_state`（22 条全杀；本轮抓出两类假通过）

- subject `ball_frame_state`（5 个 op：`fs15` / `fs3000` / `fs3001` / `fs3005` / `fs3006`），
  用例 20 个样本 / **21 行输出**，变异 **22/22 全杀**。
- **假通过一：`set` 会静默吞掉多余 token。** 值字面量个数写少时（`o 2` 写了 3 对）
  解析器**只取前 2 对**就返回，`set` 不检查剩余 token ⇒ 后半段（`behavior` / `bdy` / `actions`）
  **根本没被设置**，而 TS 侧同样被吞 ⇒ 差分“通过”。这一批 8 个幸存变异里有 6 个源于此。
  ⇒ **对策（已落地）**：两个 harness 的 `set` 分支都加了“剩余 token 非空就报错退出”的检查
  （C++：`trailing token(s)`；TS 同样），让计数写错立刻变成硬失败。
- **假通过二：`index` 无人读取。** `index 一律写 0` 变异存活 ⇒ 查证 `ball_bdy.cpp` 与 TS 侧
  都不引用 `ctx.index`（`{...ctx, bdy, index}` 只为了对齐源形状）⇒ 这是**等价变异**，已删除，
  不是用例缺口。判据：先问“它对我已覆盖的输入到底哪一步不同”。
- **真 bug（差分先过、变异杀掉）**：`_3001` 的 itr 分支我写成了宽松 `equals`，
  而 TS 是 `switch (itr.kind)`（严格）。原用例的 itr `kind` 都是数字 0 ⇒ 差分看不见；
  补一个 `kind s "0"`（字符串）的样本后立刻分开。⇒ “严格/宽松”这类点位必须有
  **同值不同类型**的样本，否则差分与变异都看不出区别。
- 覆盖要点：`kind` 用 `n 0` / `s "0"`（宽松命中）/ `n 1` / 缺键；`data.id` 用 `s "212"` /
  `n 212`（宽松命中）/ `s "209"` / `n 209`（严格不命中）/ `s "214"`（special）/ `s "1"`；
  `frame.bdy` 缺键 / `u` / `z` / 数组；itr `kind` 数字与字符串；`behavior` 1 / 10 / 3 / 缺键；
  `_3006` 的 bdy 带与不带已有 `actions`（区分 append 与 replace）。

### 6.9.20 `hit_next_frame`（26 条全杀；锚点两个新坑）

- subject `hit_next_frame`（8 个 op：`drink`/`super_punch`/`punch`/`jump`/`defend`/`weapon_atk`/
  `jump_atk`/`turn_back`），用例 8 个样本 / **15 行输出**，变异 **26/26 全杀**。
- **差分抓到的漂移（唯一一条）**：我漏了字面量的**外层 `B` 键**。
  TS 是 `assign(frame.key_down, { B: { id, wait, facing } })` ⇒ 结果是 `key_down.B` 而不是把
  三个字段直接放到 `key_down` 上。这种“嵌套字面量少了一层”的错误，只要输入里 `key_down` 已存在
  就能看出来 ⇒ 用例必须同时覆盖“已存在 / 不存在 / 空对象 / falsy”四种。
- **变异锚点坑一：行内含多语句。** `{u"id", s(u"210")},` 并不是独立行，它和
  `  return make_arr({make_obj({{` 同在一行 ⇒ 只写这一行当锚点会得到 `anchor occurs 0 times`。
  教训：从文件里**复制**锚点，不要凭印象重打。
- **变异锚点坑二：`.mjs` 里的 `\uXXXX` 会被 JS 先解释。** C++ 源码里中文写作转义序列
  `u"\u8df3..."`，而写进变异文件的模板字符串会被 JS 先变成**真实汉字** ⇒ 与源码不匹配。
  必须写 `\\uXXXX`（双反斜杠）才能匹配源码里的字面转义。
- **`assign` 的范围**：只实现对象源。JS 对 Array/String 源会拷下标键，但 LFW 的真实调用点
  只有 `turn_back` 两处（都是对象字面量）⇒ 未实现，并在 DESIGN §4.31 记录（不是隐性差异）。

### 6.9.21 `parase_indexes`（两个存活教了“测试面归属”）

- subject `parase_indexes`（op `parse`，把整段 dat 文本写成一行字面量：
  `parse s "json5" s "<object>\nid:\s100\s...\n<object_end>"`），用例 18 行，变异 **19/19 全杀**。
- **前两个存活变异都是 `match_hash_end` 的**（改成 `rfind` 取最后一个 `#`、只有 `\r` 才终止捕获）。
  原因不是用例缺样本，而是**测试面错位**：`parase_indexes` 传给 `match_hash_end` 的永远是
  `split_lines` 之后的**单行**，换行/多 `#` 的规则在那里**永不可达** ⇒ 等价变异。
  ⇒ 对策：把这两条变异交给 `string_matchers` subject（那里能直接喂多行文本），并给它的 harness
  加 `hash` op（`hash s "a#b\nc#d"`）；`string_matchers` 用例 35 → **48 行**，变异 **15/15 全杀**。
- 教训：**变异放在哪个 subject 上，取决于哪个 subject 能构造出该分支的前置条件**；
  跟单元调用的工具函数，其边界要在“能被直接调用”的那个 subject 上测。
- 另一个真实收获：`/.dat$/`（通配）与 `endsWith(".dat")`（字面）并存，样本要同时覆盖
  `a.ddat`（只被前者命中）与 `a.doc`（都不命中 ⇒ stage 报错）。
- 工艺：`trim_str`/`js_trim` 这类逐字相同的重复实现必须先合并再写新调用方，
  否则同一规则会有第三份（合并后 `cond_maker`/`bg_data` 差分重跑仍绿）。

### 6.9.22 `cook_frames`（27/27；一次 AV 调试 + 两类归因边界）

- subject `cook_frames`（op `cook`，用例用 `new/set` 构造 `{text, base}`），**20 行全对**，变异 **27/27 全杀**。
- **AV 调试法**：C++ 挂掉且**无任何输出** ⇒ 用 `Get-Content <case> -First N` 截断做二分（每步 4 行）
  定位到具体用例，再用最小样本（拆开可疑的两个分支）分离。本轮根因是
  `frame.hit` 为 **truthy 非对象**（dat 里 `hit:` 后接 `id:` 被 `match_colon_value` 解析成字符串 `"id:"`）
  ⇒ `as_object` 得 nullptr 后解引用。TS 在同样输入下抛 TypeError（属“前提不满足”），
  C++ 改为跳过并写入 DESIGN §4.33（有意差异，不写进用例）。
- **差分抓到的真差异**：`base.files` 缺失时 TS 用解构默认值 `{}`，我给了 undefined。
  注意：**解构默认值只在属性为 undefined 时生效，对象本身为 undefined 会直接抛** ⇒
  `base` 必须由用例显式提供。
- **归因边界样本**（先问“它对哪个输入才会不同”）：
  - `content ≥ 1` 的边界需要“`\s+`(2nd) 之后的第一个字符紧接 `<frame_end>`”
    ⇒ `<frame> id x<frame_end>`（name 空、content 1 字符）；
  - `zero_as` 分支需要 `next: 0`；
  - `not_zero_num` 分支需要**非数字真值**（`dvy: abc` ⇒ `to_num` 回退成字符串）。
- 无效变异提醒：把 `row` 改成 `col` 会**编译失败**（作用域外）——那不是有效度量，要换成可编译的等价破坏。

### 6.9.23 `make_ball_special`（24/24；前提条件与锚点缩进）

- subject `make_ball_special`（op `ball`），**17 行全对**，变异 **24/24 全杀**。
- **前提条件（TS 会抛错，样本必须提供）**：
  - `id` 为 `JanChase`/`FirzenChasef`/`FirzenChasei` 时必须给 `base.hit_sounds`；
  - `id` 为 `JanChase`/`JanChaseh` 时必须给 `frames['50'/'51'/'52']`（**全部三支**，代码无条件访问）；
  - 这类崩在差分里表现为 `TS failed` + `TypeError: Cannot read properties of undefined`。
- **锚点缩进要从文件里复制**：chase 组内的 `for` 是 **4 空格**、其内部是 **6/8 空格**；
  我按 8/10 写锚点直接 0 次匹配（上一轮也踩了同类的“锚点行含多语句”）。
- **嵌套锚点要分两次调用**：若一条变异的 `from` 是另一条的**子串**，同一次 `multi_replace` 里
  先执行的替换会让后面的锚点失效（本轮 `invisible`/`invulnerable` 两条就是这种关系）。
- 杂项：首行 `const Value item = make_obj({{u"oid", ...` 与后续对齐行要一起写进锚点，
  否则以 `{u"oid"...}` 开头的锚点匹配不到。

### 6.9.24 `make_weapon_special`（29/29；"少字段"也是 bug）

- subject `make_weapon_special`（op `wsp`；另外用 `piece <name>` / `pmut <name> <idx> <literal>`
  直接验证 `broken_piece_frames` 的**共享性**：改一次再读应看到同一个数组被改），**42 行全对**，变异 **29/29 全杀**。
- **差分抓到的真 bug（不崩，只是少字段）**：`as_object(make_aa(idx))` 传入**临时** `Value`，
  临时析构 ⇒ `Object` 被释放（`make_shared` 计数归零）⇒ 悬垂 ⇒ 表现为“`aa` 展开的 `dvy`/`dvx` 全没了”。
  ⇒ **少字段也要当 bug 查**，别只看崩溃。修法：先存局部变量再取指针。
- **前提条件**：`data.base` 必须存在（TS 无条件访问 `data.base.group` / `data.base.type`）
  ⇒ 每个样本都要给 `base`；这类崩在差分里表现为 `TS failed` + `Cannot read properties of undefined`。
- **等价变异提醒**：`delete frame.itr` 对“本来就没有 `itr` 的帧”是 no-op ⇒ 要区分该分支，
  样本必须让那个帧**同时**带 `state = Weapon_Rebounding` 与 `itr`（本轮就是这么补上的）。
- 共享常量：`broken_piece_frames` 的 27 个数组在 JS 里是可变的模块级常量 ⇒ C++ 必须返回
  `static` 的同一实例，否则 `pmut` 那组用例会漏。

### 6.9.25 `make_stage_info_list`（25/25；两条等价变异 + 一条写坏的变异）

- subject `make_stage_info_list`（op `mkstage`，整段 dat 文本写一行字面量），**22 行全对**，变异 **25/25 全杀**。
- **差分抓到的漂移**：只差**键插入序** —— `p.health_up = p.respawn = {...}` 是链式赋值，
  右侧先算 ⇒ `respawn` 先插入。又一次验证“链式赋值从右往左”。
- **等价变异（已删并记录）**：
  1. `collapse_ws_newlines`（`\s+\n+` → `\n`）：它的输出只拿去给 `match_colon_value` 与
     `match_hash_end`，而前者的空白无关结果、后者的结果又会被 `.trim()` 抹平 ⇒ 在当前调用面**不可观察**。
  2. `is_stage_end` 里的 `strict_equals(next, "end")`：删掉后会走 `find` 失败分支，
     而 `"end"` 永远不在 id 列表里 ⇒ 同样为真 ⇒ 等价。
- **写坏的变异**：我为了“取消排序”在前面**又插了一个**恒返回 `false` 的 `stable_sort`，
  但原来的 sort 照旧执行 ⇒ 顺序不变 ⇒ 变异无效（不是幸存，是没生效）。
  ⇒ 要破坏排序就改**比较器本身**（`return av < bv;` → `return false;`）。
- 归因补样本：`bound` 行里的 `music` 需要“同一行同时有 bound 与 music”；
  `nid < 49` 的边界需要 `id: 49`；排序需要乱序输入（30/3/12）。

### 6.9.26 `cook_file_variants` + `frame_editing`（各 13/13；一条不可观察变异）

- subject `cook_file_variants`（op `cfv <id>`）**14 行全对**、`frame_editing`
  （op `kd`/`ht`/`sq`，另有把字面量写进共享 `costs` 的 `set C <key> <literal>`）
  **24 行全对**；两个变异文件各 13 条、**13/13 全杀**。
- **DSL 陷阱（本轮踩了两次，务必记住）**：`o N` 里的 `N` 是**键值对的个数**，
  不是 token 数。`o 2 id s "50"` 会被判为“声明 2 对只给了 1 对” ⇒
  `value literal truncated at token 10 of 9`。正确写法是 `o 1 id s "50"`，
  两组就是 `o 2 id s "50" facing n 1`。
  注意报错信息里的 token 下标由 `parse_value` **先算后判**，偶尔会读到 vector 末尾之外，
  在 Windows 上表现为 `0xC0000005`（而不是干净的 exit 2）⇒ 见到“无输出 + AV”先怀疑字面量计数。
- **两条“第一版没杀掉”的教训**：
  1. `strip_suffix` 取最后一个点：只放一个多点样本没用，因为 `infos[0]` 必须**正好是**那个
     多点文件（`std::sort` 后 `a.b.pb.png` 排在 `a.b.png` 前面）⇒ 样本要构造成
     `a.a.png` / `a.ab.png` / `a.ac.png` / `a.ad.png` 这种“基准自身含两个点且字典序最小”。
  2. gap 间隔不一致：字母必须**连续**（否则中间会 break，`indexes` 长度不够），
     但下标要**跳** ⇒ 插中间干扰项 `abb.png` / `abd.png`（字典序排在 `a.png` 与 `ac.png` 之间，
     且本身不是 `a` + 单字母，不会被 `findIndex` 选中）。
- **`frame_editing` 补样本的归因**：
  - `zero_as` 写成 `repeat`：只有 `id` 为 `"0"` 才可观察 ⇒ `kd f17 s "F" n 0`
    （输出 `{id:"0"}` 而非 `{}`）。
  - `seq` 少写“跳过 falsy”：`u`/`z` 走 else 分支本来就什么都不做 ⇒ **等价**；
    必须用 `n 0`（`is_num` 为真但 falsy）与 `s ""` 才可观察。
  - `seq` 普通分支丢掉旧值：同一键要 `sq` **两次**。
  - 数字 next / 对象 next 的 cook：`sq <id> s "F" n 10`、`set C 50 {mp,hp}` + `sq <id> s "K" {id:"50"}`。
  - `facing` 用宽松相等：样本给 `facing s "2"`（字符串）。
- **一条已删的不可观察变异**：把 `cook_one` 里对象的浅拷贝改成直接用入参 `v`。
  源对象在 DSL 里只能是**内联字面量**（没有“引用 `g_objs` 中对象”的语法），
  就地改写它无法被任何观察点看到 ⇒ 记作**测试缺口**（不是等价变异，生产路径下源对象来自
  帧表、是可达的）⇒ 后续 `make_fighter_data` 的集成用例会从真实数据取源对象，届时可覆盖。
  **后续修正（§6.9.27）**：`make_fighter_data` 落地后确认这条缺口**仍在** —— `FrameEditing`
  的 `nexts` 参数在两个方面都只能是内联字面量（帧表里的引用走的是 `is_str/is_num` 分支），
  所以"对象浅拷贝"这一条目前无法用 DSL 观察；保留为已知缺口。

### 6.9.27 `make_fighter_data`（37/37；两条等价变异）

- subject `make_fighter_data`（op `reset` / `c <key> <literal>` / `f <frameKey> <literal>` /
  `s <frameKey> <subKey> <literal>` / `run`）**38 行全对**，变异 **37/37 全杀**。
- **本轮最大的教训：变异"存活"要先问「这条语句的效果最后可见吗」**。
  Walking/Running 分支会 `delete frames[frame_id]`，看上去写进去的字段都白写了；
  但 `round_trip_frames_map` 持有的是**同一对象**，`make_round_trip_frames` 又会把它重新写回
  `frames` ⇒ `wait`/`dvx`/`dvz` **依然可观察**。前提是同一个 `name` 至少有 **2** 个帧
  （否则 `2*len-2 === 0`，一个副本都不生成）。本轮 3 条"看似等价"的变异就是靠这个维度救回来的。
- **归因要顺着"谁来覆盖它"往下追**：
  1. `next` 的 cook 类型（`"next"` vs `"hit"`）需要 `next` 对象自带 `mp: -5`（只有负值才会被
     `type === "next"` 取反），并且该帧的 `state` **不能**是 Standing/Jump/Walking ——
     否则紧随其后的 `hit_next_frame_turn_back` 会把 `next` 整个换掉，差异被吞掉。
  2. `mp/hp` 互换需要 `mp: 4300` 这种 `hp` 非零的值（`mp` 在 ±1000 内时 `hp` 恒为 0，看不出互换）。
  3. `walking_speedz` 的默认值只能在 `state` **不是** Walking 的帧上观察（否则第三段 switch 会用
     真实值把 `dvz` 覆盖回去）。
- **改完用例一定要重新 `build`**：本轮出现过一次假 drift（`ctrl_x`/`ctrl_z` 键序），实际是
  变异测试还原源码后没有重编译，exe 还是旧的。
- **链式赋值再中一次**：`frame.ctrl_x = frame.ctrl_z = 1` 是 `ctrl_z` 先；而 `case 5~8` 是两条
  独立语句（`ctrl_z` 在前）。同一份代码里两种写法并存，差分直接抓出来。
- **两条等价变异（已删并记录）**：
  1. `cook_file_variants(ret)` 与 `cook_transform_begin_expression_to_hit(ret.frames)` 互换顺序 ——
     一个只碰 `ret.base.files`、另一个只碰 `frames[*].{key_down,key_up,hit,seqs}`，互不干扰，
     且我们的 `cook_file_variants` 不会抛异常 ⇒ 不可观察（TS 里顺序只影响"抛错前完成的工作"）。
  2. `if (tframes.keys().empty()) return;` 改成恒假 —— `cook_marker` 自己会用
     `tframes.get(...) == nullptr` 全部 continue ⇒ 无副作用。
- **`o N` 计数写错本轮又犯了 8 次**（`o 2` 后面给了 3 对之类）。养成习惯：
  写完 `f`/`c`/`s` 行后先数一遍键值对。

- **同一条语句在两条分支上各写了一遍时，一个变异只能覆盖其中一条**：`cook_next_frame_cost(item, "next", …)`
  在 `next` 的**数组**分支和**单值**分支里各出现一次，第一条变异只打数组分支 ⇒ 单值样本救不了它。
  修法是让样本同时覆盖两种形态（`next a 2 …` 与 `next o 3 …`），并**再补一条针对另一分支的变异**。
  这是本轮最后一条幸存（也是唯一一条）真正的归因。### 6.9.28 `obj_dat_to_json`（27/27；一条等价变异）

- subject `obj_dat_to_json`（op `reset` / `t s "..."` 逐行拼数据 / `i <key> <literal>` 设 datIndex /
  `run`，失败时输出 `E <message>` 以便和 TS 的 `throw` 对齐）**14 行全对**，变异 **27/27 全杀**。
- **手写正则时"贪心回溯"的迭代方向常常是等价的，别急着补样本**：`(\S*)\s*:` 里 `\S` 不含空白，
  `\S*` 的长度直接决定下一个字符，所以"`g` 从最长递减"与"从 0 递增"给出**同一个**解。
  本轮先把它当成用例缺口，推导后确认是等价变异并删掉。
- **规则有优先级，前一条会吃掉后一条本来能观察的输入**：`key:number`(R4) 永远优先于
  `key:value`(R5) 与 `key number`(R6)，所以 `a 1:2` 观察不到 R5/R6 的差别；要区分 R5 与 R6，
  必须构造"**有冒号但冒号后不是数字**"的行：`a: b 1`（R5 ⇒ `a = "b"`；R6 ⇒ `b = 1`）。
- **"`\S*` 取到行尾"这种变异必须用同一行里含空格的输入**（`head: a b.bmp`），
  因为 `name: davis` 取到行尾与取到空白前的**结果相同**。
- **前缀判定放宽要用"像前缀但更长"的行观察**：`startsWith(line, "file(")` → `"file"` 需要
  `file_extra 1` 这样的行（原版走 `key number`，变异会被当成第 N 个贴图文件）。
- 字节级陷阱：`PowerShell` 的 `[System.IO.File]::WriteAllLines` 会把整个文件换成 **CRLF**，
  之后 `.mjs` 里的变异锚点（LF）会**全部**失配（本轮报了 20 条 bad anchor）。
  修法：`ReadAllText` + `-replace "`r`n", "`n"` + `WriteAllText`，改完再重新编译验证。
- `utils/string_help.h` 的 `replace_all` 只有**(字符串, char16_t, char16_t)** 版本，
  两个反斜杠 → `/` 这类多字符替换要手写（`make_stage_info_list` 也是这么做的）。
### 6.9.29 `loader_helpers`（23/23；一笔平台差异）

- subject `loader_helpers`（op `b n <1|2>` / `pic|fpic|wp|phase|stage <literal>`）**22 行全对**，
  变异 **23/23 全杀**。覆盖 `make_buring_smoke` + `preprocess_pic`/`preprocess_frame_pic`/
  `preprocess_wpoint` + `preprocess_stage`/`preprocess_stage_phase`。
- **差分抓到一笔真实的平台差异**：`pic o 3 rad u deg n 45 x u` 时 `__sin_r` 在 C++ 与 TS 上
  末位不同（`0.7071067811865476` vs `…75`）。`rad = deg*PI/180` 两边完全一致，差的是
  `std::sin` 与 V8 `Math.sin` 的实现 ⇒ **不是移植错误**，用例改用 90° 避开，并在 DESIGN 记录。
  ⇒ 以后凡是走三角函数的用例，选角度要挑"两边精确一致"的点（0°/90° 等）。
- **falsy 判定的跨度容易搞错**：`preprocess_frame_pic` 的 `if (!pic) return pic` 是宽松 falsy
  （`pic z` 要返回 `null` 而不是 `undefined`）；而 `preprocess_pic` 里的 `typeof x === "number"`
  连 `0` 都算数字 ⇒ 不能用 truthy。这两个"松/紧"混在同一个文件里，变异要分别覆盖。
- **`reorder_fields` 的语义**：只重排键表里**存在**的键，不在表里的键保持原相对顺序
  ⇒ 用它来区分"两个键表"的变异，必须让输入里同时有**只在 A 表**和**只在 B 表**的键，
  否则两表结果一样（本轮 `stage`/`phase` 的"用错键表"两条变异就是这么才被杀掉的）。
- `defines::find("OID.BrokenWeapon")` **取不到**（注册表里没有该前缀）⇒ 直接用 `oid::kBrokenWeapon`。
### 6.9.30 `loader_more`（29/29；三条靠"改输入顺序/改 falsy 种类"救回来）

- subject `loader_more`（op `bf` / `bg` / `rp` / `rpm`）**25 行全对**，变异 **29/29 全杀**。
  覆盖 `preprocess_ball_frame` + `preprocess_bg_data` + `resolve_prefab`。
- **变异存活先问"我的数据真的触发了那条分支吗"**，本轮连续踩了 3 次：
  1. `kind === JohnShield` 的补充动作**有额外前提**（`frame.on_dead` 为真）⇒ 只给 kind=9
     而不给 `on_dead`，那 4 条变异全都白跑。
  2. `Whirlwind`(15) 与 `Freeze`(16) 是**两个**排除项，用 15 只能杀掉"漏掉 Whirlwind"，
     要另造一个 kind=16 的 itr 才杀得掉"Freeze 写成 Block"。
  3. `cook_ball_frame_state_3005` 与 `_3006` 都只动 `frame.bdy[*].actions` ⇒ 没有 `bdy` 时
     两者输出一样的空 frame ⇒ 分派互换的变异不可观察。**必须给 bdy**。
- **`reorder_fields` 类的变异要靠"输入顺序 ≠ 目标表顺序"观察**：`layers` 里写
  `z, x, w, file` 才能区分 `bg_layer_info_fields` 与 `bg_info_fields`；写成 `file, x, z` 时
  两张表都只命中 `file`，结果完全一样。
- **`!truthy(x)` 与 `is_undefined(x)` 的差别用 falsy 非 undefined 的值观察**：prefab 写 `n 0`
  时前者判为"缺失"、后者会继续展开。
- **`ref ?? prefab_id` 的优先级要用同时带两者的对象**（`ref: "A", prefab_id: "B"`）才可观察。
- **一条真等价变异（已删）**：`res.value = has_base ? spread_assign(base, obj) : obj;` 改成
  无条件 `spread_assign(base, obj)` —— 无 base 时 `base` 是 undefined，`spread_assign` 只复制
  `obj` 的键值 ⇒ **渲染结果完全相同**（对象身份不同但这是不可观察的）。等以后有身份敏感的比较
  （如 `===` 判断）时再考虑恢复这条变异。
- **harness 侧的桩**：`preprocess_bg_data` 在 TS 里要 `lfw.images`，且内部会 `SV.validate` →
  `Ditto.warn/error`，而 `Ditto.error` 在 node 下**不存在** ⇒ TS harness 里给
  `lfwStub = {images:{…}, sounds:{…}}` 并把 `Ditto.warn/error` 打桩成空函数。
  这些都不影响数据，C++ 侧相应逻辑本来就不移植。
### 6.9.31 `indicator_info`（22/22；一条真等价变异）

- subject `indicator_info`（op `cfi <frameLiteral>`）**16 行全对**，变异 **22/22 全杀**。
- **同一个函数里"两种默认值"并存，必须用两个 helper**：`opoint/bpoint/wpoint` 的 `z` 是
  `o.z || 0`（任何 falsy ⇒ 0），而 `cpoint` 的 `x/y/z`、`bdy/itr` 的 `z/l/w/h/x/y` 是解构默认
  （**只有 `undefined` ⇒ 0**，`null`/`""`/`0` 原样留着）。C++ 分别对应
  `or_zero_if_falsy` / `or_zero_if_undefined`。把它们的实现互换，只有**直接写进输出对象**的
  那些字段（`bdy`/`itr`/`bpoint` 的 `z`、`bdy.w` 写空串）才杀得掉 —— 这四个方向的变异都留着。
- **`"w" in pic` 是键存在性判定**：`pic` 存在但没有 `w` 键时会走 `frame.width` 分支；
  去掉这个判定（只判 `pic` 真值）的变异要用"只有 `h` 没有 `w` 的 pic"才杀得掉。
- **`!w || !h` 的"或"改成"与"** 要用"一真一假"的输入（有 `width` 没 `height`）才可观察。
- **一条真等价变异（已删）**：`cpoint` 的 `ox`/`oy` 从 `or_zero_if_undefined` 换成
  `or_zero_if_falsy` —— 这两个值**只**参与 `add`/`sub`（内部 `to_number`），`null` 与 `0`
  数值相同 ⇒ 渲染结果一致。注意同样的替换在 `or_zero_if_undefined` 本体上**是**可观察的。
- 老毛病又犯一次：`o N` 数字数错 ⇒ `parse_value` 越界 ⇒ **无输出 + 退出码 `0xC0000005`**
  （不是干净的 exit 2）⇒ 见到 AV 先数字面量计数。
### 6.9.32 `loader_actions`（33/33；无等价变异）

- subject `loader_actions`（op `bd` / `pa` / `pnf`）**68 行全对**，变异 **33/33 全杀**。
- **抛出路径也要渲染数据**：三个 op 的输出统一是 `<op> ok|throw <render(value)>`。
  如果抛出不渲染，就分不清"抛在 frames 还是 states" —— `if (!expand_comma_keys(holder))
  return false;` 的传播变异会活下来。渲染之后，"删了原键、新键没建成"这种**部分变更**
  也变成可观察量。
- **整数键重排是最好用的差分锤**：`o 2 1,2 <数组> 3 <数组>` 里 `"1,2"` 是普通字符串键、
  `"3"` 是整数键；展开后 `"1"`、`"2"` 变成整数键 ⇒ `Object.keys` 序从 `["3","1,2"]` 变成
  `["1","2","3"]`。这一条同时盖住了 `traversal` 的快照语义与 `Object::keys()` 的
  "整数升序在前"规则。
- **无逗号的键也必须留在用例里**（让 `a,b` 与 `b` 共存）：`if (ks.size() <= 1) return;`
  改成 `ks.empty()`，只有在"前面的展开已经改过键序、且该键不是整数键"时才可观察
  （删除+重插会把键挪到末尾）。
- **字符串的 `[...s]` 按码点切**：用例写 `s "\ud83d\ude00"`（**不能直接写 emoji**，否则
  `to_ascii` 丢信息）；`esc` 会把代理对逐码元转义成 `\ud83d\ude00`，两侧输出都是纯 ASCII。
- **六种 next-frame 类型要各自一条 `data z` 用例**：只写"类型匹配 + data 正常"的用例，
  删掉某个类型的变异会活下来（此时分派与不分派都返回 `true`）。同类陷阱：`is_sound_type`
  删掉 `V_SOUND` 也要一条 `v_sound data z`。
- **两条等价项没写成变异**：① `sound_path_iterable` 里的 `is_nullish(data)` 是冗余的
  （`field_of` 对 nullish 已经返回 `undefined` ⇒ 后续 `is_str`/`as_array` 同样为假）；
  ② `[...v]` 的"浅拷贝 vs 直接引用"不可观察（`Value` 渲染按值递归）。
### 6.9.33 `bots_build`（50/50；无等价变异）

- subject `bots_build`（op `fr` / `ba <name> [args]` / `bae <name> [args]`）**53 行全对**，
  变异 **50/50 全杀**。
- **写变异能反推出真实移植错**（本轮两条）：① `override_z` 一开始收 `double`，把
  `{...ray_1, z: zable}` 写成了取负后的数字 —— `zable` 是字符串 `"3"` 时 JS 的第二个 ray 里
  `z` 是**字符串**；② `cond.add(EntityVal.MP, '>=', mp)` 的右操作数被写成了 `to_number`
  之后的数字。两条都只在"参数不是数字"时可见 ⇒ 用例里放了 `s "3"` / `s "0"` / `s "5"`。
- **`if (mp > 0) cond.add(...)` 的变异必须配 `bae` 用例**：`expression` 是用
  `mp > 0 ? cond.done() : void 0` 算的，所以"把 `>` 改成 `>=`"在 `ba` 下**不可观察**
  （`cond` 的内容没进输出）；只有 `bae` 的 edit 回调读 `cond.done()` 才看得见 ⇒
  `bae ... n 0` 这类用例是必需的。
- **`min_mp` 缺省值的变异**要用"参数真正缺省"的用例（`ba bot_chasing_action s "q" a 0 u u`），
  只给显式实参的用例杀不掉。
- `zable` 的三个方向：`truthy(zable) && z > 0` → `truthy(zable) && z >= 0` 需要
  **truthy 但数值为 0** 的输入（`s "0"`）；→ `z >= 0`（去掉 truthy）需要 **zable 缺省**。
- `frames` 既要"元素个数"变异也要"键序互换"变异；`range` 是闭区间（`v > to` 才 break），
  "少一个"的变异正好卡在这个坑上。
- **harness 侧**：TS 的 rest 参数必须真展开（`...spreadArg(args, n)`），且
  `IEditBotActionFunc` 要**显式调用并传入 edit 回调** —— 否则 `bae` 与 `ba` 输出相同，
  一整批 edit 相关变异会假活。

### 6.9.34 `bots_data`（37/37；无等价变异）

- subject `bots_data`（op `mb` / `mbf` / `mbs` / `mbd` / `mba`：渲染 `bot` / 先读
  `frames`/`states` 取值器 / `bot.dataset` / `bot.actions`）**20 行全对**，变异 **37/37 全杀**。
- **取值器本身要有用例**：`frames`/`states` 取值器会在字段缺失时**补一个键**，
  只用 `set_frames` 过的角色测不出来 ⇒ 专门给 `hunter`（只调 `set_dataset`）来一条
  `mbf hunter`，`child_object` 的键名变异才杀得掉。
- **整数键**：`set_states([StateEnum.Catching], …)` 的键是 `'' + [9]` = `"9"` ⇒ 整数键，
  按 `Object.keys` 排到字符串键前；`set_frames([...])` 则是 `"0,1,2,3,walking_0,…"` 字符串键。
- **TS 的"对象或函数"两种实参**：C++ 用 `as_action` 重载收口（`Value` 原样 /
  `EditBotActionFunc` 调 `f(nullptr)`），两个方向的变异都要有（丢值 / 不调用）。
- **harness 侧**：TS 的 `m.frames` / `m.states` 要用 `void m.frames` **真的读一次**，
  否则取值器不执行、补键的差异看不见。
- **写角色文件反推出真 bug**：`bot_ball_dfa` 的 `min_x` 默认 120 被 `bot_front_test` 的 0
  覆盖 ⇒ 补 `ba bot_ball_dfa n 25`（只给一个实参）这种用例才暴露；现在构建层的用例里
  也留着 `ba bot_ball_dfa n 25 u n 80` 之外的"只传必需参数"形态。
- 当前 5 个角色里 `jan` 的 edit 是 `(a, c) => { return a }`（恒等）⇒ 这条 edit 路径
  **不可观察**（等价），而 `monk` 的 edit 会改写 `keys` ⇒ 可观察。写变异时要挑对角色。

### 6.9.35 `bots_data` 第二批（87/87；一条真等价变异已换掉）

- 差分从 20 行扩到 **42 行**（新增 davis / jack / justin / louis / mark / sorcerer +
  3 条 `reg`），变异从 37 扩到 **87 条全杀**。
- **注册表要用"显式列 oid"的 op**：`reg 38 11 31 33 36 39 37 6 32 35 34` —— 从用例里读 oid
  列表，两侧各自在**自己的**注册表里查（输出 registry 序过滤后 + 每个 oid 的 `bot.id`）。
  这样绕开了"TS 侧 22 个 vs C++ 侧 11 个角色"的必然差异，又能杀掉"工厂注册到错 oid"
  这类变异（`register_maker(oid::kMark, make_bot_data_davis)` 就靠它杀）。
- **一条真等价变异（已换）**：把 `set_frames(arr({num(39)}))`（整数键）与
  `set_frames(range_array(270,289))`（字符串键）互换 —— `Object.keys` **总是**把整数键排在
  字符串键之前，插入序不影响输出 ⇒ 渲染完全一致。要测"插入序可变"必须换**两条字符串键**的
  `set_frames`（改成了 `frames.punchs` ↔ `range(240,269)`）。
- **"就地改 + 展开"的组合**（`mark` 的 `cancel_d>j`）要注意展开的是**改过之后**的那个对象；
  且 `Array::push_back` 可能让 `at(0)` 失效 ⇒ 先拷一份再 push。
- 数值字面量一律用 `num(...)` 包一层：`Value(39)` 这种写法在 `int` → `double`/`bool`
  两条转换路径上**歧义**，编译不过。
- 三个新 include 是编译期报错逼出来的：`cond_maker.h`（`CondMaker` 不完整类型）、
  `constants.h`（`DESIRE_RATIO_X_4`）、`defines_data.h`（`defines::num`）。
- anchor 写反的教训又来一次：3 条变异的 `from` 写成了"变异后"的样子 ⇒ 报 `anchor occurs 0 times`；
  改 `from`/`to` 时顺手不要把**同一个文件里另一条变异的 to** 当成 from（`num(101)` 那次就是这么错的）。

### 6.9.36 `bots_data` 第三批（130/130）

- 差分 42 → **64 行**，变异 87 → **130 条全杀**（累计 17/22 个角色）。
- **anchor 会因新函数而"撞车"**：新角色大量复用同一句
  `cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));`、
  `as_action(bot_ball_dfa(num(150), Value(), num(120), num(800)))`、
  `arr({sv(u"d>a"), sv(u"d>j"), sv(u"d^j"), sv(u"dvj")}))` ⇒ 4 条老变异变成
  `anchor occurs N times`。**只能补上下文消歧**（往前或往后带 1~3 行）。
  其中"往后带"时要注意目标函数是不是文件里**最后一个**（`sorcerer` 就是），
  否则上下文对不上（那次写成 `BotMaker make_bot_data_woody() {` 结果 0 次匹配）。
- 改 `make_bot_data.cpp` 时踩了个自己的坑：`oldString` 里带了
  `BotMaker make_bot_data_bat() {` 但 `newString` 里漏了 ⇒ 函数头被吃掉、并且出现两个
  `register_all_bots`。**大段替换后务必 `Select-String '^BotMaker make_bot_data_'` 核一遍**。
- `select-string` 核完还要注意：CMake 不会拦"声明了没定义"（只在链接期报 `LNK2019`）——
  subject 里引用了还没实现的 `make_bot_data_john` 就会链接失败。

### 6.9.37 `bots_data` 第四批（174/174）

- 差分 64 → **82 行**，变异 130 → **174 条全杀**；**22/22 个角色完成**。
- **`reg` 行只在"顺序"上报漂移，是所有逐 oid 值都对时的典型形态**。不要急着改 C++ 注册顺序，
  先问"TS 侧的注册顺序由谁决定"：`BotMaker.register` 在**模块求值期**跑，故顺序 = `index.ts`
  里 `export *` 的书写顺序（即文件名排序，`rudolf` < `sorcerer`）。
  差分 harness 逐文件 `import` 会把自己的写序强加给 TS ⇒ **必须从 barrel 导入**，
  否则测的是 harness 而不是产品代码。
- `rays_of` 返回 `const_cast` 出来的 `Array*`，`push_ray_with_z(Array*, double)` 因此是非 const 形参；
  写成 `const Array*` 会在编译期报 `C2440`（`const Object*` → `Object*`）。
- `edit_mark_cancel()` 被 `mark` / `firen` / `dennis` **三方共用**，改动它等于同时改三个角色——
  写变异时若锚点落在这里，要确认三者的期望漂移是一起出现的。
- 新角色继续大量复用旧句子（`cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));` 等），
  本轮又出现 6 条 `anchor occurs 2/3 times`，同样只能补 1~3 行上下文消歧。

### 6.9.38 `fighters_special`（133/133）

- 差分 **93 行全对**，变异 **133 条全杀**（23 个 `make_fighter_data_*` + 分发器）。
- ⚠️ **`o N` 写小的危害比"崩溃"更隐蔽**：两侧 `parse_value` 都只读 N 对、**静默忽略**剩余 token
  ⇒ 数据没被构造出来，TS 于是抛 `Cannot read properties of undefined`，C++ 静默返回原值，
  差分报出来的是一条看不懂的崩溃行。**给 harness 加「`idx != t.length` 即报错」是必需的自检**，
  否则每写一个用例都要人工逐字核对计数。
- **分发器必须保住"严格字符串"这一点**：`switch (alias_id ?? id)` 里若把非字符串 `to_string()` 化，
  数字 id（`n 38`）会开始命中 —— 用例里 `id n 38` 与 `id s "38"` **成对**写才锁得住。
- **链式赋值 `a = b = c` 的键序**又一次生效（`max_hp, hp, max_mp, mp`）；
  沿用 `cook_frames`/`make_stage_info_list` 的经验，凡是"两个字段同值"就顺手写一例。
- **等价变异（已记录不写）**：`filter(Boolean)` 的 truthy 判定与紧随的 `as_object == nullptr`
  检查结果重合（任何 falsy 值两条路都跳过）⇒ 对那句写变异必然存活，属于测试面缺口而非代码缺口。

### 6.9.39 `translator_tail`（35/35）

- 差分 **63 行全对**，变异 **35 条全杀**；`dat_translator` 顶层**只剩 `xml` 之外的收尾**。
- **重构会打掉变异锚点**：把 `float_scaling_itr` 的函数体提升成共享的 `scale_num_field` 后，
  `cookers.mjs` 里那条打在旧函数体上的变异立刻变成 `bad anchor`。**收口"同一规则两处写"
  时必须同步迁移变异**，否则下一轮跑 `cookers` 会直接失败。迁移后要确认新锚点所在文件
  已被某个 subject 覆盖（这里是 `translator_tail`），否则变异会"合法但永远存活"。
- **等价变异（已记录不写）**：① `decode_lf2_dat` 的 `if (buf.size() <= 123) return;`
  —— 循环从 123 开始，越界自然不迭代 ⇒ 守卫与循环上界冗余；
  ② 给帧上**不存在的键**调 `round_truthy_field`（`cur == nullptr` 直接返回）是空操作。
- **"字段存在"≠"能观察到该字段被处理"**：`ctrl_z n 7` 经 `round_float` 后仍是 `7` ⇒
  "漏掉 ctrl_z"的变异存活。要用 `7.0004` 这类**取整后确实改变**的值，
  与 `cook_frames` 轮"选值要瞄准算子分岔方向"是同一条教训。
- `edit_info` 的数组源分支必须有用例（`a 2 s "x" s "y"` ⇒ 写成整数键 `0`/`1`，
  按 `Object.keys` 排在字符串键之前）。

### 6.9.40 `entity_helpers`（62/62）

- 差分 **153 行全对**（含 `NSlot` 105 项 / `SSlot` 18 项全表），变异 **62 条全杀**。
- **"我写了这个用例"≠"这个分支被执行到了"**（本轮 5 条存活里最典型的一条）：
  "数组匹配用严相等"杀不掉，是因为**前一句的松相等先短路了** —— `[5] == "5"` 经 ToPrimitive
  本来就成立，`array_contains` 根本进不去。要逼出数组分支，`a` 必须**松相等也不等于** `id`
  （`a 2 n 7 n 5` 的 ToPrimitive 是 `"7,5"`）。
  与 `make_entity_data` 轮"样本里放了目标字符 ≠ 测到了那条分支"同类。
- 其余 4 条存活的归因都是"缺边界值"：`FixedLf2` 需要**非默认 direction** 才与 `value` 区分；
  `AccTo` 的 `<=`/`<` 需要 `current == target`；`is_object_data` 要逐个枚举分支都测；
  `is_bg_data` 的松/紧相等需要**非字符串**的值（`["background"]`）。
- **`calc_v` 的 `acc`/`direction` 默认值只对 `undefined` 生效**：`null` 与 `undefined` 结果不同
  （`direction: null` 会让 `value *= null` 得 0；`undefined` 才取默认 1）。C++ 必须用
  `holds_alternative<monostate>` 而不是 `is_nullish`。
- **枚举表要"整表对拍"**：槽位布局这类常量表，只测几个点会漏掉重排/漏项；把
  `NAME=value` 全表打出来对拍，才让"顺序即语义"真正进入测试面。

### 6.9.41 `controller_helpers`（55/55）

- 差分 **102 行全对**，变异 **55 条全杀**（4 个类：双击状态机 / 序列匹配 / 按键时长 / 7 槽映射）。
- **第三种等价变异形态：与后继操作交换**。`arr.indexOf(expected)` 取**首匹配**还是**末匹配**
  不影响结果 —— `splice` 只删一个匹配项，剩下的多重集相同，后续匹配自然相同。
  （前两种是"冗余守卫/后置步骤抹平"与"死代码"。）判等价时要看这条语句的**结果集**是否被
  后继操作抹平，而不是看它"看起来有没有语义"。
- **常量表要逐个条目都摸一遍**：7 槽映射只按了 3 个槽 ⇒ 漏掉的槽上的变异不可达。
  与 `is_object_data` 漏掉 Ball 分支、`is_bg_data` 只测字符串值同类。
- **状态机的用例要按"状态转移图"来写**：`reset` 漏清 `used` 杀不掉，是因为 `reset` 出现在
  `used=b1` 那次 `load` **之前**；`hit` 漏清 `used` 杀不掉，是因为前一句 `reset` 已经清零。
  ⇒ 每条"清理/赋值"语句的前置状态必须**真的非默认**，否则该语句不可观察。
- harness 参数序统一为 `<sub> <name> ...`（`dc new a s "d"`），字符串参数一律走值字面量
  （与 `dc`/`ks` 一致），避免"裸 token vs 值字面量"两套解析混用。

### 6.9.42 `bot_helpers`（差分 201 行，变异 99/99 全杀）

- op：`de`（DummyEnum 整表 + `dummy_updaters` 键集）/ `ent`（实体注册表 put/del）/
  `dxz`（`manhattan_xz`）/ `ray`（`is_ray_hit`）/ `cl`（`closest`）/ `nt`（`NearestTargets`
  的 new/look/del/sort/clear/snap）。
- **"无 sub 的 op"要在通用 `<sub> <name>` 解析之前分流**：`dxz me a` 只有两个操作数，
  一开始按 `<sub> <name>` 读会读到越界 token（`std::string` 越界构造出空串 ⇒ 报
  `unknown entity ''`，很迷惑）。凡操作数个数与其它 op 不同的 op，一律提前处理。
- **实体注册表必须用插入序容器**（`std::vector<std::pair<...>>` + 线性查找），与 TS 侧 `Map`
  同序；否则 `name_of(值)` 反查在"两个实体值相等"时会给出不同名字（`std::map` 按名字排序）。
- `nt new <name> <max>` 的 `max` 收**值字面量**（`n 3`），与 `defendable` 一致；
  一开始按裸 token 收 ⇒ 用例里的 `n` 会变成多余的 token，被 trailing 自检抓住。
- 每个 op 都打**整表**：`targets=[名字:距离:defendable]` + `ents=[名字…]`（插入序）
  ⇒ `splice` 插到第几格、挤掉的是谁、集合里删掉的是谁，全部可见。
- **`DummyEnum` 有两个成员同值**（`"18"` ×2，TS 源码原样）。字符串枚举没有反向映射，
  `Object.keys` 就是 24 项、**声明序即遍历序** ⇒ 整表逐项对拍才能锁住重排/改名/改值。
  `dummy_updaters` 只对拍**键集**（值为 `undefined` 的那些键也在），且键序走 JS 的
  "整数键优先"规则 ⇒ C++ 侧先建 `Object` 再 `object_keys()`，别手工排序。- **第五种等价变异形态：被紧邻的"对称算子"抹平**。"`manhattan_xz` 的 `dz` 用反了 a/b"
  （`pb.z - pa.z` 代 `pa.z - pb.z`）杀不掉 —— 紧跟着就是 `abs()`，差一个负号完全不可观察。
  与"与后继操作交换"同类但成因不同：那条是**结果集**被抹平，这条是**值落在后续算子的核里**
  （`abs` 的核 = 符号）。写变异前先看下一句是不是 `abs`/平方/取整这类吃掉符号的算子。
- **"默认值写错"必须让两条路径的返回值能区分**：`is_ray_hit` 的 `max_z` 默认 10000 写成 100000
  长时间存活，因为我的越界样本走完 `continue` 之后算出的 `hit` 恰好也是 `false`
  ⇒ 早退分支（返回 `reverse`）与继续分支（返回 `hit`）都渲染成 `b0`。
  要么把 `max_d` 放大到"继续算就是命中"（`max_d n 1000000000` 配 `dz = 20000`），
  要么让 `reverse` 是真值。**两个分支的结果必须分岔，否则整条分支不可观察**。
- **"同一规则的另一种写法"要按算子分岔点选值**：`round` vs `round_float` 用 `3.3335`/`3.3334`
  （`round` 后平手、`round_float` 后有 0.001 的差，一条用例同时锁住"用哪个算子"和"平手保留先到者"）。
- ⚠️ **harness 自检抓到了两次 `o N` 计数错误**（`o 3` 当作两对、`o 4` 实际五对）：
  "解析完 `idx != t.length` 即报错"这条自检在本轮第二次证明是必需的。
- ⚠️ **`mutate.mjs` 的 baseline 检查能立刻暴露"二进制与源码不一致"**：手工恢复源码后忘了
  rebuild，会以 `baseline already failing`（exit 1、无明细）失败 —— 见到这个报错先想
  "是不是刚手工改过源码/mtime"。
