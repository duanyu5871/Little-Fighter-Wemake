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
