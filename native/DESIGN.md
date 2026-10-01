# DESIGN —— 动态值（`Value`）的设计

决定：**A2 + B2 + C2**（2026-10-01）

- **A2** —— `Value` = `std::variant<monostate, bool, double, std::u16string, shared_ptr<Array>, shared_ptr<Object>>`
- **B2** —— 只把 `Value` 用在 **I/O 边界**；加载后立即转成强类型 struct
- **C2** —— `Object` 维持 JS 的键顺序（整数样式的键升序在前，其余按插入序），插入时即维持

---

## 1. 边界：哪里真的需要 `Value`

### 1.1 仿真热路径**不需要**

`Entity.dataset()` 的键在编译期被 `keyof IWorldDataset` 检查过：

```ts
dataset<K extends keyof IWorldDataset>(name: K): IWorldDataset[K] {
  return (
    this.frame.dataset?.[name] ??
    this.data.base[name] ??
    this.world.bg.data.dataset?.[name] ??
    this.world.dataset[name]
  )
}
```

⇒ C++ 侧 **intern 成枚举**，四层 fallback 全是枚举键查表，运行时零字符串查找。
这个函数在 `update_velocity` / `handle_gravity` / `CharacterState_Jump` 里**每帧每实体**调用。

全库只有 **2 处**真正动态的键访问，都是 cheat：

| 位置 | 代码 | 解法 |
|---|---|---|
| `cmds/cheat_code_handler.ts:8` | `ctx.world.dataset[cmd]`，`cmd = ctx.str(0)` | 小的 cheat 字符串→字段表 |
| `LFW.ts:418` | `is_cheat(name: string \| CheatEnum)` | 同上（`is_cheat_type` 已限定取值） |

### 1.2 真正需要 `Value` 的只有三块

1. `base/Expression` 的操作数（`val_1` / `val_2`，宽松 `==`）—— **每帧热**
2. `dat_translator` / `schema` / `fields` / XML —— **一次性装载路径**
3. `loader/DatMgr` 的 JSON 往返校验

---

## 2. `Value` 的类型与操作

类型集合（核心里没有 Function / Symbol / BigInt）：

```
Undefined | Null | Bool | Number | String | Array | Object
```

| 操作 | 证据 | 难度 | 备注 |
|---|---|---|---|
| 宽松 `==` / `!=` | `predicate_maps["=="]` | 高 | `Abstract Equality Comparison` 完整规则表 |
| 关系 `< > <= >=` | 同上 | 中 | ToPrimitive 后：两边都是字符串→字符串比较，否则 ToNumber |
| `{{ }} !{ !}` | `a_included_b` | 低 | 用 `indexOf` ⇒ **严格相等**，与 `==` 不同 |
| 真值判断 | `!a`、`if (v)`、`!b.length` | 低 | |
| `ToNumber(string)` | 上面几条都会走到 | 中 | `StringToNumber` |
| `ToString(number)` | `==` 遇「数字 vs 字符串」 | 中高 | 最短往返 + JS 指数阈值 |
| `Object.keys` 顺序 | `DatMgr` JSON 往返、`traversal`、`get_keys` | 中 | 整数样式的键升序在前 |
| `Array.isArray` / `typeof` | `is_*` 谓词 | 低 | |

### 2.1 一个被否掉的优化

「装载期把数字字面量折叠成 number」**不成立**：

```
"003000" == "3000"   → false      两边都是字符串 → 字符串比较
"003000" == 3000     → true       ToNumber("003000") = 3000
```

所以宽松 `==` 必须真做。

---

## 3. `Value` 的表示（A2）

```cpp
class Array;
class Object;

using Value = std::variant<
    std::monostate,                 // Undefined
    NullTag,                        // Null（与 Undefined 区分：?? 与 null 的语义点）
    bool,
    double,
    std::u16string,                 // JS 字符串 = UTF-16 code unit 序列
    std::shared_ptr<Array>,
    std::shared_ptr<Object>>;
```

**已落地的部分（V1）只含到 `std::shared_ptr<Array>`** —— `Object` 连同它的键顺序一起推迟到 V6。
理由：实测 `Expression` 的价值域里**根本没有 object**（见 §4.1），所以先加 `Object` 只会得到
一段在 V6 之前没人测的代码。加 `Object` 时要同步改的是：`value.h` 的 variant、`truthy`、`type_of` 三处。

> **陷阱**：`Value` 里同时有 `bool` 和 `double` 两个替代，所以 `Value(0)` / `Value(1)` 是
> **有歧义的**（int 能隐式转到两者）。构造时一律写显式类型：`Value(false)` / `Value(0.0)`。

- 标量**零堆分配**，约 40 字节（MSVC 上 `std::u16string` 是 32 字节）
- Array / Object 用 `shared_ptr` ⇒ 引用语义，贴近 JS
- **已知问题：环引用泄漏**（`shared_ptr` 不回收环）。LFW 的数据是 JSON5 树，理论无环，
  但 `loader` 里可能有自引用 —— **这条要实测确认，不能假设**

### 3.1 `Object` 的键顺序（C2）

「整数样式」的精确定义：`k` 满足
`ToString(ToUint32(k)) === k && k !== "4294967295"`，即规范数字字符串且在 `[0, 2^32-2]` 内。

- 整数样式的键：按**数值升序**，插在所有字符串键之前
- 其余键：按**插入序**，追加在末尾

插入时即维持顺序（不是 `keys()` 时才重排），这样任意时刻 `keys()` 都对。

---

## 4. 落地顺序

| 阶段 | 内容 | 状态 |
|---|---|---|
| V1 | `Value` 基本类型 + 真值 / `typeof` / `Array.isArray` | **已完成**（89 行差分全过；`Object` 延到 V6） |
| V2 | **`string_to_number`**（`ToNumber(string)`） | **已完成**（146 + 81 行差分全过） |
| V3 | **`number_to_string`**（最短往返 + JS 指数阈值） | **已完成**（77 + 10558 行差分全过） |
| V4 | **宽松 `==`** 完整规则表 | **已完成**（89 + 61 + 84 行差分全过；`{{`/`}}` 延到 V7） |
| V5 | 关系比较 `< > <= >=` | **已完成**（107 行差分全过） |
| V6 | `Object` + `Object.keys` 顺序 | **已完成**（109 行差分全过） |
| V7 | `Expression` 跑在 `Value` 上 | **已完成**（303 + 216 行差分全过） |
| V8 | `JSON.stringify` / `JSON.parse` | **已完成**（62 + 157 行差分全过） |
| V9 | `JSON5.parse` / `JSON5.stringify` | **已完成**（详见 `JSON5.md`） |
| V10 | `fields.ts`（字段描述 DSL + `fields()` + `reorder_fields` + `validate_fields`） | **已完成**（77 + 155 行差分全过；19 条变异全杀） |
### 4.1 V1 `Value` 的落地结果

**先量后做**：把 5 张 getter 表（`get_val_from_entity` / `get_val_from_collision` /
`get_val_getter_from_stage` / `get_val_from_bot_ctrl` / `get_val_from_world`+`lfw`）读到底，
实际返回的类型只有：

| 实际类型 | 例子 |
|---|---|
| `number` | 绝大多数（`e.hp` / `e.velocity.x` / `c.itr.kind` / `round(...)`） |
| `boolean` | `HoldingHeavy: e => e.holding?.base_type == WeaponEnum.Heavy`；`ctrl.is_hit/is_start/is_db_hit` |
| `string` \| `undefined` | `HoldingOID: e => e.holding?.data.id`（`IEntityData.id: string`） |
| `string` | `AEmitter: c => c.attacker.emitter ?? ''`；`BotState: e => e.fsm.state?.key ?? ''` |
| `number[]` | `HitByState / HitByItrKind / HitByItrEffect / HitOnState`（`.map(...)`） |
| `string[]` | `S_Val.Broadcast: e => e.lfw.broadcasts`（`LFW.broadcasts: string[]`） |
| `string` | **getter 兜底**：`get_val_from_bot_ctrl` 未命中时是 `() => word`，返回字面字符串 |

`Expression` 自己还会造出两种值：字面量右侧直接是**字符串**（`let val_1 = word_1`），
以及 `{{`/`}}`/`!{`/`!}` 的右侧 `word_1.split(",")` → **`string[]`**。

⇒ **`Expression` 的价值域不含 object**，所以 V1 只需要
`Undefined | Null | Bool | Number | String | Array`。

落地：`lfw/core/value.{h,cpp}`（`Value` + `Array` + `truthy` / `type_of` / `is_array` / `as_array`）。
真值：`Undefined`/`Null` 假；`bool` 原样；`number` 在 `±0` 与 `NaN` 时假；`string` 空串假；**数组恒真**。
`type_of`：`Null` 与 `Array` 都返回 `"object"`（`typeof null === "object"`、`typeof [] === "object"`）。

差分 subject `value` 用一个**前缀记法的值字面量 + 句柄表**来构造并观测值：

```
u / z / b 0|1 / n <token> / s "..." / a <k> <v1>..<vk>      构造
 typeof <expr> | truthy <expr> | is_array <expr> | length <expr>   观测（只打印一行）
hold <expr>  → hold <idx>       把值放进句柄表
dup <idx>    → dup <idx>        拷贝这个值（对数组就是 shared_ptr 拷贝）
elem <idx> <i> → elem <newidx>  取数组元素并放进句柄表
same <i> <j> → same true|false|-   两个句柄是否同一个数组（非数组打印 "-"）
```

`hold` / `dup` / `elem` / `same` 这组的存在理由是**验证 A2 的引用语义决策**：
`dup` 出的句柄必须与原件指向同一个 `Array`（`same true`），而两个独立字面量必须不同（`same false`）。

| 变异 | 结果 |
|---|---|
| `truthy` 去掉 `isnan` 判定 | FAIL 第 24 行（`truthy n nan`） |
| `type_of` 对数组返回 `"array"` | FAIL 第 14 行（`typeof a 0`） |
| `dup` 改做深拷贝（模拟 "Value 拷贝 = 深拷贝"） | FAIL 第 59 行（`same 0 1`）—— **引用语义测试不是空转** |
### 4.2 V2 `string_to_number` 的实现要点

ECMAScript 的 `StringNumericLiteral` 语法：

```
StringNumericLiteral ::  StrWhiteSpace?
                         StrWhiteSpace? StrNumericLiteral StrWhiteSpace?
StrNumericLiteral    ::  StrDecimalLiteral | NonDecimalIntegerLiteral
StrDecimalLiteral    ::  StrUnsignedDecimalLiteral | +|- StrUnsignedDecimalLiteral
StrUnsignedDecimalLiteral ::  Infinity
                              DecimalDigits . DecimalDigits? ExponentPart?
                              . DecimalDigits ExponentPart?
                              DecimalDigits ExponentPart?
NonDecimalIntegerLiteral  ::  0b|0B | 0o|0O | 0x|0X  + 至少一位数字（**不允许符号**）
```

三个必须守住的行为：

1. **`StrWhiteSpace` 不是 ASCII 空白** —— `WhiteSpace` + `LineTerminator`，含
   `U+00A0` `U+1680` `U+2000`–`U+200A` `U+2028` `U+2029` `U+202F` `U+205F` `U+3000` `U+FEFF`。
   纯空白 → **`+0`**。
2. **非十进制不允许符号** ⇒ `Number("+0x10")` 是 `NaN`，`Number("-0b11")` 是 `NaN`。
   而 `strtod` 接受它们 ⇒ **必须先校验语法再调 `strtod`**。
3. **非十进制要正确舍入**。实测：`Number("0x" + h)` 在 90,000 个随机样本上与
   `Number(BigInt("0x" + h))` **完全一致** ⇒ V8 是正确舍入的
   （二进制 70,000 样本同样一致）。所以在 `double` 里逐步累加 `v*16+d` **会错**，
   必须做「保留高位 + sticky」式的正确舍入。

十进制则**先校验语法**，再交给 `std::from_chars(..., chars_format::general)`（它是正确舍入的，与 V8 一致；**不要用 `strtod`**，它是 locale 相关的 —— 小数点会随 locale 变）。差分测试本身就是这条的参照 —— 不需要外部参考实现。

**落地结果**：`lfw/core/js_string.{h,cpp}` + 差分 subject `core`（`cases/core/{to_number,js_num}.txt`）+ `trace_util` 的 `num_hex` / `parse_js_string_literal`。

三个变异测试全部被抓到，但**第一个变异暴露了用例的缺陷**：

| 变异 | 结果 |
|---|---|
| `parse_radix` 换成 `double` 里逐步累加 `acc = acc*base + d` | **第一次 PASS（没抓到！）** → 补搜索出的判别用例后 FAIL 第 107 行 |
| `is_str_white_space` 去掉 `0x3000` | FAIL 第 33 行 |
| 上溢/下溢分支恒返回 Infinity | FAIL 第 131 行 |

**教训（第二次踩同一个坑）**：旧用例是 `0x` + 一串 `f`，这种特例天真实现恰好也得到正确结果。不要手造「看起来很长」的用例，要**写脚本搜**（`native/build/gen/search_radix.mjs`）：随机长数字串，比 `Number(BigInt(prefix+s))` 与天真累积的位模式。搜索结果：

- **十六进制只有 16 位就能判别**（`762b4b1154c262e6`、`988e95d9b38cc4cd` …），不是长度问题而是**具体数值**问题，判别表现永远是差 1 ULP。
- 二进制需 ≥ 70 位，八进制需 ≥ 26 位。
- **十进制整数串 400 万次采样 0 个判别用例** ⇒ `a*10+d` 的 double 累积极少出错。

### 4.2.1 一条必须记住的隐含前提

`from_chars` 的 `result_out_of_range` 只用于**超出 double 范围**的判定，而次正规数（`4.9e-324`）**不算越界**。
若某个实现把次正规也报成 `result_out_of_range`，按十进制指数 `E < 0` 就会返回 `+0`，而 V8 返回 `5e-324`。
用例里的 `"4.9e-324"` / `"5e-324"` 正是钉住这一点的，**实测 MSVC 的 `from_chars` 行为正确**。

### 4.3 V3 `number_to_string`

**不实现这条，`DatMgr` 的 `JSON.stringify` 往返校验（`DatMgr.ts:135/157`）就没法验。**

不要自己写最短往返算法 —— `std::to_chars(..., std::chars_format::scientific)` 就是最短往返的，
（**无精度的 `scientific` 重载**；给了精度就不是最短了）。拿到 `d0.d1…dk-1 e±E` 后按规范重排：

```
k = 数字个数,  n = E + 1

5.b  k ≤ n ≤ 21         →  s 的 k 位 + (n-k) 个 '0'          "123"
5.c  0 < n ≤ 21          →  s 前 n 位 + '.' + 剩下 k-n 位     "1.23"
5.d  -6 < n ≤ 0          →  "0." + (-n) 个 '0' + s            "0.000123"
5.e  其他                 →  d0 [ '.' + d1..dk-1 ] 'e' sign |n-1|   "1e+21" / "1.5e-7"
```

特值：`NaN` / `Infinity` / `-Infinity` / **`±0` 都输出 `"0"`**（`-0` 不是 `"-0"`）。

**5.b 与 5.c 的界必须都是 21**（不能只改一个）：能进 5.c 就意味着 5.b 失败，
而 5.c 自己要求 `n ≤ 21`，所以必然 `k > n` —— 这正是规范把两个分支写成同一个上界的原因。
若两者不一致，5.c 会按 `n` 个数字去读一个只有 `k < n` 位的串（越界）。

**落地结果与变异测试**（`native/lfw/core/js_string.{h,cpp}`，差分 subject `core` 的 `to_string` / `to_string_bits`）：

| 变异 | 结果 |
|---|---|
| 5.b/5.c 的 `21` → `20` | FAIL 第 27 行（`1e20` 变成 `1e+20`） |
| 5.d 的 `-6` → `-5` | FAIL 第 38 行（`1e-6` 变成 `1e-6` 而非 `0.000001`）+ fuzz 第 195 行 |
| 指数位 `n - 1` → `n` | FAIL 第 28 行（`1e+22`）+ fuzz 第 1 行 |
| 5.e 的 `k > 1` → `k >= 1` | FAIL 第 28 行（`1.e+21`）+ fuzz 第 801 行 |

**最短往返的取舍是否与 V8 一致 —— 只能靠随机搜**：V8 在「两个等长候选」中挑靠近 x 的那个、
并列时挑偶尾数，而 C++ 标准的措辞不保证同样规则。实测 **569,110 个随机 double**
（均匀随机 64 位模式 + 全部 2 的幂 / 10 的幂 + ±1/±2 ULP 扰动）**全部一致**
⇒ MSVC 的 `to_chars` 取的位数与 V8 一致。

定种子的固件 `cases/core/number_to_string_fuzz.txt`（10,558 行）由
`native/build/gen/gen_number_to_string_fuzz.mjs <N> <out>` 生成（脚本在 gitignore 的 `build/gen/`）；
需要更大规模时直接跑大 N 的临时文件即可，不必入库。

### 4.3.1 不在 V3 范围内：`toFixed`

`get_short_file_size_txt.ts` 和 `dat_translator/fixed_float.ts` 用 `Number(n.toFixed(d))`，
那是**另一套舍入规则**（`toFixed` 有自成的舍入与补零逻辑）。
它们只被 `cook_frames` / `make_itr_prefabs` 调用 ⇒ **构建期 cook**，不在运行期，运行期移植不需要。

### 4.4 V4 宽松 `==` 与它的强制转换核心

先做**强制转换原语** —— `==` 只是它们的一个消费者，V5 的关系比较也要用同一套：

```cpp
Value          to_primitive(const Value&);   // 只有 Array 需要动：→ join(",")
double         to_number(const Value&);      // undefined→NaN, null→0, bool→0/1, string→string_to_number
std::u16string to_string(const Value&);      // number→number_to_string, Array→join(","), 其余按字面
std::u16string array_join(const Array&);
bool strict_equals(const Value&, const Value&);
bool equals(const Value&, const Value&);
```

**`Array.prototype.toString` 就是 `join(",")`**，而 `join` 把 `null` / `undefined` 元素变成**空串**
（不是 `"null"` / `"undefined"`）：

```
[].toString()            = ""
[undefined].toString()   = ""
[null].toString()        = ""
[undefined,1].toString() = ",1"
[null,null].toString()   = ","
[[1,2],3].toString()     = "1,2,3"     内层先 toString 再拼 ⇒ 会被"压平"
```

**`==` 的规则表**（值域已限定为 Undefined|Null|Bool|Number|String|Array）：

| 情况 | 结果 |
|---|---|
| 同类型 | `strict_equals`（`NaN != NaN`、`+0 == -0`、**Array 按引用**） |
| `null` ↔ `undefined` | `true` |
| 任一边是 Bool | 那一边 `ToNumber(bool)`，然后递归 |
| Number ↔ String | `ToNumber(string)`，然后按数字比 |
| Array ↔ Number/String | `ToPrimitive(array)`，然后递归 |
| 其余 | `false`（含 Array ↔ `undefined`/`null`；Array ↔ Array 不同引用） |

**两条最容易被漏掉的是“不对称”的那两条**：Bool / Array 出现在**右边**时同样要转换。
`eq n 1 b 1` / `eq n 0 a 0` 就是专门钉这两个方向的。

差分 subject `value` 新增 `eq` / `seq` / `ton` / `tos` 四个 op（`tos` 用 `\uXXXX` 转义输出，全是 ASCII）。

| 变异 | 结果 |
|---|---|
| 删掉 `kb == V_BOOL` 分支（规则 9） | FAIL 第 40 行（`eq n 1 b 1`） |
| 删掉 `kb == V_ARR` 分支（规则 11） | FAIL 第 61 行（`eq n 0 a 0`） |
| `array_join` 去掉 `null`/`undefined` → 空串 | FAIL `coerce` 第 22 行 + `equality` 第 65 行 |
| 数字比较加 `signbit` 区分 `±0` | FAIL 第 2 行（`eq n 0 n -0`） |

**不在 V4 范围**：`{{` / `}}` / `!{` / `!}` 的 `a_included_b` 留到 V7。它用 `indexOf` ⇒
**严格相等**（不是 `==`，也不等于 `includes` 的 SameValueZero，所以 `NaN` 永不匹配）；
另外它在非数组操作数上会**抛 TypeError**（`!b.length` 过了之后调 `b.findIndex`，或 `a.indexOf`），
而数值操作数因为 `!b.length` 为真会**直接返回 `true`** —— 这条要在 V7 逐字照抄。

### 4.5 V5 关系比较

**这是整个 `Value` 里最容易写错的一步**：规范的 `IsLessThan` 返回的是 **`true` / `false` / `undefined` 三态**
（两边沾了 `NaN` 就是 `undefined`），而四个运算符是这样映射的：

| JS | 定义 |
|---|---|
| `a <  b` | `IsLessThan(a, b) === true` |
| `a >  b` | `IsLessThan(b, a) === true` |
| `a <= b` | `IsLessThan(b, a) !== true` —— **`undefined` 也算"不是 true"，所以结果是 `false`** |
| `a >= b` | `IsLessThan(a, b) !== true` |

⇒ `NaN <= 1`、`NaN >= 1`、`NaN < 1`、`NaN > 1` **全部是 `false`**。
若把 `le` 写成 `!gt(a, b)`（因为 `gt` 对 NaN 返回 `false`），`NaN <= 1` 就会错成 `true`。
所以 C++ 侧先用 `std::optional<bool> less_than(...)` 把三态显式带出来，四个运算符都从它派生。

比较体（两边 `ToPrimitive` 之后）：

```cpp
if (两边都是 string) return sx < sy;        // UTF-16 code unit 字典序
// 否则
if (isnan(nx) || isnan(ny)) return std::nullopt;
return nx < ny;
```

- **字符串比较用 `std::u16string::operator<` 就对了** —— `char16_t` 是无符号 16 位，
  逐 code unit 比大小正是规范的语义。规范里那三段 `IsStringPrefix` 只是这个的写法优化，不用单独特判。
  （所以 `"\u00e9" > "z"` 而 `"\ud83d\ude00" > "z"` —— 比较的是 **code unit**，不是码点，也不是字节。）
- **最大的坑是两侧类型不同**：`"2" < "10"` 是 `false`（字符串序），但 `2 < "10"` 是 `true`（先 `ToNumber`）。
  用例 `lt s "2" s "10"` / `lt n 2 s "10"` / `lt a 1 n 2 s "10"` 三个成组钉这一点：
  前两个是字符串则按字符串比（**数组 `[2]` 先 `ToPrimitive` 成 `"2"`，所以也是字符串序**）。

| 变异 | 结果 |
|---|---|
| `le` 写成 `!gt(a, b)`（丢掉三态） | FAIL 第 25 行（`le n nan n 1`） |
| `less_than` 去掉字符串分支（永远转数字） | FAIL 第 34 行（`lt s "" s "a"`） |
| `ge` 的参数顺序写成 `less_than(b, a)`（从 `le` 复制粘贴） | FAIL 第 8 行（`ge n 2 n 1`） |

### 4.6 V7 `Expression`

文件：`lfw/defines/bin_op.h`（枚举 + 文本双向转换，header-only）、`lfw/base/predicate.{h,cpp}`
（`apply_predicate` + `a_included_b`）、`lfw/base/expression.h`（模板，header-only）。

C++ 形状（与 TS 一一对应，但零分配）：

```cpp
template <typename Ctx>
using ValGetter = Value (*)(const Ctx& ctx, const std::u16string& word, BinOp op);
template <typename Ctx>
using ValGetterGetter = ValGetter<Ctx> (*)(const std::u16string& word);
template <typename Ctx>
class Expression { std::vector<Expression> children; ... };
```

- TS 的 `run` 是**可被赋值的箭头属性**（常量折叠时会被换成 `() => result`），C++ 里用 `Mode { kTree, kLeaf, kConstant }` 三态模拟。
- getter 用**函数指针**而非 `std::function` —— 每个节点都要存两个，`std::function` 会变成每节点两次堆分配。
- 实测所有构造点都只写**一个**类型参数（`Expression<T>`，`T2` 从未显式出现），所以模板只参数化 `Ctx`。

#### 两条必须逐字照拄、不能“整理”的

1. **`!(` 分支的 `p = i + 2`**（而 `(` 分支是 `p = i + 1`）。`i += child.text.length + 2` 之后 `i` 正好指向那个 `)`，
   所以正确的下一个未消费位置应该是 `i + 1`。写成 `i + 2` 会**多跳一格** —— 只要 `)` 后面紧跟的字符不是分隔符
   （如 `!(A==B)C==D`），那个字符就会被**整段丢掉**。这是上游的真 bug，用例 `!(A==B)C==D` 专门钉它；
   把这个 `+2` “修”成 `+1` 会被差分测试抓到（树里多出一个 `"==D"` 子节点）。
2. **`parse` 里那两个正则的匹配位置是“最右”的**。`(\S*)(==|!=|...)(\S*)` 中 `\S*` 贪婪回溯，等价于
   **从右往左找第一个双字符运算符**；找不到才退到 `(=|<|>)` 的**单字符最右**匹配。用例 `A==B!=C` / `b a=>b` / `A=B=C` 钉的就是这个。
   因为构造函数已经把所有空白剥掉了，两个正则里的 `\s*` / `\s?` 恒匹配空串 ⇒ 可以不引入 `<regex>` 直接手写。
   （顺带：`bin_op_from_one` **不能**把 `=` 当成 `==` —— TS 里 `predicate_maps["="]` 是 `undefined`，
   所以 `A=B` 走的是 `always_false("wrong operator: =")`。所以 op 存的是**正则捕获的原文**，不是枚举。）

#### `a_included_b` 的语义

```
!b.length || b.findIndex(i => a.indexOf(i) < 0) === -1
```

- `b` 是 Array：长度 0 → `true`；否则**每个**元素都能在 `a` 里找到才 `true`。
- `a` 是 Array 时为**严格相等**查找（`indexOf` 不走 `==`）；`a` 是 String 时为子串查找。
- 本体数据里唯一形态是 `"broadcast{{gim_walk"`（左边 `lfw.broadcasts: string[]`，右边 `split(",")` 也是 `string[]`）⇒ 字符串对字符串，正常工作。
  注意 **`number[]` 对 `string[]` 是不匹配的**（严格相等），用例 `st{{1,2` 就钉了这个实际行为。
- **已知偏差**：非数组的 `b` 且非空（`b.findIndex` 不存在）、或非数组非字符串的 `a`（`a.indexOf` 不存在）时，
  JS 会**抛 TypeError**，C++ 无异常可用 ⇒ 返回 `false` / `-1`。这两条在本作数据里**不可达**。
- **已知偏差**：`collect()` / `stringify_expr_debug()` / `debug()`（调试渲染器）**未移植** —— 它需要 `JSON.stringify`，
  属于 V6 的 JSON 那一块。

差分测试的 TS 侧**直接用仓库里真正的 `Expression`**（不是重写一遍），只桩掉 `Ditto.warn`
（`Ditto` 是 `Partial<IDitto>` 可变对象，`warn` 默认不存在）—— 所以这是真对拍，不是自说自话。

| 变异 | 结果 |
|---|---|
| `!(` 的 `p = i + 2` “修”成 `p = i + 1` | FAIL `parse` 第 223 行（`!(A==B)C==D`） |
| OR 短路返回时丢掉 `not_`（`result = true`） | FAIL `eval` 第 142 行（`!((hp==100)|(mp==200))`） |
| 去掉 `&&`/`\|\|` 双字符检测里的 `++i` | FAIL `eval` 第 118 行 + `parse` 侧崩 |
| `find_op` 改成从左往右找 | FAIL `parse` 第 44 行（`A==B!=C`） |

### 4.7 V6 `Object` 与键顺序（C2）

**表示直接按规范的 `OrdinaryOwnPropertyKeys` 结构来**，不用一个容器再排序：

```cpp
class Object {
  std::map<uint32_t, Value> _ints;                              // 整数样式的键：自动升序
  std::vector<std::pair<std::u16string, Value>> _strs;          // 其余：插入序
};
// keys() = _ints 的键（转回十进制串） + _strs 的键
```

这样“插入时即维持顺序”是天然成立的，不需要在 `keys()` 里重排。普通对象只在 I/O 边界用（§1.1），
所以字符串键的线性查找是可接受的。

**“整数样式”的判定**（`ToUint32` 往返 + 显式排除 `2^32-1`）：

- 非空、长度 ≤ 10（避免溢出）
- **无前导零**（`"01"` / `"00"` 不算，但 `"0"` 算）
- 全为 ASCII 数字
- 数值 ≤ **4294967294**（`2^32-2`）—— `"4294967295"` 按规范不算

于是 `"1e2"` / `"1.0"` / `"+1"` / `"-1"` / `"\u00201"` / `"NaN"` / `"Infinity"` / `""` 全是**字符串键**，
按插入序排在整数键之后。

**`Value` 层的接入**：`to_primitive(Object)` 与 `to_string(Object)` 都是 **`"[object Object]"`**
（`Object.prototype.toString` 的默认结果 ⇒ `to_number(Object)` 是 NaN）；`kind_of` 新增 `V_OBJ`；
`strict_equals` 对 Object 按指针比；`equals` 的规则 10/11 改成 `(V_ARR || V_OBJ)` —— **依然是对称的两条**。

#### 一个测试教训

`4294967295` 的“整数键 / 字符串键”之争，**单键对象区分不出来** —— 因为
`number_to_string(4294967295.0)` 恰好就是 `"4294967295"`，两种存法 `keys()` 输出完全一样。
必须让那个键**有机会跳到字符串键前面**才可区分：
`hold o 2 a n 1 4294967295 n 2` → 正确是 `"a,4294967295"`，错成整数键则是 `"4294967295,a"`。
（第一版我写的是 `o 2 4294967295 n 1 5 n 2`，两种存法都得 `"4294967295,5"`，**变异没抓到**。）

| 变异 | 结果 |
|---|---|
| 去掉“无前导零”判定 | FAIL 第 15 行（`"01"` 与 `"1"` 合并） |
| `2^32-2` 放宽成 `2^32-1` | FAIL 第 84 行（`4294967295` 跳到 `"a"` 前面） |
| `keys()` 先输出字符串键 | FAIL 第 11 行（`o 5 b n 1 2 n 2 a n 3 1 n 4 c n 5`） |
| `set` 更新已有的字符串键改成追加 | FAIL 第 49 行（键重复成 `"a,b,a"`） |

**已知偏差**：JS 的 `Object.keys(null)` / `Object.keys(undefined)` 会**抛 TypeError**；
C++ 的 `object_keys` 对非对象返回空列表。这条在本作数据里不可达。

### 4.8 V8 `JSON.stringify` / `JSON.parse`

见 `native/README.md` 的“已知偏差”表与 `tests/differential/PROTOCOL.md` §6；要点：

- `json_stringify(const Value&) -> std::optional<std::u16string>`，`nullopt` 就是 JS 的 `undefined`。
  `undefined` 在顶层→`undefined`、在数组→`null`、在对象→**整个键被跳过**；非有限数→`null`；
  转义表 = `"` `\` `\b\f\n\r\t` + `<0x20` → `\u00XX`，**不转义 `/` 也不转义非 ASCII**。
- `json_parse` 严格：只认 `\t\n\r` 与空格、数字不许前导零/`+`/`.` 开头或结尾/hex/`Infinity`/`NaN`、
  不许尾随逗号/注释/单引号/BOM，重复键**后者胜**。
- 差分发现 1 个真 bug：`json_parse` 漏了**开头的 `ws()`** ⇒ `JSON.parse(" 1 ")` 被自己判成 err。

### 4.9 V9 `JSON5`

`lfw/core/json5.{h,cpp}` + `json5_util.h` + 生成的 `json5_unicode.{h,cpp}`。
**目标是与仓库里的 `json5@2.2.3` 逐一对齐**，不是“按规范写一个”。完整细节见 `native/JSON5.md`。

### 4.10 V10 `fields.ts`（字段描述层）

**为什么可以直接用 `Value` 实现**：§1.2 已把 `fields` 归到“一次性装载路径”，
而且 TS 里的字段描述符（`{type:'int', min:0, title:'...'}`）**本身就是普通对象**。
所以 C++ 侧不造 struct，直接对 `Value`/`Object` 操作 —— 1:1 忠实，且能用现成的值字面量 DSL 对拍。

```cpp
Value field_desc(const std::u16string& type, const std::vector<Value>& args);  // w()
Value fields_of(const Value& source);                       // fields()
Value fields_map_2_fields_obj(const Value& field_map);      // 反向
void  reorder_fields(Value& obj, const Value& field_map);   // 按 order 重排，稳定
Value to_array(const Value& v);                             // TS 里叫 as_array（改名避冲突）
bool  validate_fields(const Value& obj, const Value& field_map,
                      std::vector<std::u16string>* errors,
                      std::vector<std::u16string>* warnings);
```

**TS 的 `Map` 在 C++ 侧用 `Object` 顶替**，两边行为等价（已在差分中验证）：
`fields()` 是从对象字面量按 `for...in` 插入的 ⇒ Map 顺序 = 对象的 `Object.keys` 顺序
（整数样式键升序在前），而 C++ 的 `Object::keys()` 刚好就是这个顺序。
`reorder_fields` / `validate_fields` 只用到 `get` / `has` / 迭代，对两者都是同一套语义。
差分侧 TS 那边用 `new Map(Object.entries(obj))` 还原成真 Map。

**两个把我坑住的语义细节**（都是被测用例抓出来的）：

1. **`Array.prototype.join` 把 `undefined` 变成空串，而模板字符串变成 `"undefined"`**。
   `field.options.map(o => JSON.stringify(o.value)).join(', ')` 里若某个 option 没有 `value`，
   结果是 `"1, "` 而不是 `"1, undefined"` ⇒ C++ 需要两个 helper（`json_text` / `json_join_item`）。
   这个 bug 是“value 不在白名单里”的用例抓到的 —— **错误消息文本也要对拍**。
2. **`typeof v === 'object'` 包含 `null` 与数组**，所以 `w()` 里 `Object.assign` 对它们都会执行
   （`null` 无效果；数组会把下标变成键 `"0"`/`"1"`）；而**字符串走另一个分支**（当 title/desc）。

**不可达的 TS 抛异常路径**（C++ 无异常，能测的都写成等价行为）：

| 情形 | TS | C++ |
|---|---|---|
| `reorder_fields(obj, null)` | TypeError | 直接返回 |
| `object` 类型字段缺 `fields` 且值非空对象 | TypeError | 当空字段表 |
| `validate_value` 的 `field` 为 `undefined` | TypeError | 当空字段 |

都在 `defines/` 的真实数据里不可达（`fields` 一定存在，`field_map` 一定是 `Map`）。

**变异测试 19/19 全杀**，覆盖：`w()` 的 title/desc 计数与追加、`order` 起点、
“先合并源对象再补 key/order”的顺序、排序方向/稳定性/清空重插、`undefined` 是否算已知字段、
`to_array` 的 null/undefined 与引用语义、`assign` 的字符串索引键、`nullable` 真值、
`array===true`/`'auto'` 的严格比较、int 的整数判定与 `min` 比较、options 的严格相等与 join 语义、
未知字段告警。

### 4.11 V11 `defines/` 的枚举（46 个）

`native/lfw/defines/*.h`，**由 `native/tools/gen_defines_enums.mjs` 从 `src/LFW/defines/**/*.ts` 生成**：
数值枚举 → `enum class X : int`；字符串枚举 → `namespace x { inline constexpr const char16_t* kMember = ... }`。
另有统一注册表 `defines/all_enums.h`（生成）+ 手写扩展 `defines/all_enums_extra.h`。

**为什么生成而不是手抄**：43 个文件含 `export enum`，共 1341 行 / ~1000 个成员，纯数据零逻辑 ——
手抄的唯一价值是引入 typo。生成器**同时**产出 TS 侧的枚举清单
（`tests/differential/subjects/gen/defines_enums.ts`），所以两边都**没有手抄的枚举名单**。
配套工具 `native/tools/list_ts_enums.mjs`（剥注释、尊重字符串字面量地列出所有枚举，用于读数据）。

**验证**：差分 subject `defines` 对每个枚举都打出「成员名 → 值」以及 TS 运行时枚举的**反向映射**，
与 C++ 侧同一形式逐行比 —— `defines/all` **796 行全对**。

**⚠ 一个被变异测试抓到的设计错误（重要）**：第一版让生成的头里同时写「枚举成员初始值」
和「表里的字面量」，于是**改枚举值根本测不到**（被比对的是表），3 条变异存活。
改成表**引用枚举成员本身**（`static_cast<double>(BdyKind::Defend)`、`team_enum::kTeam_8`）后 6/6 全杀。

> ⇒ **生成的代码内部也不要重复字面量。任何「名字 → 值」的表都必须引用权威定义本身，
> 否则测试验的是那张表，而不是被测的枚举。**

**跳过的 3 个文件**（生成器不求值 → 手写，`entries()`/`name_of()` 同样引用枚举成员）：
`FacingFlag`（成员引用同文件常量 `L`/`R`/`B`）、`HitFlag`（`Both = Ally | Enemy` 等位组合）、
`EntityEnum`（`Entity = HitFlag.Ohters` 跨枚举引用）。

**两个必须复刻的 TS 语义**（差分都在验）：
- 数值枚举有**反向映射**（`BdyKind[2000] === "Defend"`）；**同值时后者胜**
  （`FacingFlag` 的 `SameAsCatcher` 与 `SameAsBearer` 都是 4 ⇒ `FacingFlag[4] === "SameAsBearer"`）。
- 字符串枚举**没有**反向映射。

#### 覆盖检查（`tools/check_defines_coverage.mjs`，已接进 `native.mjs all`）

**⚠ 这里曾有一个真实漏洞，值得记**：C++ 侧与 TS 侧的枚举名单**都出自同一个生成器**，
所以生成器漏掉的东西会被两边**同时**漏掉 —— **差分测试看不见**。

补的办法是加一条**独立于生成器**的路径：把 `defines/**/*.ts` 全部 import 进来，
在运行时扫导出并按形状分类（数值枚举 = 同时有 `名→数` 与 `数→名`；字段表 = `*_fields`；
`*Descriptions` / `*Labels` / `Record<枚举, string>` = 标签表；同一对象再次出现 = 别名），
再与 C++ 注册表比对。别名按**对象同一性**归组，只要有一个名字被注册就算覆盖。

它立刻查出 **1 个真漏洞**：`ITerrainInfo.ts` 里是 `export const enum TerrainEnum`
（生成器的正则只写 `export enum`）⇒ 补上后 `defines` 从 796 → **851 行**。
另外 `CMD`（`const enum`，3 个成员引用 `CheatEnum`）和 `BinOp`（生成器跳过）也补进了注册表。

现在：**字段表 34/34、枚举 52 个、`coverage: OK`**（唯一剩下的 `suffix_map` 是
`Record<DatTypeEnum, string>` 标签表，已人工确认不是枚举）。

> **教训：「两边同源」消除了转录错误，但也消除了交叉检查。**
> 凡是生成出来的数据，都要再配一条从**权威实现**出发的独立覆盖检查。

### 4.12 V12 `defines/` 的字段表（34 张）

字段表是 `defines/` 里**运行期真正相关**的部分：`reorder_fields(obj, table)` 只取 `order`；
`WorldDataset.ts` 用 `has`/`keys` 做 schema 比对；`IFrameInfo.ts:398` 把整张
`world_dataset_fields` 嵌成嵌套字段的 `fields`。

**做法：跑真实的 TS，把结果内嵌成 JSON5。**

`native/tools/gen_defines_fields.mjs`：用 esbuild 打包一个临时入口
（`import * as M from ".../defines/xxx"`，遍历每个模块里所有 `*_fields` 导出，
把 `Map` 递归转成保序的普通对象）→ 用 node 跑一遍 → `JSON.stringify` →
生成 `native/lfw/defines/fields_gen.h`，每张表是一个惰性解析函数：

    inline const Value& bdy_info_fields() {
      static const Value v = json5_parse(uR"J({...})J").value;
      return v;
    }

字符串按 8000 个 UTF-16 单元**分块拼接**（MSVC 单个字面量上限 16380 字节，超了报 C2026；
拼接要避开从代理对中间切开）。

**为什么不手抄**：961 行字段表里塞满了中文标题/描述/`options` —— 手抄的唯一产物是 typo。
生成法下 C++ 的字段表与 TS **同源**，而 `json5_parse` 本身已被 JSON5 那一整套差分验证过。
差分 subject `defines_fields` 再比一次「C++ 解析结果 vs TS 原表」，
顺带覆盖「JSON5 往返丢信息」的风险 —— `defines_fields/all` 34 行全对，5 条变异全杀。

**边界**：`JSON.stringify` 会把 `-0` 写成 `0`。当前 34 张表里没有 `-0`；
若将来有，diff 会在数字位模式上暴露，那时改成手工序列化即可。

### 4.13 V13 `base/{NoEmitCallbacks, Callbacks, FSM}`

`native/lfw/base/no_emit_callbacks.h`（内含 `CallbacksT`）、`native/lfw/base/fsm.h`。

**移植要点**

- `NoEmitCallbacksT<Payload>`：TS 的 `_map: Map<key, Pack>` → `std::vector<Pack> _packs`
  （保插入序 + 线性查找；键是十个量级，不值得上哈希表）。
- TS 的 `Pack._set: Set<F>` → `std::vector<Listener*>`：必须保插入序、按**身份**去重、
  `del` 后重新 `add` 要排到末尾。这就是不能用 `set` / `unordered_set` 的原因。
- **`list_fn` 的替代**：TS 用 `list_fn`（反射枚举 listener 的方法名）取 key 列表，C++ 没有反射，
  改为显式 `handlers` 列表（`{key, fn}`），`Pack` 按 key 线性查 handler。
- **重入 + 延迟操作**：`emit()` 先入队，已在派发中则直接返回，否则进 `handle_pendings()`；
  派发期间的 `add` / `remove` 进 `_waits`，**每轮结束后**统一应用（否则迭代 `_set` 时会错位）；
  `once` 的删除以「延迟 del」入队 ⇒ 一次 flush 内不会重复触发。
- **溢出保护**：单次 flush 最多派发 `kMaxPendingsPerFlush = 1000` 条；超了告警一次
  （`_overflow_warned`，队列耗尽后复位）、`compact()` 保留剩余、下次 `emit` 继续。
  `_head` 是「已消费下标」，`compact()` 会把它归零 ⇒ flush 之外恒为 0，
  所以「`size() - _head` 写成 `size()`」是**等价变异体**（见 PROTOCOL §6.9.1）。
- **告警出口**：TS 是 `Ditto.warn`，C++ 是 `callbacks_warn()`（宿主注入的 `std::function`，
  默认空 ⇒ 静默）——`Ditto` 那类全局可变宿主状态的标准做法。
- **`FSM`**：`_state_map` 用 `std::vector<std::pair<Value, IState*>>` 保序（同 `Map`），
  键比较 `strict_equals`（≈ SameValueZero）。`set_state` 的顺序必须是
  `prev = cur` → `cur.leave()` → `state_time = 0` → `cur = next` → `next.enter()` → 日志 → 通知回调；
  顺序错了差分立刻红（`E` / `LV` 行会串位）。
- `IState::update` 用 `std::optional<Value>` 表达「不切状态」：TS 的 `undefined` / `null` 都早退。
- `state_label` 的**宽松相等**是 TS 原样行为（`name == key`，`"7" == 7` 为真 ⇒ 标签不带括号），
  移植时不能「顺手改成严格相等」；数字 key 用例专门盯这一点。

**验证**：subject `base`（`cases/base/core.txt`，3127 行全对）+ 21 条变异全杀。
用例覆盖：身份去重、插入序、`del` 后重加、重入 `emit`、派发期间 `add`/`del` 延迟生效、
`once` 自删、未知 key 是 no-op、`clear`、溢出告警文案与「耗尽后复位 + 二次告警」、
数字 key 的宽松标签、`use`/`reset`/`update`/`snapshot`/`restore`、`enter`/`leave` 顺序、
`on_state_changed` 通知。

**渲染注意**：告警文案含 `U+2014`。终端里的 `to_ascii` 只做 `static_cast<char>` ⇒ 非 ASCII 被截成
控制字符，两侧看起来不一致。**自由文本一律用两侧共用的 `esc()` 渲染**（`>0x7e` 转义成 `\uXXXX`）。

### 4.14 V14 `defines/` 的运行时数据（`Defines` 命名空间）

**先验证范围，再动手。** `defines/` 有 127 个文件 / 8613 行，按「谁在用」分成三类
（`native/tools/list_defines_exports.mjs` 分类导出 + 全仓库查引用）：

| 类别 | 规模 | 使用者 | 结论 |
|---|---|---|---|
| 33 个 `xxx_new()` 默认构造 | 33 个函数 | **只被 `dat_translator/`**（XML→数据 翻译层）用 | 运行时不用，**不移植**（要移就等 `dat_translator` 一起） |
| `I*.ts` / `type` / interface | 约 3000 行 | 只有类型，无运行时面 | C++ 运行时用 `Value` 动态取字段，**不需要结构体** |
| `Defines` 命名空间的常量与数据表 | 66 条 + 7 个顶层对象 | `entity/` `collision/` `World` `bot/` `controller/` | **必须移**，且可差分 |

所以 V14 只做第三类：`native/lfw/defines/runtime_gen.{h,cpp}`（生成）+ `defines_data.{h,cpp}`（手写访问层）。

- 生成器 `native/tools/gen_defines_runtime.mjs` 跑**真实 TS**，把 `Defines` 命名空间的每个成员
  （number/string/boolean/数组/普通对象/`Map`）与 7 个顶层运行时对象
  （`EMPTY_FRAME_INFO` `GONE_FRAME_INFO` `ENTITY_PRIORITY_MAP` `CONFLICTS_KEY_MAP`
  `DifficultyList` `DifficultyNames` `DifficultyDescriptions`）序列化成 JSON5 字面量。
  这是**完整覆盖**（只跳过 2 个函数，它们单独手写并差分）——不像枚举生成器那样有「规则漏掉一类」的盲点。
- 手写层只做零重复访问：`defines::table()/find()/num()` 直接读生成表；
  `desire()` 用 `num(u"Defines.MAX_AI_DESIRE")`（**不重抄 10000**）；
  `is_cheat_type()` 用已移植的 `cheat_enum::k*`；`is_difficulty()` 用已移植的 `Difficulty`。
- 序列化用 **JSON5 而不是 JSON**：`JSON.stringify` 把 `-0` 写成 `0`、`NaN` 写成 `null`，
  而 `VOID_BG.base.near` 正是 `-0`、`EMPTY_FRAME_INFO.state` 是 `NaN`
  ⇒ 差分在数字位模式上立即暴露（`n0:8000…` vs `n0:0000…`）。
  C++ 侧本来就用 `json5_parse`，所以生成 JSON5 是零成本的正解。
- **构建陷阱**：`native/lfw/CMakeLists.txt` 用的是**显式源文件列表**（不是 glob）——
  新增 `.cpp` 必须手工登记；只有 subject 目录是 glob。

**验证**：subject `defines_runtime`，`cases/defines_runtime/all.txt` **149 行全对**；
9 条变异全杀（含 1 条打生成表里的数值，证明这些表确实在对拍范围内）。

### 4.15 V15 `dat_translator/CondMaker`（条件字符串构造器）

**第四块（`entity`/`collision`/`buff`/`state`/`controller`/`bot`/`World`）先量再做。**
实测总规模 **12,526 行**（entity 3498 / bot 1854 / loader 1839 / collision 1481 /
state 1300 / controller 1023 / World 965 / buff 566）。按「谁能独立验证」找缝隙：

| 候选 | 判断 |
|---|---|
| `collision/**` 的 32 个导出函数 | 几乎全吃 `Collision`（内含 `Entity`）⇒ 不是可独立验证的缝隙 |
| `loader/preprocess_*` | 纯数据→数据，但依赖 `CondMaker` / `set_hit_flag` / `get_val_geter_*`（需步骤 4 的数据模型） |
| `dat_translator` 的纯逻辑核心 | **可独立验证**，且是上面两者的共同前置 ⇒ 先做它 |

`CondMaker` 是所有默认条件（`bdy.test` / `itr.test`）的构造器：fluent 拼**中缀字符串**，
产出交给已验证的 `Expression` 解析 —— 所以它的可验证面是**文本 + 错误文案**，很干净。

- 迁移形态：`native/lfw/dat_translator/cond_maker.{h,cpp}`。
- TS 用 `throw` 报错，C++ 禁 `throw` ⇒ 改成**首错冻结**（`ok()` / `error()`）：一旦出错，
  后续操作与 `done()` 都不再改变状态，等价于「异常向上传播、调用方停止」。
- **`not` / `and` / `or` 是 C++ 关键字** ⇒ 改名 `not_` / `and_` / `or_`（唯一 API 偏离）。
- `one_of(...)` 家族参数从 `std::initializer_list` 改为 `const std::vector<Value>&`
  （没法从迭代器构造 `initializer_list`）。
- 复刻的 TS 细节：宽松 `== void 0` 同时抓 `undefined` **和 `null`**；`done()` 对文本片段
  做 `${v}`.trim()、最后再整串 trim 并删 `\n\r`；`wrap` 只在片段**前**无运算符时允许；
  `quote_strings` 只转义引号字符本身；子构造器继承 `term`/引号设置，且子错误向上传播。
- `js_trim` 的空白集合（含 NBSP / U+3000 / U+FEFF / LS / PS）在 `lfw_core` 里没有现成的，
  本地实现并用用例锁住（NBSP、U+3000、U+FEFF、`\t\n\r` 各一条）。

**验证**：subject `cond_maker`（55 行：29 条正常文本 + 19+ 条错误文案）+ 17 条变异全杀。

### 4.16 V16 `defines/` 的名称/标签函数（`labels`）

`set_hit_flag` / `set_bdy_kind`（数据烹饪要用）依赖这批函数，所以它们从“编辑器专用”
变成了关键路径。形态：`native/lfw/defines/labels.{h,cpp}`。

- **BdyKind/WpointKind 查表是“一个 JS 对象”**：数字枚举对象同时有正向（名→值）与反向
  （值→名）两张表，且 TS 是**逐成员**交替写入的。C++ 用 `bdy_kind_entries()` /
  `wpoint_kind_entries()` 惰性建出同样的 map（键都是 `String(v)`，所以 `String(0)` 与
  `String(-0)` 同为 `"0"`），零字面量重复。
- **两个函数的回退语义相反**（对拍抳出来的）：
  `bdy_kind_name` 用 `if (!ret)`（**falsy**）⇒ `BdyKind["Normal"]` 为 `0` 会变成
  `unknown_Normal`；`wpoint_kind_name` 用 `??`（**nullish**）⇒ `WpointKind["None"]` 就返回 `0`。
  移植时不能“统一成一个”。
- **`get_hit_flag_name` 带记忆化副作用**：算出的组合名会写回 `HIT_FLAG_NAME_MAP`，
  **键是原始值**（`v`）而不是 `Number(v)` ⇒ 传 `"3.7"` 与传 `"0b11"` 会分别留下 `"3.7"`、
  `"0b11"` 两条记录。这是**可观察行为**（表会变长），所以差分里显式 dump 整张表来锁住。
- 位筛选用 `r & v` ⇒ 必须用已移植的 `js_to_int32`（`ToInt32`），不能用 `static_cast<int>`
  （负数与大数会差）。
- `get_hit_flag_full_name` 的前缀是 `AllyFlag.` —— 原代码里的**错字**，照抄不改。
- 所需的纯数据（`HIT_FLAG_NAME_MAP` / `HIT_FLAG_DESC_MAP` / `WpointKindDescriptions` /
  `OLD_BDY_KIND_GOTO_MIN|MAX` …）继续由 `gen_defines_runtime.mjs` 提取
  （现 81 条 / 15 个顶层对象）。

**验证**：subject `labels`（107 行）+ 15 条变异全杀。

### 4.17 V17 `dat_translator` 小助手（`take*` / `set_*` / `fixed_float` / `find_float` / `copy_*`）

形态：`native/lfw/utils/type_check.h`（谓词）+ `native/lfw/dat_translator/helpers.{h,cpp}`。
全部吃 `Value`/`Object`，所以现在就能对拍，不用等步骤 4 的数据模型。

- **`take` 家族**：`take`/`take_str`/`take_num`/`take_positive_num`/`take_not_zero_num`
  （取值并**删除**键）。`take_str` 只在值确实是字符串时才删；没有就**不删**。
- **`0xCDCDCDCD` 哨兵值必须过滤**（`-842150451`，LF2 定长结构体里未初始化内存的读法）：
  `take_num` / `take_not_zero_num` 都要过滤，否则**源数据导入会出错**。
  这是**必须保留的行为**，不是可选项；用例 `o5` 锁住它，变异点扮它的删除。
- **`fixed_float` = `Number(n.toFixed(digits))`**：`toFixed` 是**对精确值**四舍五入
  （半值远离零）⇒ `fixed_float(2.675, 2)` 是 **2.67**（因为 double 2.675 实际是 2.67499…）。
  先用 `a * scale` 再取整**是错的**（乘积会被舍成 267.5，于是误判平局）⇒
  用 `std::fma(a, scale, -p)` 取**精确残差**，在 `frac == 0.5` 时用残差符号定平局。
  负数先取绝对值再补符号（所以 `-0` 返回 `+0`、`-0.4` 返回 `-0`，与 `toFixed` 一致）。
- **`find_float`**：遍历顺序是数组按下标、对象按 JS 键序；整数不算，`NaN`/`∞` 算。
- **`copy_bdy_info` 走 `JSON`、`copy_itr_info` 走 `JSON5`** ⇒ 两者对 `NaN`/`Infinity` 的结果不同
  （前者变 `null`，后者保留）——这是**真的行为差异**，不是笔误。
- 合并语义 `{...src, ...edit}`：`Object::set` 对已有键原地更新 ⇒ 已有键**保位**。

**验证**：subject `dat_helpers`（100 行）+ 19 条变异全杀。

### 4.18 V18 `dat_translator` 的帧跳转（`get_next_frame_by_raw_id` 等）

形态：`native/lfw/dat_translator/next_frame.{h,cpp}`。这是 `preprocess_next_frame` 的前置。

- **魔法 id 归一化**：`"1000"` → `NEXT_FRAME_GONE`、`"999"` → `NEXT_FRAME_AUTO`、
  `"-999"` → `NEXT_FRAME_AUTO_BACKWARD`、`"0"` → `{id:"0"}`（`zero_as == 'frame'`）或 `{}`。
  注意 `"" + id` 是 **JS String()** 归一化 ⇒ `n -0` 也走 `"0"` 分支。
- **隐身区间是数值比较**：`1100 ≤ id ≤ 1299` ⇒ AUTO；`-1299 ≤ id ≤ -1100` ⇒ AUTO_BACKWARD。
  边界（1100/1299/-1100/-1299）各有用例。
- **负数两条分支**：数字走 `-n`（`String(-n)`），字符串走 `substring(1)`；两者都带
  `facing: FacingFlag.Backward`（= 2）；非负则只有 `{id: String(id)}`。
- **`Defines.NEXT_FRAME_*` 是共享的模块级对象**（按引用返回，TS 里可直接被改）⇒
  C++ **不能返回副本**，必须返回解析表里**同一个** `Object`（`find()` 取指针后解引用，
  共享 `shared_ptr`）。用例末尾用 `nfmut` 改一次再取，两侧都能看到改动 ⇒ 身份被锁住。
- **`cook_next_frame_cost`**：`costs` 缺失就直接返回；`ret.id` 取 `string` 或 `数组[0]`；
  `type=hit` 无条件写入 mp/hp，`type=next` 只把负值取反；最后 `!ret.mp` / `!ret.hp` 的键
  **删掉**（所以 0 等于“不存在”）。
- `add_next_frame` 空 items 时**原样返回 src**（不包一层数组）；`edit_next_frame` 对数组逐项回调、
  对单对象回调一次（回调可原地改）。

**验证**：subject `next_frame`（83 行）+ 17 条变异全杀。

### 4.19 V19 `dat_translator/ColonValueReader`

形态：`native/lfw/dat_translator/colon_value_reader.{h,cpp}`。它解析 LF2 里
`name: xxx width: 100 zboundary: 550 550` 这类文本（真实调用点只有 `make_bg_data`）。

- **禁 `<regex>`** ⇒ 手写匹配。三条模式：
  - Str：`NAME\s*:\s*(\S+)[\s|\n]*`
  - Int：`NAME\s*:\s*(\d+)[\s|\n]*`
  - Int_2：`NAME\s*:\s*(\d+)\s*(\d+)[\s|\n]*`
  实现要点：`exec` 是**最左匹配** ⇒ 从每个下标试起；`\S+`/`\d+` 贪婪且后面的尾随类可为空 ⇒ 无需回溯；
  但 Int_2 的 `\d+\s*\d+` **需要回溯**（`a: 123` → [12, 3]），所以从最长第一段往短试。
- **`\s` 集合复用 `lfw/core/js_string.h` 的 `is_str_white_space`**（已由 `core/to_number` 验证）。
  顺手把 `cond_maker.cpp` 里那份重复的局部 `is_js_whitespace` 删掉——同一规则只留一处
  （见 PROTOCOL §6.9.5 的教训）。
- **原代码里有一处手误，必须照抄**：
  ```ts
  rem_txt = rem_txt.slice(0, reg_res.index) + rem_txt.slice(reg_res[0].length);
  ```
  第二次 slice 用的是 `match[0].length` 而不是 `index + match[0].length` ⇒
  它是从**字符串头部**删掉等长的一段，而不是删掉匹配到的那段。后果：
  `read(i:z)` 处理 `"width: 100 z: 7"` 得到 `rem = "width: 100 h: 100 z: 7"`（**变长**）。
  这不是我们该“修正”的地方——差分把两侧拉到一致，变异点再防止它被“修好”。

**验证**：subject `colon_reader`（52 行）+ 14 条变异全杀。

### 4.20 V20 `dat_translator` 的 cook 系列

形态：`native/lfw/dat_translator/cookers.{h,cpp}`，含 `cook_bdy` / `cook_wpoint` /
`cook_cpoint` / `cook_itr` / `float_scaling_itr`。这五个都是纯数据→数据。

**范围判定**（同样先量再做）：

| 文件 | 判定 |
|---|---|
| `cook_bpoint` | 空实现（`// TODO`）⇒ 不写 |
| `float_scaling_bdy` | 空实现（注释掉的一行）⇒ 不写 |
| `float_scaling_qube` | **未导出**（文件内局部函数）⇒ 不写 |
| `post_process_obj_data` | 依赖 `make_frames_special` ⇒ 待办 |
| 上面那五个 | 依赖已就位（`take*` / `fixed_float` / `get_next_frame_by_raw_id` / `reorder_fields`） | ✅ 做了 |

**移植要点**

- **两处宽松 `==` 不能改成严格比较**：`unsure_wpoint.kind == 1` 与
  `CPointKind.Attacker == cpoint.kind` 都是 **`==`** ⇒ 字符串 `"1"` / `"1"` 也算命中。
  用 `equals()`，不是 `strict_equals()`。
- **`o.x = take(o,"x")` 的两种语义要分清**：`x || 0` 是**取值**（回退 0）；
  而 `cpoint.x = take_not_zero_num(...)` 是**赋 undefined**（键仍在但值为 undefined）。
  两者在 `render` 下能看到差异（后者打印 `u`）。
- **`{...get_next_frame_by_raw_id(...), facing: ...}` 是浅拷贝** ⇒ C++ 用 `shallow_clone`，
  不能直接改返回的对象（可能正好是 `Defines.NEXT_FRAME_*` 共享常量）。用例末尾加 `nf n 999`
  检查共享常量没被污染。
- **通用枚举反查 `defines::js_enum_get(name, v)`**：`cook_itr` 要 `(ItrKind as any)[kind]`，
  而 `all_enums.h` 已经有 `{名字, entries, name_of}` 注册表 ⇒ 用注册表惰性建 JS 风格的
  正/反向 map。顺手把 `labels.cpp` 里 bdy/wpoint 各自的两份实现合并到这一处。

**验证**：subject `cookers`（68 行）+ 32 条变异全杀；`labels` 重构后 15/15 仍全杀。

#### 4.20.1 `cook_opoint`

- **`oid` 先归一化**：`opoint.oid = "" + take(opoint, "oid")` ⇒ `oid` 恒为字符串，
  所以后面 `case OID.FirenFlame:` 这类字符串枚举对比才有意义（`OID` 是字符串枚举，
  `kFirenFlame = "211"`）。
- **`facing` 规则**：`facing % 2 ? Backward : None`，然后 `2..19` 覆盖为 `Right`，
  `>= 20` 不改 facing 而是写 `opoint.multi = round(facing / 10)`。`%` 是 JS `%`（`fmod`）。
- **`frame.state` 用严格比较**：`switch (frame.state) case S_E.Ball_Flying:` 是 `===`
  ⇒ 帧里写字符串 `"3000"` **不会**命中数字 `3000`。C++ 用 `strict_equals`。
- **硬件编码帧列表**：`['50','54','109'].includes(action.id)` 只在 `action` 是**数字**时
  才会命中——因为 `action` 先被 `take` 删掉、只在“是数字”分支里重建成 `{id: ...}`。
  也就是说必须用 `action: -50 / -109` 这种**负数**（会经 `get_next_frame_by_raw_id` 变成 `id: "50"`）
  才能走到那段代码。
- 两个 `speedz` 常量（`DEFAULT_OPOINT_SPEED_Z` = 3.5、`DEFAULT_FIREN_FLAME_SPEED_Z` = 0.5）
  一律用 `defines::num(...)` 读生成表，不重抄字面量。
- `cook_opoint` 会往**返回的下一帧对象**上写 `facing` ⇒ 当 action 是 `999/1000` 时会直接
  写进 `Defines.NEXT_FRAME_*` 共享常量（原代码如此，已对拍）。

### 4.21 V21 `make_frame_state`（+ `foreach` / `ensure`）

形态：`native/lfw/dat_translator/make_frame_state.{h,cpp}`，纯数据→数据（就帧对象改）。

**范围**：8 个 `state` 分支（`Ball_3005` / `HeavyWeapon_OnHand` / `Weapon_OnHand` /
`Burning` / `OLD_LouisCastOff` / `Falling` / `Frozen` / `Message`）。`make_frame_behavior`
是 14 个 `frame_behavior/make_fb_*.ts` 的分发器 ⇒ 先补那 14 个模块再搬（延后）。

**移植要点**

- **`foreach` 的对象分支在本仓库里是“观测上等价”的死代码**：TS 是
  `Array.isArray(x) ? x.forEach(fn) : traversal(x, (v,k,a)=>fn(k,v,a))`，而
  `traversal` 内部是 `Object.keys(r).map(_k => { func(k, r[k], r) })` —— 参数被
  **交换两次**，于是对象路径最终也是 `fn(value, key, obj)`，与数组路径同序。
  对数组而言 `Object.keys` 给 `"0","1",…`、`r["0"]` 恒等于 `r[0]` ⇒ 两条路径
  逐个元素、同顺序、同参数。**C++ 只实现数组路径**（`as_array == nullptr` 直接返回）。
  破口只有两处：**稀疏数组**（`Object.keys` 只列存在的下标）与非下标自有键
  （如 `bdy` 是 `{a:1}` 这种普通对象）；用例 `f6` 专门盖住后者，把“等价”钉住。
- **`ensure` 的 C++ 版是另一套签名**（`optional<vector<T>>`）⇒ 为 `Value` 加重载，
  且必须真的**写回** `output`（falsy 时是“新建数组”，不是“返回数组”）。
- **`Falling` 的 `kind` 判定是严格 `!==`**：`bdy.kind` 为字符串 `"0"` / 缺键
  （`undefined`）都**跳过**。用 `strict_equals`，不是 `equals`。
- **`OLD_LouisCastOff` 的 5 个 opoint 是硬编码字面量**（`39±offset_z`、`-dvx_b`、
  `z: ±30`、`dvz: ±dvx_z`、两个 `action.facing: Backward`）⇒ 逐字手抄，差分来抓错。
  字段**顺序**也可观测（`render` 按插入序打印）⇒ 变异里加了“x/y 互换”。
- **`ensure(frame.opoint, …)` 的三条路径**都要有用例：缺键、显式 `null`（都是
  “新建数组”）、已有数组（`push` 追加）。

**验证**：subject `cookers`（`/all` 68 行不变）+ 新用例 `cookers/mfstate`（20 行，8 个分支
加非数字/缺键/`null` 反例）；新增 17 条变异，合计 **49/49 全杀**。

### 4.22 V22 `frame_behavior/*`（14 个模块 + 分发器）

形态：`native/lfw/dat_translator/frame_behavior.{h,cpp}`（14 个 `make_fb_*` +
`make_frame_behavior`），以及共享的字面量工具 `native/lfw/dat_translator/value_builder.h`
（`n` / `s` / `en` / `make_obj` / `make_arr` / `field_or` / `same_str`）。后者的目的是让
「TS 对象字面量 → C++」几乎是逐字转写（少一次转写就少一类错），同时把 `make_frame_state.cpp`
里那份同类工具统一到一处。

**移植要点**

- **链式赋值的副作用顺序是从右往左**：`frame.ctrl_x = frame.ctrl_y = frame.ctrl_z = 1`
  先执行 `ctrl_z = 1` ⇒ 键的插入序是 `ctrl_z, ctrl_y, ctrl_x`。因为 `render` 按插入序
  打印，所以 C++ 必须按这个顺序 `set`（变异「顺序颠倒」可杀）。
- **`switch (frame.id)` 是严格 `===`**：数字 `1` 不命中 `case '1'`。`same_str` 用
  `strict_equals`（变异改成 `equals` 即被杀）。
- **同名字段取值不同 ⇒ 不能硬套一张“默认速度表”**：`boomerang`（无 `dvy`/`vym`，
  `ctrl_x`/`ctrl_z` 是 `SpeedCtrl.Control`）、`chasing_same_enemy`（`dvy=8`、`acc_y=-0.25`、
  **无 `ctrl_y`**）、`julian_ball`（`dvx=12`、`acc_x=0.18`…）各自单写；只有
  `bat_chase` / `dennis_chase` / `john_chase` / `jan_angle_blessing` 四者的字段与**顺序**
  完全一致，才共用 `set_default_speed`。
- **`...set_hit_flag({}, HitFlag.AllyFighter)` 是对象展开**：`hit_flag`/`hit_flag_name`
  被插在字面量**中间** ⇒ 不能对目标对象直接调 `set_hit_flag`（那是追加到末尾）。
  用 `hit_flag_pair()`（内部就是调 `set_hit_flag` 再读回），规则仍只有一处。
- **`make_fb_*_start` 的默认参数是 `frame.centerx` / `centery`** ⇒ C++ 用
  `std::optional<Value>`，缺省时取 `field_or(frame, u"centerx")`（**raw 值**，不转数字），
  只有 `frame.centerx - 25` 这类算式才走 `to_number`（缺键 ⇒ NaN）。
- **`firzen_volcano_start` 先调 `disater_start` 再追加 20 个 opoint**；分发器传的是
  `(frame.centerx, -79)`。
- **分发器里有看着像写反的映射**：`AngelBlessingStart → make_fb_jan_chaseh_start`、
  `DevilJudgementStart → make_fb_jan_chase_start` ⇒ 照抄（变异「互换」可杀）。
- `frame.chase` 的 `stratedy` 是原代码错字，照抄；`hp_gt_0` 是 `CondMaker` 产出的**字符串**
  （`new CondMaker().and(EntityVal.HP, '>', 0).done()`）⇒ 用同一个 `CondMaker` 生成，不手抄。
- `ensure` 对非数组的**真值**（如数字）在 TS 里会 `throw`，C++ 会当成 falsy 重建数组。
  这是有意保留的差异（不可达：真实数据里 `opoint`/`itr`/`bdy` 要么是数组要么缺失）。

**验证**：subject `cookers` 新用例 `fbehavior`（52 行：14 个模块 + 分发器 14 个分支 +
字符串 `behavior` / 未知值 / 缺字段的反例）；新增 22 条变异。

### 4.23 V23 `dat_translator` 的字符串匹配器（+ `delete_undefined`）

形态：`native/lfw/dat_translator/string_matchers.{h,cpp}`（`match_colon_value` /
`match_block_once` / `take_blocks`），`delete_undefined` 落在 `helpers.{h,cpp}`。
这三条都源自 TS 的 **RegExp**，而 C++ 禁 `<regex>` ⇒ 手写匹配（与 `ColonValueReader` 同路）。

**移植要点**

- `match_colon_value` = 对 `text.trim()` 全局迭代 `/\s*(\S*)\s*:\s*(\S*)/g`。三个易错点：
  1. `:` **后面还有一个 `\s*`**（值前的空白要跳过）—— 第一版漏了它，差分第一次就报出来；
  2. 键的 `\S*` 是**贪婪**的，必须从最长往短回溯（`a:b:c` 的键是 `a:b`、值是 `c`）；
  3. 匹配区间收在**值末尾**（不是冒号处），下一轮从那继续。
- `match_block_once` / `take_blocks` 共用模板 `${start.trim()}((.|\n)+?)${end.trim()}`：**懒惰**、
  body **至少一个字符**、start/end 会 trim。C++ 把 trim 收敛成 `find_block_trimmed` **一处**
  （两个公开函数都走它），避免同一条规则写两遍。
- `take_blocks` 的 `remains` 是把**匹配段整个挖掉**后拼回（用 `match.index` / `match[0].length`），
  不是「去掉捕获组」⇒ 挖过的区间要成对用。
- `delete_undefined` 只删**顶层**值为 `undefined` 的键（`Object.keys` 快照后逐个删，
  顺序不影响结果）；嵌套数组/对象里的 `undefined` 保留。
- 有意与 TS 不同的地方：TS 传非字符串（`null` / `number`）时这三个函数返回空/原值，
  C++ 里由调用方用 `is_str` 守住（harness 已对拍）。

**验证**：subject `string_matchers`（`/all` **35 行**）+ 13 条变异全杀。

### 4.24 V24 `make_frames_special` / `make_entity_data`（+ `traversal`）

形态：`native/lfw/dat_translator/entity_data.{h,cpp}`（`make_frames_special` /
`make_entity_special`（空实现，照抄）/ `make_entity_data`）+ `utils/container_help/traversal.h`。

**移植要点**

- **`traversal` 对数组也会遍历**（`Object.keys([a,b])` ⇒ `["0","1"]`）⇒ C++ 除了对象路径还要有
  数组路径，键名是 `String(i)`。对象路径用 `Object::keys()`（已验：整数下标键升序在前，
  其余按插入序），并且**先快照键、再逐个 `r[k]`**（回调改了容器也按快照走，与 TS 一致）。
- 回调拿到的元素是**值拷贝**（C++ 的 `Object::get` 只给 const）⇒ 改**元素内部的字段**会传播
  （共享 `shared_ptr`），但替换整个元素不会。当前所有调用点都只改内部字段（`take(frame, …)`）。
- `make_frames_special` = 对 `frames` 逐个 `take` 11 个 hit 键（`hit_Ua` 在源里写了**两遍**，
  第二遍是 no-op ⇒ 照抄；删掉它是**等价变异**）。`foreach` 的两个分支都要有用例。
- `make_entity_data` 的 `info.name` 是**原地改共享的 base**（返回的 `base` 与 ctx 里的是同一个
  对象）⇒ 用例必须同时 `dump` ctx 才能锁住这一点（变异「改成改副本」可杀）。
- `hash ?? file.replace(...)` 是 **nullish**：`hash: ""` **不**回落（`""` 保留），`hash: 0` 也不
  回落（`name` 会变成数字 0 —— 源里没有类型校验）。变异「把空串也算 nullish」可杀。
- 文件名字符过滤 `[^a-z|A-Z|0-9|_]` 里的 **`|` 是字面竖线、也会被保留**（不是「或」）⇒ 用例
  必须有含 `|` 的文件名（第一版把 `|` 放在 `hash:""` 的样本里，等于没测到 ⇒ 补 e4 后才杀掉）。
- `make_entity_special` 是空函数，照抄一个空函数（保持调用点存在）。

**验证**：subject `entity_data`（`/all` 22 行）+ 11 条变异全杀。

### 4.25 V25 `make_itr_prefabs`（+ 手写的 `entry:` 匹配器）

形态：`native/lfw/dat_translator/itr_prefabs.{h,cpp}`；配套 `utils/type_cast.h`（`to_num`）
与 `utils/type_check.h` 的 `is_non_empty_str`。

**移植要点**

- `make_itr_prefabs(full_str)`：`match_block_once("<weapon_strength_list>", "…_end")?.trim()`
  → `is_non_empty_str`（**先判 falsy**：`""` 为空、`"  "` 非空）→ 逐行匹配。
- `entry:` 的正则是 `/entry:\s*(\d+)\s*(\S+)\s*\n?(.*)\n?/g`：
  - `(\d+)` 贪婪但**会回溯**：`entry:12 34` 的 id 是 `12`、名字是 `34`；必须
    “从最长数字段往短试”（`entry:abc` 不匹配，扫描继续往后找）；
  - 名字后的 `\s*` **会跳换行**（`\s` 含 `\n`）⇒ `(.*)` 拿到的是**下一行**的内容，
    空行会被吞掉 —— 这就是源数据把冒号键值写在下一行的原因；
  - `\n?` 在 `(.*)` **之前**，且 `\s*` 已贪婪吃掉换行 ⇒ `\n?` 实际总是匹配空。
- 每个 entry 先 `{kind: 0, id, name}`，再把 `match_colon_value(remain)` 的键值
  按 `to_num(v) ?? v` 写进去（**可能覆盖 `id`** ⇒ 最终映射的键取覆盖后的 id），
  最后 `cook_itr(entry)` 再规整。
- 结果 `{[id]: item}`：重复 id 后者覆盖、键位置保持首次出现；`list` 为空时返回 `undefined`
  （“块存在但无 entry”是一个**独立**分支，要有用例）。

**验证**：subject `itr_prefabs`（`/all` 14 行）+ 11 条变异全杀。

### 4.26 V26 `make_ball_data` / `make_weapon_data`

形态：`native/lfw/dat_translator/entity_kinds.{h,cpp}`（两者共用一个 `cook_kind_frames`，
差别只有 `acc_z` 的除数 1 / 2）。

**移植要点**

- **`hit_j !== 0` 是严格不等** ⇒ `hit_j` 缺失（`undefined`）时**仍然进入**分支：
  `vzm = Extra`、`acc_z = round_float((to_num(undefined, 50) - 50) / div)` = `round_float(0)` = 0。
  别“顺手”改成 truthy 判断（变异可杀）。
- `hit_a` 走 `if (hit_a)`（真值）⇒ `hp = round_float(v / 2, 10)`（第二个参数是 **multiplier**，
  不是小数位数）。
- `hit_d && hit_d !== frame.id` ⇒ 不相同时才写 `on_dead`；`get_next_frame_by_raw_id(hit_d, 'frame')`
  的第三参 `type` 用 `'frame'`。
- `hit_Fa` 的 `behavior_name = "FrameBehavior." + FrameBehavior[hit_Fa]`：无效值 ⇒
  **`"FrameBehavior.undefined"`**（用 `defines::js_enum_get`，别用生成的 `*_name_of`，
  后者是 `switch` 无 default 的实现）。
- `info.name`：`hash ?? (最后一段 → 非 `[A-Za-z0-9_|]` 换 `'-'` → 去掉 `-obj-json5` 后缀)`。
  **`|` 保留**；`hash: ""` 不回落（nullish 而非 falsy）。
- 音效：`take_str` 取出后 `if (sound)`（空串 falsy ⇒ 不写 `*_sounds`），再
  `.replace(/\\/g,'/') + ".mp3"`（ball 侧写成 `(sound + ".mp3").replace(...)`，两者等价）。
- `make_weapon_data` 的类型表是**两级**：`switch ('' + datIndex.type)`（字符串比较 ⇒
  `type: 1`（数字）不命中）+ `"1"` 分支内 `{"120":Knife,"124":Knife}["" + id] ?? Stick`。
  不命中任何分支 ⇒ `info.type` 保持缺省 ⇒ `indexes_map[undefined] ?? indexes_map[None]`（全空串）。
- `indexes` 与 `on_dead`（`Defines.NEXT_FRAME_GONE`）都是**共享对象** ⇒ C++ 用 `static` 表 +
  `defines::find` 取同一个实例（不能在每次调用里重建）。
- `drop_hurt` / `weapon_hp` 是 `if (x && Number(x))`：字符串 `"0"` **真值**但 `Number` 后为 0 ⇒
  跳过；空串 ⇒ 直接跳过。

**验证**：subject `entity_kinds`（`/all` 17 行）+ 19 条变异全杀。

### 4.27 V27 `make_bg_data`（+ `make_bg_layer` / `bg_color_translate`）

形态：`native/lfw/dat_translator/bg_data.{h,cpp}`，一个 subject 覆盖三个函数（后两个未导出，
只能通过 `make_bg_data` 间接对拍）。**前置工作**：给 `gen_defines_runtime.mjs` 加了
`NEW_FUNCS`，把 `bg_info_new()` / `bg_layer_info_new()` / `bg_data_new()` 生成为
**每次新建** 的 C++ 函数（进 `runtime_gen.{h,cpp}`）—— 它们返回新对象，
不能像 `TOP_LEVEL` 那样做成共享条目。

**移植要点**

- **整串的 `/\\\\/g` → `'/'` 是“两个连续反斜杠”换一个斜杠**（不是单反斜杠），而 `make_bg_layer`
  里的 `/\\/g` 才是单反斜杠 ⇒ 两处规则不同，各写各的。
- `shadow?.replace(/.bmp$/, ".png")` 的 `.` 是**正则任意字符**（未转义）⇒ 匹配“任意字符 +
  `bmp` 结尾”，且要求至少 4 个字符（所以 `"abmp"` 也会被换成 `".png"`）。
- `layer` 解析：`block_str.trim().split(/\n|\r/g).filter(v => v).map(v => v.trim())` ⇒
  **先 trim 整块**（块首空行因此看不见），`filter` 只丢**空串**，只有前两段有意义
  （`file` / `remains`）。
- `layer.z = ret.layers.length - blocks.length` 是**长度差值**（第一层是 `-n`），不是下标。
- `fields.rect` 为真值时才走颜色分支：`file = undefined`、`absolute = 1`、
  `color = bg_color_translate(rect)`；否则 `file` 走 `/.bmp$/` + 单反斜杠替换。
- `bg_color_translate`：先 `switch ('' + rect)`（11 个固定映射，含字符串 `"40179b"`），
  再 `is_str(rect)` 原样返回，否则 `js_to_int32` 后做位运算；`r/g/b` 的 `+7` 条件是
  `> 64 || === 0`，绿色额外的 `+4` 条件是 `((rect >> 5) & 1) && g > 80`（两个都要）。
- `layer.x/w/h` 用 `typeof === 'number'`、`width/height` 用 `??`、`cc/c1/c2` 用 `typeof`
  且 `*2` / `*2+1`，`loop` 是 `?? undefined`（键会被写进去，随后 `delete_undefined` 删掉）。
- `ret.base.name` 的 `_` → 空格；`out.id` 用 `datIndex.id ?? fields.name`（nullish）。

**验证**：subject `bg_data`（`/all` 11 行）+ 22 条变异全杀。

### 4.28 V28 `post_process_obj_data`

形态：加在 `native/lfw/dat_translator/entity_data.{h,cpp}`（它是装配层的**入口**）。

**移植要点**

- `if (ctx.data) make_frames_special(ctx.data)` 两处都用**真值**判断 ⇒ `data` 为 `null`/`0`/`""`
  都跳过。
- `ctx.index.groups` 也是**真值**判断 ⇒ `[]`（空数组）**是**真值，会把 `group` 设成 `[]`；
  而 `null` / `false` / `""` 会跳过 ⇒ 用例必须同时盖住这两种。
- `ctx.data.base.group = …` 是**原地改共享对象** ⇒ C++ 里用 `Value` 拷贝（共享 `shared_ptr`）
  再 `as_object` 取可变指针；注意 `as_object` 有 const / 非 const 两个重载，**局部变量别声明成
  `const Value`**（否则拿到 `const Object*`，编译报“无法从 const Object* 转换”）。

**验证**：subject `entity_data`（`/all` 29 行：22 + 5 个 `ppo` + 2 个假值 `groups`）+
累计 16 条变异全杀。

### 4.29 V29 `EditBdy` + `cook_ball_bdy_get_hit_to_frame_20/30`

形态：`native/lfw/dat_translator/ball_bdy.{h,cpp}`。

**移植要点**

- `EditBdy.clone` 用 **JSON 往返**（`JSON.parse(JSON.stringify(bdy))`）做深拷贝 ⇒ C++ 用
  `json_stringify` + `json_parse`。注意 **`undefined` 值的键会被丢掉**，这与 `deco` 的
  delete 分支配合才有意义。
- `deco()`：`hit_flag !== void 0` / `kind !== void 0` 是**严格 undefined 判断**
  （`null` 走**设置**分支 ⇒ 会调 `get_hit_flag_name(null)`）；两个 else 分支都要
  `delete` 键 *和* `*_name`（`remove` 对不存在的键是 no-op）；最后 `reorder_fields(raw, …)`。
- `kind_name` 用 **`bdy_kind_name`**（falsy 版，`0` → `unknown_Normal`），不是
  `bdy_kind_full_name`（`set_bdy_kind` 用的才是后者）⇒ 两个 API 别混。
- `_20`：`ctx.data.id == OID.FreezeColumn` 是**宽松**比较（数字 `212` 也命中），
  而 `ctx.data.id === OID.FreezeBall` 是**严格**的 ⇒ 同一个函数里两种并存，用例要都盖。
- `_30`：`one_of` 三元素列表 + 两级 `add/or_` 嵌套（同队相向那一支）；`hit_flag: AllBoth`
  由编辑字段**覆盖**原值。
- **前提条件**：`ctx.data.id` 是无条件访问 ⇒ `data` 缺失时 TS **抛错**，而 C++ 静默按
  `undefined` 处理 ⇒ 这种输入不进差分（有意保留的差异）。

**验证**：subject `ball_bdy`（`/all` 11 行）+ 14 条变异全杀。

---

## 5. 风险

| 风险 | 说明 |
|---|---|
| `ToString(number)` | 最短往返 + 阈值，**最容易做错的单点** |
| 宽松 `==` 的对象分支 | `ToPrimitive` 的 hint 顺序（`valueOf` → `toString`）。LFW 里对象应该走不到，但**不能假设**，要么证明、要么实现 |
| 环引用 | A2 的 `shared_ptr` 不回收环，要实测 LFW 数据树无环 |
| `-0` | `-0 == 0` 真、`Object.is(-0,0)` 假、`String(-0)` 是 `"0"` —— 三处行为不同 |
| 性能 | `Expression.run` 每帧跑，要量 |
| Unicode 空白 | `string_to_number` 的 trim 集合必须精确，差一个字符就是 NaN vs 数字 |

---

### 4.30 V30 `cook_ball_frame_state_15/3000/3001/3005/3006`

- 文件：`native/lfw/dat_translator/ball_frame_state.{h,cpp}`（另含匿名命名空间的
  `cook_shrink_and_bounce`、`ensure_field_array`、`make_bdy_ctx`、`field_or_any`、
  `as_array_mut`/`as_object_mut`）。
- `frame.bdy ? frame.bdy : (frame.bdy = [])` 是**真值**判断且**就地写回**只空数组，
  只在 `_15`/`_3000`/`_3001` 用（`_3005`/`_3006` 无此创建路径）⇒ `ensure_field_array`。
- 比较宽严**逐处不同**，照抄不统一：
  - `bdy.kind != BdyKind.Normal` 宽松（字符串 `"0"` 也算 Normal）；
  - `ctx.data.id == OID.FreezeColumn` 宽松（`_15`/`_3000`，数字 212 也命中）；
  - `e.id === OID.FreezeBall` 严格（`_3001`，数字 209 不命中）；
  - itr 侧是 `switch (itr.kind) { case ItrKind.Normal }`（= `_3000`/`_3001`/`_3005`/`_3006`）
    与 `itr.kind === ItrKind.Normal`（`_3006`）⇒ 一律**严格**。
- `{...ctx, bdy, index: i}` 是**浅拷贝**：被调函数改的是同一个 bdy 对象；
  `index: -1` 表示”新加的反弹 bdy“⇒ `make_bdy_ctx` 先拷贝整个 ctx 再 `set(bdy/index)`。
- `_3006` 两个分支的 `ensure` 语义相反：special 分支是 `ensure([], 4 个动作)` ⇒
  **恒新建**（丢弃已有 `actions`）；普通分支是 `ensure(bdy.actions, 1 个动作)` ⇒ 追加。
- `_3005` 的 bdy 是 `bdy.actions = bdy.actions || []; bdy.actions.push(...)`（等于追加）；
  外层 `if (frame.bdy)` 是真值门，**不**新建；itr 用 `foreach`。
- `_3006` 的 bdy 循环用 `foreach` 且**没有 kind 检查**（每个 bdy 都写）。
- C++ 实现注意：
  - `const` 局部 `Value` 上调用 `as_object`/`as_array` 只会拿到 `const T*` ⇒
    本文件用**不同名**的 `as_object_mut`/`as_array_mut`（内部 `const_cast`）绕开 ADL 二义性；
  - 局部变量**不要取名 `n`**：会遮蔽 `value_builder.h` 的 `n()`（爆 C2064 “项不是函数”）；
  - `edit_bdy_edit` 对 `Value` 是引用传递，但 bdy 对象是共享 `shared_ptr` ⇒ 原地改即传播；
    需要**替换**数组元素时（`_15`/`_3000` 的 `bdy_list[i] = ...`）要 `arr->at(i) = ...`。

