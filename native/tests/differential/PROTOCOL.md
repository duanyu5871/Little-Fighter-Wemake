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

### 6.9.43 `collision_helpers`（差分 243 行，变异 99/99 全杀）

- op：`ent put|del`（实体注册表）/ `col mk|del`（用已注册实体拼 collision：attacker/victim
  + itr/bframe/aframe 三个字面量）/ `ds <entity> <key>`（`Entity.dataset`）/
  `ifall` / `armor` / `civ` `<collision>`。
- **无 sub 的 op 必须提前分流**：`ds`/`ifall`/`armor`/`civ` 的操作数个数与 `ent`/`col` 不同，
  按 `<sub> <name>` 读会读到越界 token；C++ 侧越界 `std::string` 构造会直接 AV（no output），
  比报错更难查。**这条规则上轮已写过一次，本轮又踩 ⇒ 新 harness 一律先在 `op` 上分流。**
- **TS stub 要复用产品里的真规则**，不要手抄：`Entity.prototype.dataset` 用
  `Object.getOwnPropertyDescriptor` 取出后 `.call(this, name)`；`weight` / `state` 这两个
  "有逻辑的 getter" 同样挂到 stub 上（其余 `position`/`facing`/`armor`/`hp`/`fall_value`
  是普通字段或直通 getter，直接放字面量即等价）。这样对拍的是**产品实现**而不是 harness 的复述。
- **越界输入要主动排除**：`Entity.dataset` 的链里 `frame`、`world.bg.data`、`world.dataset`
  不是可选链，缺一个 TS 就抛 ⇒ 用例必须给全路径；"TS 抛、C++ 不抛"的输入记为契约外差异。
- **回看输出值确认分支**：本轮 5 条存活全部来自"样本 `position.x` 写成 0"⇒ `diff_x > 0`
  这条分支从未执行。写完用例必须抽查输出（`native/build/gen/trace.<subject>.<case>.ts.txt`），
  确认落在预期分支上。
- 注意 TS 侧 `is_fall` / `is_armor_work` / `calc_itr_velocity` 取的是 `collision` 对象，
  harness 里用 `{ attacker, victim, itr, bframe, aframe }` 一一对上，缺一个 key 就会
  "TS 抛异常" —— 与上面"越界输入"同一条。

### 6.9.44 `summary_helpers`（差分 146 行，变异 57/57 全杀）

- op：`ent put|del`（实体注册表）/ `sm <sub> <mgr> ...`（`new`/`get`/`items`/`release`/`clear`/
  `dmg`/`kill`/`apply`）/ `sg items|get <id>`（模块级单例）/ `su <sub> <mgr> <id> ...`
  （`on`/`set`/`reset`/`rel`/`snap`）。
- **`su` 是"两个名字"的 op**：`su <sub> <mgr> <id>` 的操作数与 `sm`/`ent` 都不同
  ⇒ 必须在通用 `<sub> <name>` 解析**之前**分流（这条本轮第三次踩，已上升为铁律）。
  另外 `su snap|reset|rel` 只有 4 个 token、`su set|on` 有 5+ ⇒ 入口守卫要用 `< 4`。
- **回调要"五类事件各注册一个监听器"**：漏掉哪类，那类 setter 的"缺守卫 / 守卫取反 /
  守卫用严相等"变异就会存活（重复设同值是可观察的唯一方式）。
- **模块级单例要给"数值视图"**：`sg items` 只列 id，看不到 `kill`/`dmg` 的增减
  ⇒ 必须再加 `sg get <id>`；否则 `apply_damage` 的三条分支变异全部存活。
- 结构性怪癖（TS 原样，不是 bug）：`apply_damage` 把击杀记到**单例**上，而伤害记在 `this` 上；
  只有把「命名 manager 的视图」和「单例视图」分别打出来才锁得住。

### 6.9.45 `drink_stiffness`（差分 63 行，变异 44/44 全杀）

- op：`di new|set|load|snap|empty <name|id> ...`（`DrinkInfo` 构造 / 逐字段 setter /
  `from_snapshot` / `to_snapshot` / 三个 `*_empty`）/ `stf <entity> <itr 字面量>`
  （`handle_stiffness`）/ `ent put|del`（实体注册表）。
- **`stf` 的操作数形态是 `stf <entity> <literal>`（2 个）**，与通用 `<sub> <name>` 不同
  ⇒ 必须在通用解析**之前**分流。终于是第四次；`di` 的 `new|load` 后面是**不定长**字段列表，
  但前两个 token 形态与通用解析一致（`di new d1 …`），所以 `di` 可以走通用解析。
- **TS 类字段初始值要逐字搬**：`hp_h_value: number = 0` 等 9 个字段在 C++ 侧必须是
  `Value _x = Value(0.0);`（成员初始化式）而不是 `Value()`（variant 默认是 `monostate`）
  ⇒ 否则 `snap` 少 9 个 `0`。
- **TS 默认参数 ≠ `??`**：`new Times(0, info.hp_h_ticks)` 对 `undefined` 吃默认
  `Times.MAX`，对 `null` 走 `Number(null) = 0`。用例里要同时有 `z`（`null`）与
  省略该键（`undefined`）两种输入，才锁得住 `ticks_bound`。
- **`A || B` 的变异需要两侧都"可单独决定结果"**：`mp_h_empty` 的 `>=` 被第二个
  `||` 项掩盖 ⇒ 必须给一条 `mp_h_value` 为**真值**且 `mp_h` 落在
  `hp_h_total` 与 `mp_h_total` **之间**的用例。
- `handle_stiffness` 的 `shaking` 回退**只**读 `attacker.world.dataset.itr_shaking`
  （不走 `entity_dataset`），而 `motionless` 走四级回退 ⇒ 实体样例要能区分"只有 world 级有值"
  （`shk`）与"连 world 级都没有"（`nsw`，期望 `undefined` 而非继续外扩）。

### 6.9.46 `mt_random`（差分 143 行，变异 55/55 全杀）

- op：`mt new <name> <seedLiteral>`（注册一个 `MersenneTwister`）/ `ent put|del` /
  `rn new|create <name> <mtToken> <srcLiteral> [<dupLiteral>]` /
  `rn src|get|dump <name>` / `ip spawn <name> <idLiteral>` / `ip table` /
  `ip dvx|dvy|x|y <mtToken> <entName>`。
  `<mtToken>` 是**裸 token**：`-` 表示 `undefined`（⇒ 用静态默认 mt）。
- **`ip table` 只有 2 个 token** ⇒ 必须在通用 `<sub> <name>` 解析**之前**把整个 `ip` 分流
  （铁律第 5 次）。`mt`/`rn`/`ent` 的 token 形状与通用解析一致，不用分流。
- **共享模块不能放 `subjects/` 顶层**：`run.mjs` 的 `readdirSync(SUBJECTS_DIR).filter(f => f.endsWith(".ts"))`
  不递归但会把顶层的每个 `.ts` 都当成 subject ⇒ 放 `subjects/shared/`。
- **要改 `Date.now()` 必须在 `Randoming` 之前求值**：`Randoming.mt = new MersenneTwister(Date.now())`
  是**模块求值期**执行的，所以 patch 模块要在 import 列表里排第一
  （`import "./shared/patch_date"` 在 `import { Randoming } …` 之前，ESM 依赖按 import 顺序求值）。
  这样两侧的默认 mt 种子一致 ⇒ 连"默认通道"的取值也能逐位对拍。
- **不放回抽样 ⇒ n 次覆盖全表**：`Randoming` 的非 duplicate 分支是 `cur.splice(idx,1)`
  ⇒ 连抽 n 次（n = 源数组长度）必然恰好覆盖每个元素一次
  ⇒ 只要在用例里打 9 次 `ip dvx` + 13 次 `ip dvy`，数组里任何单个元素的变异都被杀，
  不必逐条目写用例。
- **`round` vs `floor` 要用非整数半径**：`w/4` 为整数时两者同值
  ⇒ 用 `w = 2 / 10 / -10`（`ip x m1 e4 → n1`，floor 会给 0）。
- **`range(min, max)` 在 `min == max` 时不消耗随机数** ⇒ `ip x`（`w = 0`）之后必须**再抽一次**，
  否则"是否消耗"不可观察。
- **`A || B` 之外的第二类"短路掩盖"**：`if (!is_object(e)) return 0;` 在**缓存赋值之前**
  ⇒ "非实体"不重置模块级缓存 ⇒ 用例要在两次同 mt 的调用**之间**插一次非实体调用，
  才区分得开"短路在前"与"短路在后"（本轮 `ice_piece_dvy` 的守卫就是这样补杀的）。

### 6.9.47 `expression` 加固（差分 928 行，变异 69/69 全杀）

- op：`g <name> <literal>` / `x <name>`（全局取值表）/ `b <源码…>`（构造 + 打印整棵树）/
  `d <idx>`（重放整棵树）/ `r <idx>`（`run(0)`）/ `clr` + `log`（**getter 调用日志**）。
- **求值顺序与短路必须用「getter 调用日志」观察**：只打 `run` 的结果看不出短路有没有生效
  （结果常常一样），而日志能精确给出「哪几个操作数被求值、按什么顺序」。
  `r 0` 只记 `["a"]` 就证明 `a==9&b==2&c==3` 的 and 组短路真的跳过了后两个操作数。
- **每个 `.txt` 是独立进程**：全局表不跨文件共享 ⇒ 新增用例文件要自己把 `g` 再来一遍。
- **节点字段要位精确渲染**：`val_1`/`val_2` 只打类型标签（`vtag`）时，
  「值取错」类变异（取 `word_2` 而不是 `word_1`）只有在类型恰好不同的输入上才被杀
  ⇒ 换成 `render_value`（带 `n<最短串>:<位模式>`）。
- **第 6 形态的等价变异：死代码**。`&`/`|` 分支里去尾 `)` 的 `while` 循环不可达
  （见 `DESIGN.md §4.58` 的推导）⇒ 打在它上面的两条变异永远杀不掉，已删并留下推导。
  判等价不能只看「它有没有语义」，要看**可达性**。
- **冗余守卫**：`if (x === false) continue; x = x && f();` 里那个 `continue` 是多余的
  （`&&` 已经短路）⇒ 删掉它是等价变异；但把条件**取反**是可观察的。
- **TS 构造器遇到多余 `)` 会静默丢弃剩余操作数**（`(A==B))&(C==D)` 只剩 `(A==B)`），
  这是原样行为，C++ 照抄 ⇒ 用例要把它钉住。
- **构建链提速**（`mutate.mjs` 现在会打印 `总耗时 / ms per mutation / 最慢的一条`）：
  - 单条变异 ~9 s → 4.1 s（69 条约 4.6 分钟）。三个固定开销：`vcvars64.bat` 5400 ms/次、
    46 个 exe 全量重链、每条都重跑 TS 打包与 node。
  - `native.mjs build <subject>` ⇒ `--target lfw_trace_<subject>`；
    `native.mjs test <subject> --reuse-ts` ⇒ 复用 `trace.*.ts.txt`（前提：变异只改 `native/lfw/**`，
    `mutate.mjs` 会断言）。
  - 断言失败时先查「有没有残留的 `native/build/mutate-backup/`」—— 有就说明上一次跑被中断了，
    源文件可能还是变异体。
- **`native.mjs` 报的 "N clean / 0 warnings" 不是编译器告警**（那是 `check_lfw_cpp_includes.mjs`
  的纯文本检查）⇒ 编译器告警要自己看构建输出，否则会漏掉 `C4458` 这类。

### 6.9.48 `mersenne_twister` 加固（差分 7614 行，变异 49/49 全杀）

- op：`seed <n>` / `state` / `int <count>` / `float <count>` / `range <min> <max> <count>` /
  `pick <…>` / `take <…>`。
- **`state` 是内部状态的 FNV-1a 64 指纹**（两侧各一份手写实现，逐字段喂字节）：
  `_matrix` / `_upper_mask` / `_lower_mask` / `_index` / `_seed` / `_times` / `_mt[0..623]`。
  它能把"行为等价但内部状态不同"的点位（比如 `_index` 初值 625 vs 624）抓出来。
  ⚠️ 但**打指纹的时机很关键**：要在"刚好设置完"的时刻打 —— 见下条。
- **内部状态字段的指纹要在其刚被设置时打**：`_index` 初值的差异只在**首次抽取之前**可观察
  （首次 `int` 就会 `twist` 并把 `_index` 归零，之后两边完全一致）。
  原用例的 `state` 全打在若干次抽取之后 ⇒ `reset：初始 index 少 1` 存活。
  修法：`seed` 之后**立刻**打一次 `state`。
- **`pick` / `take` 要打印操作后的剩余数组**，否则"删错了哪个下标"不可观察：
  每次调用都在从 token 新建的数组上操作，删对删错只差一个元素，而打印的 `size` 都是少 1。
  顺带要覆盖：空数组（`pick` / `take` 不带参数）、单元素、重复元素。
- **`range(min, max)` 在 `min == max` 时提前返回且不消耗随机数**，返回 `min`
  ⇒ `range(-0, 0)` 的 `-0` 位模式要锁住。用例成对写：`range -0 0 1` + 紧随其后的 `int`
  （同时锁"返回的是 min"与"没有消耗随机数"）。
- **新等价形态：差异小于后继量化步长**。`next_float` 的除数 `2^32` 改成 `2^32-1` 相对差 2.3e-10，
  而 `floor_float` 把结果量化到 1/1000 ⇒ 要可观察得让 `int/2^32*1000` 落在整数下方 2.3e-7 以内
  （即 `int` 恰为 2^29 的倍数）。十来个种子里抽一万次也遇不上 ⇒ 判为不可观察，
  改成测「除数写成 2^31」。
- 「少哈希一个槽」（`for (i < k_N - 1)`）是**证明用例对 `_mt[623]` 敏感**的好变异，值得每种指纹都加一条。
- 快速路径效果：本 subject 的变异只碰一个 `.cpp` ⇒ **2.2 s/条**（49 条 109 s）。

### 6.9.49 `math` 加固（差分 154 行，变异 61/61 全杀）

- op：`clamp` / `clamp_add` / `normalize` / `float_equal`·`equal`·`eqgt`·`eqlt` / `range` /
  `probability` / `normalize_plane` / `calc_plane` / `line_plane` / `project_to_line` /
  `alias_normalize_plane`·`alias_calc_plane`·`alias_line_plane`。
- **`alias_*` 三个 op 是查"共享返回对象"的**：TS 与 C++ 都用**模块级共享 `result`**
  （`normalize_plane` 返回 `Readonly<Result>`、`calc_plane`/`line_plane_intersection` 返回指针，
  都是同一个对象）。连着用两组不同参数调用再一起打印，才能证明"第二次调用把第一次的结果
  覆盖掉了"这种共享语义被照抄了。
- **harness 不要替被测代码补默认值**：`normalize` 原先写死 `normalize(n, 1000)`，让头文件里的
  默认参数永远用不到 ⇒ "默认值写错"的变异存活。改成 2 token 走默认、3 token 显式传。
- **补样本要按"变异点的分岔方向"选参数取值**：
  - `pow(m, 2)` → `pow(m, 3)` 只在 `m ∉ {0, 1}` 时可区分（`pow(1,3)==pow(1,2)`）⇒ 补 `m = 2/3`。
  - `vx = x2` → `vx = x1` 只在 `x1 != x2` 时可区分 ⇒ 补起点≠终点的 `is_direction` 样本。
- **`round_float` 与 `== 0` 的组合会让 `abs` 变冗余**：`round_float(-0.0004)` 是 `-0`，
  `-0 == 0` 为真 ⇒ `round_float(abs(x-y)) == 0` 与 `round_float(x-y) == 0` 等价。
  这类"看起来该有区别"的变异要先算一遍 ±0/NaN 的边界再判等价。
- header 变异（`clamp.h` / `round_float.h` …）会让所有包含者重编译 ⇒ 单条可达 20 s；
  这是正确性换算力，不可省。`.cpp` 变异只要 2 s（单 target 快速路径）。

### 6.9.50 `value` 加固（差分 489 行，变异 71/71 全杀）

- op：`lit` / `arr` / `obj` / `hold` / `get`·`idx` / `elem` / `set`·`str` / `oset`·`oprop` /
  `ohas` / `odel` / `okeys` / `olen` / `eq`·`seq` / `ton` / `tos` / `lt`·`gt`·`le`·`ge`。
- **`hold <value>` 返回一个自增句柄号，且会把它打印出来**。句柄序号是**从文件开头累计**的，
  不是"块内第几个"。我一开始按"块内第 0/1/2"写 `oprop 0 0` …，结果全打到了前面某个空对象上
  （trace 里是一片 `oprop - -` / `ohas false` / `okeys ""`），两个变异因此存活。
  ⇒ **harness 现在支持 `-` 表示"最近一次 `hold`"**（两侧同步），新用例一律用 `-`；
  需要引用更早的句柄时才读 trace 里打印出来的真实序号。
- **`strtol` 在 Windows 上是 32 位 `long`**：20 位的句柄号会饱和到 `INT_MAX` ⇒ 报
  `handle 2147483647 out of range`。句柄是索引，不是数据；数据里的 20 位数字要走字符串 token。
- **想要"同一个值被读两次"的观测**：`okeys`（升序整数键在前、字符串键按插入序）用于钉住键序，
  `olen` 钉住数量，`oprop` / `ohas` 钉住取值与存在性，`odel` 钉住删除结果 —— 这四者缺一不可，
  否则"少删一个键""查错下标"这类变异会被别的输出掩盖。
- 20 位键样本同时钉住 `is_array_index` 的**长度上限**：去掉上限后 `2^64` 溢出成 `0`，
  `okeys` 的表现从 `"18446744073709551616"` 变成 `"0"`。
- `strict_equals` 的 `undefined`/`null` 关系要**左右各测一遍**（`seq u X` 与 `seq X u`），
  否则"只判了一侧"的变异存活。

### 6.9.51 `utils` 加固（差分 321 行，变异 108/108 全杀）

- op：`ease_linearity` / `ease_linearity_backward` / `ease_in_out_sine` / `ease_in_out_sine_backward` /
  `ease_in_out_quint` / `ease_in_out_quint_backward` / `cross_bounding` / `times_*` /
  `utf8_encode` / `utf8_decode` / `chk` / `tonum` / `tonum_or`。
- **⚠ 默认实参必须能在 C++ 侧"用不到"**：`ease_*` 的 `from` / `to` 有默认值，而 harness 原先
  无条件填 `0` / `1`（正好等于默认值）⇒ 头文件里的默认值**永远走不到**，"默认值写错"的变异必存活。
  修法：**按实参个数分派** —— C++ 侧
  `ease_at(tok, [](double a){...;}, [](double a,double b){...;}, [](double a,double b,double c){...;})`
  （用 3 个 lambda 分别绑定 1/2/3 实参重载），TS 侧给缺的形参传 `undefined`（JS 默认值对
  `undefined` 生效，所以 TS 不需要分派层）。**C++ 没有 `undefined`**，这层分派是跨语言共用一份
  用例时必须自己造的。
  - 同一坑的另一处：`Times` 的 `ctor(0, MAX)` / `set_lifes(-1)` / `add(1)` 三个默认值。
  - 用例写法：`ease_linearity 0.5 10`（2 实参）⇒ `to` 用默认值 1 ⇒ 结果 5.5 而不是 0.5。
- **`type_check` / `type_cast` 的观测形状**：
  - `chk <name> <valueliteral>`（name ∈ `is_num` / `is_positive` / `not_zero_num` / `is_int` /
    `is_str` / `is_non_empty_str`）⇒ `chk <name> true|false`。TS 侧一张 `Record<string, fn>` 表，
    C++ 侧 `if/else if` 链，两边都**在名字未知时报错退出**（别默默返回 false）。
  - `tonum <valueliteral>`：TS 走**1 实参重载**返回 `number | undefined`，C++ 走
    `std::optional<double>` ⇒ 有值时打位模式、无值时打 `u`。
  - `tonum_or <valueliteral> <n>`：显式给回落值，专门钉"回落值写错 / 判断的是原值而不是转换结果"。
- 三条**真等价**（已删，理由写在文件头）：
  1. `ease_in_out_quint.backward` 的 `ratio < 0.5` → `<= 0.5`（0.5 处上下两支同值 0.5）。
  2. `Times::_value` 的类内成员初值（构造器里的 `set_range` 会覆盖它，死存储）——
     但 `_lifes` / `_remains` 的初值**不会**被覆盖，是可观测的。
  3. `Times::add` 的 `_remains > 0.0` → `>= 0.0`（能到这行就说明 `!= 0`）。
- **判"某个字段有没有差分覆盖"时先看 harness 有没有读过它**：`Times::is_max` / `is_min` 原先
  从没被任何 op 读过 ⇒ 加 `times_is_max` / `times_is_min` 才可观测。

### 6.9.52 `json` 加固（差分 250 行，变异 76/76 全杀）

- op：`jstr <valueliteral>`（`JSON.stringify`，`undefined` 渲染成 `-`）、
  `jparse "<JSON文本>"`（先打 `ok`/`err`，ok 时再逐层 dump）。
- **`jparse` 的输出行数随内容变**，所以不能再用"用例行数 == 输出行数"来自检空转；
  这里靠"每个语义分支都至少有一条能翻盘的用例"来保证。
- **两条踩到的用例缺口（补样本要沿变异点的分岔方向）**：
  1. **`\uXXXX` 的十六进制有 `0-9` / `a-f` / `A-F` 三段** —— 样本里全是小写与数字时，
     "删掉大写分支"完全不可见。补 `"\u00E9"` / `"\uD83D\uDE00"` / `"\u00aB"` / `"\uABCD"`。
  2. **删除某条检查后别让下游检查兜住它**：`{"a" 1}` 看似是"为冒号检查写的用例"，
     把 `!= u':'` 删掉后它依然 err（跳过空格与 `1` 会撞上 `}`，由"键必须是字符串"兜住）⇒ 空转。
     要构造**跳掉一个字符后恰好拼得出合法文档**的输入：`{"a"9 1}` / `{"a"1 2}`。
- **故意不打的 5 类（构造上不可判别，别浪费跑批时间）**：
  1. `quote()` 的 `(c >> 12) & 0xf` —— 该分支只处理 `c < 0x20`，`c >> 12` 恒为 0。
  2. `str()` 的 `i + 4 > s.size()` → `i + 3 > ...` —— 能过检查又四字符全十六进制的输入不存在。
  3. `digit()` 的 `i < s.size()` → `i <= s.size()` —— `s[size()]` 是 `'\0'`。
  4. `write()` 末行 `if (o == nullptr) return false;` —— 不可达。
  5. 去掉 `++i` / 去掉越界判断 / 递归自调用 —— 死循环或越界读（UB），不是"漂移"。


### 6.9.53 `collections` 加固（差分 194 行，变异 68/68 全杀）

- op 新增：`fisrt_any` / `last_any`（1 实参重载）、`intersection_lt`（显式 `std::less<double>`）、
  `map_arr_nil`（空 / `null` 输入）、`ensure_val <valueliteral> | <valueliteral…>`
  （`Value&` 重载，按项数分派 1 项 / 多项）。
- **四处"harness 没把实参用上 ⇒ 变异不可见"**：
  1. 1 实参的 `fisrt()` / `last()` 此前**没有任何 op**（只有 2 实参版）⇒ `fisrt_any` / `last_any`。
  2. `map_arr` / `loop_arr` 的回调此前只用了第一个实参
     （`[k](double v, auto, const std::vector<double>&) { return v * k; }`）⇒
     **下标写错 / 第三参传空 vector 全部不可见**。改成 `v * k + i * 10 + arr.size()`（两侧同式）。
     注意 `loop_arr` 一直可观测（它本来就把收到的下标打出来），同族两个函数"一个测到一个没测"。
  3. `ensure.h` 的 `Value&` 重载从未被任何 subject 观测（哪怕生产调用点全在它上面）⇒ `ensure_val`。
  4. `intersection` 默认谓词是 `equal_to`，`push_back(c1)` ↔ `push_back(c2)` 在匹配点恒等
     ⇒ 必须显式换谓词（`intersection_lt`）才可分辨。
- **一条真等价**：`nested_map::clear()` 末尾 `_map.clear();` 删掉 —— 内层 map 已被逐个清空，
  残留的空内层 map 让 `get`/`has`/`remove` 一律未命中（与"外层键不存在"同观），
  之后的 `set` 走新建分支还是"已有 k1"分支结果一致，差别只有私有对象池里空 map 的条数 ⇒ 不可观测。
- **不值得打的（构造不可判别 / UB）**：graves 的 `_l[_i--]`、让 `_i` 越界回绕的三种改法、
  `ensure` 模板版守卫取反、`map_arr` 的 `!list.has_value()` 守卫、`loop_offset` 不可达的
  `idx >= len`、对象池回收与 `if (_map.empty()) return;` 的删除 —— 逐条理由见
  `mutations/collections.mjs` 头部第 1–12 条。
- 本 subject 最慢的一批是**头文件**变异（每条都要全量重编译）：6.9 s/变异量级；
  作为对照，`value` 那轮 2.6 s/变异（多为 `.cpp`）。


### 6.9.54 `core` 加固（差分 10969 行，变异 112/112 全杀）

- 用例：`core/js_num` 85→116、`core/to_number` 152→197（`number_to_string` 98 与 fuzz 10558 不动）。
- **三条补样本的通用形态**：
  1. **常量表缺项**：`is_str_white_space` 的 14 个码点里 `\v`(0x000b) / `\f`(0x000c) 从没被用过 ⇒
     补 `"\u000b1"` / `"1\u000c"` 一类样本后 14 条码点变异全杀。
  2. **修正分支只在极窄域生效**：`Math.round` 的 `r - x > 0.5` 要 ≥2^52 的**奇数**才会触发。
  3. **路径门槛**：`parse_radix` 的舍入（`keep/rest/half/sticky`）只在输入 >64 位（`shifted > 0`）时
     才走 ⇒ 60 位的"tie"（`0x40000000000004`）是空转。补 `0x10000000000000800`（half-even 不进位）与
     `0x10000000000000810`（sticky 真 ⇒ 进位到 `2^64+4096`）后 5 条舍入变异全杀。
- **自查工具必须先对拍**：自己写的 `parse_radix` 模拟器在**无 `0x` 前缀**的字符串上用 `from = 2`，
  等于丢了前两位十六进制，据此"搜到"的样本是别的数 ⇒ 两句无效样本、2 条变异假幸存。
  改成"模拟器 vs 真实 exe 逐条比位模式"才发现（注意 `execFileSync` 捕获的 `\r` 会造成假 MISMATCH）。
- **两条真等价**（已删）：`js_to_int32` 的分界 `0x80000000u → 0x7fffffffu`（MSVC 的窄化就是截低 32 位，
  两支恒等）；`parse_radix` 的满位阈值 `64 → 63`（只改移位时机，指数/尾数不变、低位进 sticky ⇒
  判决不变，20 万条随机长输入零差异）。
- 速度：本 subject 全是 `.cpp`，2.4 s/变异（每条都要跑一遍 10558 行 fuzz）。


### 6.9.55 `transform` 加固（差分 339 行，变异 45/45 全杀）

- **新工具 `native/tools/ts_scope.mjs <root.ts>…`**：按"值可达 / 仅类型可达"两类量依赖闭包，按目录汇总，
  并标注 `native/lfw` 是否已有同目录。判断切片顺序、区分"DI 缝（interface-only）"与"真要搬的值代码"
  全靠它 —— 本轮据此把 `ditto/`（值依赖 0）判为按需注入、把 `buff/`（值依赖 315 文件）排到 Entity 之后。
- ops：`new` / `pos` / `scale` / `rot` / `sx|sy|sz|rx|ssx|ssy|ssz`（属性入口）/ `move` / `scale_to` /
  `rotate_to` / `update` / `arrived` / `snap`（打主值 + 目标值 + smoothing）。缺参一律传"未给"
  （C++ `std::nullopt` / TS `undefined`）⇒ 默认值留在被测代码里，可被变异打到。
- 每条 op 的实参个数都过 `check(lo, hi)` 硬校验（多给/少给立刻 exit 2），防止 arg 计数写错静默吞 token。
- **第一次 drift**：`move_to` 的第 4 参在 TS 是对象 `{rate}`、在 C++ 是 `std::optional<double>`，
  harness 起初把裸数字传给 TS ⇒ TS 静默用默认 0.1。此类"两侧形参形状不同"的接口必须在 harness 里显式翻译。
- 两条用例缺口（到达吸附、setter 清 smoothing）见 DESIGN §4.66。


### 6.9.56 `ground` 加固（差分 310 行，变异 58/58 全杀）

- ops：`clear` / `seg <type> <x1> <x2> <z1> <z2> <h1> <h2> [id]` / `base` / `abyss` / `step` /
  `y <idx> <x> <z>` / `segment <x> <z>` / `enterable <idx> <x> <y> <z>` /
  `block <idx> <x> <y> <z> [px] [py] [pz]` / `intersect <6>` / `wall <6>`。
  地形用一个可变 `vector`（C++）与同一个数组对象（TS 桩 `{bg:{data:{terrain}}}`）承载，
  索引越界/多给少给 token 一律 exit 2。
- **三条补样本的通用形态**（本轮 10 个幸存者全部由此消灭）：
  1. **被跳过的对象必须能改变赢家**（同高段的边界样本是空转）。
  2. **镜像几何会命中另一分支**（`x1>x2` 的段上，"左墙"实际走的是 `seg.x1` 分支）。
  3. **阈值要构造"恰好等于"与"恰好跨过"**（`> _step` 需要墙比射线高 >10；`<= _step` 需要表面恰好高出 y1）。
- 一条**真等价**：`intersect_wall` 的 `max_h - min_y <= _step` → `<`。证明见 DESIGN §4.67。
- 观测口径：所有数值走 `num_hex`（NaN → "nan"，其余位模式）；可缺的字符串字段打印成 `-`。
- 顺带记录 `--rank`：`native/tools/ts_scope.mjs --rank <dir>` 按"未移植值闭包行数"排序，
  用它纠正了三处切片误判（`ditto` 是 DI 缝、`buff` 依赖 Entity、`WorldDataset` 并非叶子）。

### 6.9.57 `controller_input` 加固（差分 104 行，变异 72/72 全杀）

新增 subject `controller_input`，观测量来自三族 op：

- `ks <sub>`：`new` / `hit <name> <key> <值|-> <time>` / `end` / `use` / `reset` / `load` / `snap` / `slot` / `flags` / `raw`
- `res <sub>`：`new` / `fire <nf> <time> <keys> <kind>` / `fire2` / `clear` / `snap`
- `gktable <labels|label|agk|conflicts|conflict>`

值字面量一律用带标签写法：`n 5` / `s "x"` / `a 3 …` / `o 2 k v k v` / `u` / `z` / `b 1`。
**`o N` / `a N` 的计数必须与实际元素数一致**，否则解析器会越界读；已在 `trace_util.h` 的 `o` 循环
补边界断言，错误改为显式报错退出。

差分记录：`ControllerResult::fire` 的守卫改为 JS 真值判定（`fire(nf = 0)` 必须失败）。

### 6.9.58 `base_controller` 加固（差分 866 行，变异 133/133 全杀）

harness op：

- `env <sub>`：`hit_dur` / `dbl_int` / `facing` / `alive` / `human` / `bot` / `team` / `pos` / `fstate`，
  以及映射位 `kd` / `ku` / `hf` / `hl` / `pre` / `post` / `seq` / `tpre` / `tpost` / `dpre` / `dpost`
- `ctl <sub>`：`new` / `reset` / 队列 `start` `hold` `end` `db_hit` `click` `dbl_click` `kd` `ku` `ck` /
  `update` / `tst` / `flags` / `dbhit` / `lr|rl|ud|du|jd|dj` / `keys` / `dbs` / `kraw` / `seqtest` /
  `sametest` / `log` / `q` / `envdump`

写用例时必须留意的三件事（都因变异存活才暴露出来）：

1. **判空顺序**：环境里每个映射位都要显式设置；`ctl new` 之后若紧接着发 `env bot 0`，
   「默认非机器人」这条就再也观测不到。
2. **门的独立性**：同拍门（`keys.d.is_hit()`）与顺序门（`_key_list` 长度 ≥ 3）会在同一次 update 里互相吃掉。
   要单测顺序门，必须在映射**尚未挂载**时先建出 `_key_list`，等命中窗口全部过期，再挂映射。
3. **等价类**：`d0.facing != -1 || d1.facing != -1` 这类「双操作数同向」的判定，
   只有**混合朝向**才能让单侧改值产生差异；单侧朝向时两个操作数互相兜住，等于等价。

harness 教训：**会改状态的调用与状态快照不可同处一个表达式**。
`emit("… " + flag(ctl.is_db_hit(k)) + " " + render(ctl.dbc.to_snapshot()))` 在 MSVC 下会先算快照，
把 `step()`（`time = -time`、`data[0] ← data[1]`）的效果吃掉，制造假漂移。必须先取值、再格式化。

### 6.9.59 `collision_handlers` 加固（差分 60 行，变异 30/30 全杀）

harness op：

- `env aid "A"` / `env vid "V"` / `env rest <n>` / `env create_ok <0|1>`
- `env itr <值>` / `env dataset <值>` / `env itr_motionless <值>`
- `run <super|picked|stiff|goto|rest|flute>`

输出为「调用序列 + 落地值」：

```
run flute set_arest:n0 buff_get:magic_flute_to_V:b0 create:magic_flute:magic_flute_to_V:b1 \
  set_attacker:magic_flute_to_V:A set_victim:magic_flute_to_V:V mount:magic_flute_to_V \
  | motionless=b1 shaking=u arest=n0 buffs=n1
```

TS 侧要用**带 setter 的假 Buff**并在 `create_buff` 时把同一个对象放进 `buffs`，
否则第二次取回的是朴素对象，`buf.lifetime = 0` 不会进日志，两侧就对不齐。

### 6.9.60 `buff` 加固（差分 139 行，变异 61/61 全杀）

harness op：

- `env entity <id|@> <0|1>` / `env pos <id> x y z` / `env frame <id> centery height pic_h`
- `env data <值>` / `env create_entity_ok <0|1>` / `env create_buff_ok <0|1>`
- `buff new <id> <kind值>` / `buff use <id>`（切到 grant 出来的 buff，**别名**语义）
- `buff hook <none|update|tick|end>` / `buff oid <s>` / `buff fid <s>`
- `buff level|lifetime|duration|ticks <n>` / `buff attacker_id <s>` / `buff attacker_entity <s>`
- `buff victim|add_victim|del_victim <id>` / `buff del_id <id>` / `buff reset <id>`
- `buff mount|unmount|update <d>` / `buff place_center|show|del_fx <id>` / `buff clear|upd_fx`
- `buff read <快照值>` / `buff snap` / `buff log`
- `grant <kind> <攻击者id> <受害者id> <duration>`

输出 = 「状态字段 + 调用序列」，状态含 `id/lvl/mounted/lifetime/duration/ticks/dead/aid/atk/nfx/victims`。

写用例时的两个陷阱：
1. `update_effects()` 会**先清过期再重建**，所以想打 `show_effect` 的「不校验存活」必须直接调 `show`，
   走 `upd_fx` 会被前面的清理掩盖。
2. `Times.add(d)` 的差异要在**全新 buff**（`_value` 为 0）上打，否则 `add(1)` 也可能正好触顶，
   两侧都触发 hook 而看不出差别。

### 6.9.61 `collision_core`（差分 258 行，变异 72/72 全杀）

harness op：

- `env dataset <值>`（`min_vrest` / `vrest_offset` / `itr_arest`）
- `env a <字段> …` / `env v <字段> …`：
  `id`(裸字面量) `pos`(3 个值) `data_id`(裸字面量) `data_type` `frame` `prefabs` `bear`
  `marks`(0/1) `dropping`(0/1) `arest` `catcher`(0/1) `hurtable` `invul` `bot_ignore`
  `team` `emitter` `spawn` `bot`(0/1)
- `env itr <值>` / `env bdy <值>` / `env ally 0|1` / `env vrest 0|1` / `env dev 0|1`
- `env load_ok 0|1` / `env load_names <数组>` / `env idx <i> <j>` / `env new_id_base`
- `env pool <值>` / `env pool_ok 0|1` / `env pool_handlers <n>`
- `new` / `get` / `test` / `snap` / `snapset <字段> <值>` / `from_snap` / `clone` /
  `slot 0|1` / `state`

约定与陷阱：
1. **裸字面量 vs 值语法**：`env a id "A"`、`snapset aid "A"` 用裸字面量；
   其余值位置（含 `pos` 的每个分量、`snapset` 的每个数字）都要走值语法
   （`n <数>` / `s "串"` / `a <个数> …` / `o <数> <k> <v>…` / `u` / `z` / `b 0|1`）。
2. `env a pos` 与 `data_type` 在两侧都按值语法解析（历史上写成裸数字会直接解析失败）。
3. `__tester` 在真代码里是 `Expression` 实例：`run(c)` 是方法、`debug` **也是方法**，
   且判定是 `if (c.lfw.dev && bdy.__tester.debug) Ditto.Log('bdy.__tester:', bdy.__tester.debug())`
   —— **dev 为真就记录，不看返回值真假**。harness 里 TS 侧把 `__tester` 包成
   `{run, debug()}`，C++ 侧 `tester_run` / `tester_debug` 产生同样的日志；
   TS 侧同时把 `Ditto.Log` 重定向到同一日志（两边都用 `String(x)` 口径渲染）。
4. `get_bounding` 由「假世界」提供：读 `frame.__cube`（6 个数）。
5. 对象池语义：`acquire_collision` 返回带预设字段的新对象（`pool` / `pool_handlers`
   控制），`pool_ok 0` 走 TS 的 `|| {}` 分支。
6. `get` 的观察点：`new`/`get`/`from_snap` 的结果会被写进 `slot`，
   用 `slot 0|1` 切换、`state` 打印；`clone` 会切到另一个 slot，
   于是「一边 `load_handlers` 清空、另一边也变」这种共享语义可以直接看到。

### 6.9.62 `action_handlers`（差分 110 行，变异 70/70 全杀）

harness op：

- `env injury <值>` / `env real_injury <值>`（碰撞体上的两个伤害量）
- `env data_found <值>` / `env no_data`（`lfw.datas.find` 的结果）
- `env ally 0|1`（`attacker.is_ally(victim)` 的结果）
- `env mt_int <n>`（`lfw.mt.int()` 的返回值，用于 `FUSION` 的随机分支）
- `env a|v|e <字段> …`：`hp` `hp_r` `hp_max` `mp` `mp_max` `vel`(速度 x) `face` `team`
  `data` `bot`(0/1，同时影响 `is_bot_ctrl`) `src_emitter` `emitter` `bearer`(0/1) `fuse_bys`
- `act <类型字面量> <动作值>`（类型是**裸 JS 字符串字面量**，动作整体走值语法）

输出：`act <类型> ret=<返回値> || <调用日志> | <状态快照>`；
状态包含双方 hp/hp_r/mp/速度/朝向/队伍、受击方的无敌三连、`fuse_bys`(只印 id)、
`dismiss_data`/`dismiss_time`、双方 bot 标志，以及本动作创建出的 buff 的
`lifetime/duration/level`（`buff=none` 表示没建）。

三条必须注意的假实现约定：
1. `victim.data.type` 是**从 `data` 里取 `type`**，所以用例必须给实体设 `data`，
   不能只设一个独立的「类型」字段。
2. 假 `play_sound(sounds, pos = this.position)` 要带默认参数，才能复刻真 `Entity` 的行为。
3. `grant_buff` 会真的跑起来（C++ 侧用真 `buff::Buff`、TS 侧用假 buff 对象），
   所以 `BuffEnv`/`factory`/`world.buffs` 必须齐备；只有 `create_buff` 这一件事被记录，
   其余 setter 静默 —— 事后用「回读 buff 的 lifetime/duration/level」来观测施放结果。

### 6.9.63 `collision_handlers2`（差分 63 行，变异 49/49 全杀）

harness op：

- `env itr <值>`（`injury` / `arest` / `motionless` / `shaking` / `catchingact` / `caughtact` / `dvx` / `dvz`）
- `env dataset <值>`（碰撞体的 dataset，给 `handle_rest`/`handle_stiffness` 用；
  `handle_injury` 的恢复率另见 `env recov`）
- `env rest <值>` / `env recov <值>`（`world.dataset.hp_recoverability`）
- `env vel <值>` / `env velz <值>`：**仅 C++ 侧**注入 `calc_itr_velocity` 的 x/z 分量；
  TS 侧是真算，所以这两个 op 在 TS 侧只做占位消费
- `env a|v <字段> <值>`：`hp` `hp_r` `type`(决定 `is_fighter`) `weight` `fall` `fall_max`
  `defend` `defend_max` `resting` `catch_max` `catching`(0/1) `catcher`(0/1) `marks`(0/1)
  `elec_dur` `itr_fall` `hit_sounds` `motionless` `src_emitter` `ice` `state` `face` `dataset`
- `run injury [scale] [keep]` / `run catch` / `run freeze` / `run efreeze` / `run shield`

输出：`run <名字> || <调用日志> | <状态>`；状态含双方 hp/hp_r/fall/defend/resting/toughness/
catch_time/catching/catcher/shaking/motionless/速度，以及碰撞体的 `inj/inj_r/rinj/rinj_r`
和本次创建的 buff（`lifetime/duration/level`）。

三条假实现约定：
1. 假实体的 `dataset(key)` 要把 `electrify_duration` 映射到用例的 `elec_dur`，
   `world.dataset` 要是「用例 dataset + `hp_recoverability`」的合并视图。
2. `data` 要按实体返回各自的 `type`（默认 8=Fighter），`indexes.ice` 与 `base.hit_sounds`
   也从用例取值。
3. `summary_mgr.apply_damage` 与 `Ditto.warn` 都被重定向到同一日志；
   日志里实体一律用 `A:`/`V:` 前缀。
### 6.9.64 `collision_handlers3`（差分 267 行，变异 78/78 全杀）

harness op：

- `env itr <值>`（`bdefend` / `injury` / `fall` / `motionless` / `shaking`）
- `env dataset <值>`：碰撞体 dataset，供 `handle_rest` / `handle_stiffness` 与护甲的世界级兜底
- `env rest <值>` / `env recov <值>` / `env itr_motionless <值>` / `env armorwork 0|1`
- `env acube <值>` / `env bcube <值>`：`{left,right,bottom,top,near,far}`。假 `spark_point`
  固定取 `(a.left, b.right, a.top)`，两侧同式，用来捕捉「两个 cube 写反」的变异
- `env a|v <字段> <值>`：**一行只允许一个字段，出现多余 token 直接报错退出**。
  字段：`hp` `hp_r` `tough` `tough_max` `velx` `vely` `velz` `posx` `posy` `posz` `team`
  `state` `base_type` `bearer`(0/1) `type`(8/16/32) `armor` `in_the_sky` `hit_sounds`
  `itr_fall` `dataset` `ice` `marks`(0/1) `elec_dur` `src_emitter` `motionless` `fall`
  `fall_max` `defend` `defend_max` `resting` `catch_max` `catching` `catcher`
- `run whirlwind` / `run ballhit_a` / `run ballhit_b` / `run armor`

输出：`run <名字> || <调用日志> | <状态>`。日志含实体动作（统一 `A:` / `V:` 前缀）、
`spark:x:y:z:类型`、`snd:类型:x:y:z`，以及只有 `armor` 才有的 `ret:0|1`；状态含双方
hp/hp_r/tough/tough_max/速度/team/state/motionless/shaking 与碰撞体的 `inj/inj_r/rinj/rinj_r`，
外加本次施放的 buff 的 `lifetime/duration/level`。

三条约定：
1. **`env armorwork` 是 `is_armor_work()` 的替身**。TS 侧真函数由 `bframe.state` 驱动，
   harness 把 `armorwork 0` 映射成 `bframe.state = Injured`，因此**不能**在 `armorwork 1`
   的同时又把 `itr.bdefend` 设到 200——那种组合下真函数会返回 false 而 C++ 侧仍是 true。
   `bdefend >= 200` 这条分支属于 `is_armor_work` 自己的单元，不在本 harness 覆盖范围内。
2. 假实体的 `dataset(key)` 先查实体自身、再落到世界 dataset；在用例可控范围内与 TS
   `Entity.dataset()` 的 `frame.dataset ?? data.base ?? world.bg.data.dataset ?? world.dataset`
   等价。
3. `handle_armor` 结尾会跑 `handle_injury`，所以 `summary_mgr.apply_damage` 与
   `factory.create_buff` 要像 6.9.63 那样接上（Electroshock 分支靠 `marks=1` + Fighter
   受害者触发）。

### 6.9.65 `collision_handlers4`（差分 84 行，变异 81/81 全杀）

harness op：

- `env itr <值>`（`bdefend`）/ `env bdy <值>`（`kind`）/ `env aframe <值>`（`behavior`）
- `env dataset <值>`：碰撞体 dataset，供 `handle_rest` / `handle_stiffness` 使用
- `env rest <值>` / `env itr_motionless <值>`
- `env a|v <字段> <值>`：**一行只允许一个字段，出现多余 token 直接报错退出**。
  字段：`hp` `hp_r` `state` `facing` `base_type` `velx` `vely` `velz` `frame_id`
  `throwings` `in_the_sky` `arest` `dropping`(0/1) `hit_sounds` `type`(8 = Fighter)
- `run ballhitother` / `run weaponhitother`

输出：`run <名字> || <调用日志> | <状态>`。日志含实体动作（统一 `A:` / `V:` 前缀，覆盖
`set_hp` / `set_hp_r` / `set_velocity` / `set_arest` / `enter_frame` / `set_dropping` /
`play_sound`）、`handlers` 缝的 `A:set_motionless` / `V:set_shaking` / `A:set_arest` /
`add_v_rest`，以及 `faf:<frame_id>:<throwings>:<in_the_skys>`；状态含双方
`hp/hp_r/state/facing/base_type/vel/arest/dropping/frame_id`。

三条约定：
1. 假 `find_align_frame` **固定返回 `"faf_result"`**，三个入参只进日志不进返回值。因此
   「把 `enter_frame` 的实参写死成 `"faf_result"`」这类变异仍然不可观测（返回值本就等于该
   常量），变异规格里用的是「换成 `a->frame_id()`」这种可观测写法。
2. `handlers` 缝的 `attacker_set_arest` 会**回写假实体的 `arest`**：TS 侧 `attacker.arest = ...`
   走的就是实体 setter，C++ 侧若只打日志不回写，状态行会漂移。
3. `env a type` 驱动 `Handlers4Env::is_fighter`（真身是 `entity::is_fighter_data(data)`），
   所以 `type = 8` 是 Fighter、`type = 0` 不是；「非 Fighter 受害者」靠 `type = 0` 构造。

### 6.9.66 `collision_weapon_is_hit`（差分 115 行，变异 85/85 全杀）

harness op：

- `env itr <值>`（`injury` / `bdefend` / `fall` / `motionless` / `shaking`）
- `env dataset <值>` / `env recov <裸数字>` / `env itr_motionless <值>` / `env rest <裸数字>`
- `env velx|vely|velz <裸数字>`：`calc_itr_velocity` 缝的注入值，语义等同 `itr.dvx/dvy/dvz`
- `env acube <值>` / `env bcube <值>`：`{left,right,bottom,top,near,far}`。假 `spark_point`
  固定取 `(a.left, b.top, a.near)`，两侧同式，用来捕捉「两个 cube 写反」的变异
- `env a|v <字段> <值>`：**一行只允许一个字段，出现多余 token 直接报错退出**
- `run hit`

输出：`run hit || <调用日志> | <状态>`。日志含四个前置 handler 的痕迹（`set_arest` /
`set_motionless` / `set_shaking` / `set_hp(_r)` / `set_toughness` / `summary:...`），以及
`set_dropping` / `sp:...` / `spark:...:<kind>` / `set_velocity:...`（只列实际写入的分量）/
`leave_ground` / `mark:<tag>` / `pick:<indexes>` / `enter_frame_by_id` / `set_team`；状态含双方
`hp/hp_r/tough/state/face/base_type/team/bearer/dropping/on_ground/data_id/vel` 与碰撞体的
`inj/inj_r/rinj/rinj_r`。

三条约定：
1. `env velx/vely/velz` 的语义是 **`itr.dvx/dvy/dvz`**，不是最终速度：假 `calc_velocity` 还会再乘
   `attacker.facing`（即真算式里的 `x_direction`）。所以 `env a face -1` 时注入 7 得到 -7，
   与真算式一致；把朝向因子漏掉是本轮第一次差分的唯一残留漂移。
2. 受害者刻意设为非 Fighter（`type = 16`），令真 `is_fall()` 恒为真，`calc_itr_velocity` 的 y 分量
   才等于 `dvy`；否则真算式给 0 而缝给注入值，会造出假漂移。
3. `itr.fall` 的临界值是 **100**（`140 - DEFAULT_FALL_VALUE_DIZZY`）。用例卡 99/100；
   卡 139/140 会让 `is_fly` 的三条变异全部不可观测。

### 6.9.67 `collision_fall`（差分 113 行，变异 84/84 全杀）

harness op：

- `env itr <值>`（`fall` / `effect`）
- `env dataset <值>` / `env acube <值>` / `env bcube <值>`
- `env velx|vely|velz <裸数字>`：`calc_itr_velocity` 缝的注入值
- `env a|v <字段> <值>`：**一行只允许一个字段，出现多余 token 直接报错退出**
- `run fall`

输出：`run fall || <调用日志> | <状态>`。日志顺序固定为 `set_toughness` → `set_fall_value` →
`set_defend_value` → `set_resting` → `set_velocity` → `sp:` → `spark:x:y:z:<kind>` →
[`drop_holding`] → `enter_frame` / `enter_frame_by_id`；状态含双方
`hp/hp_r/tough/fall/fall_max/defend/resting/state/face/vel`。

三条约定：
1. `env velx|vely|velz` 的语义是 **`itr.dvx/dvy/dvz`**：假 `calc_velocity` 会再乘 `x_direction`，
   并把同一个方向值写进 `ItrVelocity::x_direction`（也就是 `turn_face` 的输入）。为对齐真算式，
   假实体把 `weight` 与 `ivx_f/ivy_f/ivz_f` 固定为 1，且 `fall_value` 在该函数开头就被清零
   ⇒ 真 `is_fall()` 恒为真 ⇒ y 分量正好等于 `dvy`。
2. `x_direction` 的 position-based 分支必须在缝里复刻：`effect ∈ {FireExplosion(22),
   Explosion(23)}` 或 `attacker.state == HeavyWeapon_InTheSky(2000)` 时为 -1，否则取
   `attacker.facing`。假实体位置恒为 0，所以 `diff_x > 0` / `diff_x < 0` 两条都不成立。
3. `critical_hit` 支持对象（`{1: [...], -1: [...]}`）与数组两种形态；**缺键时 TS 会抛
   `TypeError`**，所以用例只覆盖「对象含两个键」与「数组且方向为 +1」两类。

### 6.9.68 `collision_n_bdy_normal`（差分 175 行，变异 100/100 全杀）

harness op：

- `env itr <值>`（`effect` / `injury` / `bdefend` / `fall` / `motionless` / `shaking`）
- `env dataset <值>` / `env bframe <值>` / `env aframe <值>` / `env armorwork 0|1`
- `env acube <值>` / `env bcube <值>` / `env rest <裸数字>` / `env itr_motionless <值>`
- `env velx|vely|velz <裸数字>`：`calc_itr_velocity` 缝的注入值
- `env a|v <字段> <值>`：**一行只允许一个字段，出现多余 token 直接报错退出**
- `run hit`

输出：`run hit || <调用日志> | <状态>`。一次 run 只会出现其中一条路径的日志（`Ignore` 直接返回；
armour 命中短路；四个 effect 分组各自成套；impact 路径要么 `is_fall` 早退、要么走完全程）。

三条约定：
1. `env velx|vely|velz` 的语义是 `itr.dvx/dvy/dvz`，缝里要复刻真算式的三件事：`x` 乘
   `x_direction`（`position_based` 时恒为 -1）、**`y` 只在 `is_fall` 为真时才取注入值**（否则为
   0）、`z` 直传。漏掉 `y` 的门控会让 impact 路径的 y 分量出现假漂移。
2. `armorwork` 必须与 TS 侧真 `is_armor_work` 同真同假：`armorwork 1` 的用例要让真函数返回 true
   （`fulltime` 真、`bframe.state` 允许、effect 非火非冰、`bdefend < 200`、`aframe.state !== 3006`）；
   `armorwork 0` 的用例只能配「无 armor」或让 `bframe.state` 落在失效集合（如 `Injured`）。
3. 每次进入 impact 尾段之前都要**显式重置 `fall`**（`env v fall n ...`）：`handle_fall` 会把
   `fall_value` 清零，而 `is_fall` 对 `fall_value <= 0` 恒真 ⇒ 不重置就会整段尾逻辑被早退跳过
   （首轮 25 条变异存活全是这个原因）。

### 6.9.69 `collision_n_bdy_defend`（差分 110 行，变异 65/65 全杀）

harness op：

- 与 6.9.68 相同，另加 `env bdy <值>`（防御方 bdy），以及 `env a|v defend_ratio <值>`。
- `env itr <值>` 里可用 `effect` / `bdefend` / `injury` / `actions`；`bdy.actions` 走 `env bdy`。
- `run hit`。

输出：`run hit || <调用日志> | <状态>`。日志里 `dispatch:<handler_type>:<action>` 表示 action 分发；
整段只会有三种形态之一：守卫移交（出现 `handle_itr_normal_bdy_normal` 的痕迹）、破防分支
（`spark:...:broken_defend` + 两组 `A_BROKEN_DEFEND` 过滤）、未破防分支
（`spark:...:defend_hit` + 两组 `A_DEFEND` 过滤）。

三条约定：
1. **action 分发是缝**，不是真 `run_action`：`dispatch` 的第一个参数就是 TS 里的 handler key
   （`A_NEXT_FRAME` / `V_NEXT_FRAME`），第二个参数是原始 action 对象。因此「分发给哪个 key」
   与「`action.type` 的白名单」都可观测，而 handler 本体由 `action_handlers` 单元覆盖。
2. `actions` 只按数组迭代；TS 对非数组值会抛 `TypeError`，端口用 `as_array` 守卫（宽容）。
   用例只用数组与 `null` / 缺失。
3. **每条防御用例之前都要重置 `defend`**：`defend_value -= bdefend` 会累积，不重置就会从
   「未破防」掉进「破防」，三条 bdy 分支的变异会因此全部存活。

### 6.9.70 `collision_ball_frozen`（差分 151 行，变异 89/89 全杀）

harness op：

- `env itr <值>`：`kind` 用值语法（`env itr o 1 kind n 0`，字符串形式 `s "0"` 也在用例里）。
- `env a|v group <值>` / `state` / `type` / `face` / `frame` 走值语法；
  `posx` / `posy` / `posz` 走裸数字；`spawn 0|1` 是裸标志。
- `run hit`。

输出：`run hit || <调用日志> | <双方状态> | ret=0|1`。
日志里 `A:spawn:oid:kind:x:y:z:action:face` 与 `V:enter_frame:gone` 带实体身份。

三条约定：
1. **`frame` 是对象**：`env a frame o 4 centerx n 5 centery n 6 width n 0 height n 0`。
   只要对象里字段数与 `o <n>` 不符，harness 会直接报 `value literal truncated`。
2. **`spawn` 的返回标志要分别设在 `a` 和 `v` 上**：守卫读的是 `a.spawn` 的返回值，
   只改 `v` 那条不能覆盖「忽略返回值」这一支。
3. 用例里凡是「影响分支判定」的字段（`group` / `state` / `type` / `face` / `frame` /
   `pos*` / `spawn`）在切换场景时都要显式重置，别依赖上一轮残留。

### 6.9.71 `collision_healing`（差分 55 行，变异 22/22 全杀）

harness op：

- `env itr <值>`：`injury` 用值语法，字符串（`s "6"`）与缺失都要覆盖。
- `env a|v dataset <值>`：**只用对象**。`dataset` 是实体上的普通属性读，传 `null` 属于契约外。
- `run heal`。

输出：`run heal || <调用日志> | <双方 dataset> buff=<id>/<lifetime>/<duration>/<level>/<attacker>`；
无 buff 时写作 `buff=none`。

三条约定：
1. **`duration` 必须显式打印**：handler 返回 void，公式算错只能从 `grant_buff` 之后的 buff 状态
   里看出来。harness 的 `create_buff` 要把返回的 `Buff*` 存进全局，状态文本再读它。
2. **假 buff 实体的 id 要和实体一致**（见 DESIGN §19.3），否则 buff id 会分叉。
3. 用例要同时覆盖：`injury` 的假值（`0` / `""` / 缺失 / 键名写错）、`ceil` 的进位与整除、
   两个 `max(1, …)` 的钳制（含 `0` 与 `0.5`）、dataset 缺键造成的 NaN、字符串 `injury` 的强转。

### 6.9.72 `collision_keeper`（差分 108 行，变异 45/45 全杀）

harness op：

- `env a|v type <裸数字>`：`collision.attacker/victim.data.type`。
- `env a|v state <裸数字>`：供 `a_state` / `v_state` 过滤使用（目前只有 `v_state` 有用例）。
- `env itr <值>` / `env bdy <值>`：必须**都存在**，真实现会直接读它们的 `.kind`。
- `env handlers <值>`：预置 `collision.handlers`（数组，元素被当作名字）。
- `run load`。

输出：`run load || handlers=[<名字,…>] | ret=0|1`。

三条约定：
1. **一条用例一个探针**：表格里有 19 条配置，每条至少要有一个能命中的 `(a,v,itr,bdy)` 组合，
   否则「删掉该条」或「改该条的枚举」都不可观测。
2. **要专门造「同 key 多命中」的用例**（`(Ball, Normal, Ball, Normal)`）以及
   **状态过滤**的用例（武器三条配置的 `v_state`）。
3. `pack_a` / `pack_b` 的位移与 `|` / `+` 都是不可观测的（双向自洽、无碰撞）；
   想让它可观测必须**制造键碰撞**（丢掉 kind）。

### 6.9.73 `collision_keeper_handle`（差分 77 行，变异 48/48 全杀）

harness op：

- `env dev 0|1`：对应 `Ditto.DEV` / `CollisionCoreEnv::dev`。
- `env data <值>`：包成 `{base: <值>}`，模拟 `victim.data`。
- `env a|v id s "…"` / `env a|v type <裸数字>` / `env a|v state <裸数字>`。
- `env itr <值>` / `env bdy <值>`：`kind` 与 `actions` 都放这里；`actions` 的每个元素是
  `o 3 type s "…" pretest b 0|1 tester o 1 r b 0|1`。
- `env handlers <值>`：handler **名字**数组（两端都会做成同名桩函数）。
- `run hunt`。

输出：`run hunt || <日志>`，无状态文本。

三条约定：
1. `itr` 与 `bdy` 必须都存在（真实现直接读它们的 `.kind`）。
2. **`actions` 只能是数组或 `null`**：`actions?.map` 对非数组会抛 `TypeError`，端口用
   `as_array` 宽容 ⇒ 非数组属契约外。
3. 想观测「测试预计算的时机」，同一个 run 里必须**同时**有 handler 日志与 tester 日志。

### 6.9.74 `buff_marks`（差分 41 行，变异 30/30 全杀）

harness op：

- `env cls s "group_attack"` / `s "electrify"`：选类。
- `env kind s "…"` / `env id s "…"`：构造参数（`kind` 不参与子类逻辑，只为快照/工厂）。
- `env victim s "V1"`：追加一个受击者（累计；`run make` 时全部挂上）。
- `env vpos o 3 x n … y n … z n …`、`env vframe o 3 centery n … height n … pic_h n …`：
  作用于**最后一个**追加的受击者。
- `env mark o 2 key s "…" value s "…"`：给最后一个受击者**覆盖式**写一个标记。
- `run make` / `run mount` / `run unmount` / `run effect`（`effect` 即 `update(0)`）。

输出：`run <op> || <日志> | <状态>`；状态含 `id=`、`victims=[…]`、`marks=[<实体>{key=value,…}]`、`fx=x/y/z`。

三条约定：
1. **受击者是累计的**：第二次以后 `run make` 会带上之前所有受害者，所以「只处理第一个受害者」
   这类变异从第二个场景起就可观测。
2. **想观测 `del_mark` 的条件删除**，必须在 `mount` 之后用 `env mark` 把标记值改成外来值，
   再 `run unmount`。两个类各要一个这样的场景。
3. 状态文本里**不要打印 `mounted`**：端口有 public `mounted()`，但 TS 的 `_mounted` 是
   protected 且没有 getter，TS 侧读不到。挂载与否改从 `world_buffs_set:` 日志观测。

### 6.9.75 `buff_healing`（差分 52 行，变异 42/42 全杀）

harness op：

- `env cls s "healing"` / `s "mp_healing"`；`env kind` / `env id`。
- `env victim s "V1"`（累计）；`env vdata o 4 hp_healing_value n … hp_healing_ticks n …
  mp_healing_value n … mp_healing_ticks n …`、`env vhp o 2 hp n … hp_r n …`、
  `env vmp o 2 mp n … mp_max n …`、`env mark o 2 key s "…" value s "…"`：都作用于**最后一个**受害者。
- `env ticks n …` / `env duration n …`：需要已有 buff（先 `run make`）。
- `run make|mount|unmount|tick <d>|duration <amount>`。

输出：`run <op> || <日志> | <状态>`；`duration` 的输出是 `d=<值>`。

三条约定：
1. **`run tick <d>` 的 `d` 要写成 `>= tips`**（即当前 `ticks`）才能每次触发 `on_tick`；
   想观测「累积」本身就把 `d` 写小、多调几次。
2. `duration_of` 的用例要覆盖：整除与不整除、`value`/`ticks` 为 `0` 的钳制、`0` 与负数金额。
3. 「越界钳制」类变异（`min(hp_r, …)` / `min(mp_max, …)`）必须把**起始值放到离上限不足一次回复量**
   的位置，否则钳制永远不生效。

### 6.9.76 `buff_electroshock`（差分 60 行，变异 22/22 全杀）

harness op：

- `env id s "B1"` / `env kind s "Electroshock"`。
- `env victim s "V1"`：**重新选中**语义（已在列表里就先移除再压到末尾），后续 `env v*` 作用于它。
- `env vtype n 8` / `env vstate n 11`（**值字面量**，可用 `s "12"` 造字符串状态）；
  `env vwait 0`（**裸数字**）；`env vpos o 3 x n … y n … z n …`、
  `env vframe o 3 centery n … height n … pic_h n …`。
- `env duration 8`（**裸数字**）；需要已有 buff（先 `run make`）。
- `run make|init|mount|unmount|tick <d>`。

输出：`run <op> || <日志> | <状态>`；状态含 `id=` / `victims=[…]` / `ticks=` / `dur=` / `life=`，
以及逐受害者的 `state=[…]` / `type=[…]` / `wait=[…]`。

覆盖面（这是杀掉全部 22 条变异的关键，逐条都要留）：

1. **`mount` 的守卫场景必须把列表里**每个**受害者都固定成能被守卫拦下的状态**。
   用例先只用一个受害者（`V1`）跑完 Injured/Falling 的 `n` 与 `s` 四种组合，
   **最后**才引入第二个受害者（`V2`，状态 `n 0`）来观测「两个受害者都被处理」。
   反过来写（先有 `V2`）会让时长无论如何都被 `V2` 减半，四条守卫变异全部存活。
2. **数值与字符串两种状态形态都要覆盖**：`n 11` / `s "11"`、`n 12` / `s "12"`、
   `n 14` / `s "14"`。`mount` 是宽松比较（两种形态都拦），`on_tick` 是严格比较
   （只拦数值形态），差异只能靠字符串用例暴露。
3. **特效居中要用 `centery != height / 2` 的帧数据**：例如 `centery n 4 height n 10`
   ⇒ 居中挂点比裸 `y` 少 `1`。（`centery 4 height 8` 时两者恰好相等，变异会存活。）
4. **`run tick <d>` 的 `d >= ticks`**（`init` 后是 `3`），否则 ticker 不触发、`on_tick` 与
   特效创建都不会发生。
5. 非战士（`env vtype n 16`）用例用来观测 `is_fighter_data` 守卫。

### 6.9.77 `buff_magic_flute`（差分 82 行，变异 58/58 全杀）

harness op：

- `env cls s "mf1"` / `s "mf2"`、`env id` / `env kind`（字符串）。
- `env attacker s "A1"`（字符串；`s ""` = 无攻击者，`run make` 时不再 `set_attacker`）。
- `env victim s "V1"`：**重新选中**语义（已在列表里就先移除再压到末尾），后续 `env v*` 作用于它。
- `env vtype` / `env vstate` / `env vhp` / `env vhp_r` / `env vfallinjury` / `env vtough` /
  `env vteam` / `env vindexes`：**值字面量**；`env ateam`：值字面量，作用于 `attacker` 指向的实体。
  例：`env vindexes o 2 falling o 2 -1 a 2 s "30" s "31" 1 a 1 s "35" in_the_skys a 2 s "40" s "41"`
- `env vvy`：**裸数字**。
- `run make|init|tick <d>`。

输出：`run <op> || <日志> | <状态>`；状态含 `id=` / `victims=[…]` / `ticks=` / `dur=` / `life=`，
以及逐受害者的 `hp=[…]` / `hp_r=[…]` / `fall=[…]` / `tough=[…]` / `team=[…]` / `vy=[…]` / `type=[…]`。

覆盖面（杀掉全部 58 条变异的关键）：

1. **`on_update` 每次 `tick` 都跑**，`on_tick` 只在 `_ticker.add(d)` 为真时跑
   ⇒ 想同时观测两段就必须 `run tick 3`（`init` 后 `ticks = 3`）。
2. **`calc_v` 的四种输入都要给**：`vy = 0`（未达目标，回 `current + acc`）、
   `vy = acc`（恰达目标，原样保留）、`vy > acc`（超过目标，原样保留）、`vy < 0`（反向仍在加速）
   —— 少任何一种，「方向取反」「换模式」「当前与目标互换」都会漏杀。
3. **类型比较是严格的**：必须有 `env vtype s "8"` 与 `env vtype s "16"` 两条
   （否则 `strict_equals` → `equals` 的两条变异必存活）。
4. **状态比较也是严格的**：`env vstate s "12"` 用来区分
   `!strict_equals(state, Falling)` 与 `!equals(state, Falling)`。
5. **两个天空状态要各给一次**（`n 1000` 与 `n 2000`）：前者杀 `&&` → `||`，
   后者杀「第二个条件写成了第一个枚举」。
6. **`in_the_skys` 的数组长度必须 ≥ 2**，才杀得掉 `index_0` 的 `at(0)` → `at(1)`；
   `falling` 也要同时给 `"-1"` 与 `"1"` 两个键才杀得掉键写错。
7. **「字段不重置」陷阱**：每个 `run make` 之前都要**显式重置** `vstate` / `vhp` / `vhp_r` /
   `vvy`（前一轮 `on_tick` 已经把 `hp` 扣过、`vy` 被写回），否则下一段场景静默走偏。

### 6.9.78 `state_base`（差分 46 行，变异 37/37 全杀）

harness op：

- `env victim s "V1"`（字符串）。
- `env state`：**值字面量**（可写 `n 1700`，也可写 `s "1700"` 造字符串状态）。
- `env dataset`：值字面量对象，例 `env dataset o 2 hp_healing_value n 10 hp_healing_ticks n 2`；
  两个键都可以缺（缺 `hp_healing_value` 时 `max(1, undefined)` 是 NaN，时长整体变 NaN）。
- `env pos`：对象 `o 3 x n … y n … z n …`。
- `env velx` / `env velz`：**值字面量**，`u` = 速度缺失。
- `run make|leave|restrict <x> <y> <z>|update`（`restrict` 的三个参数是**裸数字**）。

输出：`run <op> || <日志> | <状态>`；状态含 `state=` / `pos=[x:y:z]` / `vel=[vx:vz]` /
`granted=<buff id>:<duration>` / `marks=[key=value,…]`。

覆盖面（杀掉全部 37 条变异的关键）：

1. **`leave` 要覆盖四种状态输入**：`n 1700`（命中）、`n 0`（不命中）、`s "1700"`（严格比较不命中）、
   以及缺 `hp_healing_value` 的场景（渲染出 `nNaN`）。
2. **`on_restrict` 要覆盖四条路径**：只有 x 动（`restrict 10 0 0`）、只有 z 动（`0 0 10`）、
   只有 y 动（`0 10 0`，会把 x 与 z **一起**刷）、什么都没动（先 `env pos o 3 x n 5 …` 再 `restrict 5 6 7`
   —— 这条是「恒调用 `set_velocity`」与「`||` 变 `&&`」的唯一观测点）。
3. **两个轴的速度必须不同符号**（`velx n 2` / `velz n -2`）：否则「x/z 写反」「取错速度」都不可观测。
4. **必须有不触发钳制的速度**（`0.25` / `-0.25`）来区分「越界返回上下界」与「原样返回」。
5. **必须有缺失速度**（`velx u` / `velz u`）：`undefined !== null` 为真，
   这条是「`is_null` 把 missing 也算 null」的唯一观测点，也是 `clamp` 原样返回非数的观测点。

### 6.9.79 `character_state_base`（差分 68 行，变异 50/50 全杀）

harness op：

- `env victim s "V1"`（字符串）；`env onground 0|1`（**裸数字**）。
- `env state`：**状态对象**的 state（构造参数，值字面量，可用 `s "2"` 造字符串）。
- `env vstate`：**实体**的 state（值字面量，可用 `s "2"`）；`up` / `landing` 读的是它。
- `env hp` / `facing` / `holding` / `onlanding` / `velx` / `velz`：值字面量
  （`holding`/`onlanding`/`velx`/`velz` 可写 `u`）。
- `env dataset` / `indexes` / `frames`：值字面量对象。典型：
  `env indexes o 5 landing_2 s "40" default s "50" heavy_obj_walk s "60" in_the_skys a 2 s "70" s "71" falling o 2 1 a 2 s "80" s "81" -1 a 2 s "90" s "91"`
- `run make|update|landing|up|auto|sudden|caught`。

输出：`run <op> || <日志> | ret=<返回值|-> | <状态>`；
状态含 `state=`（状态对象）/ `estate=`（实体）/ `hp=` / `face=` / `onground=` / `holding=` / `vel=[x:z]`。

覆盖面（杀掉全部 50 条变异的关键）：

1. **`run up` 之前必须同时设 `env vstate`**（实体状态）与 `env holding`；
   四种状态各跑一次，再加一次 `vstate s "2"` 区分严格/宽松比较。
2. **`holding` 要覆盖 `u`（缺失）、`0`、`1`（非 Heavy 的数值）、`2`（Heavy）、`s "2"`（字符串 Heavy）**：
   `s "2"` 是「严格比较 → 宽松比较」的唯一观测点。
3. **`run sudden` / `run caught` 之前必须把 `falling` 两层键都恢复到索引对象里**
   （本单元为「缺 falling 时返回 undefined」另有一段无 falling 的场景，
   若忘了恢复，六条「取到哪一帧」的变异全部不可观测）。
4. **`indexes` 里 `in_the_skys` 要 `a 2`**，才杀得掉 `index_0` 的 `at(0)` → `at(1)`；
   `frames` 要同时给出 `"40"` / `"50"` / `"60"` / `"70"` 四帧，
   才分得清「取错索引键」与「取错帧表」。
5. **`env onground` 的三种组合都要有**（地面 / 空中 hp>0 / 空中 hp<=0），
   再叠加 `holding` 的 Heavy，才能把 `get_auto_frame` 的三段优先级逐条钉住。
6. **ESM 循环依赖坑**：harness 里必须**第一行**写
   `import "../../../../src/LFW/entity/Entity";`（只为副作用），否则
   `CharacterState_Base.ts` 的**值导入** `import { Entity } from "../entity/Entity"`
   会把 `Entity → ENTITY_STATES → CharacterState_Caught → CharacterState_Base` 的环拉起来，
   打包结果在 `CharacterState_Caught extends undefined` 上直接崩。
   （`State_Base.ts` 用的是 `import type`，所以上一单元没踩到。）

### 6.9.80 `character_state_basic`（差分 82 行，变异 37/37 全杀）

harness op：

- `env cls s "standing"` / `s "running"` / `s "injured"`（选被测类）。
- `env usedefault 1`：不带状态构造（`new CharacterState_Standing()` 等），验证默认参数。
- `env victim s "V1"`（字符串）；`env onground 0|1`（**裸数字**）。
- `env state` / `vstate` / `hp` / `facing` / `vteam` / `ground_y` / `holding` / `onlanding` /
  `dataset` / `indexes` / `frames` / `pos` / `velx` / `velz`：值字面量。
- `run make|update|enter`。

输出：`run <op> || <日志> | <状态>`；
状态含 `state=` / `hp=` / `ground=` / `pos=[x:y:z]` / `vel=[x:z]` /
`holding=` / `holding_team=` / `team=`。

覆盖面（杀掉全部 37 条变异的关键）：

1. **`Standing` 要覆盖四种位置/血量组合**：存活且贴地（`y == ground_y`，什么都不做）、
   存活且高于地面（进 `in_the_skys[0]`）、`hp = 0`（进骤死帧）、
   **`hp = 0` 且高于地面**（这一条专门用来观测 `return` —— 少了它就杀不掉「穿透到地面分支」）。
   再加 `hp = u`（缺失血量按存活算）。
2. **`Running` 的四种 vx/vz 组合缺一不可**：`vz = 0`（不拖拽）、`vx > dz`（减）、
   `vx < -dz`（加）、`|vx| <= dz`（原样）。
   另外必须有 **`vz < 0`** 的一次，才杀得掉「丢掉 `abs`」；`vx = u` 与 `vz = u` 各一次。
3. **`Injured` 的 holding 要覆盖 `u` / `1` / `2` / `s "2"`**：
   `s "2"` 是「严格比较 → 宽松比较」的唯一观测点，`1` 是「换枚举值」的观测点。
4. **`holding.team` 的写入必须有日志**（见 DESIGN §28.4），否则「顺序」类变异存活。
5. `env holding` 要**重置** `holding_team`（对齐 TS 里换了一个全新的 holding 对象）。

### 6.9.81 `character_state_walking`（差分 52 行，变异 34/34 全杀）

harness op：

- `env victim s "V1"`（字符串）。
- `env ctrlud 0|1` / `env ctrllr 0|1` / `env hweapon 0|1`：**裸数字**。
- `env waitflag <数字>`：**裸数字**，`handle_wait_flag` 的返回值。
- `env state|hp|vwait|frame|ground_y|holding|indexes|pos`：值字面量（`vwait` 可 `u`）。
- `run make|update`。

输出：`run <op> || <日志> | <状态>`；
状态含 `hp=` / `wait=` / `waitflag=` / `frame=` / `pos=[x:y:z]` /
`ground=` / `holding=` / `hweapon=` / `ctrl=UD LR`。

覆盖面（杀掉全部 34 条变异的关键）：

1. **`e.wait` 被写回之后必须显式重置**（`env vwait u`）：
   Heavy 分支会把 `wait` 设成 `waitflag`（真值），此后所有「空闲」场景都会被静默跳过。
   本轮首轮 2 条存活正是这个老坑的第四次命中。
2. **四种 holding 组合都要有**：`hweapon 1` + `holding 2`（进 wait 分支）、
   `hweapon 1` + `holding 1`（是武器但非重型）、`hweapon 1` + `holding s "2"`（严格比较）、
   `hweapon 0` + `holding 2`（是重型但不是武器）。
3. **ctrl 的三种输入**：`ud=1`、`lr=1`、两者都 0；`wait` 的 `u` / `0` / `3` 三种。
4. **`hp<=0` 且高于地面**必须单独造一条，否则「穿透到地面分支」不可观测。
5. `indexes.in_the_skys` 要 `a 2`，才杀得掉 `index_0` 的 `at(0)` → `at(1)`。

### 6.9.82 `character_state_caught_rowing`（差分 59 行，变异 39/39 全杀）

harness op：

- `env cls s "caught"` / `s "rowing"`；`env victim s "V1"`（字符串）。
- `env has_holding 0|1`（**裸数字**）。
- `env state|prevstate|fall|fallmax|vteam|velx|vely|holding|onlanding|dataset|indexes`：值字面量
  （`velx`/`vely`/`fall`/`fallmax` 可 `u`）。`prevstate` 是传给 `enter` 的 `{ state: … }`。
- `run make|enter|update|landing`。

输出：`run <op> || <日志> | <状态>`；
状态含 `fall=` / `fallmax=` / `vel=[x:y]` / `has_holding=` / `holding=` / `holding_team=` / `team=`。

覆盖面（杀掉全部 39 条变异的关键）：

1. **`has_holding` 与 `holding` 必须交叉组合**：
   `has_holding 1 + holding 2`（掉落 + 改阵营）、`has_holding 1 + holding 1`（只掉落）、
   `has_holding 1 + holding s "2"`（严格比较）、**`has_holding 0 + holding 2`**（都不做）。
   最后一条是「掉落守卫被删」「改阵营不看守卫」两个变异的唯一观测点；
   **注意进入该场景前要把 `holding` 也设成重型** —— 只设 `has_holding 0` 而沿用上一条的
   `holding 1`，两条变异都会因为 `1 !== 2` 而**静默存活**（本轮唯一一次存活）。
2. **`Rowing` 的 `prevstate` 要有 4 种**：`n 12`（命中）、`n 0`（不命中）、
   `s "12"`（严格比较）、以及配 `S = Falling` 之外的状态值。
3. **`calc_v` 的 `Default` 分支要三种 `vely`**：正值（`5` → 取目标 `-12`）、
   小负值（`-5` → 取目标 `-12`）、**更小的负值（`-20` → 原样保留）**。
   少了最后一种，「把 `prev_vy` 写成常量」与「换模式」两条变异杀不掉。
4. **`velx` 要有正、负、`0`、`u` 四种**：`0` 是 `>= 0` 与 `> 0` 的唯一分界；`u` 观测 `>=` 对非数的结果。
5. `indexes` 要同时给 `landing_1` 与 `default`，才分得清「回落索引取错」。

### 6.9.83 `state_misc`（差分 58 行，变异 42/42 全杀）

harness op：

- `env cls s "weapon_broken"` / `s "to_catching"` / `s "to_louisex"` / `s "to_8xxx"` / `s "ball"`。
- `env victim s "V1"`（字符串）。
- `env state|vstate|vtype|finddata|findfighter|transformtype`：值字面量。
  `finddata` / `findfighter` 是 `datas_find*` 的返回值；`transformtype` 是 `transform()` 后实体新的 `data.type`。
- `run make|landing|update|enter|leave`。

输出：`run <op> || <日志> | <状态>`；
状态含 `state=` / `vstate=` / `type=` / `shaking=` / `motionless=` / `vel=[x:y:z]`。

覆盖面（杀掉全部 42 条变异的关键）：

1. **`to_8xxx` 要覆盖 4 段**：`vtype 4 + transformtype 8`（新变成 Fighter，报告）、
   `vtype 8`（类型没变，不报告）、`transformtype 16`（新类型不是 Fighter，不报告）、
   `finddata u`（没查到数据 ⇒ 不 transform、`new_type` 仍是旧的）、
   `state s "8008"`（字符串状态被 `typeof` 守卫挡掉）、以及 `transformtype s "8"`（严格比较）。
2. **`ball` 要覆盖 4 个球状态 + 一个非球状态 + 一个字符串状态**：
   `3000` 是「四个条件用 `||` 连」的观测点，`s "3001"` 是「严格 → 宽松」的观测点。
3. **`to_louisex` 要覆盖 `findfighter` 有/无两种**。
4. `to_catching` 与 `to_louisex` 的 `enter_frame(find_auto_frame())` 都要能观测到
   （假实体把 `find_auto_frame()` 打成 `{id:"AUTO"}` 并记日志）。

### 6.9.84 `character_state_dash`（差分 63 行，变异 42/42 全杀）

harness op：

- `env victim s "V1"`（字符串）；`env ctrlud -1|0|1` / `env ctrllr -1|0|1`（**裸数字**）。
- `env state|prevstate|pos|ground_y|velx|vely|velz|facing|dataset`：值字面量（`vely` 可 `u`）。
- `run make|enter`。

输出：`run <op> || <日志> | <状态>`；
状态含 `pos=[x:y:z]` / `ground=` / `vel=[x:y:z]` / `face=` / `ctrl=ud,lr`。

覆盖面（杀掉全部 42 条变异的关键）：

1. **早退守卫要有 4 种组合**：贴地 + 有垂直速度（不早退）、悬空 + 有垂直速度（早退）、
   悬空 + 垂直速度 `0`（不早退）、悬空 + `vely u`（不早退）；再加贴地 + `vely 0`。
2. **x 方向五条支路各至少一次**，且**每次都显式设 `velx`**（上一场景会写回）：
   `prevstate 2` + `velx 5`（Running 支）、`prevstate s "2"` + `velx 5`（宽松比较观测点）、
   `prevstate 1` + `velx 5`（「别的状态」观测点）、`ctrllr 1` / `ctrllr -1`（LR 支）、
   `velx 5` / `velx -5` / `velx 0` / `velx u`（四条速度支）。
3. **两组零速度场景**：`facing 1` 与 `facing -1`（见 DESIGN §32.2 的第二条警告）。
4. **`ctrllr -1` 与 `ctrlud -1`**：符号必须能透传到速度上（`LR * dx`、`UD * dz`）。
5. **`ctrlud` 三种取值**（`0` 保留原 z、`1` 正、`-1` 负）。

### 6.9.85 `character_state_burning`（差分 51 行，变异 37/37 全杀）

harness op：

- `env victim s "V1"`（字符串）；`env catcher 0|1`（**裸数字**）。
- `env indexes|wdata|facing|bounced|velx|vely|velz|landingvel|onlanding`：值字面量
  （`velx` 可 `u`）。`wdata` 是 `world.dataset` 的内容；`landingvel` 是 `{x, y}`。
- `run make|enter|update|leave|landing`。

输出：`run <op> || <日志> | <状态>`；
状态含 `bounced=` / `facing=` / `vel=[x:y:z]` / `catcher=`。

覆盖面（杀掉全部 37 条变异的关键）：

1. **每个 `landing` 场景都要显式设 `env bounced n 0`**（见 DESIGN §33.3 的第二条警告）。
2. **`||` 的两侧要各自单独成立一次**：
   `y=5, x=0`（左侧成立、右侧短路不读）、`y=6, x=0`（两侧都不成立 ⇒ else）、
   `y=6, x=4`（右侧成立）、`y=6, x=-4`（右侧靠 `abs` 成立）。
   少了 `y=6, x=0` 就杀不掉「左右阈值互换」；少了 `x=-4` 就杀不掉「去掉 `abs`」。
3. **`bounced` 要有 `0` / `1` / `u` 三种**：`1` 是「已弹跳 ⇒ 直接躺下」的观测点，
   `u` 是真值判定（缺失即假）。
4. **`facing` 的三种速度输入**：`0`（不转头）、正、负、以及 `u`（缺失 ⇒ 不转头）。
5. **`enter` 要跑两次**（有/无 catcher），并观察它写出的
   `set_bounced:false` 与 `handle_ground_velocity_decay` 日志。
6. **`leave` 之前先把 `bounced` 置真**，才能观测到它被清回 `false`。

### 6.9.86 `character_state_teleport`（差分 106 行，变异 61/61 全杀）

harness op：

- `env cls s "nearest"|"farthest"`；`env state`（值字面量）；`env victim s "M1"`。
- `env facing|seg`（值字面量）；`env gy <裸数字>`。
- `env pos o 3 x n .. y n .. z n ..`；
  `env ent o 6 id s ".." fighter b .. ally b .. hp n .. x n .. z n ..`
  （按 id 覆盖或追加，保留插入序）。
- `run make|default|enter`。

输出：`run <op> || <日志> | <状态>`；
状态含 `id=` / `pos=[x:y:z]` / `face=` / `seg=` / `gy=`。

覆盖面（杀掉全部 61 条变异的关键）：

1. **`env cls` 之后必须重新 `run make`**（见 DESIGN §34.3 的第二条警告）。
2. **自身实体要真的进列表**（TS 侧放 `victim` 本身），并把自身位置调到「最近」
   （`env pos o 3 x n 1 ..`），否则杀不掉「丢掉自身过滤」。
3. **非战斗实体要排在最近处**：`N1(fighter=0, x=1)` + `N2(fighter=1, x=20)`，
   否则 `!is_fighter || is_self` 的 `||` → `&&` 不可观测。
4. **死实体要排在最近处**：`D1(hp=0, x=1)` + `D2(hp=10, x=30)`。
5. **两个 z 不同的候选**：`Z1(x=6, z=100)` + `Z2(x=7, z=0)`。
   原文 z 项恒为 0 时选 `Z1`（`x = -114`）；z 项一旦变成真的就选 `Z2`（`-113`）。
6. **等距平局**：`T1(5)` / `T2(-5)`（近敌保留先到的）；
   `V1(5)` / `V2(-5)`（远盟同理）。少了它们杀不掉 `dis < best` → `<=`。
7. **远盟要有非等距场景**（`A1(5)` / `A2(40)` ⇒ `x = -80`）：
   只有等距场景时「谓词忽略搜索方向」两边同解，杀不掉。
8. **`facing` 要 `1` / `-1` 各一次**，且要有 `x != z` 的场景（`-89 / 4`），
   否则看不见 `ground_segment(x, z)` / `ground_y(seg, x, z)` 的实参顺序。
9. **最后一条候选要能胜出**（`W1(40)` / `W2(2)`），否则杀不掉「跳过最后一个」。
10. **同一个受害者的 x 要中途改变**（`MC`：`x=0` 时选 `C1(6)`，改成 `x=-6` 后选 `C2(-7)`），
    否则「不读自身 x」不可观测。
11. **`run default`** 覆盖两个构造默认实参（`state=400` / `state=401`）。

### 6.9.87 `weapon_state_base`（差分 163 行，变异 73/73 全杀）

harness op：

- `env state|indexes|ionground|ithrow|isky|frames|onlanding|base|wt|dh|vstate|fid|vel|nf|align`
  （值字面量）；`env hp|hpr <裸数字>`；`env onground <真值>`。
- `run make|auto|landing|update|leaveground|rebound`。

输出：`run <op> || <日志> | <状态>`；状态含 `hp=` / `hpr=` / `dh=`，
`auto` 额外带 `fid=`。

覆盖面（杀掉全部 73 条变异的关键）：

1. **`auto` 要覆盖「贴地 / 空中 / `indexes` 为假值 / 键不在表里 / 空数组」五种**：
   空数组那条会让 `to_string(缺失)` 变成键 `"undefined"`，正好也是 `indexes` 守卫的观测点。
2. **`rebound` 的反弹系数要分别用「表值 / `base` 覆盖 / 表外的武器类型」跑一遍**：
   表外类型（`wt n 9`）会同时触发三处 `??` 兜底；
   `wt n 5`（Drink，表尾）与 `wt n 2.5`（小数）分别锁住范围检查的两半。
3. **`is_bounce` 的五个项各自都要有「只靠它成立」的用例**，尤其第 4 项
   （`dvx >= min_z`）要**正反各一条**：`x=3` 时原文成立、`z=3` 时原文不成立，
   互换 `dvx`/`dvz` 的变异才杀得掉。
4. **`Heavy` 不可用于 align 分支**（见 DESIGN §35.2 第 6 条），
   要用 `wt n 0`（None）与 `wt n 4`（Baseball）来覆盖 align。
5. **`min_x` / `fast_*` 的边界要卡在等号上**：
   `base` 里把阈值设成 `2`，再用 `x=±4`（`bounce_x=0.5`）让 `dvx` 恰好等于 `±2`。
6. **速度要出现小数**（`x/y/z = -1.2345 / -1.2345 / 1.2345`），
   否则三条 `round_float` 变异不可观测。
7. `dh` / `hp` / `hpr` 每段重置；`vstate` 要出现**字符串** `s "1002"` 一次，
   用来看 `==` 与 `===` 的区别。

### 6.9.88 `state_base_proxy`（差分 113 行，变异 55/55 全杀）

harness op：

- `env cls s "proxy"|"15"|"frozen"`、`env state`（构造参数）、
  `env data|indexes|frames|onlanding|wdata|hbtype|vx|vz|vel|rid|pos|rxyz`（值字面量）、
  `env vstate`（实体状态）、`env hp <裸数字>`、`env catcher|onground <真值>`。
- `run make|default|update|leave|restrict|preupdate|enter|dead|landing|leaveground|gravity|auto|sdf|cef|ffbi`。

覆盖面（杀掉全部 55 条变异的关键）：

1. **类型矩阵要走全四种**：`data.type = 8`（Fighter）/ `16`（Weapon）/ `32`（Ball）/
   `4`（Ohters，落到 `_proxy`），外加一次 `env data u`。
   ⚠️ `EntityEnum::Ball = HitFlag::Ball = 0x20 = 32`，**不是 4**。
2. **每种类型都要有一组「只有它能产生」的日志**：
   Fighter → `auto` 返回 `stats["4"]`、`landing` 用 `landing_2="22"`、`sdf`/`cef` 有返回、
   `leaveground` 在读 `falling`；
   Weapon → `auto` 用 `on_ground="9"`、`landing` 用 `on_ground`、`leaveground` 无条件进 auto 帧；
   Ball → `enter` 里 `set_shaking`/`set_motionless`/`set_velocity`；
   `_proxy` → 全空。
3. **`HealSelf` 场景必须先改 `env state` 再 `run make`**（见 DESIGN §36.2 第 7 条），
   否则 `leave` 走不进 buff 分支，「调用两次」不可观测。
4. **`State_Frozen.enter` 的 `super_enter` 要能看见**：只有 Ball 目标装了 `enter`，
   所以要用 `data.type = 32` + `vstate = 3001` 再跑一次 `enter`。
5. **`hbtype` 要给一次字符串**（`s "2"`），才能区分 `==` 与 `===`。
6. **落地的速度阈值要卡三档**：`y = 20`（不触发）、`y = 10`（恰好等于 `5*2`，杀 `<`）、
   `y = -7`（在 `5` 与 `10` 之间，杀「不乘 2」）、`y = -10`。
7. `env state n u` / `run default` 两个场景用来锁住「构造默认实参」。
