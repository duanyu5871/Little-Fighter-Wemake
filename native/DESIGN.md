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

---

### 4.31 V31 `hit_next_frame_*`（8 个）+ `utils/container_help/assign`

- 文件：`native/lfw/dat_translator/hit_next_frame.{h,cpp}`（`drink` / `super_punch` / `punch` /
  `turn_back` / `jump` / `defend` / `weapon_atk` / `jump_atk`）、
  `native/lfw/utils/container_help/assign.h`（header-only，与同级 `foreach.h`/`ensure.h` 一致）。
- **键插入序逐个对象不同，必须照抄**（差分按插入序打印）：
  - `drink`：`id, desc, mp_mode, expression`
  - `super_punch`：`id, mp_mode, facing, desc, expression`
  - `punch`：`mp_mode, id, facing, desc`
  - `jump` / `defend`：`id, facing, mp_mode`
  - `weapon_atk`：两支都是 `mp_mode, id, facing, expression`
  - `jump_atk`：前两支 `id, facing, desc, mp_mode, expression`；第三支只有 `id, desc, facing`
- `id` 允许是**字符串或字符串数组**（`punch` 的 `["60","65"]`、`weapon_atk` 的 `["20","25"]`）。
- **`assign(output, item)` = `Object.assign(output || {}, item)`**：`output` falsy 时新建对象，
  否则**就地合并并返回同一个对象**（`turn_back` 就是靠这个改 `frame.key_down`）。
  C++ 版只实现**对象源**：LFW 里 `assign` 的真实调用点只有 `hit_next_frame_turn_back` 两处，
  都是对象字面量；JS 对 Array/String 源会拷下标键，本项目不出现，未实现（已记录，不是隐性差异）。
- `hit_next_frame_turn_back(frame, back_frame?)`：
  - `back_frame == void 0` 是**宽松**比较 ⇒ 显式传 `null` 也走 `facing = Ctrl` 分支（用例 `f2` 钉住）；
  - 否则写入 `{ B: { id: <原值>, wait: "i", facing: Backward } }` —— **外层 `B` 键容易漏**
    （本轮唯一的漂移就是它）；`id` 放的是**原值**，不转字符串（数字 5 就写数字，用例 `f7`）；
  - `key_down` 与 `hit` 的赋值**先后有序**（影响键插入序，变异可杀）。
- `CondMaker` 用法：`one_of(key, {v...})`、`add(key, op, v)`、`or_/and_(lambda)`（lambda 返回 `CondMaker*`）。

---

### 4.32 V32 `parase_indexes` + `match_hash_end`（+ `utils/string_help.h`）

- 文件：`native/lfw/dat_translator/parase_indexes.{h,cpp}`（TS 原文件名就是 `parase_indexes.ts`，拼写照抄）、
  `match_hash_end` 落在 `dat_translator/string_matchers.{h,cpp}`、
  `js_trim` / `split_lines` / `replace_all` 落在新建的 `native/lfw/utils/string_help.h`。
- **throw → 结果结构**：TS 抛异常的三处（text 为空 / `[NOT_READY]` 残留 / stage 的 file 后缀不支持）
  在 C++ 里用 `ParseIndexesResult{ok, error, lists}` 返回；错误文本逐字照抄，`lists` 保持 undefined。
- `match_block_once` 三次取 `<object>` / `<background>` / `<stage>` 块；**块缺失 ⇒ 空数组**。
- 每个块内：`split(/\n|\r/)` → **只丢空串**（`filter(v=>v)`）→ 逐行 `trim()` →
  以 `#` 开头的行跳过 → **其余行都会产出一条 item**（包括 trim 后变空的空白行）。
- item 初始键序固定 `id, type, file, src`；`file` 是后缀替换后的名字，`src` 是仅做
  反斜杠→斜杠替换的原值；`groups` 用 `,` 切分（空值 ⇒ `[""]`）。
- **`/.dat$/` 的 `.` 是通配**（需 ≥4 字符且第 4 个字符不能是行终止符）⇒ `"a.ddat"` → `"a..obj.json5"`；
  而 stages 的 `.dat`/`.txt` 判定用 `endsWith`（**字面**比较）⇒ 两者语义不同，不能合并。
- `id`/`alias` 同时非空时**互换**（键位置不变，只换值）；随后 4 个硬编码 id **覆盖** `hash`
  （顺序：先 `match_hash_end` 再硬编码 ⇒ 硬编码胜出）。
- `match_hash_end` = `/#(.*)[\n|\r|\0]*/` 的第一个捕获组：取**第一个** `#` 之后的字符，到第一个
  行终止符（`\n`/`\r`/`\u2028`/`\u2029`）或串尾为止；无 `#` ⇒ undefined；`#` 后为空 ⇒ `""`。
  字符类里的 `|` 是**字面竖线**，不是或。
- 重构：`cond_maker.cpp` 的 `js_trim`、`bg_data.cpp` 的 `replace_all`/`split_lines`/`trim_str`
  原本各一份（`trim_str` 与 `js_trim` 逐字相同）⇒ 统一到 `utils/string_help.h`（两处差分重跑仍绿）。
  ⚠️ 提升时删掉了 `parase_indexes.cpp` 里自己写的 `is_non_empty_str`（与 `utils/type_check.h` 同名冲突）。

---

### 4.33 V33 `cook_frames`（+ `take_sections`）

- 文件：`native/lfw/dat_translator/cook_frames.{h,cpp}`；`take_sections` 落在 `string_matchers.{h,cpp}`
  （`take_blocks` + 每块 `match_colon_value` + `to_num(v) ?? v`）。
- **`<frame>` 正则是手写模拟的**，原文 `/<frame>\s+(.*?)\s+(.*)((.|\n)+?)<frame_end>/g`：
  - 第 1 个 `\s+` 贪婪吃满空白（至少 1 个）；
  - `(.*?)` 因为 `\s+`(2nd) 必须先失败再回退 ⇒ **id = 第一个词**（不跨行）；
  - `(.*)` 不跨行 + `((.|\n)+?)` 懒惰 + 字面 `<frame_end>` ⇒
    `name_len = min(到行尾的长度, e - m - 1)`、content **至少 1 字符**，`e` 取**第一个** `<frame_end>`；
  - 匹配失败要从 `p0 + 1` 继续扫描；成功则从 `e + 10` 继续（非重叠）。
- **`ctx.base` 必须存在**（TS 的 `{ text, base: { files = {} } }` 解构默认值救不了 `base` 本身为 undefined）；
  `files` 缺失 ⇒ **空对象**（不是 undefined，否则 `__ERROR__.files` 就会漂移）。
- `wait = Number(fields.wait) * 2 + 2` ⇒ 缺失时是 **NaN**（照抄，不要换成 0）。
- 隐身区间用 `raw_next` 的**宽松数值**比较（字符串 `"1200"` 也命中）；而 `vy === 550` 是**严格**。
- 键插入序：`id, name, pic, wait, next, width, height` + `...fields`（覆盖同名但**保位**），
  然后按 `itr, bdy, opoint, wpoint, cpoint, bpoint` 顺序补。`frames[frame_id]` 用 `Object::set`
  （整数键升序，与 JS 一致）。
- `delete` 分两类：`.length` 类（itr/bdy/opoint）判“数组且非空”，真值类（wpoint/bpoint/cpoint）判 truthy。
- `cook_dvxyz`：`not_zero_num(v)` 才处理；`v === 550` ⇒ `[Fixed, 0, None]`；否则 `[undefined, round_float(v)]`。
- **有意差异**：`frame.hit = frame.hit || {}` 之后 TS 会给它赋属性 —— 若 `hit` 是 truthy 非对象
  （dat 里 `hit:` 后面接 `id:` 会被 `match_colon_value` 解析成字符串 `"id:"`）TS 会抛 TypeError，
  C++ 选择跳过（这类输入不写进用例）。

---

### 4.34 V34 `make_ball_special`

- 文件：`native/lfw/dat_translator/make_ball_special.{h,cpp}`；`find_value_index` 加到
  `native/lfw/utils/container_help/find.h`（TS 的 `find` 对数组返回**元素**，C++ 返回**下标**再取）。
- `switch (data.id)` 是**字符串严格**匹配；`data.id` 缺失或非字符串则直接返回。11 组分支：
  1. `FirenFlame`(211)：逐 frame 逐个 `itr` → `set_hit_flag(itr, AllEnemy)`；
  2. `FirzenBall`(223) / `BatBall`(224)：逐 frame 写 `no_shadow=1`、`dvz=0`、`vzm=Fixed`；
  3. `JanChase`(220)：`base.drop_sounds = base.hit_sounds`；`frames[50/51/52].invisible = .wait`；
     对 `behavior === ChasingSameEnemy(7)` 的 frame 插入 opoint（`x: centerx`、`y: centery`）；
     再改 `opoint` 里第一个 `oid === JanChase && action.id === "40"` 的项；
  4. `JanChaseh`(219)：`frames[50/51/52]` 的 `invulnerable ??= invisible ??= wait`（**nullish**，不是 falsy）；
     只改 tail（**不**插入 opoint）；
  5. `FirzenChasef`(221)：同 3，但 frames 是 `59/80/81`，opoint 坐标用 `floor(pic.w/2)`/`floor(pic.h/2)`；
  6. `FirzenChasei`(222)：只有 drop_sounds + 插入 opoint（**没有** tail 处理）；
  7. `DeepBall`/`DennisBall`/`WoodyBall`/`DavisBall`/`DennisChase`/`JackBall`/`JohnBall`：
     `base.group = ensure(base.group, FreezableBall)`；
  8. `JohnBiscuit`：无操作；`FreezeBall`(209)：`group = ensure(..., Freezer)`；
  9. `BatChase`(225)：`behavior === Bat(12)` 的帧逐个 `itr` →
     `actions = ensure(actions, {type: VALUE_STEAL, data:{target:1, itr_hp_ratio:0.2, itr_hp_r_ratio:0.2}})`；
  10. 其余一大串 case 无操作。
- tail 的字段写入顺序（链式赋值从右往左）：`unimportant` → `dvz, dvy, dvx, speedz` → `ghost`。
- **有意差异**：TS 里 `data.base.hit_sounds`、`data.frames['50']` 缺失会抛错（前提条件），C++ 跳过。
- 教训：**不要为“好看”把三组 chase 逻辑抽成带 `with_opoint` 开关的共享函数** ——
  `JanChase` 用 `centerx/centery` 而 `FirzenChase*` 用 `pic.w/2`，抽共享会掩盖真实差异
  （本轮一度这么写，已撤回；抽出但逐字相同的只保留 `pic_half`/`tail_index`/`edit_tail`）。

---

### 4.35 V35 `make_weapon_special` + `broken_piece_frames`

- 文件：`native/lfw/dat_translator/make_weapon_special.{h,cpp}`、
  `native/lfw/dat_translator/broken_piece_frames.{h,cpp}`。
- `broken_piece_frames` 是 **27 个模块级数组常量**（TS 里可被就地改写）⇒ C++ 用
  `static const Value v = ...` 惰性构造并**返回同一实例**（每次返回 `shared_ptr` 副本）。
  `range(a, b)` 是**闭区间**：`range(0, 3)` ⇒ `["0","1","2","3"]`。
- `make_weapon_special`：
  - `num_data_id = Number(data.id)`；`100..199` ⇒ `group = ensure(group, [VsWeapon, StageWeapon])`；
  - `switch (data.base.type)`：`Heavy`→`w_atk_r_x ??= 200`、`Knife`→`70`、`Stick`→`100`、
    `Baseball`/`Drink`→`w_atk_m_x ??= 100` + `w_atk_r_x ??= 200`（`w_atk_m_x ??= -1` 只在前三个）——
    全部是 `??=`（**nullish**，不是 falsy）；
  - `switch (data.id)`：`HenryArrow1`（weight=ARROW、删 group、非 Rebounding 的帧给 itr 加动作）、
    `RudolfWeapon`（weight=ARROW、删 group、删 Rebounding 的 itr）、
    `Weapon_Stick`/`_Hoe`/`_Knife`/`_baseball`/`_milk`/`_Stone`/`_WoodenBox`/`_Beer`/
    `_Boomerang`/`_LouisArmourA`/`_LouisArmourB`/`_IceSword`（weight + brokens，其中 A 组是**直接覆盖**、
    其余 `??=`；milk/Beer 额外写 `group` 与 `drink`）。
  - `broken_pieces_opoints(frame_ids)` 的键序：`kind,x,y,pos_type,action,oid,unimportant,
    inherit_speed_x/y/z`，再把 `aa[idx % 10]` 的 `dvy`/`dvx` **追加上去**（`aa` 有 10 个模板，其中 3 个只有 `dvy`）。
  - `handled` 是**模块级 `Set`**（按**对象身份**去重）；TS 里重复会 `throw`，C++ 用 `continue`
    跳过（**有意差异**，正常数据不会触发）。
- **前提条件**：`data.base` 必须存在（TS 无条件访问 `data.base.group` / `data.base.type`）
  ⇒ **所有**样本都要给 `base`，否则 TS 直接抛错。
- **C++ 坑（本轮差分抓到的真 bug）**：`as_object(make_aa(idx))` 把**临时** `Value` 传进去，
  临时析构后 `Object` 被释放（`make_obj` 用 `make_shared`，引用计数归零）⇒ 悬垂指针，
  表现为“`aa` 展开的那整段字段全不见了”（不是崩溃）。⇒ 取指针前必须把临时存进局部变量。

---

### 4.36 V36 `make_stage_info_list`

- 文件：`native/lfw/dat_translator/make_stage_info_list.{h,cpp}`。
  `stage_info_new()` / `stage_phase_info_new()` 在 TS 里都只是 `return {}`（无字段）⇒
  C++ 直接建空对象，**不需要**扩展 `gen_defines_runtime.mjs`。
- 前置加工两步：`replace(/\\\\/g, "/")`（**两个**连续反斜杠 → `/`，手写而非 `replace_all`），
  再 `replace(/<phase_end>[\n|\s|\r]*<stage>/g, "<phase_end><stage_end><stage>")`
  （字符类里的 `|` 是**字面竖线**；贪婪吃空白，所以 `<phase_end>` 与 `<stage>` 之间可隔任意空白）。
- 每个 `<stage>` 块：先建空 `phases` 数组并 `stage_info.phases = phases`（**同一个数组**），
  再用 `take_blocks` 抽 `<phase>`；每个 phase 逐行（**只按 `\n` 切**，再逐行 `trim`）处理三类行：
  `bound`（顺带 `music`，最后写 `desc = match_hash_end(line)?.trim() ?? ""`）、
  `music`（只认 `music`）、`id`（建 object：`{id: [value], x: phase_info.bound}`，
  `<soldier>`/`<boss>` 标记，`id`/`act` 走专用分支，其余键 `to_num(value) ?? 旧值`，
  **每个键之后**都重算 `facing = (x && x < 0) ? 1 : -1`；`is_soldier && !times` ⇒ `times = 50`；
  `phase_info.objects ??= []` 后 push）。
- `head = stage_str.replace(/\s+\n+/g, "\n").trim()` 后 `match_colon_value` 的键值**一律当字符串**赋给 stage；
  `name = (match_hash_end(head) ?? id)?.replace(/stage/gi, "").trim()`（`gi` 忽略大小写）。
- `nid % 10 === 0` ⇒ `is_starting` + `starting_name = "" + (1 + nid / 10)`。
- 每 phase：`enemy_r = (bound ?? 0) + 300`、`enemy_l = -300`、`on_end = [EnterNextPhase]`；
  最后一段改 `[LoopGoGoGoRight]`，`i > 0` 的段加 `on_start = [GoGoGoRight]`。
- **链式赋值 `p.health_up = p.respawn = {...}` 从右往左** ⇒ 键插入序是 `respawn` 再 `health_up`
  （差分会直接抓到，和 `frame_behavior` 那轮的教训同源）。
- `nid === 50` 两段（每 phase 的 `respawn*`；然后整体改成 Survival + 每 phase 的 `drink_l/drink_r/title/on_start=undefined/on_end`）；
  `nid <= 9/19/29/39/49` 五段 chapter 分段。
- 收尾三循环：第一段的 `cam_jump_to_x/player_jump_to_x/player_facing`；
  `stable_sort`（JS `Array.sort` 自 ES2019 是**稳定**的 ⇒ 必须用 `std::stable_sort`）；
  `next` 不在列表里则按区间改写；`is_stage_end` 的 stage 把 `last_phase.on_end` 置 undefined。


---

### 4.37 V37 `cook_file_variants` + `FrameEditing`

两个 `make_fighter_data` 的前置单元，同轮完成。

**`cook_file_variants`**（`native/lfw/dat_translator/cook_file_variants.{h,cpp}`）

- `strip_suffix` 是 `/\.[^.]*$/` 的等价物 ⇒ 找**最后一个** `.` 并把其后全部丢掉；
  没有 `.` 时**整串保留**（不是丢弃）。
- `letter_of(offset) = char16(98 + offset)` ⇒ 0 是 `b`。
- 取 `ret.base.files` 的键（`Object.keys` 顺序），**必须按字典序 `std::sort`**：
  因为 `infos[0]` 与后面的 `findIndex` 下标都依赖顺序，顺序错了整段结果落在**别的文件**上。
- 键为空或**奇数**个 ⇒ 直接返回。
- 基准串 `first_str = strip_suffix(infos[0].path)`；然后 16 个字母逐个
  `findIndex(path_of === first_str + letter)`，`found < 1` 即 break ⇒ 找不到任何变体时
  `indexes = [0]`，后面的 gap 归约为 0 ⇒ `truthy(0)` 为假 ⇒ 直接返回（不写 `variants`）。
- 间隔计算是 `reduce` 语义：`idx === 0` 用当前项（`n`），之后 `gap_v = (to_number(gap_v) == diff) ? gap_v : null`，
  其中 `diff = idx - indexes[idx-1]`。**一旦不一致就变成 `null` 并保持**（`null` 是假值）。
- `is_match` 逐项检查：`path_of(infos[j]) == path_of(infos[i]) + letter_of(idx - 1)`
  且 `col/row/cell_w/cell_h` 与模板 **严格**相等（`same_field` 是 `===`）。
- 通过后 `infos[i].variants = indexes.slice(1).map(j => infos[j + i].id)` ⇒
  直接**就地改写共享的 `files` 对象**（不是拷贝）。

**`FrameEditing`**（`native/lfw/dat_translator/frame_editing.{h,cpp}`）

- 结构：`frame`（被编辑的帧对象）+ `costs`（`Map<string, {mp, hp}>`，可为 `nullptr`）。
- `cook_one`：`is_str || is_num` ⇒ `get_next_frame_by_raw_id(to_string(v), "frame", "hit", costs)`；
  否则**浅拷贝**后 `cook_next_frame_cost(out, "hit", costs)`。注意 `zero_as` 是 `"frame"`：
  只有 `id` 为 `"0"` 时才看得出来（此时输出 `{id:"0"}`，写成 `"repeat"` 就变成 `{}`）。
- `keydown` / `hit` 逻辑完全相同，只有目标键不同（`key_down` / `hit`）：
  先 `??= {}` 保证容器存在，再对每个键 `add_next_frame(已有值, cooks)`。
  `key` 既可以是字符串也可以是**字符串数组**（`keys_of` 只取其中的字符串项）。
- `seq`：
  1. `seqs ??= {}`；遍历 `nexts`，**先跳过所有 falsy**（注意 `0` 与 `""` 也是 falsy，
     所以这里不能只靠 `is_str || is_num` 判分支 —— 少了这句 `n 0` 会被 cook 出来）；
     `is_str || is_num` ⇒ `get_next_frame_by_raw_id(...)`；对象 ⇒ 浅拷贝 + `cook_next_frame_cost`；
     其余（`null`/`undefined`/`false`）不产生任何元素。
  2. `key[0] === 'F'` ⇒ 写 `"L" + key.slice(1)` 与 `"R" + key.slice(1)` 两个键：
     每个 cooked 项各生成一份，`facing` 用**宽松**比较判定是否 Backward（所以字符串 `"2"`
     也算 B），B 的写 `L`，非 B 的写 `R`（两个数组的 `facing` 互为反向）。
  3. 否则写 `seqs[key]`（同样 `add_next_frame(已有值, cookeds)`）。
- 两处都必须调用 `add_next_frame` 并传入**旧值**，否则同一键第二次调用会丢掉第一批。


---

### 4.38 V38 `make_fighter_data`（594 行 TS）

- 文件：`native/lfw/dat_translator/make_fighter_data.{h,cpp}`；配套新增
  `native/lfw/dat_translator/bots_frames.{h,cpp}`（`fids.defends`，其余 `bots/frames` 字段等
  `make_bot_data_*` 落地时再补）与 `helpers` 里的 `take_number`。
- **`take_number(v, k, or)` 的判定必须是 `typeof v[k] === "number"` 而不是"有限数字"**：
  JS 里 `NaN` 也是 `"number"` ⇒ C++ 用 `std::holds_alternative<double>`，**不要**用 `is_num`。
  它无条件 `delete v[k]`（无论是否命中）。
- `switch (Number(frame.id))` **不能**直译成 C++ `switch`（不支持 `double`）：用 `to_number` +
  `if/else` 链比较 double，语义与 JS `case` 的 `===` 一致（`NaN` 不等于任何 case）。
  `Number("")` 是 0 ⇒ 空 id 也会落进 `case 0`，这一点被链式比较天然保留。
- `frame_mp_hp_map` 在 C++ 里用 `Object` 表示（键 = frame id，值 = `{mp, hp}`），由
  `traversal(frames, …)` + `take_raw_frame_mp` 填；注意 `take_raw_frame_mp` 会**先从帧里删掉 `mp`**，
  所以 `infos` 里的 `mp/hp` 与帧上残留的 `mp` 无关。
- `k9 = ["Fa","Fj","Da","Dj","Ua","Uj","ja"]`：`take` 无条件删键；随后
  `if (!is_str && !is_num) return; if (next === "0" || next === 0) return;`（**严格**相等，
  字符串 `"0"` 与数字 `0` 都要挡）。
- `switch (frame.state)` 的两处（回头 / cook）以及 `frame.state` 可能被前面的 case 改过
  （`200` → Frozen、`220~225` → Injured、`226~229` → Tired、`213/216` 的 Jump → Dash），
  顺序敏感，必须照抄。
- **`frame.ctrl_x = frame.ctrl_z = 1` 是链式赋值 ⇒ 右到左 ⇒ `ctrl_z` 先插入**（`case 12~15`）。
  `case 5~8` 里写的是两条独立语句（`ctrl_z` 然后 `ctrl_x`），顺序不同，别"统一"。
- `traversal` 的对象路径**先快照键**再逐个取值 ⇒ 遍历中 `delete frames[frame_id]` 安全（与 JS 一致）。
- `round_trip_frames_map[frame.name]` 用 `to_string(undefined)` = `"undefined"` 作键
  （JS 里 `map[undefined]` 就是这个键）；随后 `for (const key in ...)` 只遍历**已收集的键**，
  而 `make_round_trip_frames` 只往 `frames` 里加、不往 map 里加，所以互不干扰。
- `make_round_trip_frames`：`i < src_frames.length` 时**直接用源帧对象**（会改它的 `id`/`next`），
  否则 `{...src_frames[2*(len-1)-i]}` 浅拷贝；`next.id` 的回环点是 `2*len-3`。
  ⚠️ Walking/Running 的帧在循环里被 `delete frames[id]` 了，但**对象仍被 map 持有**，
  并被这里重新写回 `frames` ⇒ 这些帧上写过的 `dvx/dvz/wait/ctrl_*` 依然可观察。
- `indexes` 常量：`{-1:…, 1:…}` 里的 `-1` **不是**数组索引 ⇒ 是普通字符串键（排在整数键 `1` 之后），
  与 `Object::set(u"-1")` 的表现一致。
- `ret` 键序固定 `id, type, base, indexes, frames, processed`；`base` 与 `frames` 都是**共享**的对象
  （不是拷贝），`cook_transform_begin_expression_to_hit` 之后才 `cook_file_variants`，
  最后 `datIndex.bot` 真值才写 `base.bot_id`。
- `cook_transform_begin_expression_to_hit`：先扫出 `state === TransformToCatching_Begin(500)` 的帧表
  （键是 `frame.id`）；`tframes` 为空直接返回；然后遍历每帧的 `key_down/key_up/hit/seqs` 的**值**
  （值是数组就逐项，否则单值），命中即写 `expression = "has_transform_data==1"`。
- **有意保留的差异**：TS 在 `ctx.index` 缺失时抛 TypeError，C++ 返回空值（与之前几轮同策略）。

---

### 4.39 V39 `obj_dat_to_json`（121 行 TS）

- 文件：`native/lfw/dat_translator/obj_dat_to_json.{h,cpp}`；配套新增
  `native/lfw/utils/container_help/set_obj_field.h`（header-only，与 `ensure.h` 同风格）。
  `post_process_obj_data` 在 `entity_data` 那轮就已经落地，本轮直接复用。
- 入口返回 `ObjDatToJsonResult{ok, error, data}`，把 TS 的 `throw new Error('[dat_to_json] failed, 3')`
  变成 `ok = false`。
- 预处理两步：`replace(/\\\\/g, "/")`（**两个**连续反斜杠 → `/`；`string_help.h` 的 `replace_all`
  只有**单字符**版本，手写）；`match_block_once(text, "<bmp_begin>", "<bmp_end>")`
  （懒惰 + 中间至少一个字符，取**最早**的 `<bmp_end>`）。
- 块内容 `trim()` 后 `split("\n")`，**不过滤空串** ⇒ 空行会一路落到最后一条规则，
  写进 `base[""] = ""`（这是原样行为，别"顺手"过滤掉）。
- 每行按**固定顺序**依次尝试 8 条规则，第一条命中即 `continue`：
  1. `/name:\s*(\S*)/`、2. `/head:…/`、3. `/small:…/`（键内无空白 ⇒ 直接 `find("name:")` 再跳空白取 `\S*`）
  4. `startsWith("file(")`
  5. `/(\S*)\s*:\s*([+-]?([0-9]*[.])?[0-9]+)/`、6. `/(\S*)\s*:\s*(\S*)/`
  7. `/(\S*)\s*([+-]?…)/`、8. `/(\S*)\s*(\S*)/`
- 5~8 都是**最左匹配 + `(\S*)` 贪婪**：手写版逐个起始位置 `p`、每个 `p` 内 `g` 从"连续非空白长度"
  递减到 0（等价于正则回溯），**先命中的起始位置最左、同一位置里 key 最长**。
  注意 `(\S*)` 不可能含空白 ⇒ 对 `**key**:\s*` 这一形式解是**唯一**的。
- `/.bmp$/` 里的 `.` 是**未转义任意字符** ⇒ 长度 ≥4 且以 `bmp` 结尾（`abmp` 会匹配、`bmp` 不会），
  替换成 `.png`；随后 `replace(/\\/g, '/')`（单个反斜杠）。
- `file(...)` 行交给既有的 `match_colon_value`；`file_id` = 当前 `base.files` 的键数；
  先按 `id, path, row, col, cell_w, cell_h` 顺序建字段，随后 colon 里同名的键**覆盖但保持位置**；
  键以 `file` 开头 ⇒ `path`，`w`/`h` ⇒ `cell_w`/`cell_h`，其余 ⇒ `Number(value)`。
- 数字段 `[+-]?([0-9]*[.])?[0-9]+`：`+.5`/`.5` 可以，`5.` 退化成 `5`，单独的 `.` 不匹配；
  `[0-9]` 是 **ASCII**（不是 `\d` 的 Unicode 版）。
- `switch (datIndex.type)` 是**字符串**比较：`"0"` → `make_fighter_data`；`"1","2","4","6"` →
  `make_weapon_data`；`"3"` → `make_ball_data`；其余（含 `"5"`）→ `make_entity_data`。
- 顺序：先 `ctx.frames = cook_frames(ctx)`，再算 `data`，**先写回 `ctx.data`** 才调
  `post_process_obj_data(ctx)`（后者读 `ctx.data` 与 `ctx.index.groups`）。
- **有意保留的差异**：`datIndex` 缺失（`ctx.index` 为 `undefined`）时 TS 抛 TypeError，
  C++ 返回 `ok = false`（与之前几轮同策略）。

---

### 4.40 V40 loader 第一批纯助手（`make_buring_smoke` / `preprocess_pic` / `preprocess_stage`）

- `native/lfw/dat_translator/make_buring_smoke.{h,cpp}`：
  `make_buring_smoke(foo)` 里 `foo === 1` 决定 `gen_x`/`gen_y` 的表达式文本；
  `interval_id` 是 `` `buring_smoke_${foo}` ``；`oid` 用 `OID.BrokenWeapon`（= `"999"`，见 `defines/oid.h`
  的 `oid::kBrokenWeapon`，**不要**走 `defines::find("OID.…")`，注册表里没有这个前缀）。
- `native/lfw/loader/preprocess_pic.{h,cpp}`（合并了 TS 的 `preprocess_pic` / `preprocess_frame_pic` /
  `preprocess_wpoint` 三个文件，都是纯函数）：
  - `preprocess_pic`：`typeof pic.rad === "number"` **优先**，否则 `typeof pic.deg === "number"`；
    两个分支都补 `__cos_r`/`__sin_r`（**用 `rad` 算，deg 分支是先算出 `rad` 再算三角函数**）。
    `typeof x === "number"` 的判定要用 `std::holds_alternative<double>`（`0` 也是数字，不能用 truthy）。
  - `preprocess_frame_pic`：`if (!pic) return pic;` 是**宽松** falsy 判定（`0`/`""`/`null` 都直接返回）。
  - `preprocess_wpoint`：恒等返回。
- `native/lfw/loader/preprocess_stage.{h,cpp}`（合并 `preprocess_stage` / `preprocess_stage_phase`）：
  - `delete_undefined(v)`（在 `dat_translator` 命名空间，**要写全称**）→ `reorder_fields(v, 键表)`。
  - `preprocess_stage` 先对 `v.phases` 数组逐项调 `preprocess_stage_phase`，再做自己的两步。
  - `reorder_fields` 只重排**键表里有的**键，不在表里的键保持原相对顺序（用例里 `junk` 原地不动即是证据）。
- **已知差异（Expression 类）**：TS 会往数据上挂 `Expression`/`ValExpression` 实例字段 ——
  `preprocess_next_frame.__judger`、`preprocess_opoint.__gen_*`、`preprocess_stage_phase.__end_testers`、
  `preprocess_bot_data.judger`、`make_buring_smoke.action.__gen_facing`。`Value` 只有 7 种类型装不下函数，
  而这些字段**只被运行时**（`Entity.ts` 等）读取 ⇒ C++ 侧一律**不生成**，差分时由 TS harness 侧
  删掉同名字段对齐。等步骤 4 移植运行时再补。
- **已知差异（浮点）**：`__cos_r`/`__sin_r` 用 `std::cos`/`std::sin`，与 V8 的 `Math.cos`/`Math.sin`
  在个别角度上会差 **1 ULP**（例如 45° 时 `sin` 的尾位不同）。这是库实现差异，不是移植错误；
  差分用例里避开这类角度（90° 时 `cos`/`sin` 完全一致）。

---

### 4.41 V41 loader 第二批（`preprocess_ball_frame` / `preprocess_bg_data` / `resolve_prefab`）

- `native/lfw/loader/preprocess_ball_frame.{h,cpp}`：`ctx = {data, frame}`；两轮遍历 `frame.itr`：
  第一轮给 `kind === JohnShield` 的 itr 补 `A_NEXT_FRAME`（`data` 用 `frame.on_dead`，**必须先判
  `frame.on_dead` 真值**）、再给"有 hit_sounds 且 kind 不属于 Whirlwind/Freeze/Block/Heal"的 itr 补
  `A_SOUND`（只取 `hit_sounds[0]`）；然后 `gravity_enabled ??= false`；再按 `frame.state` 分派
  `cook_ball_frame_state_3000/3001/3005/3006/15`（严格相等，默认 15）；第二轮给
  `Normal/JohnShield/CharacterThrew/WeaponSwing` 的 itr 补 `A_SOUND`（`path` 用**整个** `hit_sounds`）。
  `ItrKind` 的值：Normal 0 / CharacterThrew 4 / WeaponSwing 5 / Heal 8 / JohnShield 9 / Block 14 /
  Whirlwind 15 / **Freeze 16**。
- `native/lfw/loader/preprocess_bg_data.{h,cpp}`：`jobs`（图片加载）与 `SV.validate + Ditto.warn/error`
  （schema 校验）**都不改数据**，C++ 略过；其余照抄 —— `base`/`dataset`/`layers[*]`/`terrain[*]`
  各自 `reorder_fields + delete_undefined`，`base.height ??= MODERN_SCREEN_HEIGHT`，
  `shadowsize`/`zoom` 解构后用 `typeof === "number" ? v : 0`（**不是 truthy**）写
  `shadow_w/shadow_h`、`zoom_x/zoom_y/zoom_z`，最后 `reorder_fields(data, bg_data_fields)` + `delete_undefined`。
- `native/lfw/loader/resolve_prefab.{h,cpp}`：返回 `ResolvePrefabResult{ok, cycle, chain, value}`，
  `prefab_error` 在 C++ 里只返回**消息字符串**（不构造 `Error`）。
  `ref ?? prefab_id` 是 nullish；`while (ref !== void 0)` 用**严格 undefined** 判停（`null` 会停下）；
  `{...prefab, ...base}` 与 `{...base, ...obj}` 的展开顺序不能反（后者决定谁覆盖谁）。
- **有意保留的差异**：`resolve_prefab` 返回 `ok` 时不带 `chain`（TS 的成功结果里也没有 `chain`）。

---

### 4.42 V42 `cook_frame_indicator_info`（122 行 TS）

- 文件：`native/lfw/dat_translator/cook_frame_indicator_info.{h,cpp}`，纯数据、零外部依赖。
- 尺寸来源：`const w = pic && "w" in pic ? pic.w : frame.width`，**`"w" in pic` 是键存在性判定**
  （不是 truthy）；`h` 那行同样用 `"w" in pic`（原代码笔误，照抄）。`!w || !h` 都是宽松 falsy ⇒ 直接 return。
- `f_qube_1` 键序 `x,y,w,h,z,l`，值 `{-centerx, centery-h, w, h, 0, 0}`；
  `f_qube_2 = {...f_qube_1, x: centerx - f_qube_1.w}`（**覆盖保持位置** ⇒ 键序不变）。
  `frame.__indicator_info = { 1: f1, [-1]: f2 }`（`1` 是整数键排前，`-1` 是普通字符串键排后）。
- 五个子块 `opoint / cpoint / bpoint / wpoint / bdy / itr` 各写一份 `__indicator_info`；
  **它们的 rect 键序不同**：`opoint/cpoint/bpoint/wpoint` 是 `w,h,x,y,z,l`，
  `bdy/itr` 是 `w,h,z,l,x,y` —— 不能合并成一个 helper。
- **`|| 0` 与解构默认 `= 0` 语义不同**：`opoint/bpoint/wpoint` 的 `z` 用 `o.z || 0`
  （任何 falsy 都给 0），`cpoint` 的 `x/y/z` 与 `bdy/itr` 的 `z/l/w/h/x/y` 用解构默认
  （**只在 `undefined` 时**生效，`null`/`""`/`0` 原样保留）。C++ 用两个 helper 分别对应：
  `or_zero_if_falsy` 与 `or_zero_if_undefined`。
- `cpoint` 的 `x/y` 只参与 `add`/`sub` 等数值运算 ⇒ `null` 与 `0` 经 `to_number` 后结果相同
  （见 PROTOCOL §6.9.31 的等价变异说明）。

---

### 4.43 V43 `loader/preprocess_action` / `preprocess_bot_data` / `preprocess_next_frame`

- 文件：`native/lfw/loader/preprocess_action.{h,cpp}`、`preprocess_bot_data.{h,cpp}`、
  `preprocess_next_frame.{h,cpp}`。三个函数都返回 `bool`：`false` 表示"TS 会抛异常"
  （沿用"TS 的 throw ⇒ 用返回值表达"的既有约定）。
- `preprocess_next_frame`：只给 `nf.__judger` 赋一个 `Expression` ⇒ **零数据可见行为**。
  C++ 只保留它唯一可观察的副作用 —— `nf` 是 `null`/`undefined` 时 `typeof nf.expression`
  **会抛**（数字/字符串/布尔读属性只得到 `undefined`，不抛）。数组分支递归，遇错即停。
- `preprocess_action`：
  - `action.tester = …`（`Expression`）**不生成**（TS harness 删掉这个键）；
  - `action.type = action.type.toUpperCase()` 是唯一的纯数据变更。JS 的 `toUpperCase` 走
    Unicode Default Case Conversion，C++ 只做 ASCII（`a`..`z` 减 32）—— 动作类型全是
    ASCII ⇒ 可接受，已记入差异表；
  - `A_SOUND`/`V_SOUND` 分支只往 `jobs` 里塞加载任务（宿主副作用，不改数据）⇒ 不移植；
    但它的**抛出条件**保留：`action.data` nullish 会抛，`action.data.path` 不是字符串/数组
    （含 nullish）也会抛（`for…of` 不可迭代）；
  - 六种 next-frame 类型（`A/V_NEXT_FRAME`、`A/V_DEFEND`、`A/V_BROKEN_DEFEND`）会调用
    `preprocess_next_frame(action.data)`；其余类型**不碰** `data`（所以 `data` 缺了也不抛）。
- `preprocess_bot_data`：
  - `traversal(data.actions, …)` 写 `judger` ⇒ 同 `tester`，不生成；
  - `frames` 与 `states` 各自做"逗号键展开"：`k.split(',')` 长度 >1 时 `delete o[k]`，
    再对每个新键写 `[...v]`。**`traversal` 的键是快照**（`Object.keys(r).map`）⇒ 新写入的键
    不会被本轮再访问，但**同一个键被删掉后重新插入会改变键序**；
  - `[...v]` 的可迭代语义照搬：数组 ⇒ 浅拷贝；**字符串 ⇒ 按码点切**（代理对算一个元素）；
    其他类型 ⇒ **抛**（C++ 返回 `false`，此时原键已被删、新键一个都没建，与 JS 中途抛出的
    状态一致）；
  - `states` 分支的 `("" + k)` 在 C++ 里是恒等（键本来就是字符串）⇒ 与 `frames` 共用 helper；
  - `data` 是 `null`/`undefined` 时 `data.actions` 就抛 ⇒ C++ 也返回 `false`；非对象
    （数字/字符串/数组）时 `data.actions` 只是 `undefined` ⇒ 不抛、原样返回。
- `preprocess_opoint`（26 行）**没有任何数据可见行为**（只写 `__gen_*` 的 `ValExpression`）
  ⇒ 不建文件，与 `make_buring_smoke.action.__gen_facing` 同类处理。
---

### 4.44 V44 `dat_translator/bots` 的动作构建层

- 文件：`native/lfw/dat_translator/bots/{bots_types.h, constants.h, frames.{h,cpp}, bot_actions.{h,cpp}}`。
- **两种可调用类型**（对齐 TS 的 `IEditBotAction` / `IEditBotActionFunc`）：
  `EditBotAction = std::function<Value(Value&, CondMaker&)>`、
  `EditBotActionFunc = std::function<Value(const EditBotAction*)>`。
  返回 `EditBotActionFunc` 的构建器在 C++ 里靠 `apply_edit(fn, ret, cond)` 收口：
  `fn == nullptr` 就直接返回 `ret`，否则调 `fn(ret, cond)` 并返回**它的返回值**
  （edit 返回 `undefined` 时整体就是 `undefined`，与 JS 一致）。
- **JS 默认参数只对 `undefined` 生效** ⇒ C++ 端每个默认值敏感的数值参数都收 `Value`，
  用 `num_or(v, 默认值)` 还原；**不能**直接 `to_number(v)`（那会把缺失参数变成 NaN）。
- `bot_uppercut_dua` / `bot_uppercut_duj` 返回的是 **`IBotAction` 本体**（不是函数），
  `bot_uppercut_dva` 返回的才是函数；`set_actions` 两种都吃。另外 `bot_uppercut_duj`
  **没有** `max_d` 参数（与 `dua`/`dva` 不同），ray 里也没有 `max_d` 键。
- `bot_front_test` 的 `zable` 展开：`if (zable && zable > 0) rays.push({...ray_1, z: -zable},
  {...ray_1, z: zable})` —— **第二个 ray 的 `z` 是原值**（`zable` 是字符串时 `z` 就是字符串），
  只有第一个是数值取负。`{...ray_1, z: …}` 是**覆盖保持位置** ⇒ 键序仍是 `x,z,min_x,max_x`。
- `pow(z_len, 2)`：已用 3.7 / 0.1 / 1e10 / 1e-3 / 1.7 做探针，`std::pow(x, 2.0)` 与
  V8 `Math.pow(x, 2)` 逐位一致 ⇒ `lfw::pow` 可直接用。
- `frames`：`range(0,5,1)` 是**闭区间**（`v > to` 才 break）⇒ `walkings` 有 6 个元素、
  `standings` 4 个、`punchs` 是 60..69 十个。C++ `frames_object()` 每次返回**新建**对象
  （TS 是模块级常量；只读使用，无可观察差异）。
- `bot_ball_cancelling` 的返回对象里**没有** `expression` 键（它的 `cond` 是空的），
  其余构建器都有（值可能是 `undefined`）⇒ 构建对象字面量时不能"顺手补齐"。
- `constants.h` 的 12 个 `DESIRE_RATIO*` 是普通十进制字面量 ⇒ C++ `constexpr double` 逐位相同。

---

### 4.45 V45 `dat_translator/bots` 的 `BotMaker` 与角色数据（前 5 个）

- 文件：`native/lfw/dat_translator/bots/bot_maker.{h,cpp}`、`bots/make_bot_data.{h,cpp}`
  （已实现 `bat` / `hunter` / `jan` / `knight` / `monk`，其余 18 个待补），
  公共 `utils/container_help/spread_assign.h`（从 `loader/resolve_prefab.cpp` 提出来复用）。
- `BotMaker` 的 `_bot` 是 `id, oid, actions` 三键对象；`frames()` / `states()` 取值器
  在字段**缺失或 falsy** 时补一个 `{}`（`if (!frames) frames = bot.frames = {}`）
  ⇒ 即使 `set_frames` 没调用过，读一次取值器也会**多出一个键**（差分里 `mbf hunter` 就是这条）。
- `set_actions` 收 `std::initializer_list<Value>`：TS 的"对象或函数"两种实参在 C++ 里由
  `as_action` 收口 —— `as_action(Value)` 原样返回，`as_action(EditBotActionFunc)` 调 `f(nullptr)`。
  C++ 角色函数因此写成 `as_action(bot_ball_dfa(...))` / `as_action(bot_uppercut_dua(...))`，
  与 TS 的 `set_actions(bot_ball_dfa(...), bot_uppercut_dua(...))` 一一对应。
- `set_frames(frame_ids, ...)` / `set_states(...)` 的键是 `'' + ids`：数组会 `join(',')`；
  **单元素数组会退化成整数键**（`[StateEnum.Catching]` → `"9"`），按 `Object.keys`
  规则排到所有字符串键之前。`states` 的分组键是 `"9"` / `frames` 的是 `"0,1,2,3,walking_0,…"`。
- `set_dataset` 是 `{...bot.dataset, ...dataset}`（已有的键保持位置、新键追加）；
  当前 5 个角色都只调用一次 ⇒ 覆盖顺序暂时不可观察（变异里没写）。
- `register_maker` 对齐 `Map.set`：同 oid **覆盖且保持位置**。TS 的注册发生在模块求值期，
  顺序 = `bots/index.ts` 的 `export *` 顺序（字母序）；C++ 改成显式 `register_all_bots()`
  （**不能**用静态初始化，跨 TU 顺序未定义），由 harness 调一次。
- `check(entity)` 只 `BotMaker.warn` 不改数据 ⇒ 不移植。
- **写角色文件时又抓到一个真 bug**：`bot_ball_dfa` / `bot_ball_dfj` 的 `min_x` 默认值是
  **120**（`bot_front_test` 的是 **0**），原来把 `min_x` 原样透传 ⇒ 缺省时会被下游的 0 覆盖。
  现在在 `bot_ball_dfa`/`dfj` 里先 `num_or(min_x, 120.0)` 再传下去。

---

### 4.46 V46 `dat_translator/bots` 角色数据（第二批 6 个：davis / jack / justin / louis / mark / sorcerer）

- 累计 11/22 个角色（bat / davis / hunter / jack / jan / justin / knight / louis / mark / monk /
  sorcerer），仍在 `bots/make_bot_data.{h,cpp}` 里。
- `bot_actions.h` 新增 4 个命名常量，对应 TS 里挂在**构建器函数对象上的静态属性**
  （`bot_uppercut_dua.MIN_X` 等）：`kUppercutDuaMinX/MaxX`、`kUppercutDvaMinX/MaxX`；
  `bot_actions.cpp` 的默认值改成引用它们，角色文件里就能照样写 `num(kUppercutDuaMinX)`。
- **9 个 edit 回调**（`EditBotAction`）全部落到 C++：`edit_davis_dva_j/dj/dj_a`、
  `edit_louis_dja`、`edit_sorcerer_status`、`edit_mark_rays_max_d/dfj/cancel`。
  两个助手：`ray_with_z`（`{...ray, z}` 覆盖保持位置）、`set_rays_max_d`（对应
  `foreach(a.e_ray, r => r.max_d = …)`，**给不存在的键是追加**）。
- `mark` 的第三个 edit 最麻烦：先就地改 `e_ray[0].reverse`，再 `push({...ray, z: 0.2},
  {...ray, z: -0.2})` —— 展开的是**已经改过 reverse 的那个 ray**，所以两个新 ray 也带
  `reverse`；`Array::push_back` 可能重分配 ⇒ 先把 `rays->at(0)` **拷一份**再 push。
- `davis` 的 `set_frames` 混用两类键：`['87']` / `[282]` / `[39]` ⇒ **整数键**（`39 < 87 < 282`），
  `[...standings,...walkings,...runnings]` / `frames.punchs` / `range(...)` ⇒ 字符串键；
  渲染出来就是"整数键升序在前、字符串键按插入序"。这一条被测到了。
- `sorcerer` 的 `c.and(...).and(...)` ⇒ `cond.and_(v1, op, v2)` **连续调用两次**；
  注意 `bot_idle_action` 只在 `min_mp > 0` 时才 `add`，所以 `and_` 之前 cond 里已有条件。
- `entity_val::kHP_P` / `kHpRecoverable`、`bot_val::kSafe` / `kEnemyY` / `kEnemyOutOfRange` /
  `kEnemyDiffX`、`bot_state_enum::kIdle/kChasing/kAvoiding` —— 名字都在 `native/lfw/defines/` 里核对过。

---

### 4.47 V47 `dat_translator/bots` 角色数据（第三批 6 个：firen / firzen / henry / julian / louisex / woody）

- 累计 **17/22** 个角色；剩 deep / dennis / freeze / john / rudolf。
- `bot_actions.h` 又补了一批"构建器函数对象上的静态属性"：`kBallDfaId` / `kBallDfjId` /
  `kUppercutDvaId` / `kExplosionDua{Id,MinX,MaxX,ZLen}` / `kExplosionDuj{...}`
  （`deep` 会写 `bot_ball_dfj.ID`、`bot_uppercut_dva.ID`，`dennis` 会写 `bot_explosion_dua.MAX_X`）；
  `bot_actions.cpp` 内部全部改成引用这些常量。
- **5 个射线助手**收掉了新出现的所有写法：
  `rays_of`（取 `e_ray` 数组）、`push_ray_with_z`（`e_ray.push({...e_ray[0], z})`）、
  `set_ray0_max_d`（`e_ray[0].max_d = v`）、`set_ray0_z`（`e_ray[0].z = v`）、
  `set_rays_reverse`（`e_ray.forEach(v => v.reverse = true)`）。
  **`push_ray_with_z` 一定先拷 `at(0)` 再 push**（push 可能重分配）。
- `firen` 的 `cancel_d>j` 与 `mark` 的**完全同构** ⇒ 直接复用 `edit_mark_cancel()`（在注释里记明）。
- `julian` 的两个动作是**内联对象字面量**，`expression` 在构建时就算好
  （`new CondMaker().add(...).done()`）⇒ C++ 里先建 `CondMaker c1/c2` 再 `obj({...})`，
  注意 `CondMaker` 不可拷贝，要放在 `set_actions` 之前的同一作用域里。
- `julian` 的 `bot_uppercut_dua(0, void 0)` ⇒ `bot_uppercut_dua(num(0))`（尾参默认值全开）。
- `woody` 的 `d>a` 是"**先就地改** `ray[0].z = 0.1`，再 push 一个 `{...ray, z: -ray.z}`"
  ⇒ C++ 必须 `set_ray0_z(action, 0.1)` 后再 `push_ray_with_z(action, -0.1)`（字面量）。

---

### 4.48 V48 `dat_translator/bots` 角色数据（第四批 5 个：deep / dennis / freeze / john / rudolf）

- **22/22 个角色全部完成**，`bots` 子模块的数据层收口。
- **注册表顺序是真信号，必须由真实模块图定义**：C++ `register_all_bots()` 的顺序 vs TS
  的 `BotMaker.register()` 调用顺序（在**模块求值期**执行）。TS 侧顺序由
  `bots/index.ts` 的 `export *` 顺序决定（`rudolf` 在 `sorcerer` 前）。
  差分 harness 一开始逐文件 `import`，顺序跟着 harness 写序走 ⇒ 只有 `reg` 行报漂移
  （`5,34` vs `34,5`，所有逐 oid 的值都对）。
  **修法：harness 改成从 barrel（`../../../../src/LFW/dat_translator/bots`）导入**，
  让真实求值顺序生效，而不是在 harness 里手摆顺序。
- `make_bot_data.*` 又新增 5 个角色函数 + 20 个 `EditBotAction` 回调 + 5 个射线助手。
  `push_ray_with_z` 的参数是 `Array*`（非 `const`）——`rays_of` 返回 `const_cast` 后的指针，
  `Object::get` 是 `const` 成员因而返回 `const Value*`，写回时必须显式去 const。
- `dennis` 的 `cancel_d>j` 与 `firen` / `mark` **完全同构** ⇒ 继续复用 `edit_mark_cancel()`。
- `dennis` 的 `run_atk` 帧段是 `arr([88, 89])`（整数键）——**整数键在 `Object.keys` 里永远排在
  字符串键之前**，所以"交换整数键与字符串键的插入顺序"是**等价变异**（不可观测），
  写变异时要避开这类组合。
- 变异 130 → **174 条全杀**；差分 64 → **82 行**。

---

### 4.49 V49 `dat_translator/fighters` 角色装配（23 个 `make_fighter_data_*` + `make_fighter_special`）

- `native/lfw/dat_translator/fighters/fighters.{h,cpp}`（23 个角色函数）+
  `make_fighter_special.{h,cpp}`（OID 分发器）；subject `fighters_special`（op `fd`/`fs`）
  **93 行全对**，变异 **133/133 全杀**。
- TS 的分发是 `switch ((data.alias_id ?? data.id) as OID)` ⇒ **严格字符串匹配**：
  `id` 是数字（`n 38`）时区间判断照做、但 switch 不命中。C++ 用 `std::get_if<std::u16string>`
  保住这个语义（有一条例变专门把它放宽成 `to_string(selected)`）。
- `alias_id ?? id` 是 **nullish**：`alias_id: ""` 不回落（用例 `alias_id s ""` 专门锁住）。
- `ensure(data.base.group, X)` 的 C++ 版本必须**先取局部副本再写回**：`ensure` 在非数组分支会
  给 `output` 赋值，只把副本写回对象才会生效。已收口为 `ensure_base_group`（fighters 与
  分发器共用，避免两处写同一条规则）。
- **`hit_flag_pair` 移入 `helpers.{h,cpp}`**：原先 `frame_behavior.cpp` 里有一份局部副本
  （"同一规则写两处"）⇒ 已统一，`cookers` 差分复验仍绿。
- `julian` / `rudolf` 的 `opoint.hp = opoint.max_hp = 20` 是**链式赋值（右到左）**⇒
  新键插入序是 `max_hp, hp, max_mp, mp`；已有键则保位。
- `henter` 的 `data.frames[3].opoint = void 0` 是**写 undefined 而不是删键**
  （键仍在、值为 undefined，`render` 可见）⇒ C++ 用 `Object::set(..., Value())`。
- `rudolf` 的 `frame.seqs = frame.seqs || {}` 是 **falsy** 判定（`z`/`""`/`0` 都新建）；
  `state === Standing/Walking/Defend` 是**严格**数值比较。
- `firen` / `freeze` 的 `[frames["running_0"]..["running_3"]].filter(Boolean)`：
  `!truthy` 那句与紧随的 `as_object == nullptr` 检查**结果重合** ⇒ 对它写变异是等价变异
  （已记录，不写）。而 `running_*` 是**真值非对象**时 TS 会抛（严格模式给原始值加属性），
  C++ 跳过 ⇒ 有意差异，用例避开。
- ⚠️ **`o N` 写小会静默吞掉多余 token**（两侧同源 ⇒ 同时被吞 ⇒ 差分"通过"但是假的）。
  本轮为此踩了 6 次（症状：TS 报 `Cannot read properties of undefined`、C++ 静默返回）。
  **已给 `fd`/`fs` 两个 harness 都加「剩余 token 非空即报错」自检**（此后计数写错立刻硬失败）。
- ⚠️ **`ensure` 对「非数组真值」在 TS 侧抛**（`output.push is not a function`），
  C++ 会转成新数组 ⇒ 有意差异（生产中 `group`/`itr`/`bdy` 只可能是数组或缺失）。
- `fighters/index.ts` 的自动导出列表**不含** `make_fighter_data_template`（TS 生产代码也是
  直接 import 该文件）⇒ harness 必须同样直接 import，barrel 取不到。

---

### 4.50 V50 `dat_translator` 收尾三件（`float_scaling_entity` / `decode_lf2_dat` / `edit_info`）

- `new` `float_scaling_entity.{h,cpp}`（82 行）+ `decode_lf2_dat.{h,cpp}`（14 行）；
  `edit_bdy_info` / `edit_itr_info` 合成 `helpers` 的 `edit_info`（两者函数体**完全相同**，
  都是 `Object.assign(src, ...edit)`）；subject `translator_tail`（op `ei`/`fse`/`dlf2`）
  **63 行全对**，变异 **35/35 全杀**。
- **范围核验**：`edit_itr_info`、`float_scaling_entity` 在 `src/**` 里**零调用点**（仅由
  `dat_translator/index.ts` 再导出）；`edit_bdy_info` 唯一的生产调用点在
  `cook_ball_bdy_get_hit_to_frame_20/30`（我们已移植，原先内联了同一逻辑）；
  `decode_lf2_dat` 由 **`src/pages/dat_viewer/DatViewer.tsx`** 调用（真实功能）。
- **两处"同一规则写两处"已收口**：
  ① `cookers.cpp` 的 `float_scaling_itr` 与新代码的 `scale_field` 是同一条规则
  （`is_num` 判定 + `floor(10000*x)` 写回）⇒ 提升为 `float_scaling_entity.h` 的公开
  `scale_num_field`，`float_scaling_itr` 改为调用它（`cookers` 差分/70 条变异复验仍全绿）；
  ② `ball_bdy.cpp` 的局部 `assign_fields` 就是 `Object.assign` ⇒ 改用 `edit_info`
  （`ball_bdy` 差分/14 条变异复验仍全绿）。
  代价：`cookers.mjs` 里打在 `float_scaling_itr` 函数体上的那条变异**失锚**，已搬到
  `translator_tail.mjs` 并改锚到 `scale_num_field`。
- **`decode_lf2_dat` 的边界是"空操作"**：`if (buf.size() <= 123) return;` 与循环上界
  `i = 123` 的配合意味着越界时循环自然不迭代 ⇒ 该守卫删掉、`<=` 改 `<` 都**不可观察**
  （等价变异，不写）。真正可观察的是 `kPwd` 长度（`sizeof-1` 去掉结尾 NUL）、
  `i % pwd_len`、减号、以及 `uint8_t` 的 mod-256 回绕。
- **TS 用 `String.fromCharCode(...arr)` 展开**：解码结果过长时 V8 会
  `RangeError: Maximum call stack size exceeded`；C++ 无此限制 ⇒ 有意差异，用例保持小尺寸。
- `edit_info` 的支持面：对象源（含 `undefined` 值）与**数组源**（按下标写键，`length` 不可枚举
  因而不复制）；其他原始值源 TS 侧也复制不了自有可枚举属性 ⇒ 忽略。字符串源的索引属性未复制，
  属未达差异（真实调用点只传对象）。
- ⚠️ 又一次踩 `o N` 计数：本轮 5 处（`o 2` 实际只有 1 对）⇒ 症状是 `value literal truncated`
  或"该有的键没有、TS 报 undefined"。**harness 的「剩余 token 即报错」自检必须与新用例同时上线**。

---

### 4.51 V51 步骤 4 第一块 `entity/` 纯助手层

- 新增 `native/lfw/entity/`：`calc_v.{h,cpp}` / `find_frame_direction.{h,cpp}` /
  `face_helper.{h,cpp}` / `entity_type_check.{h,cpp}` / `entity_snapshot.{h,cpp}`；
  subject `entity_helpers`（op `cv`/`fd`/`sf`/`tf`/`tc`/`non`/`tri`/`ftri`/`ns`/`ss`/`nslots`/`sslots`）
  **153 行全对**，变异 **62/62 全杀**。
- **范围核验（本轮主要产出）**：`entity`/`buff`/`collision`/`state`/`controller`/`bot` 六个目录
  **完全不依赖 `three`/DOM**（渲染相关都在 `animation`/`bg`/`ui`/`stage`/`Camera`/`Ground`）。
  但 `entity/Entity.ts`（2717 行 / 29 import）依赖 `Factory`/`Ground`/`World`/`Ditto`/`States`
  ⇒ 「必须整块移植」的判断仍成立，**不能**把 `Entity` 拆成纯函数。本轮只做它周围可独立验证的缝隙。
- **`calc_v` 的 `switch (mode)` 是严格 `===`**：`mode` 为字符串/`null`/小数时**不命中任何 case**，
  落到 `default`。C++ 用 `std::get_if<double>` 取模值，非数字一律走 default；
  用例 `mode s "3"` / `mode z` / `mode n 1.5` 专门锁这条。
  另外 `acc`/`direction` 的默认值只在传 `undefined` 时生效（JS 默认参数语义）⇒ C++ 用
  `holds_alternative<monostate>` 判定，不能用 `is_nullish`（`null` 与 `undefined` 结果不同）。
- **`is_boss` 的 `_bossing` 是松相等**（`v == "Boss"`）：`group` 元素可以是数组，
  而 `["Boss"] == "Boss"` 经 ToPrimitive 后为真 ⇒ 用例 `group a 1 a 1 s "Boss"` 锁住。
- **`NSlot`/`SSlot` 的顺序即语义**（槽位下标）：C++ 用 `enum class` 按 TS 原序声明 +
  `nslot_entries()`/`sslot_entries()`（每项 `static_cast<double>(NSlot::X)`，**不重复字面量**），
  差分把整表 `NAME=value` 打出来对拍 ⇒ 任何重排/漏项立刻可见。
- **`field_or` 三重复制收口**：原先 `dat_translator/value_builder.h` 一份、新写的两个 entity 文件
  各一份 ⇒ 统一到 `utils/container_help/field_or.h`，三处共用（`value_builder.h` 删掉本地副本）。
- ⚠️ **`o N` 又一次在"多层级对象"上写错**：`tc boss o 2 data o 2 ...` 里顶层其实只有 `data`
  一个键（`o 1`），我却写了 `o 2` —— 内层把 `base ...` 一并吃掉，于是顶层少一对。
  **写出嵌套字面量后要按层级各数一遍**。

---

### 4.52 V52 步骤 4 第二块 `controller/` 纯助手

- 新增 `native/lfw/controller/`：`double_click.{h,cpp}` / `seq_keys.{h,cpp}` /
  `key_status.{h,cpp}` / `controller_double_clicks.{h,cpp}`；subject `controller_helpers`
  （op `dc`/`sk`/`ks`/`cdc`，参数序 `<sub> <name> ...`）**102 行全对**，变异 **55/55 全杀**。
- **`KeyStatus` 的 `ctrl.time` / `world.dataset.key_hit_duration` 降为方法参数**
  （`is_hit(time, duration)` / `hit(t, time)` / `end(time)`）：类只是**按需读取**这两个值、
  从不缓存，所以参数化后就不需要 `BaseController`/`World` 了。TS harness 侧搭
  `{ time, world: { dataset: { key_hit_duration } } }` 桩，调用前写入 ⇒ 两侧同语义。
- **`ControllerDoubleClicks` 虽持有 `owner: BaseController` 但从不调用它**（只把按键名字符串
  传给 `DoubleClick`）⇒ 整个类是纯的；那 7 个名字的映射（`L`→`"d"`/`R`→`"a"`/`U`→`"j"`/
  `D`→`"L"`/`d`→`"R"`/`j`→`"U"`/`a`→`"D"`，看着像错位但确实是原样）成了可对拍的常量表。
- `ControllerResult` **本轮未做**：它唯一的逻辑 `fire()` 要调 `owner.entity.get_next_frame(nf)`，
  依赖 `Entity` ⇒ 留到整块移植时一起。
- **`SeqKeys.press` 的匹配是"消耗式多重集匹配"**：`idx` 推进 + `arr.splice(j,1)`；失配清 `idx`/`hit`
  并返回，`idx == len-1` 时置 `hit=1` 且 `idx` 归零。⚠️ **`indexOf` 取首匹配还是末匹配不可观察**
  —— 删掉一个匹配项后多重集相同，后续匹配结果不变（本轮唯一的等价变异，已删）。
- **快照数值字段的前提条件**：`from_snapshot` 的字段必须是声明的类型。TS 不校验，`undefined`
  会流进数值字段（`+` 甚至可能变成字符串拼接）；C++ 用 `to_number` 归一化。
  真实快照恒由 `to_snapshot` 产出 ⇒ 属于不可达差异，用例只用良构快照。
- ⚠️ **常量表要"逐个条目都摸一遍"**：`ControllerDoubleClicks` 的 7 槽映射我只按了 3 个槽
  ⇒ "`slot` 把 `j` 指到 `U`"的变异不可达而存活。与上轮 `is_object_data` 漏掉 Ball 分支同类。

---

### 4.53 V53 步骤 4 第三块 `bot/` 纯助手（含 `helper/manhattan_xz`）

- 新增 `native/lfw/bot/`：`closest.{h,cpp}` / `is_ray_hit.{h,cpp}` / `dummy_enum.{h,cpp}` /
  `nearest_targets.{h,cpp}`，并补上 `native/lfw/helper/manhattan_xz.{h,cpp}`
  （TS `helper/manhattan_xz.ts`，此前一直没实现，本轮 `NearestTargets` 需要它）。
  subject `bot_helpers`（op `de` / `ent` / `dxz` / `ray` / `cl` / `nt`）**201 行全对**，
  变异 **99/99 全杀**。
- **`is_ray_hit` 的前两个提前返回返回的是 `reverse` 本身（不是 `bool`）** ⇒ C++ 必须返回 `Value`。
  `reverse = false` 是**默认参数**，只在 `undefined` 时生效 ⇒ 用例 `min_x n 5`（不给 reverse）
  必须渲染出 `b0` 而不是 `u`；用例 `min_x n 5 reverse n 7` 必须渲染出 `n7:...`。
  `reverse` 的真值判定与"取反命中"是**两步**（`reverse ? !hit : hit`），
  用例 `reverse n 0.5` + `d_sq == 0` 锁"提前返回原值"，`reverse b 1` + `max_d` 使命中为假 才锁"取反"。
- **`closest` 用的是 `round`，不是 `round_float`**（而且把曼哈顿公式内联，没调 `manhattan_xz`），
  与 `manhattan_xz`/`NearestTargets` 的 `round_float` 是**两条规则** ⇒ 故意不合并。
  选值 `3.3335` / `3.3334`：`round` 后都是 3（平手 → 先到者胜），`round_float` 后是 3.334 / 3.333
  ⇒ 一条用例同时锁住"用 `round`"与"严格 `<`（平手保留先到者）"。
- **`NearestTargets.entities` 是 `Set<Entity>`，身份判定用 `strict_equals`**（对象即指针相等）。
  与 `Set` 的 SameValueZero 只在 `NaN` 上不同（`NaN` 不是合法实体）⇒ 记为差异，
  不为此另造一套相等。`targets`/`entities` 都按**插入序**对拍（JS `Map`/`Set` 语义）
  ⇒ C++ 侧**不能**顺手用 `std::map`（按名字排序），必须 `std::vector` + 线性查找。
- **`sort` 的定序要照抄 `Array.prototype.sort`**：`d = a.distance - b.distance`，`d !== 0` 时返回 `d`；
  但 `NaN !== 0` 为真 ⇒ 规范里 `SortCompare` 把 `NaN` 当 `+0` ⇒ C++ 里 `isnan(d)` 必须
  `return false`（等价"平手"，保住稳定序），**不能**把 `NaN` 当成排序键。
  平手用 `a.entity.id < b.entity.id`（JS `<`，混类型走 ToPrimitive）⇒ 用 `lfw::lt/gt`，
  不是字符串比较。稳定序 ⇒ `std::stable_sort`。
- **`look` 的满员分支是"插入 + 挤掉末位 + 截断"三步**：`splice(i,0,x)` → `entities.add(x)` →
  取 `targets[max]` 的 entity 从集合里删掉 → `targets.length = max`。
  C++ 里 `targets_.resize()` 会因为 `BotTarget` 不能默认构造而编译失败 ⇒ 用
  `erase(begin()+max, end())` 截断。
- **`defendable` 原样存储而不是转成 `double`**：TS 里它就是"传进来什么存什么"，
  只有 `undefined` 才吃默认 0 ⇒ C++ 存 `Value`，用 `holds_alternative<monostate>` 判默认。
- **`DummyEnum` 有两个成员同值**（`LockAtMid_dDj_auto = "18"` 与 `LockAtMid_dLa_auto = "18"`，
  原样如此）。字符串枚举没有反向映射，所以 `Object.keys` 仍是 24 项且**声明序即遍历序**
  ⇒ C++ 表按声明序逐项对拍（`de <i> "<name>" "<value>"`），任何重排/改名/改值立刻可见。
  `dummy_updaters` 的**键集**（`""` + `"1"`..`"22"`，`_auto` 系列值为 `undefined`）是可观察的，
  但它的键序是 **JS 对象的整数键优先** ⇒ C++ 侧先用声明序建一个 `Object` 再走
  `object_keys()`，复用已验证的排序规则，**不**手工再排一遍。
  闭包体（`self.key_down(...)`）要等 `BotController` ⇒ 本轮不做，
  也**不**把不同形状的 updater 塞进一张带标志位的表（`make_ball_special` 的教训）。
- `DummyEnum.ts` 的 `import { BotController }` 是**值导入**（虽然只当类型用），
  esbuild 打包会连带 `Entity` 链；实测可打包运行（`bot` 链不碰 `three`/DOM）
  ⇒ 枚举表可以直接从产品模块 import，不必手抄"影子表"。
- ⚠️ **`o N` 又错了两次**（本轮的 `o 3` 以为有两对、`o 4` 实际有五对），
  都被 harness 的"解析完 `idx != t.length` 即报错"自检当场抓住 ⇒ 这条自检是必需的。- ⚠️ **中断 `mutate.mjs` 后手工恢复源码必须刷新时间戳**：`Copy-Item` 会保留备份文件的旧 mtime
  ⇒ ninja 认为 `.obj` 比源码新、**跳过重编译** ⇒ 随后 `native.mjs test` 用的是"最后一次变异"
  的二进制，表现为莫名其妙的漂移（本轮误报过一次 `is_ray_hit` 漂移）。
  `mutate.mjs` 自带 `recoverFromInterruptedRun()`，能让它自己恢复就别手工抄。
- **变异体必须能编译**：`closest.cpp` 只 include 了 `base.h`，"`round` → `round_float`"的变异
  直接 `C3861`（`round_float` 未声明）⇒ 那测的是"符号在不在这个头文件里"，不是语义。
  改成同样意图、但能编译的"完全不做四舍五入"，照样被 `3.3335`/`3.3334` 那条用例杀掉。

---

### 4.54 V54 步骤 4 第四块 `collision/` 纯助手（含 `Entity.dataset`）

- 新增 `native/lfw/collision/`：`is_fall.{h,cpp}` / `is_armor_work.{h,cpp}` /
  `calc_itr_velocity.{h,cpp}`；并补上 `native/lfw/entity/entity_dataset.{h,cpp}`
  （TS `Entity.dataset`，`calc_itr_velocity` 的必需依赖）。subject `collision_helpers`
  （op `ent` / `col` / `ds` / `ifall` / `armor` / `civ`）**243 行全对**，变异 **99/99 全杀**。
- **`Entity.dataset(name)` 是四级 `??` 链**：
  `frame.dataset?.[name] ?? data.base[name] ?? world.bg.data.dataset?.[name] ?? world.dataset[name]`。
  `??` 只在 `null`/`undefined` 时下探 ⇒ `0` / `false` / `""` 会**截断**链条（写成 `||` 会穿透）。
  用例 `e_zero` / `e_false` / `e_empty` 各锁一条；四个层级各有一条"只在这一级命中"的用例。
  ⚠️ 链条里 **`frame`、`world.bg.data`、`world.dataset` 都不是可选链**（只有 `.dataset?.[k]`
  那一处有 `?.`）⇒ 缺 `frame` 或 `world.bg` 时 TS 直接抛异常。这类输入**不属于契约**，
  用例一律给全路径（记为差异）。
- **`victim.state` / `attacker.state` 是 getter（= `frame.state`）**，不是实体自有字段
  ⇒ C++ 必须读 `frame.state`。而 `collision.bframe.state` / `aframe.state` 里的 `bframe`/`aframe`
  是**帧对象**，其 `state` 是普通字段 ⇒ 两者读法不同，用例 `frozen_s` 与 `bframe state n 13`
  分别锁住。`position` / `facing` / `armor` / `is_on_ground` / `hp` / `fall_value` 都是普通字段
  或直通 getter ⇒ 直接读字段等价；只有 `weight`（`data.base.weight ?? 1`）与 `state` 要走规则。
- **`is_armor_work` 的三处严/松相等**：`armor?.fulltime === false`（严 ⇒ `0` 不算 `false`，
  用例 `cc_atk` + `cd_ff0` 锁住）、`bframe.state === ...` 与 `itr.effect === ...` 逐项严相等；
  而 `bdefend >= 200` 是关系运算（`"200"` 数值强转后仍触发破防）。
  `const { bdefend = 0 } = itr;` 这个默认值在 `>=` 里**不可观察**（`undefined >= 200` 与
  `0 >= 200` 同为 false）⇒ C++ 直接比较，并在此写明这条等价（不写对应变异）。
- **`calc_itr_velocity` 混用松/严相等**：`itr.effect == IE.FireExplosion/Explosion` 是**松**相等
  （`effect s "22"` 也算命中 ⇒ 用例 `cv_effstrr`），而 `attacker.state ===
  StateEnum.HeavyWeapon_InTheSky` 是**严**相等（`state s "2000"` 不命中 ⇒ 用例 `cv_hws_sr`）。
- **返回四元组里的 `x_direction` 保留原值**（`attacker.facing` 可能是字符串）⇒ C++ 用 `Value`
  而不是 `double`，只在参与乘法时 `to_number`。用例 `cv_fs`（`facing s "1"`）把第 4 位渲染成
  `s"1"`，`cv_nof`（无 facing）渲染成 `u` 且 `x` 变 NaN。
- ⚠️ **"我写了这个用例"≠"这条分支被执行到了"（本轮 5 连杀）**：`civ` 的 `position_based`
  分支要 `diff_x > 0` 才走到 `x_direction = -1`，而我给 `vf` 写的 `position.x` 是 `0`
  （本意是 25）⇒ 所有 position_based 用例都只落到 `diff_x < 0` ⇒
  "`effect == FireExplosion` 用严相等"、"`state === 2000` 用松相等"、"2000 写成 2001"、
  "attacker 状态读成顶层字段"、"`diff_x > 0` 分支方向写错"这 **5 条全部存活**；
  补 `vright`(x=25) 后一次性全杀。⇒ 写完用例要**回看输出值**确认落在预期分支。
- ⚠️ **同一"形状"的 op 要一起设计**：`ent`/`col` 带 sub，`ds`/`ifall`/`armor`/`civ` **不带**
  ⇒ 后者必须在通用 `<sub> <name>` 解析**之前**分流（本轮又踩一次；C++ 侧越界 token 会直接 AV，
  比报错更难查）。

---

### 4.55 V55 步骤 4 第五块 `entity/Summary` + `SummaryMgr`（含 `js_add`）

- 新增 `native/lfw/entity/summary.{h,cpp}` / `summary_mgr.{h,cpp}` / `native/lfw/utils/js_add.{h,cpp}`，
  并在 `defines/team_enum.h` 补上 `is_independent`（TS `defines/TeamEnum.ts`）。
  subject `summary_helpers`（op `ent` / `sm` / `sg` / `su`）**146 行全对**，变异 **57/57 全杀**。
  （`buff/grant_buff.ts` 依赖 `Entity`/`World`/`factory.create_buff` ⇒ 仍留在整块移植里；
  本轮把它前面的这块缝隙补完。）
- **`Summary` 的五个 setter 都用松相等守卫**（`if (o == v) return;`）⇒ `5` 与 `"5"` 视为相同
  （不触发回调、不改值），而 `null == undefined` 为真、`undefined == false` 为假。
  用例把这三条都锁住；写成 `===` 或 `Object.is` 都会被杀。
  ⚠️ 只在**同一事件**上重复设值才可观察 ⇒ 五个事件必须**各注册一个监听器**，
  否则"漏掉守卫"的变异在四个字段上全部存活（本轮 8 条存活里有 4 条是这个原因）。
- **`sum += value` 必须走 JS 的 `+`**：`SummaryMgr.add_*_sum` 用的是 `+=`，
  所以字段是字符串时会**拼接**（`"6" + 1 = "61"`），对象/数组会先 ToPrimitive
  （`0 + {} = "0[object Object]"`、`0 + [] = "0"`）⇒ 新增 `js_add`
  （ToPrimitive → 任一为字符串则拼接，否则数值相加），**不能**直接用 `to_number` 相加。
  ⚠️ 只测"b 是对象"不够：`js_add` 的 `a` 侧 ToPrimitive 需要有"字段本身是对象"的用例。
- **`SummaryMgr._items` 是 `Map`，必须保插入序**（新条目追加在末尾、`release` 删除、
  复用墓碑后重新追加到末尾）⇒ C++ 用 `std::vector<std::pair<...>>` 而不是 `std::map`。
  `clear()` 先 `Array.from(keys)` 快照再逐个 `release`（否则边遍历边删），C++ 照抄这条。
  `_graves` 是 LIFO 池，但"复用哪一个对象"不可观察（复用前会 `reset(id)`）；
  可观察的是**墓碑数量**与"复用后旧回调已被清"。`Summary` 内嵌不可拷贝的 `Callbacks`
  ⇒ `_items`/`_graves` 存 `shared_ptr<Summary>`。
- **`SummaryMgr.apply_damage` 的击杀走模块级单例 `summary_mgr`，不是 `this`**（TS 源码原样）
  ⇒ 对非单例实例调用时，伤害记在自己身上、击杀记到单例上。这是可观察的怪癖，
  harness 用 `sm <mgr> items` 与 `sg items` / `sg get <id>` 两个视图分别对拍。
  ⚠️ 一开始 `sg` 只输出**条目 id**，看不到数值 ⇒ "不判 fighter"/"fighter 判定用 `||`"/
  "`prev_hp > 0` 用 `>=`" 三条存活；补 `sg get <id>` 后一次性全杀。
- **第 3 个回调参数 `target: Summary` 未移植**：只有监听方（运行时/UI）会读它，
  C++ 传 `undefined`，差分 harness 也不渲染它 ⇒ 记为已知差异（与 `Expression` 同类）。

### 4.56 V56 步骤 4 第六块 `entity/DrinkInfo` + `collision/handle_stiffness`

- 新增 `native/lfw/entity/drink_info.{h,cpp}` / `native/lfw/collision/stiffness.{h,cpp}`，
  subject `drink_stiffness`（op `di` / `stf` / `ent`）**63 行全对**，变异 **44/44 全杀**。
  这是步骤 4 里两个"小而独立"的叶子：`DrinkInfo` 只依赖 `Times` 与 `is_num`，
  `handle_stiffness` 只依赖 `entity_dataset`（上一轮已移植）。
- **TS 类字段初始值必须逐字搬到 C++ 成员初始化式**。`DrinkInfo` 的
  `hp_h_value: number = 0` / `hp_h_total: number = 9999999` / `hp_h: number = 0` …（3 组 × 3）
  若只在构造里赋 `Value()`，`null` 输入（`??` 不穿透 `null`… 实为 `undefined`）的
  快照就会少掉 9 个 `0` ⇒ `di snap` 全线漂移。C++ 侧写成
  `Value _hp_h_value = Value(0.0);`（**不是** `Value()`）。
- **TS 默认参数对 `undefined` 生效，对 `null` 不生效**。`new Times(0, info.hp_h_ticks)` 里
  `hp_h_ticks` 为 `undefined` 时 `_max = Times.MAX`（`Number.MAX_SAFE_INTEGER`），
  而不是 `Number(undefined) = NaN`。因此 `di new d5 o 2 hp_h_ticks z hp_h_value z`
  得到的是 `_min = 0, _max = MAX`。C++ 抽成 `ticks_bound(info, key)`
  （`undefined` → `Times::MAX`，其余 `to_number`）。⚠️ 早先猜成"`null`/`undefined` 都吃默认值"
  导致 `_min` 算错：`Math.min(0, NaN)` 是 `NaN`，`Math.min(0, MAX)` 才是 `0`，
  而快照里 `_min` 恒为 `0`（TS 端 `set_range(0, ...)` 的第一参数就是字面量 0）。
- **`*_empty()` 的三段判定不可互替**：`hp_h_empty() = ge(_hp_h, _hp_h_total) || !truthy(_hp_h_value)`，
  `hp_r_empty()` / `mp_h_empty()` 同构但各读自己的字段。
  ⚠️ "`mp_h_empty` 读成 `hp_h_total`" 一度存活：所有用例的 `mp_h_value` 都是假值，
  第二个 `||` 项恒真，把 `>=` 的结果整个吞掉了。补一条 `mp_h_value n 1` + `mp_h n 5`
  落在 `hp_h_total(0) < 5 < mp_h_total(9999999)` 之间的用例后即可杀死。
  ⇒ **教训：`A || B` 的变异要被杀，必须让两侧都出现"只由该侧决定"的输入。**
- **`handle_stiffness` 两条回退链长度不同**：
  `motionless` 走 `entity_dataset`（可穿透 `data.base` / `world.bg.data.dataset` / `world.dataset` 四级，
  且按 `data.type === EntityEnum.Ball` 选 `ball_itr_motionless` 还是 `itr_motionless`）；
  `shaking` **只**读 `attacker.world.dataset.itr_shaking`（不走 `entity_dataset`）。
  用例里 `shk`（frame 3 / bg 4 / world 7）验证 `shaking = 7`（只有 world 级有值），
  `nsw`（无 `world.dataset.itr_shaking`）验证得到 `undefined`（而不是继续往下找）。
- **`calc_stiffness` 的 itr 同名字段优先于回退**：`motionless = itr.motionless ?? 回退值`，
  `shaking = itr.shaking ?? 回退值`，且 `??` 只在 `null`/`undefined` 时穿透
  （`0` / `false` / `""` 都会保留）⇒ `or_default` 与 `field_or` 的组合语义各出一条变异。
### 4.57 V57 步骤 4 第七块 `helper/Randoming` + `state/spawn_ice_piece`

- 新增 `native/lfw/base/clock.h` / `native/lfw/helper/randoming.{h,cpp}` /
  `native/lfw/state/spawn_ice_piece.{h,cpp}`；subject `mt_random`（op `mt` / `rn` / `ent` / `ip`）
  **143 行全对**，变异 **55/55 全杀**。
  `MersenneTwister` 早期就移植过（subject `mersenne_twister`，7564 行）⇒ 本轮只补它上面的
  `Randoming` 包装与 `spawn_ice_piece` 那两个叶块。
- **`<chrono>` 被 lint 禁止**（"host clock; use injected IClock"）⇒ 新增 `base/clock.h`：
  `class IClock { virtual double now_ms() const; }` + `clock()` / `set_clock()` 的
  **函数局部静态槽**（header-only，不用登记 CMake）。`Randoming::default_mt()` 用
  `clock() != nullptr ? clock()->now_ms() : 0.0` 作种子。
  差分里两侧都装假时钟（`1700000000000`）：C++ 侧 `set_clock(&TestClock)`，
  TS 侧靠 `subjects/shared/patch_date.ts`（`Date.now = () => 1700000000000`）**在 import 顺序上先于**
  `Randoming` 被求值（`Randoming.mt = new MersenneTwister(Date.now())` 是模块求值期执行的）
  ⇒ 两边默认 mt 的种子一致，**默认通道的取值也能逐位对拍**。
  ⚠️ `run.mjs` 只 glob `subjects/*.ts`（不递归）⇒ 共享模块必须放在子目录（本轮 `subjects/shared/`），
  否则它会被当成一个 subject 去跑。
- **TS 默认参数 ≠ `??`**（本轮唯一的漂移）：`create(name, src, mt = Randoming.mt, duplicate = false)`
  与构造函数的 `duplicate = false`，传 `undefined` 要回落 `false`，传 `null` 不回落
  ⇒ C++ 用 `duplicate_or_default()`（`monostate → Value(false)`），并且 `duplicate` 存 **`Value`**
  而不是 `bool`（否则 `dup=z` 与 `dup=b0` 不可区分）。差分直接抓到这个：
  `rn create c1 - … u` 输出 `dup=u`（错）vs `dup=b0`（对）。
- **`Randoming` 要点**：`_src` 与 `cur` 是两份，`set_src` **不动 `cur`**；`get()` 按 `truthy(duplicate)`
  分 `random_get`（从 `_src` 取，**不动 `cur`**）/`random_take`（从 `cur` 里 splice，抽空后重填）；
  重填条件是 `_src.size() > 1`，用 `v != taken` 的**松相等**过滤（`null == undefined`）
  ⇒ `taken` 为 `null` 时 `null`/`undefined` 元素会被一起滤掉；`_src` 为空时重填后仍空，
  越界读要返回 `undefined`（TS 的 `array[i]` 语义）。
  ⚠️ 「`taken` 初值 `null`」只在**首次重填**时生效，而首次重填要求 `cur` 已空 ⇒ 必然已抽过一次；
  但 `dump` 里打印 `taken=` 就能观察到初值 ⇒ 这条变异仍可杀（不要当等价变异删掉）。
- **`spawn_ice_piece`**：每次返回**新对象**，键序固定
  `kind,x,y,oid,action,dvx,dvy,ghost,speedz,unimportant`，`oid` 硬编码 `OID.BrokenWeapon`(999)。
  `ice_piece_opoints` 是 16 个模块级实例（130×3 / 120×2 / 125×4 / 135×7）。
- **`__gen_*` 是函数对象** ⇒ 按既有「Expression 类字段」约定**不移植**，差分时 TS harness 侧
  删掉 `__gen_dvx/__gen_dvy/__gen_x/__gen_y` 四个键。但**体内的逻辑**移植成 4 个具名纯函数
  （`ice_piece_dvx/dvy/x/y`，把 `e.lfw.mt` 降为参数），TS 侧直接调**真实的 getter**
  （`spawn_ice_piece("0").__gen_dvx.get({...ent, lfw:{mt}})`）⇒ 打在这些逻辑上的变异是可杀的，
  不是死代码。
  - `__gen_dvx`/`__gen_dvy` 的缓存是**模块级**（`dvx_randoming()`/`dvy_randoming()` 两个函数局部静态
    `shared_ptr`，各一个槽，别合并），身份判据是 **mt 指针**（`cached->mt() != &mt`）⇒ 换 mt 会
    重开一个 `Randoming`；而 `!is_object(e)` 的短路在**缓存之前**，所以"非实体"不会重置缓存。
  - `__gen_x`/`__gen_y` = `round(w/2 + mt.range(-w/4, w/4))`。
    ⚠️ `range` 在 `min == max` 时**提前返回且不消耗随机数**，`w = 0` 正好走这条
    ⇒ 用例必须在 `w = 0` 之后**再抽一次**才能观察到"有没有消耗"。
  - `mt.mark = ...`、`debugging`、`mt_cases` 只被调试探针读（`pure()` 在 `src/LFW` 里无调用点）
    ⇒ 不移植（沿用既有 `MersenneTwister` 实现，它本来就没有 `mark`）。
- **变异覆盖技巧（不放回抽样 ⇒ 一次覆盖全表）**：`random_take` 是"抽走不放回"
  ⇒ 连抽 n 次（n = 数组长度）必然**恰好**把数组每个元素各抽到一次
  ⇒ 打 9 次 `ip dvx` + 13 次 `ip dvy` 就足以杀死数组里任何单个元素的变异，不必逐个写条目。
- **`round` vs `floor` 的分岔要用非整数半径**：`w/4` 是整数时（如 80）`round` 与 `floor` 同值
  ⇒ 必须用 `w = 2 / 10 / -10` 这种 `w/4` 带 `.5` 的宽度（本轮 2 条存活之一就是这样补杀的）。
### 4.58 V58 覆盖审计 + `base/Expression` 加固 + 构建链提速

**背景（本轮查出的真问题）**：有 8 个早期 subject **从来没有过变异文件** ——
`expression` / `value` / `json` / `math` / `mersenne_twister` / `core` / `utils` / `collections`
（`git log --diff-filter=A -- 'native/tests/differential/mutations/*'` 里只有 44 个文件）。
README §1.6 记的"共 48 条变异全杀"当时是**临时脚本**跑的，产物没留下 ⇒ 不可复现。
这不影响差分（差分一直在跑），但"每条单元都要有可复现的变异全杀"这条纪律出现了 8 个缺口。
本轮先补最关键的一个：`Expression`（条件引擎，所有 `__judger` / 帧条件都过它）。

- **harness 强化**（两侧同步改）：`val_1`/`val_2` 从只打类型标签改成 `render_value` 的**位精确**输出；
  新增 `result` 字段；**新增 getter 调用日志**（`clr` / `log`）—— 求值顺序与短路**只能**靠它观察；
  新增 `d <idx>` 在 `run` 之后重放整棵树。
  用例 187 行 → **928 行**（`parse` 303 / `eval` 348 / `short` 146 / `more` 131）。
  ⚠️ 每个 `.txt` 是**独立进程**，全局符号表不跨文件共享 ⇒ 新用例文件要自己 `g` 一遍。
- 顺手修掉 `expression.h` 里两个被静默吞掉的 `C4458`（构造函数局部 `text` / `before` 遮蔽同名成员）
  ⇒ 改名 `src_text` / `pending`。**注意**：`native.mjs` 报的 "N clean / 0 warnings" 来自
  `check_lfw_cpp_includes.mjs`（纯文本检查），**不统计编译器告警** —— 编译器告警要自己看构建输出。
- `mutations/expression.mjs`：**69 条全杀**（覆盖 `expression.h` / `bin_op.h` / `predicate.cpp` 三处）。
- **删掉 3 条已证明等价的变异**（第 6 形态：死代码 / 冗余守卫）：
  1. `&`/`|` 分支里 `while (!sub.empty() && sub.back() == u')') sub.pop_back();` 改成 `if` / 去掉 ——
     **死代码**。推导：扫描遇到任何 `)` 都会 `break`；而 `p` 在每次子节点分支后都已被推到已消费区之后
     （`(` 分支 `p = i + 1`、`!(` 分支 `p = i + 2`），所以区间 `[p, stop)` 的每个下标都被扫描过，
     **不可能藏着 `)`** ⇒ `sub` 恒不以 `)` 结尾。用例 `b (A==B))&(C==D)` 的输出也印证：
     树只剩 `(A==B)` 一个节点、`&(C==D)` 整段被多余 `)` 提前截断（TS 原样行为，两边一致）。
  2. `run` 里 and 组的 `if (and_set && !and_result) continue;` 去掉 —— **冗余守卫**：
     紧随其后的 `and_result && child.run(ctx)` 本身就会在 `and_result` 为假时短路。
     （把条件取反成 `and_result` 则会跳过本该求值的子节点，那条**被杀掉了**。）
- **构建链提速**（`native.mjs` / `run.mjs` / `mutate.mjs`），单条变异 **~9 s → 4.1 s**：
  1. **VS 环境缓存**：每次 `native.mjs build` 都 `cmd /c → call vcvars64.bat`，实测
     **5400 ms/次**（与变异内容无关的纯浪费）。改成捕获一次存 `native/build/gen/vsenv.json`
     （带 `vcvars` 路径 + mtime 校验），之后用 `spawnSync(cmakeExe, args, { env })` 直接调 cmake，
     连 `cmd.exe` 都省掉。`where cmake` / `where ninja` / `vswhere` 结果也 memo 化。
  2. **`native.mjs build [<subject>]`**：带 subject 时加 `--target lfw_trace_<subject>`
     ⇒ `lfw_core.lib` 变化后**只重链被测那 1 个 exe**（原来 46 个）。
  3. **`run.mjs --reuse-ts`**：变异只改 `native/lfw/**`，TS 侧产物**不可能变**
     ⇒ 若 `trace.<subject>.<case>.ts.txt` 比用例文件与 subject `.ts` 都新就直接复用
     （0.94 s → 0.34 s）。`mutate.mjs` 会**断言每条变异的 `file` 都在 `native/lfw/` 下**，
     越界直接报错退出 —— 这是该优化成立的依据。
  4. `mutate.mjs` 新增进度点与 `总耗时 / ms per mutation / 最慢的一条`，以后一慢就看得见。
- 实测单条变异的构建内容（ninja 日志）：`predicate.cpp` + `labels.cpp` + `cook_frames.cpp`
  + subject 的 `expression.cpp` 共 4 个 obj，再链 `lfw_core.lib` 与 1 个 exe，外加 cmake 的
  `verify_globs`（测试 subject 是 GLOB 出来的，每次构建都会校验）。这部分是**必要成本**，不可省。
### 4.59 覆盖审计（二）：`utils/math/MersenneTwister` 加固

- 用例 42 行 → **71 行**（输出 7614 行），变异 **49/49 全杀**（109 s / 2.2 s 每条 —— 变异只碰
  `mersenne_twister.cpp`，走新的单 target 快速路径）。
- 意义：`MersenneTwister` 是**全游戏唯一的随机源**（`Randoming` / `RandomVisible` /
  `team_randoming` / `World.random_*` 全在它上面），之前只有差分、没有变异 ⇒ 现在补上了。
- **harness 扩展**：`pick` / `take` 现在把**操作后的剩余数组**一起打印。
  否则"take 删了哪个下标"不可观察 —— 每次调用都在从 token 新建的数组上操作，
  删对删错只差一个元素，而打印的 `size` 两种情况都少 1。
- 两条存活与修法：
  1. **`reset` 的 `_index = k_N + 1` 改成 `k_N`**：行为**完全等价**（625 与 624 都大于等于 624，
     首次 `next_int` 都会先 `twist` 并把 `_index` 归零），只能靠 `state` 指纹抓；而原有用例的
     `state` 都打在若干次抽取之后，那时 `_index` 早已走平 ⇒ 必须在 `seed` 之后**立刻**打一次
     `state`。**教训：内部状态字段的指纹要在"刚好设置完"的时刻打。**
  2. **`next_float` 的除数 2^32 → 2^32-1**：相对差 2.3e-10，被 `floor_float` 的 1/1000 量化吃掉
     ⇒ **新等价形态：差异小于后继量化步长**（要可观察，得让 `int/2^32*1000` 落在整数下方
     2.3e-7 以内，即 `int` 恰为 2^29 的倍数；十来个种子里抽一万次也遇不上）。
     改成测「除数写成 2^31」，保留了"除数参与计算且量级正确"这条。
- 顺带：`range(min, max)` 在 `min == max` 时**提前返回且不消耗随机数**，返回的是 `min`
  ⇒ `range(-0, 0)` 必须保留 `-0` 的位模式。用例成对写：`range -0 0 1` + 紧随其后的 `int`
  （既锁 `-0`，又锁"没消耗随机数"）。
### 4.60 覆盖审计（三）：`utils/math/` 全家族加固

- 用例 `scalar` 90 → **95 行**、`plane` 52 → **59 行**，变异 **61/61 全杀**（327 s / 5.4 s 每条，
  最慢单条 20.8 s —— 变异目标是被大量 TU 包含的 header，必须重编译所有包含者，这部分不可省）。
- 覆盖：`clamp` / `clamp_add` / `normalize` / `float_equal`·`equal`·`eqgt`·`eqlt` /
  `round_float` / `floor_float` / `range` / `probability` / `normalize_plane` / `calc_plane` /
  `line_plane_intersection` / `project_to_line`。三者（含既有的 `mersenne_twister`）合起来
  `utils/math/` 就整块有差分 + 变异了。
- **harness 修正**：`normalize` 之前被 harness 硬写成 `normalize(n, 1000)` ⇒ 头文件里的
  **默认参数永远用不到**，`normalize：默认倍数写成 100` 因此存活。改成 2 个 token 时走
  `normalize(n)`（两侧同步改）后即可观察。**教训：harness 不要替被测代码补默认值。**
- 两条**真等价**（已删，理由写在文件头）：
  1. `float_equal` 去掉 `abs`：`round_float(x - y) == 0` 与 `round_float(abs(x - y)) == 0` 等价 ——
     `round_float(-0.0004)` 得到的是 **`-0`**，而 `-0 == 0` 为真 ⇒ `abs` 冗余。
  2. `floor_float` 的默认倍数 1000 → 100：唯一调用点 `normalize` 总是显式传 `p` ⇒ 默认值不可达。
- 三条**用例缺口**（已补）：
  1. `line_plane` 的 `is_direction` 分支：原来两个 `is_direction=1` 的样本恰好都 `x1 == x2`
     ⇒ `vx = x2` 改成 `vx = x1` 不可区分。补 `x1 != x2` 的样本。
  2. `project_to_line` 的分母：原来的斜率 `m` 只取 `{1, 0, -0}`，而 `pow(1,3) == pow(1,2)`、
     `pow(0,3) == pow(0,2)` ⇒ `pow(m,2)` 改成 `pow(m,3)` 不可区分。补 `m = 2 / 3` 的样本。
  3. `normalize` 的默认倍数（见上）。
  ⇒ 补样本的**通用办法**：先把变异点想成"哪一类输入才分岔"，再检查现有用例的参数取值集合
  是否覆盖了那个分岔方向（这里是"斜率非 0/±1"与"起点≠终点"）。

### 4.61 覆盖审计（四）：`base/Value` + `Object`/`Array` 加固

- **覆盖面**：`native/lfw/base/value.h`、`object.h`、`array.h`、`kind_of`、`is_array_index`、
  `equals` / `strict_equals` / `less_than`、`to_number` / `to_string`。用例 187 → 489 行
  （`basic` 89 / `coerce` 61 / `equality` 99 / `object` 131 / `relational` 107），变异
  **71/71 全杀**。
- **harness 修正（第 2 例，与 §4.60 的 `normalize` 同源）**：`odel` 之前只打印被删的键名，
  把 `Object::remove` 的**返回值吞掉了** ⇒ "删除失败却报成功"这类变异不可见。改成
  `odel <key> <existed>`（两侧同步：C++ 取 `remove()` 的返回值，TS 用 `Object.hasOwn` 先探再删）。
  **教训：harness 必须透明转发被测代码的每一个可观察输出，既不能替它补默认值，也不能吞返回值。**
- **用例缺口（已补）**：
  1. `Object::get` 的**整数键分支**完全没有样本（只有字符串键）⇒ 补 `oprop` / `ohas` 打整数键
     `0/1/2/3`、`01`、`"1:2"`、`0.0`（非规范下标必须落到字符串键路径）。
  2. `is_array_index` 的**长度上限**需要 uint64 溢出样本才可观察：`18446744073709551616`
     （2^64）在"去掉长度上限"后会溢出成 `0` ⇒ 被当成整数键 0，`okeys` 从
     `"18446744073709551616"` 变成 `"0"`。补 20 位键样本。
  3. `strict_equals` 的**方向性**：`undefined`/`null` 与任意值的相等关系必须左右各打一遍
     （补 15 条 `seq` 用例）；只测单侧会漏掉"少判一个分支"的变异。
- 一条**死代码**（已删，理由写在文件头）：`strict_equals` 末尾的 `return false;` —— 上面 7 个
  variant 分支已覆盖全部 kind，该行不可达。"等价"判定要看**可达性**，不是看语义相同。

### 4.62 覆盖审计（五）：`utils/` 全家族加固

- **覆盖面**：`cross_bounding.h`、`easing/`（`ease_linearity` / `ease_in_out_sine` / `ease_in_out_quint`
  各含 `backward`）、`Times`、`utf8`，另补上此前**只有"能编译"、没有任何差分**的
  `type_check.h` / `type_cast.h`。用例 184 → 321 行（`cross_bounding` 8 / `easing` 63 /
  `times` 111 / `type` 98 / `utf8` 41），变异 **108/108 全杀**。
- **harness 修正（第 3 例，与 §4.60 `normalize`、§4.61 `odel` 同源）**：`ease_*` 这 8 个函数的
  `from`/`to` 两个默认实参此前**永远由 harness 显式填 `0` / `1`** ⇒ 头文件里写的默认值不可达。
  改成**按实参个数分派**：C++ 侧 `ease_at(tok, f1, f2, f3)` 收 3 个 lambda，TS 侧给缺的形参传
  `undefined` 走原生默认参数（JS 的默认值对 `undefined` 生效）。用例补 `ease_linearity 0.5 10`
  这类"只给 2 个实参"的写法。
  **注意 C++ 没有 `undefined`** —— 这是"用同一份用例驱动两种语言"时必须自己造的一层分派。
- **同类问题**：`Times` 的 `ctor(min = 0, max = MAX)` / `set_lifes(v = -1)` / `add(d = 1)` 三个默认值
  也被 harness 的兜底值掩盖 ⇒ 一并改成按实参个数分派，并在 `cases/utils/times.txt`
  **首行**加 `times_state`，观测"默认构造出来的 `Times`"。
- **新增观测**：`chk <name> <valueliteral>`（6 个谓词）、`tonum <v>`（TS 的 1 实参重载返回
  `undefined`，C++ 用 `std::optional` 的 `nullopt`，两侧都渲染成 `u`）、`tonum_or <v> <n>`；
  以及 `times_is_max` / `times_is_min`（否则这两个 getter 的 `>=` / `<=` 边界完全不可观测）。
- 一条**死代码**：`type_check.h` 的 `is_nan_num` 在整个 `lfw/` 里**零调用点**（只有定义），
  已记入"已知偏差"，不为其造观测。
- 三条**真等价**（已删，理由写在文件头）：
  1. `ease_in_out_quint.backward` 的 `ratio < 0.5` → `<= 0.5`：`ratio == 0.5` 时
     下支 `pow(0.5/16, 0.2) = pow(1/32, 0.2) = 0.5`，上支 `1 - pow(1, 0.2)/2 = 0.5`，两值相等
     （0.5 是这条曲线的不动点，与 `ease_in_out_quint` 正向的 `< 0.5` → `<= 0.5` 同源）。
  2. `Times::_value` 的**类内成员初值**：构造器体里 `set_range` 立刻写 `_value` ⇒ 构造完必被覆盖，
     是死存储（TS 侧同理）。**但 `_lifes` / `_remains` 的初值不会被 `set_range` 碰**，
     所以它们是可观测的 —— 改初值这类变异要逐个确认"有没有人在构造期覆盖它"。
  3. `Times::add` 的 `if (ret && _remains > 0.0)` 把 `> 0.0` 改成 `>= 0.0`：能走到这一行就说明
     `_remains != 0`（前面已有 `if (_remains == 0.0) return false;`）⇒ 两条件等价。

### 4.63 覆盖审计（六）：`core/json` 加固

- **覆盖面**：`lfw/core/json.cpp` 的 `json_stringify`（`quote()` + `write()` + `number_to_string` 委派）
  与 `json_parse`（`Parser` 的 `str/num/arr/obj/val` + `digit()` + `is_json_ws`）。
  用例 163 → 250 行（`parse` 94→161、`stringify` 69→89），变异 **76/76 全杀**。
- **两条用例缺口（都不是等价，是样本没打在变异点的分岔方向上）**：
  1. **`\uXXXX` 的十六进制有 `0-9` / `a-f` / `A-F` 三段**，而原样本是 `0041`、`00e9`、
     `d83d`、`0000` —— **一个大写字母都没出现** ⇒ "不认大写十六进制"（删掉 `A-F` 分支）存活。
     补 `"\u00E9"`、`"\uD83D\uDE00"`、`"\u00aB"`（混大小写）、`"\uABCD"`。
  2. **`{"a" 1}` 这条"专为冒号检查写的用例"其实没测到那条检查**：把 `!= u':'` 检查删掉后，
     跳过空格与 `1` 会撞到 `}` 上，由**下游**的"键必须是字符串"检查兜住 ⇒ 两侧都 err。
     要让它可分辨，必须构造"跳掉一个字符后**恰好拼得出合法文档**"的输入：
     `{"a"9 1}` / `{"a"1 2}`。
     ⇒ **教训：删掉某条检查后，要确认没有别的检查接着兜住它** —— 否则"报错"这个结果
     在两侧都会出现，用例是空转的。
- **故意不打的 5 类变异**（不是覆盖缺口，是构造上不可判别）：
  1. `quote()` 的 `(c >> 12) & 0xf`：该分支只在 `c < 0x20` 时进入 ⇒ `c >> 12` 恒为 0，改不改都输出 `'0'`。
  2. `Parser::str()` 的 `if (i + 4 > s.size())` 改成 `i + 3 > ...`：能过检查又"四字符都是十六进制"的
     输入不存在（真到那一步后面一定跟着收尾引号或越界 ⇒ 两版都 fail）。
  3. `digit()` 的 `i < s.size()` 改成 `i <= s.size()`：`s[size()]` 是 `'\0'`，不是数字，结果相同。
  4. `write()` 末尾 `if (o == nullptr) return false;`：走到那里时 variant 只可能是 Object ⇒ 不可达。
  5. 去掉 `arr()`/`obj()` 里的 `++i`、去掉 `str()` 的越界判断、把 `write(*p, out)` 写成 `write(v, out)`：
     分别是死循环与越界读 —— 它们给出的是 UB/挂死而不是"漂移"，**不能作为覆盖证据**，故不打。


### 4.64 覆盖审计（七）：`collections` 全家族加固

- **覆盖面**：`base/graves.h`（`add`/`take`/`l`）、`utils/array/{loop_arr,make_arr,map_arr}.h`、
  `utils/container_help/{filter,find,fisrt,ensure,loop_offset,map_no_void,nested_map,nested_multi_map}.h`。
  用例 144 → 194 行（`basic` 87→109、`nested` 57→85），变异 **68/68 全杀**（6.9 s/变异，总计 471.6 s）。
- **四处观测盲区（都在 harness 侧补齐，不用改被测代码）**：
  1. **1 实参的 `fisrt()` / `last()` 一条用例都没有**（此前只有 `fisrt_gt`/`last_gt` 这两个 2 实参版）
     ⇒ 新增 `fisrt_any` / `last_any`；它们的 `return std::nullopt;` / `ret = item;` 也才第一次有观测。
  2. **`map_arr` / `loop_arr` 回调的后两个实参被 harness 的 lambda 忽略**
     （原式 `[k](double v, auto, const std::vector<double>&) { return v * k; }`）⇒
     "下标写错"与"第三参传空 vector"这两类变异全部不可见。改成
     `v * k + i * 10 + arr.size()`（两侧同式），下标与数组长度同时进入输出。
  3. **`ensure` 只有模板重载被覆盖**，`Value&` 版（含单元素转发重载）从没被任何 subject 观测过
     （而 `ensure.h` 的生产调用点全在它上面）⇒ 新增
     `ensure_val <valueliteral> | <valueliteral…>`，复用 `trace_util` 的
     `parse_value`/`render_value`，并按**项数**分派 1 项重载与多项重载。
  4. **`intersection` 的默认谓词是 `equal_to`**，于是 `ret.push_back(c1)` 换成 `push_back(c2)`
     在"能匹配上"处恒等（c1 == c2）⇒ 新增 `intersection_lt`（显式传 `std::less<double>`）
     才可分辨，顺带第一次真正走到第三个参数 `F fn` 上。
- **一条真等价（已删，理由写进规格头部）**：`nested_map::clear()` 末尾的 `_map.clear();` 删掉恒等 ——
  循环里每个内层 map 都已被 `kv.second.clear()` 清空，残留的空内层 map 让
  `get`/`has`/`remove` 一律"未命中"，与"外层键不存在"完全同观；之后 `set(k1,k2,v)` 走
  `ref()` 的新建分支（从对象池取一个空 map）还是"已有 k1"分支（就地写那个空 map）结果一样；
  唯一差别只有私有对象池里空 map 的条数（只影响性能）⇒ 不可观测。
- **七类不打（构造上不可判别或会引入 UB，写进规格头部第 1–12 条）**：
  1. `graves.add` 的 `_l[--_i] = t;` → `_l[_i--] = t;`：`_i == _l.size()` 时（`basic` 里
     `graves_add 3` 紧跟最后两次 `take`）会越界写 ⇒ UB。
  2. `take` 的 `_i >= _l.size()` → `>`、`add` 的 `_i == 0` → `_i != 0`、`size_t _i = 0;` → `1`：
     三者都让 `_i` 在越界状态下继续被 `_l[_i]` / `_l[--_i]`（回绕成 SIZE_MAX）使用 ⇒ UB。
  3. `ensure` 模板版 `if (!output.has_value()) return items;` 取反：`ensure | 1 2 3` 这类
     "目标为空"的用例会走进 `output->insert`（对 nullopt 解引用）⇒ UB。
  4. `map_arr` 的 `if (!list.has_value()) return ret;`：取反或删除后紧跟 `*list` ⇒ UB。
     空输入的**正方向**改由新 op `map_arr_nil` 用**差分**覆盖（两侧都返回空数组，
     与"空 vector"在输出上无法区分，因此这条守卫注定打不出变异）。
  5. `loop_offset` 的 `if (idx >= static_cast<double>(len))`：`idx` 来自 `fmod(x, len)` 恒 `< len`，
     NaN 已被上一行 `!(idx >= 0.0)` 拦下 ⇒ 不可达，`>=` 改 `>` 恒等。
  6. `nested_map::clear()` 里往对象池塞回收 map 的 `_graves.add(kv.second);` 删掉、
     `ref()` 里不复用池只取 `Inner{}`、`clear()` 开头的 `if (_map.empty()) return;` 删掉：
     池只影响性能（回收对象已被清空），早退只是跳过空循环 ⇒ 恒等。
  7. `find.h` 的 `find_value_index`（`const Array*` 版）不属于本 subject 的输入种类，
     由 `make_ball_special` subject 覆盖。
- **教训**：**harness 的回调要把每个实参都用上**。`loop_arr` 因为"顺手把收到的下标打出来"
  一直是可观测的，同族的 `map_arr` 因为 lambda 只用了第一个实参，四条相关变异全部不可见 ——
  同族函数之间这种"一个测到一个没测"的差异，最容易在写变异规格时被整体漏掉。


### 4.65 覆盖审计（八）：`core` 加固（`js_num` / `js_string`）

- **覆盖面**：`lfw/core/js_num.cpp`（`js_round`/`js_floor`/`js_ceil`/`js_abs`/`js_to_uint32`/`js_to_int32`/
  `f64_bits`/`f64_from_bits`）与 `lfw/core/js_string.cpp`（`is_str_white_space`/`digit_value`/
  `parse_radix`/`scan_decimal`/`string_to_number`/`shortest_digits`/`number_to_string`）。
  用例 10893 → 10969 行（`js_num` 85→116、`to_number` 152→197），变异 **112/112 全杀**
  （2.4 s/变异，268.4 s）。这是早期 subject 里最后一块覆盖缺口。
- **三处补样本，都是"分支的常量表/边界"**：
  1. **空白常量表缺两项**：`is_str_white_space` 的 switch 有 14 个码点，而 `to_number` 用例只覆盖了
     `\t \n \r \s`、NBSP、`\u3000`、`\ufeff`、`\u2028/\u2029/\u202f/\u205f/\u1680/\u2000/\u200a` ——
     **`\v`(0x000b) 与 `\f`(0x000c) 一条都没有** ⇒ 这两条"码点写错"的变异原本不可见。补
     `"\u000b1"`、`"1\u000c"` 等 9 条后 14 条码点变异全部可杀（按"常量表逐项打"的规矩一次打齐）。
  2. **`Math.round` 的"x + 0.5 进位到偶数"修正只在 ≥ 2^52 的奇数上生效**：`if (r - x > 0.5) r -= 1.0;`
     要 `floor(x + 0.5)` 真的跳到偶数上才会触发（4503599627370497 → 4503599627370498 → 减回），
     原用例只有 `4503599627370495.5` 与 `.4` ⇒ 补 `round 4503599627370497/…499`、`-…497/-…499`
     以及 7 条 `nan` 传播样本。
  3. **`parse_radix` 的舍入路径（`keep/rest/half/sticky`）只在输入 > 64 位时才走**：小于 64 位时
     `shifted == 0` 直接精确返回 ⇒ 之前所有"看着像 tie"的样本（如 `0x40000000000004`，60 位）
     其实都没走到舍入，5 条舍入变异存活。补 >64 位样本后全杀：
     `0x10000000000000800`（`drop=8`、`rest==half`、keep 偶、sticky 假 ⇒ round-half-even 不进位）、
     `0x10000000000000810`（同前但 sticky 真 ⇒ 进位到 `2^64+4096`）、`0xbc1fb2f56a0dbea88c0` 等。
- **两条踩坑（"样本看似命中、其实没打到分岔点"）**：
  1. 60 位的"tie"走的是 `shifted == 0` 的精确返回 ⇒ **样本必须落在变异点真正经过的那条路径上**：
     先确认 `shifted > 0` / `drop > 0`，再谈 `rest == half`。
  2. **自查工具本身要先对拍**：我写的第一版 `parse_radix` 模拟器在**不带 `0x` 前缀**的字符串上用
     `from = 2`（等于丢掉前两位十六进制），据此"搜到"的样本根本不是被检验的那个数 ⇒ 两句无效样本、
     2 条变异假幸存。改成"模拟器 vs 真实 exe 逐条比位模式"后才定位（顺带发现探针里
     `execFileSync` 的 `\r` 会制造假 MISMATCH）。**凡自己写的判据搜索器，先拿真实实现校准再用它下结论。**
- **首轮 3 个幸存者里 2 个是真等价（已删，理由写进规格头部）**：
  1. `js_to_int32` 的 `u < 0x80000000u` → `u < 0x7fffffffu`：只有 `u == 0x7fffffff` 会从正支掉到负支，
     而 `(int32_t)(2147483647 - 4294967296)` 在 MSVC 上是"截低 32 位" ⇒ 仍得 2147483647，两支恒等。
  2. `parse_radix` 的 `acc >> (64 - log2base)` → `>> (63 - log2base)`：只改"什么时候把满位半字节
     移出去"，多移一次等价于 `acc >>= 4; shifted += 4;`，指数 `drop + shifted` 与尾数 `keep` 都不变，
     移出窗口的低位变成 `sticky`（正是舍入判决需要的全部信息）⇒ 判决不变。已在 20 万条 68~160 位
     随机 hex 上验证零差异。
  3. `number_to_string` 的 `const double a = neg ? -v : v;` 改成反号：`shortest_digits` 一开头就把
     `to_chars` 输出里的 `'-'` 跳掉，符号由后面 `if (neg) out.push_back(u'-')` 统一给出 ⇒ 同果。
- **不值得打的**（构造不可判别或 UB，逐条写在 `mutations/core.mjs` 头部第 1–12 条）：`js_round` 的
  `isnan/isinf` 早退（NaN/inf 走主公式同值）、`js_to_uint32` 的 `isfinite` / `x == 0.0` 守卫
  （去掉即 UB 或恒等）、`f64_bits`/`f64_from_bits`（`bit_cast` 无可注入漂移）、`shifted > 1100`
  快速路径（数值至少 2^1101，`ldexp` 反正溢出成 inf）、`scan_decimal` 的 `e < 1000000`
  （有符号溢出保护）、`e10 == 0`（不可达）等。


### 4.66 V59 步骤 5 第一片 `Transform`（Entity/World 整块的入口）

- **为什么从它开始（先量再做）**：新增工具 `native/tools/ts_scope.mjs`，把 `entity/Entity.ts` + `World.ts`
  的依赖拆成"值可达 / 仅类型可达"两类来量。结果：值闭包 **319 文件 / 24773 行**，其中 `defines`(7145)、
  `dat_translator`(3464)、`loader`(1905)、`collision`(1567)、`bot`(1500)、`state`(1394)、`controller`(1068)、
  `utils`(897)、`base`(800)、`entity`(807) 已有对应目录（部分是缝隙级），**真正全新**的是
  `stage`(724) / `buff`(379) / `ui`(127) / `bg`(116) / `ditto`(110) 以及顶层 `World.ts`/`LFW.ts`/
  `Factory.ts`/`Ground.ts`。
- **两个"先量再做"纠正掉的误判**：
  1. `ditto/`（55 文件 / 约 1370 行）**跨目录值依赖 0 个**，全是被引擎反向调用的服务接口
     （`ISounds`/`IXMLElement`/`IImageMgr`/`IKeyboard`/`IZip`…）⇒ 它是**依赖注入缝**，不是要整块搬的代码；
     按 `IClock` 的先例**按需注入**即可。反过来 `buff/`（12 文件 / 602 行）跨目录**值依赖 315 文件 /
     28157 行**（依赖 `entity`/`World`/`LFW`）⇒ 必须排在 `Entity` **之后**，不能当叶子先做。
  2. `Entity.ts` 的值导入只有 24 个，其中 `defines`/`utils`/`entity/*` 已移植；真正是新的：
     `Factory`/`Ground`/`World`/`base`/`buff/Buff`/`collision/Collision`/`controller/BaseController`。
- **本片**：`native/lfw/transform.{h,cpp}`（TS `src/LFW/Transform.ts`，147 行）——位置/缩放/旋转的平滑补间。
  依赖只有已移植的 `pow` 与 `round_float`，是干净叶子。用例 `cases/transform/*.txt` 4 个共 **339 行**，
  差分 4/4 通过，变异 **45/45 全杀**（2.87 s/变异，129.3 s）。
- **接口简化（已记录，语义等价）**：
  1. TS `move_to(x, y, z, opts: ITransformTweenOpts = {})` 里 `opts.rate ?? 0.1` ⇒ C++
     `std::optional<double> rate` + `rate.value_or(0.1)`。
     ⚠️ **harness 必须做翻译**：把裸 rate 直接当 `opts` 传给 TS，`opts.rate` 会是 `undefined` 而静默回落
     0.1 —— 本轮第一次 drift 正是这么来的（`move 10 0 0 0.5` 之后 C++ 走 0.5、TS 走 0.1）。
  2. TS 可以同时有属性 accessor `set rotation(v)` 与同名方法 `set_rotation(...)`；C++ 不能重载 get/set
     ⇒ 属性 setter 记为 `set_x/y/z`、`set_scale_x/y/z`、`set_rotation_value`，方法保持原名。
     两者的语义差别被完整保留：**属性 setter 不动 `_smoothing`，方法会清**（用例专门锁这条）。
  3. TS 模块私有的 `wrap_angle` ⇒ C++ 私有静态 `Transform::wrap`；harness 只经 `rx`（属性）与
     `rot`（方法）两个入口间接观测，不去造 TS 侧取不到的入口。
- **NaN 观测口径**：状态打印用 `num_hex`（NaN 统一成 `"nan"`，其余打位模式）——避免两侧 NaN 载荷不同
  造成假差异，同时保留 `-0`/次正规的可分辨性。
- **两条踩到的用例缺口（都属"到达吸附"这一类）**：
  1. **"到达即吸附到目标值"要"差值 < eps 但量化后不等于目标"的样本才有分辨力**：
     `move 0.005 0 0 0.5` 后一次 `update`（`round_float(0.0025, 100)` 把值量化成 0，不吸附就停在 0）。
     原先只有 `move 10 …` 反复 `update` 的用例，压根没走到"到达"⇒ 该变异存活。
  2. **`set_position`/`set_scale`/`set_rotation` 清 `_smoothing` 要"`move` 之后再调 setter"的样本**；
     原先只在 `move` 之后调了**属性** setter（它本来就不清）⇒ 三条"不清 smoothing"的变异存活。


### 4.67 V60 步骤 5 第二片 `Ground`（地形高度 / 阻挡 / 地面碰撞几何）

- **本片**：`native/lfw/ground.{h,cpp}`（TS `src/LFW/Ground.ts`，467 行），并顺带补上它需要的
  `defines/i_terrain_info.h` 的 `ITerrainInfo`。依赖只有已移植的 `clamp`/`abs`/`line_plane_intersection`，
  是 `Entity.ts` 的直接值依赖（`Ground.ts` 自己的值闭包只剩 `fields`/`defines`/`utils`）。
  用例 `cases/ground/*.txt` 3 个共 **310 行**（terrain 129 / edge 100 / edge2 81），差分 3/3，
  变异 **58/58 全杀**（2.33 s/变异）。
- **接口简化（语义等价，逐条记录）**：
  1. TS 构造时吃 `World`、每次调用现读 `world.bg.data.terrain` ⇒ C++ 改成
     `set_terrain(const std::vector<ITerrainInfo>*)` 注入指针，**方法内每次现读**（保留"地形可被整体替换"
     的语义）；harness 侧 TS 用 `{ bg: { data: { terrain } } }` 桩对象、C++ 用同一个 vector 的地址。
  2. `block()`/`intersect()` 在 TS 里返回**复用**的 `_ret`/`_intersectResult` ⇒ C++ 同样复用成员
     （返回 `const&`），不是每次新建。
  3. `enterable(): number | null` ⇒ `std::optional<double>`；`intersect_wall(): {x,z} | null` ⇒
     `std::optional<BlockPoint>`。
  4. `ITerrainInfo` 的 `id?`/`name?`（可缺）⇒ `std::u16string`（缺省空串），**打印时两侧都映射成 `-`**，
     否则"缺省"与"空串"在输出上分不开。
- **"数据形状不单独立片"这条决策在本片落实**：`ITerrainInfo` 是纯数据、没有逻辑 ⇒ 变异无处可打，
  硬开一轮只会重复覆盖默认值与枚举表。**做法是让形状跟着第一个真实消费者一起落地**
  （`IQube`/`IQubePair`/`IBdyInfo` 一族同理，将在 dat 侧或 Entity 的 bdy 判定路径里带出来），
  靠消费者的 subject 去差分验证它。同理暂缓了 `Ticker.ts`：它只有 104 行，但逻辑全在
  `Ditto.Clock`/`Timeout` 的异步调度上，移植它等于先定下整个运行时调度模型（harness 要模拟事件循环），
  性价比远低于先啃 `Ground` 这种纯逻辑依赖。
- **首轮 11 个幸存者：1 条真等价 + 10 条用例缺口**。真等价（已删，证明写进规格头部）：
  `intersect_wall` 的快速跳过 `if (max_h - min_y <= _step) continue;` 改成 `<` —— 能走到那里时若
  `max_h - min_y == _step`，则任意交点都有 `ty <= max_h`（高度函数把 t 夹在 [0,1]，值必落在 [h1,h2] 内）
  且 `iy >= min_y`（交点是两端凸组合）⇒ `ty - iy <= _step` ⇒ 后面四条墙面的 `ty - iy > _step`
  都不可能成立 ⇒ 跳不跳过同值。
- **10 条缺口的根因是同一类：边界样本没有分辨力。**三种典型：
  1. **被跳过的对象必须能改变赢家**：`segment` 的 x 边界样本里，内外两段**同高**（都是 4）⇒ 把内层段
     跳过之后赢家还是外层段，结果不变。改成"内层段更高（8）"立刻可分辨。
  2. **镜像几何会命中另一分支**：想打"右墙（`seg.x2`）分支"，却把 `seg` 的 `x1`/`x2` 对调成镜像 ⇒
     实际命中的是代码里的 `x1` 分支（它按字面用 `seg.x1`），变异打在 `x2` 分支上自然无声。
     正解是保持 `x1 < x2`、让**右端更高**、射线自 `+x` 方向来。
  3. **阈值类要构造"恰好等于"和"恰好跨过"两种几何**：`ty - iy > _step` 需要墙比射线高 10 以上
     （否则顶面命中先落地、墙面分支根本不执行）；`seg_y - y1 <= _step` 需要"表面恰好高出 y1 十"的射线
     （`intersect 0 10 0 0 50 0`：向上穿过 h=20 的平地）。

## 6. 控制器中枢（步骤 5 第三片）

TS `controller/` 里与输入判定相关的部分已移植完毕，落点如下。

### 6.1 常量与查表

`native/lfw/defines/game_key.h` 补齐 `GKLabels` / `AGK` / `CONFLICTS_KEY_MAP` 三张表，以
`gk_label_table()` / `all_game_keys()` / `conflicts_key_table()` 暴露；查表用 `gk_label_of(key)`
（未知键返回空串）与 `conflicts_key_of(key)`（未知键返回 `nullptr`）。

`AGK` 的**顺序是语义的一部分**：`BaseController::update()` 的按键扫描按该顺序判定，先命中者生效。

### 6.2 `ControllerKeyStatus`

`native/lfw/controller/controller_key_status.{h,cpp}`：7 个 `KeyStatus` 槽（L R U D d j a）。
与 TS 的唯一差异是把 `owner` 引用去掉，时间与 `key_hit_duration` 作为实参传入，于是
`is_hit(time, dur)` 可以脱离控制器单独差分。

TS 的快照是 `number[][]`，喂短数组会直接抛错；C++ 侧对越界做了守卫，所以**短快照属于不可差分观测**，
只测完整 7×3。

### 6.3 `ControllerResult`

`clear` / `fire` / `fire2`。`fire` 里 `owner.entity.get_next_frame(nf)` 换成 `set_resolver` 注入的
`std::function`。

⚠️ 守卫必须是 **JS 真值判定**：`if (!truthy(result)) return false;`。TS 写的是 `if (!result)`，
于是 `fire(nf = 0)` 必须失败；只查 `monostate` 会漂移（本轮由差分抓到并修正）。

### 6.4 `BaseController`

`native/lfw/controller/base_controller.{h,cpp}` 是主控全链。实体/世界读面用 `CtrlEnv` 注入：
`facing` / `hp`→`alive` / `team` / `position` / `frame.state` /
`frame.{hold,hit,key_down,key_up,__seq_map}` / `data.{pre_hitkeys,post_hitkeys,__*_hitkeys_map}` /
`transforms[0].__*_hitkeys_map`；动作面注入 `get_next_frame` / `world_etc` / `team_come` /
`team_stay` / `team_move` / `team_follow`。

`is_human_ctrl` / `is_bot_ctrl` 按 TS 语义落成 `set_kind(human, bot)`，**默认都是 false**
（TS 要求 `__is_*_ctrl__ === true`）。

`update()` 的判定顺序严格照抄：

1. 队列（UP / DOWN / HOLD；DOWN 只在 `is_end` 时受理，并顺手清冲突键的 `dbc`）
2. 相对方向 `kd` / `ku`
3. `pre_hitkeys` / `hit` / `post_hitkeys` 的单击（F / B）
4. 同三张表的双击（FF / BB，**不受** `ret.time` 保护）
5. `hld`（F / B）
6. 按 `AGK` 顺序的单键 `check_key_act`（kd / ku / pre / hit / post / hld）
7. 三张 seq 表：`transforms[0].__pre ?? data.__pre`、`frame.__seq_map`、`transforms[0].__post ?? data.__post`
8. 按键链长度 ≥ 10 时清空

必须保留的两个 TS 怪癖：`??` 是 **nullish** 合并（空对象也算「有值」）；`is_db_hit` 的站立判定
两侧都写 `d0`（不是 `d1`）。

`LR` / `UD` / `jd` 返回 `int`，而取负的 `RL` / `DU` / `dj` 返回 `double`：
JS 的 `-0` 与 `0` 在 `num_hex` 下可区分，用整数取负会把负零丢掉。

## 7. 步骤 6 的叶子单元：碰撞处理器

`src/LFW/collision/` 目录里，核心的两个文件（`Collision.ts` 312 行、`CollisionKeeper.ts` 320 行）
以及绝大多数 `handle_*` 处理器都要读写 `Entity` / `World` / `LFW` 的运行状态；
用 `native/tools/ts_scope.mjs --rank` 量过三处闭包：

| 目录 | 代表文件 | 未移植值闭包 |
|---|---|---|
| `src/LFW/collision` | `Collision.ts` / `CollisionKeeper.ts` | 需 Entity 缝（本次只取闭包为 0–31 行的叶子） |
| `src/LFW/state` | `State_Base.ts` | 1348 行 |
| `src/LFW/buff` | `Buff.ts` | 932 行 |

结论：步骤 4/5/6 的**核心**不是可以像 `Transform` / `Ground` 那样窄缝隔离的单元，
它们要么等 `Entity` / `World`（步骤 7/8）落地后再做，要么用一层很厚的缝硬包
——后者会把「被测逻辑」和「被注入的行为」搅在一起，反而降低差分与变异的意义。

本次先把闭包最干净的一批叶子处理完整落地，作为步骤 6 的先头单元：

- `native/lfw/collision/handlers.{h,cpp}`：`handle_stiffness` / `handle_body_goto` /
  `handle_super_punch_me` / `handle_weapon_picked` / `handle_rest` / `handle_itr_kind_magic_flute`
- 实体/世界动作面用 `HandlersEnv` 回调注入（与 `BaseController` 的 `CtrlEnv` 同型），
  harness 记录**调用序列 + 落地值**。差分校验的是处理器的判定逻辑与调用顺序，
  `add_v_rest` / `pick` / `Buff` 的内部实现留给后续切片。

### 7.1 必须保留的惰性

TS 里 `itr.motionless ?? attacker.itr_motionless` 与 `itr.arest || world.dataset.itr_arest`
都要求**惰性**：右侧只在左侧缺失/为假时才求值。C++ 侧若写成 `coalesce(a, b)` 这类
「先求值再选」的辅助函数，回落回调会被无条件调用 —— 差分直接抓到
（harness 的日志里多出一次 `get_itr_motionless`）。必须写成显式分支。

另外注意 `??`（nullish：`undefined` / `null` 才回落）与 `||`（falsy：`0` / `""` 也回落）
在同一个文件里混用：`handle_stiffness` 用 `??`，`handle_rest` 用 `||`。

## 8. `Buff` 基类（步骤 4 主体）

`native/lfw/buff/buff.{h,cpp}` 落地了 TS `buff/Buff.ts` 的全部状态机：

- 生命周期：`_ticker`（每帧 `add(d)`）与 `_lifetime`（`Times(0,1).set_lifes(1)`）两个计时器，
  `update(d)` 按 TS 顺序执行 `on_update` → `on_tick`（`add(d)` 为真时）→ `on_end`（`add()` 为真时）→ `update_effects()`。
  施放者只在**第一次**需要时解析（`attacter()`），三个 hook 共享同一次解析结果。
- 受害者表：`set_victim` / `add_victim` / `del_victim`，以及 `_del` 的快/慢指针原地压缩
  （保留 TS 的 `slow < fast` 搬移写法）。
- 特效实体：`show_effect` / `update_effects` / `place_effect(_center)` / `del_effect` / `clear_effects`，
  特效帧用 `defines/gone_frame_info.h` 的 `gone_frame_info()`（与 TS 的 `GONE_FRAME_INFO` 逐字段一致）。
- 快照：`to_snapshot` / `read_snapshot`，其中两个 `Times` 以 `{nums:[5]}` 包装（TS 的 `ITimesSnapshot`）。
- `grant_buff`：`world.buffs.get(id) ?? factory.create_buff(kind, lfw, id)`，再复位寿命、设时长、加等级、
  绑施放者/受害者并 `mount()`。

单元边界：`IBuffEntity`（受害者的读面 + 动作，含特效实体的 `outline_*`/`enter_frame_by_id`/`attach`）
与 `BuffEnv`（`find_entity` / `create_entity` / `find_data` / `world_buffs_set/get` / `create_buff`）。

### 8.1 本单元抓到的两个真实端口 bug

1. **覆盖语义**：TS `show_effect` 用 `this._effects.set(victim.id, effect)`，是**同键覆盖**；
   C++ 侧写成 `emplace_back`（追加），同一受害者会留两条记录。差分直接在 `nfx` 上暴露（2 vs 1）。
   已改为「同键替换、否则追加」，并补了针对它的变异。
2. `_effects` 用 vector 承载，覆盖语义必须手写 —— 不能指望容器帮忙。

### 8.2 harness 教训：别名 vs 拷贝

TS 的 `Map.get` 返回**同一个对象**。C++ harness 若写 `g_buff = *it->second`（拷贝），
会出现「改了副本、容器里没变」的假漂移（第二次 `grant` 后 `dead` 不一致）。
正确做法是保留 `TestBuff* g_sync`，每次操作结束把 `g_buff` 回写到 `*g_sync`。

另外：特效实体 id 是运行时生成的（`E1/E2/…`），静态用例拿不到，故 harness 约定 `env entity "@"`
表示「最近创建的实体」。

## 9. 切片 6 核心：`Collision`（碰撞对象与判定）

`native/lfw/collision/collision.{h,cpp}` 落地 TS `collision/Collision.ts`：
`collision_new` / `collision_get` / `collision_test` / `collision_to_snapshot` /
`collision_from_snapshot` / `collision_clone`。

### 9.1 单元边界
- `CollisionActor`：**值结构**读面（id / 位置 / data_id / data_type / frame /
  itr_prefabs / bear 的 wpoint.attacking / marks / dropping / arest / catcher(hurtable) /
  invulnerable / bot_ignore / team / emitter / spawn_time / is_bot_ctrl）。
- `CollisionCoreEnv`：动作与外部世界（get_bounding / dataset / get_v_rest / is_ally /
  acquire_collision / new_id / dev / log / tester_run / tester_debug / find_entity /
  find_object_data / priority_of / load_handlers）。
- `HandlersEnv` 与 `Collision` 结构上移到 `collision.h`（`handlers.h` 只 include 它），
  使两个单元共用同一个碰撞对象，且 `handlers.cpp` 无需改动。

### 9.2 三处必须保真的表示
1. `acquire_collision` 返回 **`Collision&`**：TS 是对象池复用，复用对象上「上一轮的残留字段」
   本身可观测，不能改成每次新建。
2. `handlers` 用 `shared_ptr<vector<u16string>>`：TS `{...src}` 中该数组是**浅拷贝引用**，
   用 `vector` 会变深拷贝，`collision_clone` 之后「一边 `length = 0`、另一边也变空」就没了。
3. `priority` 是 `Value` 而非 `double`：`ENTITY_PRIORITY_MAP` 对未列出的实体类型返回
   `undefined`，用 `double` 无法表示。

### 9.3 保留的 TS 怪癖
`collision_from_snapshot` 里查 `bdy` 用的是 **`aframe.bdy`**（不是 `bframe.bdy`）。
用例专门构造了「FA 有 bdy[0]/bdy[1]、FB 只有 bdy[0]，且 bdy[0] 的 hit_flag 决定性不同」
来锁住它：把这里「改对」会立刻被差分打回。

### 9.4 本轮抓到的两个真实端口 bug
1. **`index_by` 只认对象**：TS 里 `frames[id]` 与 `itr[i]` 都走 JS 属性访问，
   数组也能用数字字符串下标；C++ 首版只处理 `Object`，于是 `from_snapshot` 一取 `itr`/`bdy`
   就返回 null（差分表现为 `from_snap no` vs `yes`）。现已支持 `Array` + 数字下标，
   并显式做 `!(index >= 0)` 以避开 NaN 转型。
2. **`dataset` 不是 `Collision` 的字段**：TS 只在 `collision_new` 内部解构
   `world.dataset` 用于算 `rest`，从不写回碰撞对象。首版多写了 `c.dataset = dataset;`，
   被差分抓到（对象字段多出一个键）。`Collision::dataset` 保留，但只作为 handlers 单元
   的行为缝字段、由那个单元的 harness 直接设置。

### 9.5 「看起来必然可观测」的三条变异其实不可观测
- `if (!truthy(prefab_id)) itr_index = -1;` 取反 **以及** 整段短路：
  `prefab_id` 为假时，紧随其后的 `index_by(a.itr_prefabs, to_string(prefab_id), ...)`
  必定同样失败（拿不到 `"undefined"` 键），两条路径都得 `-1`。
- `truthy(v.emitter)` → `truthy(a.emitter)`：同一条 `&&` 链上，
  `strict_equals(v.emitter, a.emitter)` 在两者真假性不同时必为 false ⇒ 结果恒等。
- `get` 的 `if (!ni || !nj) return nullptr;` → `&&`：纯提前退出，
  任一侧为空数组时循环体一次都不执行，返回值仍是 `nullptr`。
- `get` 双层循环次序反转：成对成立性 = 两侧各自谓词的合取，
  可通过的组合构成乘积集，其字典序最小元在两种遍历次序下相同。
这些都已写进 `mutations/collision_core.mjs` 头注（连同证明），不列入变异集。

## 10. 切片 6 核心：动作表 `collision_action_handlers`

`native/lfw/collision/action_handlers.{h,cpp}` 落地 TS `entity/collision_action_handlers.ts`：
一个按动作类型字符串分发的表，21 种动作全部覆盖（`A/V_SOUND`、`A/V_NEXT_FRAME`、
`A/V_SET_PROP`、四个「防御」项、`A/V_REBOUND_VX`、`V_TURN_FACE`、`V_TURN_TEAM`、
`FUSION`、`BROADCAST`、`VALUE_STEAL`、`A/V_BUFF`（含 `apply_buff`）、`ERROR`、`NONE`）。

### 10.1 单元边界
- `IActionEntity`：读面 + 动作（hp/hp_r/hp_max/mp/mp_max、velocity_x、facing、team、
  data/data_type、src_emitter/emitter/bearer、fuse_bys、dismiss_data/dismiss_time、
  invisible/motionless/invulnerable、play_sound/enter_frame/transform/set_prop、
  `buff_entity()` 供 `grant_buff` 使用）。
- `ActionEnv`：`mt_int`/`mt_set_mark`/`find_data`/`broadcast`/`alert`/`find_entity`/
  `is_ally`/`buff_env`。
- `run_action(env, type, action, attacker, victim, injury, real_injury)` 返回该动作的返回値
  （四个防御项返回 `0`，其余返回 `undefined`）。

### 10.2 从真源码里挖出来的三处约定（都靠差分打出来）
1. `play_sound(sounds, pos = this.position)` 的默认位置在**实现侧的参数默认值**上，
   调用点原样传 `a.data?.pos`（可能是 `undefined`）。端口因此把默认值交给 `IActionEntity`
   的实现去处理，而不是在分发处补。首版在调用点补默认值，被差分立刻打回。
2. `fighter_2.invisible = fighter_2.motionless = fighter_2.invulnerable = 1000000`
   是**从右往左**求值的：先 `invulnerable`、再 `motionless`、最后 `invisible`。
   端口按同样的顺序调用 setter（顺序本身可观测）。
3. `is_bot_ctrl(attacker.ctrl)` 读的是实体上的 `ctrl.__is_bot_ctrl__`，
   不是实体自身的某个布尔字段；`VALUE_STEAL` 的 `target` 是 `switch`：
   `0`/其他 → 攻击者自身、`1` → `src_emitter`、`2` → `emitter`、`3` → `bearer`，
   取不到就整体放弃；`over_hp_r` 决定 hp 的钳制上限是 `hp_r` 还是 `hp_max`。

### 10.3 `apply_buff` 的两级判定
`hitflag` **只有是数字时**才做判定：先 `hitflag & victim.data.type`（类型位），
再 `hitflag & (attacker.is_ally(victim) ? Ally : Enemy)`（队伍位）；
是字符串（或缺失）则跳过两级，直接施放。`duration`/`buff` 用解构默认值 `0`/`''`。
这一点在写用例时必须显式构造队伍位，否则「队伍位判定取反」这类变异全部存活。

### 10.4 harness 观测量教训
- 无状态动作（`play_sound`/`enter_frame`/`transform`）的日志必须带**实体身份**
  （`A:play_sound:…`），否则「打到受击方」这类主客互换完全不可观测。
- 两侧假实体的 `data` 必须能区分主客（这里给了 `id: "DA"`/`"DV"`），
  否则 `FUSION` 的 `dismiss_data`/`hp_r` 取值错误不可观测。
- 数值边界要挑在**钳制会生效**的地方：`hp_r` 的上限、`mp` 的封顶、
  `hp_max` 与 `hp_r` 的大小关系，都要让「上限取错」真的改变结果。

## 11. 切片 6 核心：`handlers2`（受伤 / 抓取 / 冰冻 / 护盾）

`native/lfw/collision/handlers2.{h,cpp}` 落地 5 个 handler：
`handle_injury`（含 scale / keep_toughness 参数与「施放者转嫁」）、`handle_itr_catch`、
`handle_itr_kind_freeze`、`handle_itr_effect_freeze`、`handle_john_shield_hit_other_ball`。

### 11.1 单元边界
- `IHandlerEntity`：接口式读面 + 动作（hp/hp_r/toughness、fall_value(+max)、
  defend_value(+max)、resting、catch_time(+max)、catching/catcher、itr_fall、
  src_emitter、shaking/motionless、set_velocity、enter_frame(_by_id)、play_sound、
  `data()`、`dataset(key)`、`marks_has(kind)`、`buff_entity()`）。
- `Handlers2Env`：`warn` / `hp_recoverability` / `find_entity` / `summary_apply_damage` /
  `buff_env` / `is_fighter` / `calc_velocity`。
- 缝用**模块级全局**（`set_handlers2_env`），因为真源码里这批 handler 依赖的是模块级单例
  （`summary_mgr`、`Ditto`、`world.dataset`），而且 `Collision` 结构属于已冻结单元，不动它更安全。

### 11.2 保真要点（都由差分逼出来）
1. **JS 参数默认值发生在被调函数里**：TS 写 `grant_buff(KIND, a, v, a.dataset('electrify_duration'))`，
   当该值为 `undefined` 时由 `grant_buff(duration = 0)` 兜成 `0`。端口不能直接传 `to_number(...)`
   （会得到 `NaN`），必须在**调用点**补默认：`missing(x) ? 0 : to_number(x)`。
2. `is_fighter(victim)` 读的是**实体自身** `data.type === Fighter`，所以假实体的 type 必须
   按实体分开，否则「看错实体」这种变异不可观测。
3. `handle_itr_catch` 调的是**方法** `set_catch_time(...)`（不是属性 setter）。
4. `find_entity` 与 `attacker_itr_motionless` 是**缝的实现细节**（TS 侧分别是直接引用与属性直读），
   两侧都必须静默，否则会造出假差异。
5. `calc_itr_velocity` 在本单元是注入的（返回固定分量），因此用例必须让 TS 侧的真算式
   恰好算出同一组值：`dvx*ivx_f*facing/weight`、`(!is_fall ? 0 : dvy*ivy_f/weight)`、
   `dvz*ivz_f/weight`；`is_fall` 还会看 `hp <= 0`，所以打伤之后要把 hp 还原再测冰冻。

### 11.3 观测量教训
- 「抖动 = 无动作值」这类变异要求 `itr.motionless` 在同一个 handler 里被显式设置：
  `handle_john_shield_hit_other_ball` 先跑 `handle_stiffness`，会把 `attacker.motionless`
  重新写成 `itr.motionless ?? itr_motionless`——用例不设 `itr.motionless` 就永远是 `undefined`，
  两侧都取不到差异。
- 「`injury_r` 为 0 也写」这类分支要用 `hp_recoverability = 1` 构造出 0 才能杀。

## 12. `collision/handlers3`：旋风、球受击与护甲

`native/lfw/collision/handlers3.{h,cpp}` 落地 4 个 handler：`handle_itr_kind_whirlwind`、
`handle_ball_is_hit_a`、`handle_ball_is_hit_b`、`handle_armor`（唯一带回返回值的一个）。
另外新增 `native/lfw/defines/collision_defaults.h`，镜像 `Defines.ts` 里被本单元用到的
6 个常量：`DEFAULT_FALL_VALUE_DIZZY/CRITICAL`、`DEFAULT_FORCE_BREAK_DEFEND_VALUE`
以及三个护甲比率。

### 12.1 单元边界
- `IH3Entity`：速度/位置读写、`team`、`bearer`/`base_type`/`state`、
  `data_in_the_skys_first()`、`data_base_hit_sounds()`、`armor`、`toughness(+_max)`、
  `itr_fall(itr)`、`dataset(key)`、`set_motionless`/`set_shaking`、
  `enter_frame_by_id`、`play_sound`、`buff_entity()`。
- `Handlers3Env`：`find_entity` / `is_ball` / `is_weapon` / `spark_point(a_cube, b_cube)` /
  `spark` / `play_sound_global` / `is_armor_work`；与 `handlers2` 一样用**模块级全局缝**
  （`set_handlers3_env`）。
- `handle_armor` 结尾会调 `handlers2` 的 `handle_injury`，所以假实体必须**同时实现
  `IH3Entity` 与 `IHandlerEntity`**，用例也要同时装好 `set_handlers2_env` + `set_handlers3_env`。

### 12.2 保真要点
1. **临界值比较不统一**：两个 `ball_is_hit` 用的是严格 `>`，而 `handle_armor` 用的是 `>=`
   （`fall >= DEFAULT_FALL_VALUE_CRITICAL`）。端口用同一个 helper 上的 `inclusive` 参数区分，
   绝不「顺手统一」。
2. **`victim.hp = victim.hp_r = 0` 是右到左赋值**：先写 `hp_r` 再写 `hp`，顺序可观测。
3. **JS 解构默认值只认 `undefined`**：`fall = attacker.itr_fall(itr)`、`dead_sounds = hit_sounds`、
   三个护甲比率都属这类语义。所以端口里的 `missing()` 只判定 `monostate`，`null` 要当真值
   继续参算（`to_number(null)` 恰为 0，与 JS 一致）。
4. `handle_armor` 的 `fall` 缺省来自 `attacker.itr_fall(itr)`，其真身是
   `itr.fall ?? dataset('itr_fall')`；端口只在 `itr.fall` 缺失时才去问实体。
5. 护甲那两处 dataset 兜底（`itr_shaking` / `itr_motionless`）读的是**实体**上的
   `dataset()`，与 `handle_stiffness` 读**碰撞体** `dataset` 不是同一个来源。

### 12.3 观测量教训
- **harness 的 `env a|v` 行只消费一个字段**，多余 token 会被静默丢弃。第一版用例把
  `posx/posz`、`velx/vely/velz`、`hp/hp_r` 写在同一条 `env` 行里，结果 z 方向恒为 0，
  三条只影响 `vz` 的变异因此存活。现已改为「一行一字段」，并让两侧 harness 在
  `walk_side` 之后检查 `i == 行 token 数`，否则直接报错退出——这类静默丢字段
  比变异存活更难发现。
- 方向类变异要靠**符号组合**杀：用例必须同时覆盖 `(+x,+z)`、`(-x,+z)`、`(+x,-z)` 三种位移，
  否则 `normalize(dx)`/`normalize(dz)` 互换这类变异不可观测。
- `is_ball` / `is_weapon` 复用了 `entity/entity_type_check` 的 `*_data(data)`，所以假实体要按
  各自的 `data.type` 区分（Fighter 8 / Weapon 16 / Ball 32），「看错实体」才可观测。

## 13. `collision/handlers4`：球击中他人与武器击中他人

`native/lfw/collision/handlers4.{h,cpp}` 落地 `handle_ball_hit_other` 与
`handle_weapon_hit_other`，对应 `src/LFW/collision/handle_ball_hit_other.ts` 与
`handle_weapon_hit_other.ts`。

### 13.1 单元边界
- `IH4Entity`（独立接口，**不**继承 `IH3Entity`）：`id`、`data()`、`hp(+_r)` 读写、`state`、
  `facing`、`base_type`、`velocity`/`set_velocity`、`frame_id()`、
  `data_indexes_throwings()`、`data_indexes_in_the_skys()`、`arest()`/`set_arest`、
  `enter_frame(info)`、`set_dropping(bool)`、`data_base_hit_sounds()`、`play_sound(sounds)`。
- `Handlers4Env`：`find_entity` / `is_fighter` /
  `find_align_frame(frame_id, throwings, in_the_skys)`；同样是**模块级全局缝**
  （`set_handlers4_env`）。
- 两个函数各自还要 `handlers` 的 `handle_rest` + `handle_stiffness`，所以用例必须同时装好
  `set_handlers_env`（`handlers` 缝）与 `set_handlers4_env`。

### 13.2 保真要点
1. **`attacker.hp = attacker.hp_r = 0` 复用 `handlers3` 的右到左语义**：`zero_hp()` 先写
   `hp_r` 再写 `hp`，日志顺序可观测。三处调用点（Normal / 破防 / 同朝向）共用它。
2. **`bdefend` 的 `truthy && >= 200` 里 `truthy` 是死代码**：JS 的 `>=` 会先 `ToNumber`，
   凡是数值 ≥ 200 的值都不可能是假值；其余假值（`0` / `-0` / `""` / `null` / `undefined` /
   `false` / `NaN`）要么数值 < 200，要么 `NaN`——而端口的 `ge` 在 `ToNumber` 得到 `NaN` 时
   返回 false（`less_than` 回传 `std::nullopt`）。端口照抄该前缀以保持逐行一致，并在变异
   规格头部记录为不可观测。
3. **`Weapon_OnHand` 前置守卫是冗余的**：TS 原文就是「先 `if (state === Weapon_OnHand) return;`
   再 `switch (state) { case Weapon_Throwing: ... }`」这种同构双层结构，端口按 `if` + `if` 落地。
   因为 `1001 !== 1002`，去掉第一层守卫对任何单一 `state` 值都不可观测；只有让它**在其他
   `state` 上误触发**（取反，或把 `Weapon_OnHand` 换成 `Weapon_Throwing`）才可观测。
4. **`-0.3 * vx` 必须保住负零**：`vx = 0` 时结果是 `-0`，与 `0` 的 `render` 不同，用例专门
   覆盖了 `velx = 0`。
5. **`arest` 是先取快照再回写**：`const arest = a->arest(); a->enter_frame(nf);
   a->set_arest(arest);` 顺序不可调换，否则 `enter_frame` 对 `arest` 的副作用会被覆盖。
   差分日志把 `enter_frame` 与 `set_arest` 都打印出来，所以顺序类变异可观测。
6. **`handle_ball_hit_other` 结尾的 `play_sound` 在 `switch` 之外**：非 JohnChase 的
   `behavior`、非 Fighter 的受害者都不影响它发声。
7. TS 的 `switch (aframe.behavior)` 里 `DennisChase` … `JulianBall` 一批 `case` 的 body 只有
   一个 `break`，与「整段省略」语义完全等价，端口按省略落地；变异规格头部记录了这一点。

### 13.3 harness 观察点
- `env a|v` 沿用「**一行一字段**」硬约束（多余 token 直接报错退出），字段：`hp` `hp_r`
  `state` `facing` `base_type` `velx` `vely` `velz` `frame_id` `throwings` `in_the_sky`
  `arest` `dropping` `hit_sounds` `type`。
- 假 `find_align_frame` 把三个入参全部打进日志
  （`faf:<frame_id>:<throwings>:<in_the_skys>`）并固定返回 `"faf_result"`，这样「参数接错」
  与「`enter_frame` 实参写死」两类变异都能被杀。
- `handlers` 缝的 `attacker_set_arest(double)` 也必须**回写假实体的 `arest`**：TS 侧
  `attacker.arest = ...` 走的就是实体 setter，若 C++ 侧只打日志不回写，实体状态行会停在
  `undefined`，造成纯 harness 建模差异（首轮差分就是这么失败的）。

## 14. `collision/weapon_is_hit`：武器被命中

`native/lfw/collision/weapon_is_hit.{h,cpp}` 落地 `handle_weapon_is_hit`
（对应 `src/LFW/collision/handle_weapon_is_hit.ts`）。

### 14.1 单元边界
- `IWeaponIsHitEntity : IHandlerEntity`：在 handlers2 的实体缝之上补 `has_bearer` /
  `set_dropping` / `base_type` / `facing` / `team`+`set_team` / `data_id` /
  `data_indexes_throwings` / `data_indexes_in_the_skys` / `leave_ground`。这里**继承**而不是
  另开接口，因为本单元前后要串 `handle_rest` / `handle_stiffness` / `handle_injury`，
  而 `handle_injury` 直接吃 `IHandlerEntity`。
- `WeaponIsHitEnv`：`find_entity` / `spark_point(a_cube, b_cube)`（返回 `SparkPoint` 三值结构，
  免得把 JS 的元组展开搬进端口）/ `spark(x, y, z, kind)` / `mt_mark` / `mt_pick` /
  `calc_velocity`；模块级全局缝（`set_weapon_is_hit_env`）。

### 14.2 保真要点
1. **`calc_itr_velocity` 是注入缝**：TS 里 `handle_weapon_is_hit` 直接 import 它，端口把它挂到
   `WeaponIsHitEnv` 上。于是差分两侧必须满足「真算式 == 注入值」：harness 把 `weight` 与
   `ivx_f/ivy_f/ivz_f` 固定为 1，用 `env velx/vely/velz` 当作 `itr.dvx/dvy/dvz`，并让假
   `calc_velocity` 复刻 `x_direction = attacker.facing` 这个因子——否则「朝向 -1」的用例会漂移。
2. **`is_fly` 在 TS 里并不是布尔**：`itr.fall && itr.fall >= D.DEFAULT_FALL_VALUE_CRITICAL`
   在 `itr.fall` 为 `undefined` 时返回 `undefined`。端口收成 `bool`，只参与真值判断，语义等价。
3. **临界值是 100 而不是 140**：`DEFAULT_FALL_VALUE_CRITICAL = 140 - DEFAULT_FALL_VALUE_DIZZY = 100`。
   用例必须卡在 99/100；卡在 139/140 时 `is_fly` 的三条变异会全部不可观测（本轮就是这么栽的）。
4. **`truthy(itr.fall)` / `truthy(itr.bdefend)` 前缀是死代码**：JS 的 `>=` 先 `ToNumber`，数值一旦
   达到阈值必然是真值；其余假值要么低于阈值、要么是 `NaN`（而端口的 `ge` 遇 `NaN` 返回 false）。
   端口照抄以保持逐行一致，并在变异规格头注里记录为不可观测。
5. **`victim.set_velocity(vx)` 只写 x**：TS 的 `Entity.set_velocity` 会跳过 `null/undefined`
   分量，并在末尾 `if (this.velocity.y > 0) this.leave_ground()`。harness 的假实体复刻了这两条
   （日志只列实际写入的分量；y > 0 时多一次 `leave_ground`），否则「三参写一参」不可观测。
6. **`data.indexes?.throwings?.[0]` 要区分数组 / 对象 / 缺失**：端口的 `index_0` 依次尝试
   `as_array` → `as_object` → `undefined`，空数组提前返回 `undefined`，所以「数组下标」与
   「对象键」两条分支都能挂变异，而空数组不会越界。

### 14.3 harness 观察点
- `env a|v` 一行一字段，字段：`hp` `hp_r` `tough` `state` `face` `base_type` `team` `bearer`
  `drop` `data_id` `throwings` `in_the_sky` `velx` `vely` `velz` `on_ground` `fall` `fall_max`
  `defend` `resting` `type` `src_emitter` `ice` `hit_sounds` `motionless` `dataset`。
- `env acube` / `env bcube`、`env itr`、`env dataset`、`env rest`、`env recov`、`env itr_motionless`。
- `env velx/vely/velz`、`env rest`、`env recov` 用**裸数字**（与 handlers2 的约定一致），其余数值
  一律走值语法 `n ...`。首次跑差分时把 `env velx n 7` 写成了值语法，结果两侧各错各的
  （C++ `strtod("n") = 0`、TS `Number("n") = NaN`），直接漂移。
- 受害者必须是**非 Fighter**（`env v type n 16`），这样 TS 的真 `is_fall()` 才为真，
  `calc_itr_velocity` 的 y 分量才等于注入值。

## 15. `collision/fall`：倒地

`native/lfw/collision/fall.{h,cpp}` 落地 `handle_fall`（对应 `src/LFW/collision/handle_fall.ts`）。

### 15.1 单元边界
- `IFallEntity : IHandlerEntity`：补 `facing` / `velocity_x` / `spark_point(a, b, out x, y, z)` /
  `data_indexes_fire` / `data_indexes_critical_hit` / `holding_base_type` / `drop_holding`。
- `FallEnv`：`find_entity` / `is_fighter` / `spark` / `calc_velocity`；模块级全局缝
  （`set_fall_env`）。
- 本单元不调用其他 handler，只额外依赖 `entity/face_helper` 的 `turn_face`（已由
  `entity_helpers` 门禁覆盖）。

### 15.2 保真要点
1. **`is_critical` 读的是 `itr.fall`**，与实体自身的 `fall_value` 无关——后者在本函数开头就被清零。
2. **`victim.velocity.x / victim.facing` 的除法语义必须保住**：`0 / 0` 是 `NaN`，而 `NaN >= 0`
   为 false ⇒ 方向 -1；`±x / 0` 是 `±Infinity` ⇒ 方向由符号决定。用例专门覆盖了 `0/0`、
   `±/0` 以及 `facing = 0` 这几种组合。
3. **`x_direction` 同时扮演两个角色**：它既是速度方向，也是 `enter_frame` 的朝向。
   `Fire/MFire2` 用 `turn_face(x_direction)`，`MFire1/FireExplosion` 直接用原值——两处不能
   「顺手统一」。
4. **`FireExplosion`/`Explosion` 会让 `calc_itr_velocity` 走 position-based 分支**：此时
   `x_direction` 恒为 -1（假实体两侧位置都为 0，`diff_x` 既不 > 0 也不 < 0）。harness 的
   `calc_velocity` 缝复刻了这段判断，否则 effect = 22 的用例会漂移。
5. **`let effect = SparkEnum.Hit` 是死初始化**：紧随其后的 `if / else if / else if / else`
   穷尽了所有情况，初值必被覆盖。端口照抄以保持逐行一致，并在变异规格头注记录为不可观测。
6. **`critical_hit[direction]` 缺键时 TS 会抛 `TypeError`**（`undefined[0]`），整个碰撞判定中断；
   端口不允许抛异常，所以 `index_by` 返回 `undefined`。这是**有意的行为分歧**，用例也因此不构造
   缺键场景——那条路径在 TS 侧不是一行可比对的输出。
7. **`data.indexes.critical_hit` 既可能是对象也可能数组**：真实数据是
   `{[1]: [...], [-1]: [...]}`，但 JS 的 `obj[key]` 对数组同样成立，所以 `index_by` 按
   「`as_object` 优先、`as_array` 兜底」实现，并采用数组下标语义（负下标越界即 `undefined`，
   不抛）。用例两种形态都覆盖。
8. **`index_0` 的三态**（数组 / 对象键 `"0"` / 缺失）与 `fall.cpp` 内的 `index_by` 配套，
   空数组提前返回，避免「下标写成 1」这类变异退化成越界 UB。

### 15.3 harness 观察点
- `env a|v` 一行一字段：`hp` `hp_r` `tough` `fall` `fall_max` `defend` `resting` `state` `face`
  `type` `velx` `vely` `velz` `fire` `crit` `holding` `ice` `hit_sounds` `src_emitter`
  `motionless` `dataset`。
- `env itr` / `env acube` / `env bcube` / `env dataset`，以及裸数字的 `env velx|vely|velz`
  （`calc_itr_velocity` 缝的注入值）。
- 数值写法沿用既有约定：`face` / `fall` / `hp` 等走值语法 `n ...`，`velx` / `tough` 走裸数字。

## 16. `collision/n_bdy_normal`：普通 bdy 受击

`native/lfw/collision/n_bdy_normal.{h,cpp}` 落地 `handle_itr_normal_bdy_normal`
（对应 `src/LFW/collision/handle_itr_normal_bdy_normal.ts`）。

### 16.1 单元边界
- `INbdyNormalEntity : IFallEntity`——**必须是一条继承链**。本单元要串 `handle_fall`，而假实体若
  同时实现两个各自继承 `IHandlerEntity` 的接口，就会出现两个 `IHandlerEntity` 子对象 ⇒ 纯虚二义。
- `NbdyNormalEnv`：`find_entity` / `is_fighter` / `is_fall(Collision&)` / `spark` /
  `calc_velocity`；模块级全局缝（`set_nbdy_normal_env`）。
- 子处理器**直接调用已闭合的真函数**，不再另开缝：`handle_armor`(handlers3，返回 bool)、
  `handle_injury`(handlers2)、`handle_rest`/`handle_stiffness`(handlers)、
  `handle_itr_effect_freeze`(handlers2)、`handle_fall`(fall)。
- 已移植的 `is_fall` 吃的是 **`Value` 图**，所以缝定义为 `std::function<bool(Collision&)>`，
  harness 现场构造 `{victim:{fall_value, hp, is_on_ground, frame:{state}, data:{type}}}` 再调真函数。

### 16.2 保真要点
1. **`if (itr.effect == ItrEffect.Ignore)` 用的是宽松 `==`**，端口用 `equals()`；而 `switch`
   内部是 `===`，端口用 `strict_equals()`。两者只在「能强转为 10000 但本身不是数字」时分离
   （例如字符串 `"10000"`），用例专门构造了这一条。
2. **三个分组的调用顺序各不相同**，不可统一：
   - `Fire / MFire1 / MFire2 / FireExplosion` → injury → rest → stiffness → fall
   - `Ice` 且 `state === Frozen` → injury → **stiffness → rest** → fall
   - `Explosion / Normal / Sharp / undefined` → 长路径（injury → rest → stiffness → …）
   - 其余 effect（`Through(4)` / `None(5)`）**什么都不做**
3. **`switch` 的 `case void 0` 只匹配 `undefined`**（不含 `null`）⇒ 端口判定 `monostate`。
4. **`id && id.length > 0` 的 `length` 是三态的**：string 是 UTF-16 单元数、数组是元素数、
   其余是 `undefined`（比较结果恒 false）。端口用 `std::optional<double> value_length()` 配
   `has_value() && *len > 0`。注意 `[]` 在 JS 里是**真值** ⇒ 空数组能走到长度判断，「数组长度 +1」
   的变异可观测；而 `""` 是**假值** ⇒ 空字符串被 `truthy(id)` 拦下，字符串长度分支永远只见到
   长度 ≥ 1，「字符串长度 +1」的变异**不可观测**（已记入规格头注）。
5. **`a(r) = Math.floor(r / 50) % 2 > 0 ? 1 : -1`**：用 `lfw::floor` + `std::fmod`（JS `%` 与
   `fmod` 同号）。注意 `r >= 100`（等价于 `fall_value <= 0`）会被 `is_fall` 提前拦走，所以
   「k = 2」只能靠 `fall_max - fall_value` 的差构造；负 `r`（`fall_value > fall_value_max`）也在
   用例里覆盖。
6. **`is_fall` 为真时整段 impact 逻辑被跳过**（`handle_fall` + `return`），因此「非 Fighter
   受害者」永远到不了 `SparkEnum.SilentHit` 那一支——该 `else` 分支在本单元里**不可达**，
   规格头注连同 `&& v_is_fighter` 的不可观测一起记录。
7. **`critical_hit` / `grand_injured` 既可能是对象也可能是数组**，`injured` 是**字符串对**：
   `index_by` 按「`as_object` 优先、`as_array` 兜底」实现，负下标越界返回 `undefined` 而不抛。
8. **impact 尾段之前必须重置 `fall_value`**：`handle_fall` 会把 `fall_value` 清零，而 `is_fall` 对
   `fall_value <= 0` 恒真 ⇒ 不重置就整段尾逻辑被早退跳过（首轮 25 条变异存活全因于此）。

### 16.3 harness 观察点
- `env a|v` 一行一字段：`hp` `hp_r` `tough` `tough_max` `armor` `state` `face` `team`
  `base_type` `bearer` `type` `velx` `vely` `velz` `posx` `posy` `posz` `ground_y` `fall`
  `fall_max` `defend` `resting` `itr_fall` `fire` `crit` `dizzy` `grand_injured` `injured`
  `backhurtact` `fronthurtact` `holding` `ice` `hit_sounds` `src_emitter` `motionless`
  `in_the_sky` `dataset`。
- 另有 `env itr` / `env dataset` / `env bframe` / `env aframe` / `env armorwork 0|1` /
  `env acube` / `env bcube` / `env rest` / `env itr_motionless`，以及裸数字的 `env velx|vely|velz`。
- **日志前缀必须跨缝统一**：C++ 侧本可以给每套 env 各自的 `spark`/`spark_point` 配不同前缀，
  但 TS 侧只有一个 `world.spark` 与一个实体 `spark_point` ⇒ 两条路径共用前缀。只有「同一次 run
  内两条路径互斥」时才安全（本单元 group B 的 `is_fall` 早退与 impact spark 正是互斥）。
- **`handle_armor` 的 TS 实现走 `victim.spark_point()`（实体方法）**，而 C++ 端口走
  `handlers3_env().spark_point` 缝 ⇒ 本 harness 让缝的公式与前缀都对齐实体方法
  （`x = a.left, y = b.top, z = a.near`）。handlers3 自己的 harness 用的是另一套公式
  （`b.left / a.top / b.near`）以便「拿错 cube」可观测，两者不可混用。
- `armorwork` 是 `is_armor_work` 的替身：`armorwork 1` 的用例必须让 TS 侧真函数也返回 true
  （`fulltime` 为真、`bframe.state` 在允许集合内、effect 不是火/冰、`bdefend < 200`、
  `aframe.state !== 3006`）。

## 17. `collision/n_bdy_defend`：bdy 防御

`native/lfw/collision/n_bdy_defend.{h,cpp}` 落地 `handle_itr_normal_bdy_defend`
（对应 `src/LFW/collision/handle_itr_normal_bdy_defend.ts`）。

### 17.1 单元边界
- `INdbdyDefendEntity : INbdyNormalEntity`（只多一个 `defend_ratio()`）+ 模块级 `NbdDefendEnv`
  （`find_entity` / `calc_velocity` / `spark` / `dispatch`）。
- 子处理器直接调真函数：`handle_itr_normal_bdy_normal`、`handle_injury`、`handle_rest`、
  `handle_stiffness`。
- **action 分发做成了缝**：TS 是 `collision_action_handlers[AT.A_NEXT_FRAME](action, collision)`，
  而端口的 `run_action` 会牵进 `ActionEnv` 与 34 个纯虚的 `IActionEntity`；本单元真正要保证的只有
  「分发给哪个 handler key」与「`action.type` 的过滤条件」两件事，handler 本体已由
  `action_handlers` 单元门禁。所以端口写为 `g_env.dispatch(handler_type, action)`，harness 两侧
  都打成 `dispatch:<handler_type>:<render(action)>`。

### 17.2 保真要点
1. **`const { bdefend = DEFAULT_BREAK_DEFEND_VALUE } = itr` 的默认值只在 `undefined` 时生效**
   ⇒ 端口用 `missing()`（只认 `monostate`），默认值 **32**；破防阈值是 **200**。
2. **守卫是德摩根前的三段式**：`(effect !== FireExplosion && effect !== Explosion &&
   attacker.facing === victim.facing) || bdefend >= 200`。三个 `!==` 是**严格**比较；端口写成
   `explosive = (effect === FireExplosion || effect === Explosion)` 之后即
   `(!explosive && same_face) || bdefend >= 200`。
3. **爆炸类伤害无视朝向**：`FireExplosion` / `Explosion` 会让第一个括号整个为假，于是**同向**的
   爆炸类伤害也会走防御分支（只要 `bdefend < 200`）。
4. **`victim.defend_ratio` 是 `this._defend_ratio ?? world.dataset.defend_ratio`**（`??` 把 `null`
   也算缺失），并且它被当作 `handle_injury` 的 **scale**（`keep_toughness` 取默认值 `false`）。
5. **`const [vx] = calc_itr_velocity(collision); if (vx) victim.set_velocity(vx / 2);`**：只写 x
   （y/z 传 `undefined`），且 `if (vx)` 对 `0` / `-0` / `NaN` 全为假。
6. **两个分支对 `bdy.actions` 的过滤条件不同**：破防时看 `A_BROKEN_DEFEND` / `V_BROKEN_DEFEND`，
   未破防时看 `A_DEFEND` / `V_DEFEND`；`itr.actions` 两边都看 `A_DEFEND` / `V_DEFEND`。
   这是本单元最重要的变异目标。
7. **`actions` 只按数组迭代**：TS 用 `actions?.forEach`，`undefined` / `null` 会短路，但**其它非数组
   值会抛 TypeError**（`(5)?.forEach` 不是函数）。端口用 `as_array` 守卫 ⇒ 对非数组值**宽容**。
   这是有意的行为分歧，用例只用数组与 `null` / 缺失。
8. 破防分支会把 `defend_value` 夹到 `0`（递减后可能已经是负数）。

### 17.3 harness 观察点
- 沿用 `n_bdy_normal` 的全套 `env a|v` 字段，另加 `defend_ratio`；`env` 侧另加 `env bdy`。
- 一次 run 只会走「守卫移交 → `handle_itr_normal_bdy_normal`」或「防御路径（破防 / 未破防）」其中
  一条，三者的日志形态区别明显。
- **每条防御用例之前都要重置 `defend`**：递减会累积，不重置就会从「未破防」掉进「破防」
  （本轮 3 条变异存活全因于此）。

## 18. `collision/ball_frozen`：冻结飞球

`native/lfw/collision/ball_frozen.{h,cpp}` 落地 `handle_ball_frozen`
（对应 `src/LFW/collision/handle_ball_frozen.ts`）。

### 18.1 单元边界
- `IFrozenEntity`（**独立接口**，不继承 `IHandlerEntity`）+ 模块级 `BallFrozenEnv
  { is_ball, is_fighter }`。
- 与其它 collision 单元不同，本单元**没有被调用的子处理器**：全部是纯判定 + 一次 `spawn` +
  一次 `enter_frame`，所以不需要 `IHandlerEntity` 那一族。
- `is_ball` / `is_fighter` 做成缝：端口侧用 `lfw::entity::is_ball_data` / `is_fighter_data`
  作用在 `e.data()` 上，TS 侧直接调真函数。

### 18.2 保真要点
1. **模块级可变 opoint**：`freeze_ball_opoint` 是 TS 的模块级单例对象，只有 `x` / `y` / `z`
   三个字段会被重写。端口用 `static Value`（持 `shared_ptr<Object>`）复刻，于是连续两次调用
   共享同一个对象；字段顺序也照抄（`oid,kind,x,y,action,z`）。
2. **比较运算符逐处不同**：
   - `[Normal, CharacterThrew, WeaponSwing].some(v => v == itr.kind)` 是**宽松**比较
     ⇒ `kind` 写成字符串 `"0"` / `"4"` 也算命中；
   - `v.state != StateEnum.Ball_Flying`、`a.state == StateEnum.Weapon_OnHand` 是宽松；
   - 分组比较 `v != EntityGroup.Freezer` 同样是宽松（`!=`）。端口统一用 `equals`。
3. **分组守卫是两个方向**：先试「v 含 Freezer **且** a 含 FreezableBall」⇒ 交换角色；
   否则若「a 里有任何元素不是 Freezer」**或**「v 里有任何元素不是 FreezableBall」⇒ 直接失败。
   即：走到后面要求 *a 全是 Freezer* 且 *v 全是 FreezableBall*。
4. **`do { ... } while (0)` 的三条 `break` 是短路或**：`v.state` 必须是 `Ball_Flying`，
   然后 `is_ball(v)` / `is_fighter(a)` / `a.state == Weapon_OnHand` 任一成立才继续。
   端口保留求值顺序（球在前、战士次之、武器在手上最后）。
5. **`cx1` 的两个分支不等价**：`a.facing > 0 ? x1 - centerx : x1 + centerx - centerx`。
   右边**不能**约成 `x1`：`x1 + c - c` 与 `x1 - c` 差 `2c`，但 `c == 0` 时两者又相等
   —— 所以用例必须给非零 `centerx` 才能让「取错分支」可观测。
6. **`cy2` 减的是 `height / 2`，`cx2` 加的是 `width / 2`**；`cx2` 的另一支是
   `(x2 + centerx - width) + width / 2`（中间是减法，不是加法）。
7. **只有 `x` 分量乘 `a.facing`**；`y` / `z` 不带朝向符号。
8. **`round` 出现在三处**，且 `x` 是先 `round(cx2 - cx1)` 再乘 `a.facing`
   （舍入发生在乘朝向符号之前，两者不可交换）。
9. `a.spawn(opoint, undefined, turn_face(v.facing))` 的 falsy 结果会直接 `return false`，
   并且**不会**执行 `v.enter_frame(GONE_FRAME_INFO)`。
10. `turn_face(undefined)` 返回 `undefined`（不会退化成 `1`）；`a.facing` 缺失时
    `a_facing * round(...)` 得到 `NaN`，只污染 `opoint.x`。

### 18.3 harness 观察点
- 假实体的 `group` / `state` / `type` / `face` / `posx` / `posy` / `posz` / `frame` 都是
  「一行一字段」的 `env a|v` 输入；`frame` 是对象（`centerx` / `centery` / `width` / `height`）。
- `spawn` 与 `enter_frame` 的日志**必须带实体身份**（`A:spawn:` / `V:enter_frame:`），
  否则「`spawn` 问错实体」不可观测。
- `a.spawn` 与 `v.spawn` 的返回值分别由 `env a spawn 0|1` / `env v spawn 0|1` 控制；
  **守卫读的是 `a` 的返回值**，只设 `env v spawn 0` 杀不掉「忽略返回值」的变异
  （首轮就是这个坑，靠 `A.spawn=` / `V.spawn=` 出现在状态文本里才看出来）。
- 状态文本同时打印双方的 `spawn` 标志，所以「谁被问了」也能从状态侧看出来。

## 19. `collision/healing`：治疗 buff

`native/lfw/collision/healing.{h,cpp}` 落地 `handle_healing`
（对应 `src/LFW/collision/handle_healing.ts`）。

### 19.1 单元边界
- `IHealingEntity { id, dataset(key), buff_entity() }`（**最小读面**，只有三个方法）+
  模块级 `HealingEnv { find_entity, buff_env }`。
- 这里**没有**复用 `handlers2.h` 的 `IHandlerEntity`（34 个纯虚）：本单元只读 `dataset` 与
  `buff_entity` 两处，复用会把 harness 逼成一份 150 行的空实现，反而掩盖边界。
- `buff::grant_buff` 用**真实现**（它属于已门禁的 `buff` 单元），本单元只负责「把谁和多少时长
  交出去」。
- `Buff_Healing.duration_of` 一度是**就地实现**；切片 4 落地 `Buff_Healing` 之后（见 §23）
  已改成 `buff::Buff_Healing::duration_of(*v->buff_entity(), to_number(injury))`。
  要点：端口的 `IHealingEntity` 与 `IBuffEntity` **不是同一个接口**，时长必须从
  **受击者的 buff 实体**读 dataset（真实游戏里 `entity.buff_entity()` 就是实体本身）；
  harness 里假 buff 实体的 `dataset` 必须**转发**给所属实体，否则读到空数据集。
  搬走后，本单元只保留「`itr.injury` 守卫 + 两次 id 查表 + `Healing` 这个 kind +
  交给 `grant_buff` 的实参」，时长公式的变异归 `buff_healing` subject 覆盖。

### 19.2 保真要点
1. **`if (!itr.injury) return;` 在一切查表之前**：`0` / `""` / 缺失都会直接返回，
   连 `create_buff` 都不会发生。
2. **`Math.max(1, x)` 的 NaN 语义**：`x` 缺失时 `Math.max(1, undefined)` 是 **NaN**，
   不是 1。端口的 `lfw::max` 也显式传播 NaN（`if (std::isnan(x)) return x;`）。
3. **时长公式**：`ceil(injury / max(1, hp_healing_value)) * max(1, hp_healing_ticks)`。
   两个 `max(1, …)` 分别在分母与乘数上，位置不可换。
4. **buff id 是 `kind + "_" + victim.id`**，由 `grant_buff` 内部拼出；`kind` 是 `"Healing"`。
5. `attacker` 允许为空：`grant_buff` 里是 `if (attacker) buf.set_attacker(attacker)`，
   所以 attacker 为空串 id 时 buff 的 `attacker_id` 保持 `""`（不是崩溃）。
6. `v == nullptr` 守卫是**端口产物**（TS 是解构 `collision.attacker/victim`，不存在查表失败），
   在契约内不可达；已写进变异规格头注。

### 19.3 harness 观察点
- 输入：`env itr <值>`、`env a|v dataset <值>`；输出：
  `run heal || <日志> | <双方 dataset> buff=<id>/<lifetime>/<duration>/<level>/<attacker>`。
- 日志里的 `create_buff:<kind>:<id>` 与状态里的 `buff=` 一起，把「kind 对不对」「时长算没算对」
  「谁是 attacker」三件事都变成可观测；少了 `duration` 这一项，handler 返回 void 会让整个公式
  不可观测。
- **假 `IBuffEntity` 的 id 必须等于它所属实体的 id**：`grant_buff` 用 `victim.id` 拼 buff id，
  第一版我把 buff 实体的 id 写成 `"VB"` 而实体是 `"V"`，差分立刻在 buff id 上分叉。
- 每个 op 之前清空 `g_granted`，否则早退用例会打印出上一条留下的 buff。

## 20. `collision/keeper`：碰撞处理表（注册半边）

`native/lfw/collision/keeper.{h,cpp}` 落地 `CollisionKeeper` 的**注册与查表**半边
（对应 `src/LFW/collision/CollisionKeeper.ts` 的 `pack_a` / `pack_b` / `is_u8,u12,u16` /
`IHandlerEntry` / `add` / `register` / `load_handlers` / `HANDLER_CONFIGS` / `collisions_keeper`）。

### 20.1 作用域说明（重要）
本单元**暂不包含** `handle(collision)`。它需要三样目前还不存在的东西：
1. 实体侧的 `victim.collided_list` / `victim.lastest_collided` / `attacker.collision_list`；
2. `collision_action_handlers[action.type]` 的分派入口（端口里是 `run_action`，需要
   `ActionEnv` 与 28 个纯虚的 `IActionEntity`）；
3. `handle_ball_frozen(victim, attacker, itr)` 所需的 `IFrozenEntity` 适配器。
现在硬做会把本单元 90% 的差分内容变成「测缝」。这些缝齐了之后再在同一个文件里补 `handle`，
并重跑两道门禁。

### 20.2 保真要点
1. **`pack_a` / `pack_b` 是双向自洽的**：插入与查表都走同一个函数，所以任何**确定的、单射的**
   位移或加一都会把两侧一起挪走 —— 改位移、改加一都**不可观测**。唯一可观测的破坏是让它**丢掉一个
   参数**（制造键碰撞）。变异规格里用的是后者。
2. **`ALL_STATES` 是身份哨兵，不是内容**：`add` 里 `if (a_state_list !== ALL_STATES)` 是**数组身份
   比较**；`register` 走默认参数时给的正是 `ALL_STATES` 这同一个对象 ⇒ 永不写入 `entry.a_state`
   ⇒ 表示「全状态通过」。端口用显式 `has_a_state` / `has_v_state` 标记复刻，**不能比内容**。
   推论：`HANDLER_CONFIGS` 里没有任何一条设置 `a_state`，所以 `g_env.attacker_state()` 的值
   在本单元**永远不会被读到**。
3. **`a_type` 是 `collision.attacker.data.type`，`v_type` 是 `victim.data.type`**，
   `itr_kind` 是 `collision.itr.kind`，`bdy_kind` 是 `collision.bdy.kind`；
   端口里前两者取自 `CollisionActor::data_type`（已是数字），后两者取自 `Value`。
4. **四个守卫的语义是 `Number.isInteger(v) && v >= 0 && v < hi`**，`hi` 分别是
   `256`(a_type) / `4096`(itr) / `256`(v_type) / `65536`(bdy)。任一不满足就**直接返回 false**
   （`handlers` 已经被清空）。端口对 `Value` 形状的 kind 用 `std::get_if<double>` 复刻
   （字符串 kind 一律不通过），对 `double` 形状的 type 用整数 + 区间判定。
5. **`load_handlers` 先清空 `collision.handlers` 再查表**，所以「早退」与「没命中」都会让
   `handlers` 变成空数组；返回值是 `handlers.length > 0`。
6. **表格顺序即处理顺序**：`HANDLER_CONFIGS` 的条目顺序决定同一 key 下多个 handler 的先后
   （例如 `(Ball, Normal, Ball, Normal)` 会先命中 `handle_ball_hit_other`，再命中
   `handle_ball_is_hit_b`）。
7. **`handle_body_goto` 的注册名是「声明名」**：TS 里是
   `import { handle_body_goto as handle_criminal_hit }`，但 `fn.name` 报的是 `handle_body_goto`。
8. `adds(...)` 用 `i.run.bind(i)`，注册进去的函数名会变成 `"bound run"`；当前表格全部走
   `register`，所以**本单元不实现 `adds`**（结果与 `register` 等价），待有真实调用方时再补。

### 20.3 harness 观察点
- 输入：`env a|v type|state <裸数字>`、`env itr|bdy|handlers <值语法>`；`run load`。
- 输出：`run load || handlers=[<名字,…>] | ret=0|1`。
- `collision.handlers` 在端口里是 `shared_ptr<vector<u16string>>`（**名字**，与 TS 的
  `Ditto.debug` 打印 `fn.name` 一致），harness 直接打印这个名字列表 —— 名字列表同时暴露了
  「命中哪几条」「几条」「顺序」三件事。
- `env handlers` 预置脏数据，用来观测「查表前必须清空」。

## 21. `collision/keeper`：`handle()` 调度半边

`CollisionKeeper::handle(Collision&) const`（同一个 `keeper.{h,cpp}`，与 §20 的注册半边共用
`collisions_keeper` 单例）。

### 21.1 单元边界：六个新缝
`KeeperEnv` 在原来的 `attacker_state` / `victim_state` 之外再加六个：

| 缝 | 对应 TS | 理由 |
|---|---|---|
| `call_handler(name, c)` | `handlers.forEach(fn => fn(collision))` | 端口的 `collision.handlers` 存的是**函数名**（`collision_core` 单元定下的形态，与 `Ditto.debug` 打印 `fn.name` 一致），所以调用点必须由一个名字→函数的映射来兑现 |
| `ball_frozen(first, second, itr)` | `handle_ball_frozen(victim, attacker, itr)` | 真函数要 `IFrozenEntity`（11 个方法），端口此刻只有 `CollisionActor`；本单元只负责「在什么时候、用哪个顺序调它」 |
| `run_action(type, action, c)` | `collision_action_handlers[action.type](action, collision)` | 同 `n_bdy_defend`：`run_action` 要 `ActionEnv` + 28 个纯虚的 `IActionEntity`，handler 本体由 `action_handlers` 单元门禁 |
| `victim_push_collided(c)` / `attacker_push_collision(c)` | `victim.collided_list.push(...)` / `attacker.collision_list.push(...)` | 实体侧列表尚未端口化 |
| `victim_play_sound(sounds)` | `victim.play_sound(sounds)` | 实体动作面 |

`dev` / `log` / `tester_run` / `find_object_data` 复用已有的 `CollisionCoreEnv`，不新造缝。

### 21.2 保真要点
1. **`handle_ball_frozen(victim, attacker, itr)` 的实参顺序是 `(v, a, itr)`** —— 与其它所有
   handler 的 `(collision)` 不同，这是最容易写错的一处。
2. **`itr_tests` / `bdy_tests` 在 handler 循环之前算好、循环之后才用**：`itr.actions?.map(v =>
   v.pretest && v.tester?.run(collision) !== false)`。所以带 `pretest` 的动作，其 `tester` 会在
   **handler 日志之前**被调用一次；不带 `pretest` 的则在分派时调用。日志顺序即可观测这个时机。
3. **`test_result = action.pretest ? itr_tests?.[idx] : action.tester?.run(collision)`**，
   只有**严格 `=== false`** 才 `return`（即跳过该动作）；`undefined` 照跑。
   推论：`pretest` 为假时缓存的元素永远用不到（所以缓存的「假值」分支不可观测）。
4. **`?.` 只是空值保护**：`v.tester?.run(...)` 在 `tester` 缺失时得到 `undefined`，
   `undefined !== false` 为真 ⇒ 该动作照跑。端口里必须**先判 `truthy(tester)` 再调用**。
5. **末尾六个 kind 用严格 `!==` 判定静音**：`Block` / `Whirlwind` / `MagicFlute` /
   `MagicFlute2` / `Pick` / `PickSecretly`。所以 `itr.kind` 是字符串 `"14"` 时**不会**静音
   （`"14" !== 14`），端口用 `strict_equals`。
   另外 `sounds = victim.data.base.hit_sounds` 是**两层**取值。
6. **`ball_hit` 是死代码**：TS 里累计完再也没被读过。端口照抄累计并显式 `(void)ball_hit;`，
   相关变异在规格头注里标记为不可观测。
7. `handle()` 不改 keeper 状态 ⇒ 端口声明为 `const`。

### 21.3 harness 观察点
- 本单元的 harness 是**独立的一对** `collision_keeper_handle.{cpp,ts}`，不复用注册半边的用例
  （注册与调度各有各的输入面）。同一个 TS 文件因此被两个 subject 覆盖。
- 输入：`env dev 0|1`、`env data <值>`、`env a|v id/type/state`、`env itr|bdy|handlers <值>`；
  输出：`run hunt || <日志>`，日志里 `dbg:` / `handler:` / `tester:` / `action:` /
  `frozen` 相关 / `v_collided:` / `a_collision:` / `sound:` 都是可观测点。
- **缝写成对实参顺序敏感**：`env.ball_frozen` 的 C++ 实现是 `return first.id != u"V";`，
  于是把参数写成 `(attacker, victim, itr)` 会翻转返回值、跳过 action 分派，而 TS 侧的真
  `handle_ball_frozen` 在两种顺序下都因空 `group` 返回 `false` ⇒ 差分立刻抓到顺序错误。
  这是「TS 侧的真函数无法打桩」时的通用手法。
- **TS 侧打桩**：`Ditto` 是普通对象（`src/LFW/ditto/Instance.ts` 的 `const _Ditto`），
  可以直接 `Ditto.DEV = …` / `Ditto.debug = …`；`collision_action_handlers` 是普通对象，
  逐键换成日志桩；假 handler 要 `Object.defineProperty(fn, "name", …)`，否则 debug 行打印不出名字。
- **打完桩的对象要洗一遍**：给 action 注入 `tester.run` 后先用 `JSON.parse(JSON.stringify(x))`
  去掉函数再 `renderValue`，才能与 C++ 侧的 `render(action)` 对齐（注意保留 `r` 键）。

## 22. `buff` 子类（一）：`Buff_GroupAttack` / `Buff_Electrify`

`native/lfw/buff/buff_group_attack.{h,cpp}`、`native/lfw/buff/buff_electrify.{h,cpp}`
（对应 `src/LFW/buff/Buff_GroupAttack.ts`、`Buff_Electrify.ts`）。这两类是切片 4 的第一对，
形状完全一致：**给每个受击者打/清一个标记 + 挂一个居中特效实体**。

### 22.1 为了可继承而对基类做的改造（`native/lfw/buff/buff.h`）
1. `Buff::place_effect` / `mount` / `unmount` / `init` 全部改成 **`virtual`**：
   TS 里子类正是覆写这四个。`place_effect_center` 保持非虚（TS 也非虚，子类只在
   `place_effect` 里转调它）。这些改动**不改变任何既有行为**，只是打开继承点。
2. `IBuffEntity` 新增 `data` / `state` / `wait` / `set_wait` / `mp` / `set_mp` / `mp_max` /
   `dataset` / `marks_get` / `marks_set` / `marks_delete`，全部是**带默认实现的虚函数**，
   **不是纯虚**。原因：有 **10 个已门禁 harness** 实现了 `IBuffEntity`，加纯虚会让它们全部
   编译失败；带默认实现则零改动、零回归。
   ——「给已门禁接口补充能力」一律用这个办法。
3. 新增 `set_mark(IBuffEntity&, key, value, prev)` / `del_mark(IBuffEntity&, key, value)` 自由函数，
   照抄 `Entity.set_mark` / `del_mark` 的**条件语义**：
   - `prev == void 0` 只认 `undefined`（用 `std::holds_alternative<std::monostate>`，**不是** `missing()`，
     后者把 `null` 也算缺失）；
   - 值比较用宽松 `equals`；
   - 条件不成立时**什么都不做**（`del_mark` 连日志都不产生）。
   这套语义是 `unmount()` 的关键：标记值被别人改过时就不能删。

### 22.2 保真要点
1. **`static KIND` 是类的身份**：`Buff_GroupAttack.KIND === "GroupAttack"`、
   `Buff_Electrify.KIND === "Electrify"`。`mount` 用它当**标记键**，`unmount` 也用它。
   端口里它是 `const char16_t*` 静态成员（`grant_buff` 的调用方需要它）。
2. `static GROUPS` 目前是给（尚未端口化的）buff 工厂用的纯数据，本单元没有任何读取点。
3. **`effect_oid` / `effect_frame_id` 是受保护的 getter**：`"fx"` + `"16"` / `"32"`。
   `effect_oid` 为空串时 `update_effects()` 会提前返回（不建特效实体）。
4. **`place_effect` 被覆写成 `place_effect_center`**：特效贴在受击者**当前帧的视觉中心**
   （`y + centery - h/2`，`h = height || pic.h`），而不是脚底。这是与基类的唯一行为差异。
5. **`mount()` 必须转调 `Buff::mount()`**（负责 `_mounted` 与 `world.buffs.set`），
   **`unmount()` 必须在清完标记后转调 `Buff::unmount()`**（负责删特效与逐受害者清理）。
6. `unmount()` 遍历的是**转调基类之前的**受害表副本语义：端口先把 `victims()` 循环跑完再调
   `Buff::unmount()`（基类会在内部清空 `_victims`）。
7. 端口用 `using Buff::Buff;` 继承构造函数（真实签名是 `(const BuffEnv*, id, kind)`，
   与 TS 的 `(lfw, id, kind)` 对应）。

### 22.3 harness 观察点
- 一对 harness `buff_marks.{cpp,ts}` 同时覆盖两个类（`env cls s "group_attack"|"electrify"`）。
- 输入：`env id|kind|cls|victim`（字符串值）、`env vpos|vframe|mark`（对象）、
  `run make|mount|unmount|effect`。
- 可观测面：`<victim>:buffs_set:` / `world_buffs_set:` / `<victim>:set_mark:key:value` /
  `<victim>:del_mark:key` / 特效实体的 `outline_alpha,outline_width,outline_color,set_position,
  enter_frame_by_id,attach` / `find_data:<oid>` / 状态文本里的 `marks=[…]` 与 `fx=x/y/z`。
- **`find_data` 必须落日志**，否则 `effect_oid` 换成另一个非空串完全不可观测（任何非空 oid
  都返回真值对象）。空串那一支靠 `update_effects` 的提前返回观测。
- **字符串日志有两种写法**：表示「值」的字符串（`enter_frame_by_id`、`outline_color`）
  两端都要走 `renderValue`（C++ 用 `render(Value(u16string))`），否则 TS 打 `s"16"` 而
  C++ 打 `16`；表示「键/身份」的字符串（`buffs_set:` 的 key、`set_mark:` 的 key、victims 列表）
  两端都用裸拼接。同一个 harness 里两种并存。
- `env mark` 必须是**覆盖**语义（对应 `marks.set`），所以 C++ 侧走实体的 `marks_set()`
  而不是往 `_marks` 里 `emplace_back`（后者会造出重复键，而 TS 的 `Map` 不会）。

## 23. `buff` 子类（二）：`Buff_Healing` / `Buff_MpHealing`

`native/lfw/buff/buff_healing.{h,cpp}`、`native/lfw/buff/buff_mp_healing.{h,cpp}`
（对应 `src/LFW/buff/Buff_Healing.ts`、`Buff_MpHealing.ts`）。
这对是切片 4 第二组：**打标记 + 按 ticker 周期回血/回蓝**。

### 23.1 单元边界（`native/lfw/buff/buff.h` 的补充）
`IBuffEntity` 又补了三个**带默认实现的虚函数**：`hp` / `hp_r` / `set_hp`。
和上一轮一样，**不能加纯虚**（10 个已门禁 harness 会被打断）。

### 23.2 保真要点
1. **`static duration_of(e, amount)`**：`ceil(amount / max(1, <kind>_healing_value)) *
   max(1, <kind>_healing_ticks)`。两个 `max(1, …)` 分别在**分母**与**乘数**上，位置不可换；
   `Math.max(1, undefined)` 是 **NaN**（见 DESIGN §19.2）。
2. **`mount()`**：先 `Buff::mount()`，再对每个受害者 `set_mark(KIND, this.id)`，
   并把 `this.ticks` 设成该受害者的 `<kind>_healing_ticks`（循环里最后一个受害者胜出）。
   ⇒ 标记键是**类常量**，而 tick 间隔是**每受害者读一遍**。
3. **`has_on_tick()` 必须覆写为 `true`**，否则基类的 `update()` 根本不会调 `on_tick`。
   端口的基类默认返回 `false`（这是给「无周期行为」的子类省的）。
4. **`on_tick`**：
   - `Buff_Healing`：`hp >= hp_r` 时只把 `lifetime` 刷成 `duration` 并返回；
     否则 `hp = min(hp_r, hp + hp_healing_value)`。
   - `Buff_MpHealing`：**没有** `mp >= mp_max` 那一段 —— TS 原文里它是**注释掉的**，
     端口照抄（不要「顺手补上」）。只做 `mp = min(mp_max, mp + mp_healing_value)`。
5. **`unmount()`**：用 `del_mark(KIND, this.id)` 的**条件删除**（标记值被别人改过就不删），
   最后 `Buff::unmount()`。
6. `Times` 的**累积语义**（`native/lfw/utils/times.cpp`）：`add(d)` 把 `d` 累加到 `_value`，
   只有 `_value >= _max` 才返回 `true` 并把 `_value` 归位。
   ⇒ 写用例时 `run tick <ticks>` 才会**每次**触发；`run tick 1` 三次只触发一次。
   这是本轮 5 条变异存活的主因（用例的 tick 根本没触发到 `on_tick`）。

### 23.3 harness 观察点
- 一对 harness `buff_healing.{cpp,ts}`（`env cls s "healing"|"mp_healing"`）。
- 输入：`env id|kind|cls|victim`（字符串）、`env vdata|vhp|vmp|mark`（对象）、
  `env ticks|duration`（裸数字，需先 `run make`）。
- `run make|mount|unmount|tick <d>|duration <amount>`；`duration` 直接调类的**静态** `duration_of`。
- 状态文本逐受害者打印 `hp=[…]` / `mp=[…]`，另有 `ticks=` / `life=` / `dur=` / `marks=`。
- **数值日志一律走 `render`**（两端都是），别一端 `renderValue` 一端裸数字 ——
  字符串日志则按 §22.3 的「值型 vs 键型」分别处理。

## 24. `buff` 子类（三）：`Buff_Electroshock`

`native/lfw/buff/buff_electroshock.{h,cpp}`（对应 `src/LFW/buff/Buff_Electroshock.ts`）。
切片 4 第三组：**没有标记，只按 ticker 周期推进受害者的 `wait`，并在 `mount` 时缩短自身时长**。

### 24.1 单元边界
本单元**不需要**给 `IBuffEntity` 增补任何成员：只用到已有的 `data` / `state` / `wait` /
`set_wait`，加上继承来的 `victims` / `find_entity` / `set_duration` / `duration` / `set_ticks` /
`place_effect_center`。

### 24.2 保真要点
1. **`init()` 覆写为 `set_ticks(3)`**：只设 tick 间隔，不碰 `duration`。
2. **`effect_oid()` 覆写为 `"fx"`**，但**不覆写 `effect_frame_id()`**（基类 `"0"`）。
   曾经想把 `effect_frame_id` 也写成 `"32"`（照抄 `Buff_Electrify`）——那是**新增行为**，
   TS 原文没有，端口也没有。
3. **`place_effect(effect, victim)` 覆写为 `place_effect_center(effect, victim)`**：
   特效挂点是 `y + centery - height / 2`，不是 `Buff::place_effect` 的裸 `y`。
4. **`mount()` 用宽松 `==`**：
   ```text
   Buff::mount();
   for each victim:
     if (victim.state == Injured) continue;   // 宽松
     if (victim.state == Falling) continue;   // 宽松
     set_duration(round_float(duration() / 2));
   ```
   两个 `if` 是**分开的两条语句**（TS 原文如此），不能合并成一条 `||`——分开写时
   每条都要能被单条删除的变异观测到。
5. **`on_tick()` 用严格 `===`**：
   ```text
   if (!is_fighter_data(victim.data)) return;
   if (victim.state === Falling) return;
   if (victim.state === Lying)  return;
   victim.wait = victim.wait + 1;
   ```
   ⇒ 同一个方法族的 `mount` 是宽松、`on_tick` 是严格，**不可统一**。
   字符串状态（`s "11"` / `s "12"` / `s "14"`）就是用来把这两者区分开的：
   严格比较会「认不出」字符串形态的数字。
6. **`round_float` 在本单元不可观测**：`Times::set_max`（即 `set_duration`）内部做 `floor`，
   所以 `duration()` 永远是整数，`/ 2` 最多产生 `.5` 一位小数，
   `round_float`（`round(x * 1000) / 1000`）**永远不会改变结果**。
   ⇒ 这条变异移入规格头部的「不可观测」清单，而不是硬造用例。
7. **`KIND` 在本单元不可观测**：本类从不读它（没有 `set_mark` / `del_mark`），
   它只是 `grant_buff(...)` 的调用方（例如 `handlers2`）传进来的字面量，归那些单元断言。
8. `if (victim == nullptr) return;` / `continue;` **不能反向**（`!= nullptr` 是空指针解引用路径），
   反向形式才是可观测变异。

### 24.3 harness 观察点
- 一对 harness `buff_electroshock.{cpp,ts}`。
- 输入：`env id|kind|victim`（字符串）、`env vtype|vstate`（**值字面量**）、
  `env vwait|duration`（**裸数字**）、`env vpos|vframe`（对象）。
- **`env victim` 是「重新选中」而不是「总是追加」**：同一个 id 再次出现时会先把它从列表里
  移除再压到末尾。这样后续 `env v*` 能重新指向第一个受害者（`mount` 的守卫要逐个固定状态，
  列表里只要还剩一个中性状态的受害者，时长就会被它偷偷减半、变异藏进噪声里）。
- `run make|init|mount|unmount|tick <d>`；`run tick <d>` 的 `d >= ticks` 才会每次都触发 `on_tick`。

## 25. `buff` 子类（四）：`Buff_MagicFlute` / `Buff_MagicFlute2`

`native/lfw/buff/buff_magic_flute.{h,cpp}`（对应 `src/LFW/buff/Buff_MagicFlute.ts`、
`Buff_MagicFlute2.ts`）。切片 4 第四组：**唯一同时实现 `on_tick` 与 `on_update` 的子类**。

### 25.1 单元边界（`native/lfw/buff/buff.h` 的补充）
`IBuffEntity` 又补了一批**带默认实现的虚函数**（依旧不能加纯虚：10 个已门禁 harness 会全崩）：

- 读：`data_type` / `velocity_y` / `team` / `data_indexes_falling` / `data_indexes_in_the_skys`
- 写：`set_velocity(x,y,z)` / `handle_velocity_decay(accx)` / `set_hp_r` / `set_fallinjury` /
  `set_toughness` / `set_team`

`data.indexes.falling` / `data.indexes.in_the_skys` 按 `fall` / `weapon_is_hit` 的既有做法
拆成 `data_indexes_*()` 缝（`IFrameIndexes` 的字段名与形状见 `fall.h`）。
`summary_mgr.apply_damage` 用**模块级缝**（与 `handlers2` 的 `summary_apply_damage` 同形）：
`MagicFluteEnv { summary_apply_damage }` + `magic_flute_env()` / `set_magic_flute_env()`。

### 25.2 结构说明：两个类共用文件内 helper
两个 TS 类的 `on_tick` / `on_update` **逐字相同**，只差两个常量
（`injury` / 加速度）。端口把方法体抽成文件内匿名 namespace 的
`apply_flute_tick(attacker, victim, injury, injury_r)` 与
`apply_flute_update(attacker, victim, acc)`，类方法只负责选常量。
这是本仓端口化里**唯一一处**主动合并重复体的地方，理由是 TS 侧除常量外连标识符序列都一致，
合并后没有引入任何 TS 里不存在的分支；变异规格的锚点也相应落在 helper 文本与常量定义上。

### 25.3 保真要点
1. **`init()`**：`ticks = 3; duration = 3;`（两个类一致）。
2. **`on_tick()`** 顺序不可换：
   ```text
   prev_hp = victim.hp              // 必须在扣血之前读
   victim.hp_r -= injury_r          // 0.5
   victim.hp   -= injury            // 2 / 1
   victim.fallinjury = 20
   victim.toughness  = 0
   if (attacker) summary_mgr.apply_damage(attacker, injury, victim, prev_hp)
   ```
   `hp` 每次减少固定值（不是按比例），且 `injury_r` 两个类都是 `0.5`。
3. **`on_update()` 的 `calc_v` 目标与加速度是同一个数**：
   `calc_v(victim.velocity.y, acc, SpeedMode.AccTo, acc, 1)`，`acc` = `3`（Flute）/ `1.5`（Flute2）。
   `AccTo` 分支里 `target` 与 `acc` 都乘 `direction`（这里是 `1`，所以看不出差别），
   且 `if (!acc) return current` 在前 —— **`acc = 0` 是合法的早退分支**。
4. **`set_velocity(null, vy)`**：`x` 是 `null`、`z` 省略（`undefined`）；真 `Entity.set_velocity`
   对两者一视同仁（都跳过），所以只有 `y` 可观测。
5. **`handle_velocity_decay(0.25)`** 只传一个实参 ⇒ `accz = accx`、`factor = 1`（真实现的默认参数）。
6. **type 分支是严格比较**：`victim.data.type` 与 `EntityEnum.Fighter`(8) / `Weapon`(16)
   **严格**相等才进对应分支（字符串 `"8"` / `"16"` 都不进）。
   Fighter 分支里 `victim.state !== StateEnum.Falling` 也是**严格**。
7. **Weapon 分支内层 `switch` 的两个 `break` 等价于空分支**（`Weapon_InTheSky`=1000 /
   `HeavyWeapon_InTheSky`=2000，两个条件用 `&&` 与早退合并），`default` 才是
   `team = attacker.team` + 进 `in_the_skys[0]`。
8. **`indexes?.falling?.[-1][0]`**：`falling` 是 `{"-1": [...], "1": [...]}` 形状的对象
   ⇒ `index_by(falling, u"-1")` 再 `index_0(...)`；`indexes?.in_the_skys?.[0]` 是数组 ⇒ 只 `index_0`。
   ⚠️ `falling` 缺失或没有 `"-1"` 键时 TS 会 `undefined[0]` **抛异常**（契约外，端口不复制）；
   索引缺失时 TS 传 `undefined` 给 `enter_frame_by_id`，端口传 `to_string(Value())` ⇒
   用例必须始终给出索引（规格头注已写明）。
9. `index_by` 的数组分支、`index_0` 的对象分支在本单元都是**死代码**（照抄自 `fall.cpp`），
   写进规格头注而不是硬造用例。
10. Fighter 分支结尾那个 `return;` 是 if/else-if 的等价变换，**不可观测**（删掉后
    落到 Weapon 测试，而该测试在 Fighter 为真时必为假）。

### 25.4 harness 观察点
- 一对 harness `buff_magic_flute.{cpp,ts}`（`env cls s "mf1"|"mf2"` 选择类）。
- 输入：`env id|kind|cls|attacker|victim`（字符串；`env attacker s ""` 表示无攻击者）、
  `env ateam|vtype|vstate|vhp|vhp_r|vfallinjury|vtough|vteam|vindexes`（**值字面量**）、
  `env vvy`（**裸数字**）。
- `run make|init|tick <d>`。`on_update` **每次 `tick` 都跑**；`on_tick` 需要 `d >= ticks`。
- 观察面：`set_velocity` / `handle_velocity_decay` / `enter_frame_by_id` / `set_hp` / `set_hp_r` /
  `set_fallinjury` / `set_toughness` / `set_team` 日志，`apply_damage:<a>:<injury>:<v>:<prev_hp>`
  （TS 侧 `summary_mgr.apply_damage` 被**打桩**，与 `handlers2` 的处理一致），
  以及逐受害者的 `hp()/hp_r()/fall/tough/team/vy/type` 状态文本。
- **假实体的 `data` getter 要把 `_indexes` 合并进去**：TS 真类读的是
  `victim.data.indexes.falling`，而端口读的是 `data_indexes_falling()` 缝；
  若 TS 侧 `data` 只返回 `_data`，`env vtype` 会连带把 `indexes` 抹掉，差分立刻分叉。

## 26. 切片 5 起点：`state/State_Base`

`native/lfw/state/state_base.{h,cpp}`（对应 `src/LFW/state/State_Base.ts`）。
切片 5 的第一块：所有状态类的基类。

### 26.1 单元边界
```text
IStateEntity : buff::IBuffEntity   // 只加 velocity_x() / velocity_z()
StateEnv { const buff::BuffEnv* buff_env; }   // 模块级缝，同 handlers4 的 x_env()
```
`IStateEntity` 继承 `IBuffEntity`，因为 `leave()` 里要调
`Buff_Healing::duration_of(e, …)`（它的参数就是 `const IBuffEntity&`），
而 `grant_buff` 的 victim 形参也是 `IBuffEntity*`。

### 26.2 可选钩子用 `std::function` 成员，不用「虚函数 + has_ 标志」
`State_Base.ts` 里的 `pre_update?` / `enter?` / `on_dead?` / `on_landing?` /
`get_gravity?` / `get_sudden_death_frame?` / `get_caught_end_frame?` / `get_auto_frame?` /
`find_frame_by_id?` / `on_leave_ground?` 都是**声明式可选成员**：基类原型上根本没有这些属性。

调用方写的是 `this._state?.get_gravity?.(this)`，也就是**先判存在再调用**
（`Entity.ts:623 / 815 / 1571 / 1661 / 1756 / 1810 / 1824` 都这么写）。
端口用**空 `std::function` 成员**建模：

```text
if (s->get_gravity) { Value g = s->get_gravity(*e); }
```

恰好等价于 `?.` 的存在性判断，且比 `virtual + has_xxx()` 少一半声明
（`Buff` 用 `has_on_tick()` 是因为那边是必需/可选的二选一，这里 10 个钩子用虚函数会写 20 个函数）。
`update` / `leave` / `on_restrict` 在 TS 里**基类就有实现** ⇒ 保持 `virtual` 且始终存在。

### 26.3 保真要点
1. **`leave(e, next_frame)` 只处理 `HealSelf`**，用**严格** `===`：
   `grant_buff(Buff_Healing.KIND, void 0, e, Buff_Healing.duration_of(e, Defines.STATE_HEAL_SELF_HP))`。
   `STATE_HEAL_SELF_HP = 104`（端口写成文件内 `kStateHealSelfHp`，同 `n_bdy_defend` 的
   `kDefaultBreakDefendValue` 约定）。攻击者是 `void 0` ⇒ 端口传 `nullptr`。
2. **`on_restrict(e, x, y, z)`**：
   ```text
   vx = vz = null ; vy = null            // 注意 vy 恒为 null（源码里那行被注释掉了）
   if (!float_equal(x, e.position.x)) vx = clamp(e.velocity.x, -0.5, 0.5)
   if (!float_equal(z, e.position.z)) vz = clamp(e.velocity.z, -0.5, 0.5)
   if (!float_equal(y, e.position.y)) { vx = clamp(e.velocity.x, …); vz = clamp(e.velocity.z, …) }
   if (vx !== null || vz !== null || vy !== null) e.set_velocity(vx, vy, vz)
   e.position.x = x ; e.position.y = y ; e.position.z = z
   ```
   - `MIN_V = 0.5`；y 那一支是「贴地保留速度」的补丁，会**同时**把 x 与 z 都刷一遍。
   - 判空是 **`!== null`**，不是 `undefined`：`undefined !== null` 为真 ⇒
     **速度缺失时仍会调用 `set_velocity(undefined, null, vz)`**。端口的 `is_null` 因此
     **只认 `NullTag`**，绝不能把 `monostate` 也算进去（变异规格里那条「missing 算 null」专门盖这个）。
3. **`clamp` 的 JS 语义要单独建模**：`clamp(value, min, max) = value < min ? min : value > max ? max : value`。
   `value` 非数（`undefined` / 字符串）时两次比较都为假 ⇒ **原样返回**。
   端口的 `lfw::clamp` 是 `double` 版，所以 `on_restrict` 里用一个 `Value` 版的
   `clamp_velocity(v)`：`d = to_number(v)`，越界返回**数值上/下界**，否则**原样返回 `v`**
   （这样 `undefined` 仍是 `undefined`、`NaN` 仍是 `NaN`、字符串仍是字符串）。
4. **`e.position.x = x` 是直接写字段，不是 `set_position`**：端口用 `IBuffEntity::set_position`，
   其语义（写 `_px/_py/_pz`）与 TS 的字段写一致；**harness 里这一处不能打日志**，
   否则 TS 侧（字段写，无法拦截）与 C++ 侧（函数调用，可打日志）立刻分叉。
5. `update(e)` 在 TS 里是**空方法**，照抄成空方法。

### 26.4 harness 观察点
- 一对 harness `state_base.{cpp,ts}`。
- 输入：`env victim s "V1"`、`env state` / `dataset` / `pos` / `velx` / `velz`（**值字面量**，
  `velx`/`velz` 可写 `u` 造缺失速度）。
- `run make|leave|restrict <x> <y> <z>|update`。
- 观察面：`dataset:` / `create_buff:` / `world_buffs_set:` / `buffs_set:` / `set_mark:` / `set_velocity:`
  日志，以及 `state=` / `pos=[…]` / `vel=[…]` / `granted=<id>:<duration>` / `marks=[…]` 状态文本。
- **`set_mark` 的 key 走裸插值**（C++ `s_of`、TS `${String(key)}`），value 走 `render`
  —— 与 `buff_marks` 的「值型 vs 键型」约定一致。
- `run make` 必须把「上一个创建出来的 buff」清空，否则 `granted=` 会把上一场景的结果带进来。

## 27. 切片 5：`state/CharacterState_Base`

`native/lfw/state/character_state_base.{h,cpp}`（对应 `src/LFW/state/CharacterState_Base.ts`）。
角色状态的基类，覆写了 `update` 并**首次**给 `State_Base` 的可选钩子装上了实现。

### 27.1 单元边界（`native/lfw/state/state_base.h` 的补充）
`IStateEntity` 又补了一批**带默认实现的虚函数**（不能加纯虚：`state_base` 的 harness 已门禁）：

- 读：`facing` / `holding_base_type` / `is_on_ground` / `frame_on_landing` / `data_frames` /
  `data_indexes_default` / `data_indexes_landing_2` / `data_indexes_heavy_obj_walk`
  （`data_indexes_falling` / `data_indexes_in_the_skys` 已在上一单元加好）
- 写/动作：`enter_frame(next_frame)` / `drop_holding` / `handle_ground_velocity_decay`

### 27.2 可选钩子的「安装」而不是「覆写」
`State_Base.ts` 里 `on_landing?` / `get_auto_frame?` / `get_sudden_death_frame?` /
`get_caught_end_frame?` / `on_leave_ground?` 是可选成员，端口把它们建模成空 `std::function` 成员
（见 DESIGN §26.2）。于是 `CharacterState_Base` **不是覆写虚函数，而是在构造函数里赋值**：

```text
CharacterState_Base::CharacterState_Base(Value state) : State_Base(std::move(state)) {
  on_landing = &csb_on_landing;                 // 等等五个
}
```

五个实现体放在文件内匿名 namespace 里（无捕获 ⇒ 可以直接取函数地址）。
`update` 因为基类本来就有实现，保持 `override`。

### 27.3 保真要点
1. **`update`**：`State_Base::update(e)`（空）之后调 `e.handle_ground_velocity_decay()`。
2. **`on_landing`**：先看 `e.frame.on_landing`（`truthy` 判定），有就 `enter_frame` 并返回；
   否则 `enter_frame_by_id(e.data.indexes?.landing_2)`。
3. **`get_auto_frame`** 的三段优先级（**不可换序**）：
   `holding.base_type === Heavy` → `indexes.heavy_obj_walk`；
   否则 `is_on_ground` → `indexes.default`；
   否则 `hp > 0` → `indexes.in_the_skys[0]`。
   然后 `if (!fid) return void 0`，最后 `frames[fid]`。
   `holding.base_type` 的比较是**严格** `===`。
4. **`get_sudden_death_frame`**：**先** `set_velocity(2 * facing, 2)`（只给 x/y，z 省略），
   再看 `if (indexes?.falling)` 决定要不要返回 `{ id: falling[1][1] }`。
5. **`get_caught_end_frame`**：`cvx = dataset('cvx_d')`、`cvy = dataset('cvy_d')`，
   `set_velocity(-1 * cvx * facing, cvy)`（y 直接是 `cvy`，可能是 `undefined` ⇒ NaN），
   再看 `if (indexes?.falling)` 决定要不要返回 `{ id: falling[-1][1] }`。
6. **`on_leave_ground`**：四个状态 `Running(2) / Walking(1) / Standing(0) / Rowing(6)` 用**严格** `===`
   逐个判否（德摩根后是 `&&` 链），命中后 `holding.base_type === Heavy` 则 `drop_holding()`，
   最后 `enter_frame(NEXT_FRAME_AUTO)`（`{ id: "auto" }`）。
7. **`falling[1][1]` 这类取值会让 TS 抛异常**：`falling` 存在但没有 `"1"` 键时
   `falling["1"]` 是 `undefined`，再 `[1]` 就 TypeError。端口不复制这个异常，用例始终给全两个键。
8. `index_by` 的**数组分支**与 `index_0` 的**对象分支**在本单元仍是死代码（照抄 `fall.cpp`）。

### 27.4 harness 观察点
- 一对 harness `character_state_base.{cpp,ts}`。
- 输入：`env victim s "V1"`、`env onground`（**裸数字**）、
  `env state`（状态对象自己的 state）/ `vstate`（**实体的** state）/
  `hp` / `facing` / `holding` / `onlanding` / `dataset` / `indexes` / `frames` / `velx` / `velz`（值字面量）。
- `run make|update|landing|up|auto|sudden|caught`。
- 输出 `run <op> || <日志> | ret=<返回值或 -> | <状态>`；五个可选钩子在端口的返回值
  用 `ret=` 观测（`undefined` 打成 `-`，与 TS 侧一致）。
- ⚠️ **两处命名坑**：
  1. `env state` 是**状态对象自己**的 state（构造参数），`env vstate` 才是**实体**的 state。
     `on_leave_ground` 读的是后者 —— 只设 `env state` 会让 `run up` 静默什么都不做
     （本轮 7 条变异存活全因此而起）。
  2. `on_landing` 的 velocity 形参 TS 原文没用，所以 `run landing` 不传值。

## 28. 切片 5：`state/CharacterState_Standing` / `_Running` / `_Injured`

`native/lfw/state/character_state_basic.{h,cpp}`
（对应 `src/LFW/state/CharacterState_Standing.ts`、`CharacterState_Running.ts`、`CharacterState_Injured.ts`）。
三个都是几十行的「地面小状态」，共用一份 harness 与用例，合成一个单元。

### 28.1 单元边界（`native/lfw/state/state_base.h` 的补充）
`IStateEntity` 又补 3 个**带默认实现的虚函数**：

- `ground_y()`（`e.ground_y`）
- `get_sudden_death_frame()` —— **注意这是「实体侧」的同名方法**，与 `State_Base` 上那个
  「钩子」不是一回事（见 §28.2）
- `holding_set_team(v)` —— 对应 `holding.team = e.team`（写的是**持有物实体**的字段）

### 28.2 两个 `get_sudden_death_frame` 必须分清
```text
Entity.get_sudden_death_frame(): TNextFrame      // 无参，内部 this._state?.get_sudden_death_frame?.(this) || NEXT_FRAME_AUTO
State_Base.get_sudden_death_frame?(e): INextFrame // 有参，是「状态对象」上的可选钩子
```
`CharacterState_Standing/Running` 写的是 **`e.get_sudden_death_frame()`**（实体侧、无参）
⇒ 端口把它建成 `IStateEntity::get_sudden_death_frame()` **缝**（无参、返回 `Value`），
由 harness 的假实体给出返回值。真正的「钩子 → 实体方法」那一层委托属于 `Entity`（切片 7）。

### 28.3 保真要点
1. **`CharacterState_Injured` 的 `super.enter?.(e, prev_frame)` 是死代码**：
   `State_Base.ts` 与 `CharacterState_Base.ts` 都只用 `?` **声明** `enter`，
   谁都没实现 ⇒ `super.enter` 恒为 `undefined`。端口因此什么都不写（不是漏）。
   而 `Injured` 自己是「可选钩子」的实现 ⇒ 端口在构造函数里 `enter = &csi_enter;`。
2. **`Injured.enter`**：`holding?.base_type === WeaponEnum.Heavy`（**严格**）时
   先 `e.drop_holding()`，再把 `holding.team = e.team`。
   **顺序可观测**（两件事都有日志），所以不能合并。
3. **`Standing.update`**：`super.update(e)`（含 `handle_ground_velocity_decay`）之后，
   `e.hp <= 0` ⇒ `enter_frame(e.get_sudden_death_frame())` 并 **return**；
   否则 `e.position.y > e.ground_y` ⇒ `enter_frame_by_id(indexes?.in_the_skys?.[0])`。
   `hp <= 0` 是**宽松**数值比较（`undefined` ⇒ NaN ⇒ 假）。
4. **`Running.update`**：`super.update(e)` 之后，`{ vz, vx } = e.velocity`；
   `if (vz)` 为真时 `dz = abs(vz / 4)`，若 `vx > dz` 则 `vx -= dz`，若 `vx < -dz` 则 `vx += dz`，
   然后 `set_velocity(vx)`（**只写 x**）。最后 `if (e.hp <= 0) enter_frame(e.get_sudden_death_frame())`
   —— 注意与 `Standing` 不同，这里**没有 else**，也没有 return。
   `if (vz)` 是**真值**判定（`0` / `NaN` / `undefined` 都为假）。
5. 三个类的构造函数默认参数（`Standing` / `Running` / `Injured`）在本单元**不可观测**
   （没有任何路径读状态对象自己的 `_state`），写进规格头注。

### 28.4 harness 观察点
- 一对 harness `character_state_basic.{cpp,ts}`。
- `env cls s "standing"|"running"|"injured"` 选类；`env usedefault 1` 不带状态构造（验证默认参数可省）。
- 输入：`env victim s "V1"`、`env onground|usedefault`（**裸数字**）、
  `env state|vstate|hp|facing|vteam|ground_y|holding|onlanding|dataset|indexes|frames|pos|velx|velz`（值字面量）。
- `run make|update|enter`。
- 状态文本：`state=` / `hp=` / `ground=` / `pos=[x:y:z]` / `vel=[x:z]` /
  `holding=` / `holding_team=` / `team=`。
- ⚠️ **TS 侧的「普通字段写」要改成带日志的属性**：
  `holding.team = e.team` 在 TS 里是纯字段写（无日志），而端口那一步是缝调用。
  若只在 C++ 侧打日志，差分立刻分叉；若两侧都**不打**日志，则「先掉落再改阵营」与
  「先改阵营再掉落」两条变异的**顺序**不可观测（本轮唯一存活项）。
  最终做法：TS 假实体的 `holding` getter 返回一个带 **`get/set team`** 的对象，
  setter 里打 `holding_set_team:<值>` 日志；C++ 的 `holding_set_team` 也打同样的日志。

## 29. 切片 5：`state/CharacterState_Walking`

`native/lfw/state/character_state_walking.{h,cpp}`（对应 `src/LFW/state/CharacterState_Walking.ts`）。
第一个同时使用「输入（ctrl）」与「等待值（wait）」的状态。

### 29.1 单元边界（`native/lfw/state/state_base.h` 的补充）
`IStateEntity` 补 6 个**带默认实现的虚函数**：

- `frame_info()`（`e.frame`，整帧对象）、`ctrl_ud()` / `ctrl_lr()`（`e.ctrl.UD` / `.LR`）
- `holding_is_weapon()`（`is_weapon(e.holding)`）
- `handle_wait_flag(wait, frame)` → `double`
- `enter_frame_by_id_fallback(id, fallback)`

### 29.2 为什么新开一个 `enter_frame_by_id_fallback` 而不是给已有虚函数加形参
TS 的签名是 `enter_frame_by_id(id: string | undefined, fallback: boolean = false)`。
**绝不能**把已有的 `IBuffEntity::enter_frame_by_id(const u16string&)` 改成两参数版本：
现有 harness 覆写的是**一参数**版本，基类签名一变，它们的函数就不再是覆写，
调用会静默落到基类的空实现上（编译通过、行为全错）。
所以新开一个**带默认实现**的虚函数，默认实现转发给一参数版本：

```text
virtual void enter_frame_by_id_fallback(const std::u16string& id, bool fallback) {
  (void)fallback;
  enter_frame_by_id(id);
}
```

新增成员永远安全，改已有签名永远不安全 —— 这是本仓反复确认的规则。

### 29.3 保真要点
1. **`update`**：`super.update(e)`（含 `handle_ground_velocity_decay`）之后：
   ```text
   if (!ctrl.UD && !ctrl.LR && !e.wait) {
     if (is_weapon(e.holding) && e.holding?.base_type === Heavy) e.wait = e.handle_wait_flag(void 0, e.frame)
     else e.enter_frame_by_id(e.data.indexes?.default, true)
   }
   if (e.hp <= 0) e.enter_frame(e.get_sudden_death_frame())
   else if (e.position.y > e.ground_y) e.enter_frame_by_id(e.data.indexes?.in_the_skys?.[0])
   ```
   - 三个条件是 **`&&`**；`e.wait` 走**真值**判定（`0` 也算「不等待」）。
   - Heavy 分支把 `handle_wait_flag(void 0, e.frame)` 的结果**写回 `e.wait`**，
     `void 0` 与 `e.frame` 的**实参顺序**可观测。
   - `is_weapon(e.holding)` 与 `holding.base_type === Heavy` 是**两个独立条件**：
     前者看 `holding.data.type === EntityEnum.Weapon(16)`，后者看 `holding.base_type`。
     用例必须分别构造「是武器但不是重型」「是重型但不是武器」两种输入。
   - `enter_frame_by_id(indexes?.default, **true**)` —— 第二个实参是 `true`。
2. **`e.hp <= 0` 之后 `return`**：与 `Standing` 同形（`else if` 的等价变换），
   必须构造「hp<=0 **且** 高于地面」的输入才能观测这个 `return`。
3. 构造函数默认参数（`StateEnum.Walking`）在本单元不可观测（没有路径读状态对象自己的 `_state`）。

### 29.4 harness 观察点
- 一对 harness `character_state_walking.{cpp,ts}`。
- 输入：`env victim s "V1"`、`env ctrlud|ctrllr|hweapon|waitflag`（**裸数字**）、
  `env state|hp|vwait|frame|ground_y|holding|indexes|pos`（值字面量，`vwait` 可 `u`）。
- `run make|update`。
- 状态文本：`hp=` / `wait=` / `waitflag=` / `frame=` / `pos=[x:y:z]` /
  `ground=` / `holding=` / `hweapon=` / `ctrl=UD LR`。
- ⚠️ **TS 侧要让真 `is_weapon()` 能工作**：`is_weapon(v) = v?.data?.type === 16`，
  所以假实体的 `holding` getter 必须返回带 **`data: { type }`** 的对象
  （由 `env hweapon` 驱动成 `16` / `0`），否则 TS 侧永远进不了 Heavy 分支、差分立刻分叉。
- ⚠️ **C++ 侧的一参数 `enter_frame_by_id` 也要打上 `:0` 后缀**：
  TS 侧只有一个带默认参数的 `enter_frame_by_id(id, fallback=false)`，
  两条调用路径打出的文本必须一致。

## 30. 切片 5：`state/CharacterState_Caught` / `_Rowing`

`native/lfw/state/character_state_caught_rowing.{h,cpp}`
（对应 `src/LFW/state/CharacterState_Caught.ts`、`CharacterState_Rowing.ts`）。
两个都在 `enter` 钩子里干活的小状态，合成一个单元。

### 30.1 单元边界（`native/lfw/state/state_base.h` 的补充）
`IStateEntity` 补 5 个**带默认实现的虚函数**：

- `has_holding()`（`const holding = e.holding; if (holding)` 的**真值**判定，
  与 `holding_base_type()` 是两件事：前者看对象是否存在，后者看它的 `base_type` 字段）
- `fall_value()` / `fall_value_max()` / `set_fall_value(v)`
- `data_indexes_landing_1()`

### 30.2 保真要点
1. **`CharacterState_Caught.update` 不调 `super.update(e)`**：因此**没有**
   `handle_ground_velocity_decay`，只做 `e.set_velocity(0, 0, 0)`。
   这是与 `Standing` / `Running` / `Walking` 不同的地方，用例专门断言「update 无日志」。
2. **`Caught.enter`**：
   ```text
   e.fall_value = e.fall_value_max      // 读的也是实体属性，不是 dataset！
   e.set_velocity(0, 0, 0)
   const holding = e.holding
   if (holding) e.drop_holding()                                   // 任何持有物都丢
   if (holding?.base_type === Heavy) holding.team = e.team         // 只有重型才改阵营
   ```
   两个 `if` 的守卫不同：**掉落看「有没有持有物」，改阵营看「是不是重型」**。
   用例必须给出「有持有物但非重型」与「没有持有物但 `base_type` 是重型」两种输入。
3. **`Rowing.on_landing`** 与 `CharacterState_Base` 的同名钩子结构一致，
   但回落索引是 `indexes?.landing_1`（基类用的是 `landing_2`）——
   所以它**覆盖**掉基类装好的那个钩子（在构造函数里重新赋值）。
4. **`Rowing.enter`**：
   ```text
   if (prev_frame.state !== SE.Falling) return          // 严格比较
   vx = dataset('rowing_distance') * dataset('bfall_x_f')
   vy = dataset('rowing_height') * dataset('bfall_h_f')
   next_vx = velocity.x >= 0 ? vx : -vx
   next_vy = calc_v(velocity.y, vy, SpeedMode.Default, 0)   // 只传 4 个实参，direction 默认 1
   set_velocity(next_vx, next_vy)                            // z 省略
   ```
   - `SpeedMode.Default` 分支：`target = value * direction`；
     `current < target && target > 0` 取 `target`；`current > target && target < 0` 取 `target`；
     否则**返回原值**。所以 `current` 比负数目标更小的时候会**原样保留**（用例专门覆盖）。
   - `acc` 在 `Default` 模式下**不被使用**（变异不可观测，已写进规格头注）。
5. **`velocity.y` 缺失在端口无法表示**：`IBuffEntity::velocity_y()`（以及 `calc_v` 本身）
   是 `double` 型，缺失值变成 `NaN`，而 TS 会原样保留 `undefined` 并把它返回。
   ⇒ 用例只给数值型 `vely`（`velocity.x` 是 `Value` 型，`u` 已覆盖）。已写进规格头注。

### 30.3 harness 观察点
- 一对 harness `character_state_caught_rowing.{cpp,ts}`。
- `env cls s "caught"|"rowing"`；`env victim s "V1"`；`env has_holding 0|1`（**裸数字**）。
- 输入：`env state|prevstate|fall|fallmax|vteam|velx|vely|holding|onlanding|dataset|indexes`
  （值字面量；`velx`/`vely`/`fall`/`fallmax` 可 `u`）。
  `prevstate` 是传给 `enter` 的 `{ state: … }`。
- `run make|enter|update|landing`。
- 状态文本：`fall=` / `fallmax=` / `vel=[x:y]` / `has_holding=` / `holding=` / `holding_team=` / `team=`。
- ⚠️ **C++ 二元运算符的求值顺序是未指定的**：
  `to_number(e.dataset(A)) * to_number(e.dataset(B))` 在 MSVC 上会**先算右边**，
  而 TS 严格从左到右 ⇒ harness 的 `dataset` 日志顺序分叉。
  修法：把两次读取**拆成独立语句**先读进局部量，再相乘。
  **凡是一个表达式里有两次带副作用的读取/调用，都必须拆开。**

## 31. 切片 5：五个小状态（`state_misc`）

`native/lfw/state/state_misc.{h,cpp}`，一个文件装五个各几十行的小状态：

| 类 | 对应 TS | 覆写点 |
| --- | --- | --- |
| `State_WeaponBroken` | `State_WeaponBroken.ts` | `on_landing` 钩子 → `enter_frame(GONE_FRAME_INFO)` |
| `State_TransformToCatching` | `State_TransformToCatching.ts` | `update` |
| `CharacterState_TransformToLouisEX` | `CharacterState_Transform2LouisEX.ts` | `enter` 钩子 |
| `State_TransformTo8XXX` | `State_TransformTo8XXX.ts` | `leave`（**虚函数**，唯一一个读自己 `_state` 的） |
| `BallState_Base` | `BallState_Base.ts` | `enter` 钩子 |

### 31.1 单元边界（`native/lfw/state/state_base.h` 的补充）
`IStateEntity` 补 8 个**带默认实现的虚函数**：
`transfrom_to_another` / `find_auto_frame` / `transform(data)` / `datas_find(oid)` /
`datas_find_fighter(oid)` / `set_shaking` / `set_motionless` / `world_callbacks_call(name)`。

### 31.2 保真要点
1. **`State_WeaponBroken`**：`on_landing` 直接 `enter_frame(GONE_FRAME_INFO)`，
   连 `truthy` 判断都没有（对比 `CharacterState_Base` / `Rowing` 的 on_landing 都有帧判定）。
2. **`State_TransformToCatching.update`** 与 **`CharacterState_TransformToLouisEX.enter`**
   **都不调 `super`**（前者没有 `handle_ground_velocity_decay`，后者没有基类 `enter` 实现可言）。
3. **`CharacterState_TransformToLouisEX`**：TS 的**类名与文件名不一致**
   （类叫 `CharacterState_TransformToLouisEX`，文件叫 `CharacterState_Transform2LouisEX.ts`），
   端口按**类名**命名。查找字面量是 `"50"`，找不到就整个跳过（不 transform、不进帧）。
4. **`State_TransformTo8XXX.leave` 是本片唯一读「状态对象自己的 state」的地方**：
   ```text
   if (typeof this.state !== "number") return
   oid = "" + (this.state - 8000)
   data = e.lfw.datas.find(oid)
   old_data = e.data
   if (data) e.transform(data)
   e.enter_frame(e.find_auto_frame())
   new_type = e.data.type
   if (old_data.type !== new_type && new_type === EntityEnum.Fighter)
     e.world.callbacks.call("on_fighter_add", e)
   ```
   - `typeof … !== "number"` ⇒ 端口用 `!std::holds_alternative<double>(state())`，
     **字符串形态的 state 会被挡掉**（用例专门覆盖）。
   - `"" + (state - 8000)` 是 JS 的数字转字符串 ⇒ `to_string(Value(num))`。
   - `old_data` 必须在 `transform` **之前**取。
   - 两个类型比较都是**严格**（`!==` 与 `===`）。
5. **`BallState_Base`**：TS 里**没有构造函数**（直接继承 `State_Base.enter` 的位置是空的），
   端口为了装 `enter` 钩子才写了一个构造函数。四个球状态用**严格** `===` 逐个判否（德摩根后是 `&&` 链），
   命中后清 `shaking`、`motionless`、速度（三个都清）。
   注意它覆写的是 `State_Base` 里的**钩子**，不是虚函数。

### 31.3 harness 观察点
- 一对 harness `state_misc.{cpp,ts}`（`env cls` 选 5 个类之一）。
- 输入：`env victim s "V1"`、`env state|vstate|vtype|finddata|findfighter|transformtype`（值字面量）。
  `finddata` / `findfighter` 是 `datas_find*` 的返回值；`transformtype` 是 `transform()` 之后
  实体新的 `data.type`。
- `run make|landing|update|enter|leave`。
- 状态文本：`state=`（状态对象自己的）/ `vstate=`（实体）/ `type=` / `shaking=` / `motionless=` / `vel=[x:y:z]`。
- ⚠️ 同名调用要去重：`e.enter_frame(e.find_auto_frame());` 在本文件里出现 **3 次**
  （`cs2_enter` / `update` / `leave`），写变异锚点必须带上前后一行。
- ⚠️ 「宽松比较」类变异需要**字符串形态**的输入：`transformtype s "8"` 是
  `strict_equals(new_type, Fighter)` → `equals` 的唯一观测点（本轮唯一一次存活）。

### 29.5 修订：`ctrl_ud()` / `ctrl_lr()` 由 `bool` 改为 `double`

`CharacterState_Dash`（§32）要把上下/左右输入**当数字用**（`UD * dz`、`LR * dx`），
而 §29 落地时这两个缝是 `bool`。现已改为：

```text
virtual double ctrl_ud() const { return 0; }
virtual double ctrl_lr() const { return 0; }
```

- **`Walking` 的端口文本一个字都不用改**：`!e.ctrl_ud()` 对 `double` 依然成立
  （`!0.0` 为真、`!(-1.0)` 为假），与 JS 的 `!UD` 完全一致。
  ⇒ 它的变异锚点也没变，门禁复跑仍 **34/34 全杀**。
- **改返回类型是「大声失败」而不是静默失配**：旧 harness 里写的是
  `bool ctrl_ud() const override`，返回类型不协变 ⇒ **编译报 C2555**（与
  `character_state_caught_rowing` 里 `velocity_y()` 那次同一类错误）。这正是「可以改已有缝的类型」与「不可以改已有缝的形参个数」
  的区别：前者会报错，后者会静默退化成隐藏而非覆写。
- 语义边界：控制器取值只可能是 `0 / ±1`，`NaN` 不可能出现，所以 `double` 足够；
  这点已写进 `character_state_dash` 的规格头注。

## 32. 切片 5：`state/CharacterState_Dash`

`native/lfw/state/character_state_dash.{h,cpp}`（对应 `src/LFW/state/CharacterState_Dash.ts`）。
本片第一个**用数字型 ctrl** 的状态。

### 32.1 保真要点
1. **`enter` 的早退守卫要求两个条件同时成立**：
   ```text
   if (e.position.y > e.ground_y && e.velocity.y !== 0) return
   ```
   贴地（`y == ground_y`）时即使有垂直速度也**不**早退；
   悬空但垂直速度为 `0`（或缺失 ⇒ NaN ⇒ `NaN !== 0` 为真）时**要**早退。
   `!==` 是严格比较。
2. **六个 dataset 的读取顺序即日志顺序**：
   `dash_distance` → `dash_x_f`（→ `dx`）→ `dash_distancez` → `dash_z_f`（→ `dz`）→
   `dash_height` → `dash_h_f`（→ `vy`）。端口把六次读取**拆成独立语句**再相乘，
   否则 MSVC 的求值顺序与 TS 的左到右不一致（同 §30.3 的坑）。
3. **`next_vz` 的初值会被保留**：`let next_vz = vz`，只有 `if (UD)` 为真才被覆盖
   ⇒ 没有上下输入时 z 速度**原样保留**（`next_vx` 的初值则总是被后面的 if/else 链覆盖，
   属死代码，已写进规格头注）。
4. **x 方向的五路选择**（顺序即优先级）：
   ```text
   if (prev_frame.state === Running) next_vx = facing * dx     // 严格比较
   else if (LR)                      next_vx = LR * dx          // 真值判定，乘出来的符号是 ±
   else if (vx > 0)                  next_vx = dx
   else if (vx < 0)                  next_vx = -dx
   else                              next_vx = facing * dx
   ```
   注意第 4、5 支用的是**实体当前 x 速度的符号**决定冲刺方向，
   而两个兜底支用的是 `facing`。
5. `vy = dash_height * dash_h_f` 是**向上初速度**，与 x/z 一起交给 `set_velocity(next_vx, vy, next_vz)`。
6. 构造函数默认参数（`StateEnum.Dash`）在本单元不可观测。

### 32.2 harness 观察点
- 一对 harness `character_state_dash.{cpp,ts}`。
- 输入：`env victim s "V1"`、`env ctrlud|ctrllr`（**裸数字**，`-1/0/1`）、
  `env state|prevstate|pos|ground_y|velx|vely|velz|facing|dataset`（值字面量）。
- `run make|enter`。状态文本：`pos=[x:y:z]` / `ground=` / `vel=[x:y:z]` / `face=` / `ctrl=ud,lr`。
- ⚠️ **`set_velocity` 会写回 `velx`/`vely`/`velz`**，所以「上一场景留下的速度」会污染下一场景。
  本单元首轮 3 条存活全部源于此：`prevstate n 2` 的场景写回 `velx = -5`，
  于是 `prevstate s "2"` 场景里的 `vx` 已是 `-5`，走的是 `vx < 0` 支（结果与
  Running 支**相同**），宽松比较变异因此不可观测。
  ⇒ **每个场景都要显式设置它依赖的每一个速度分量。**
- ⚠️ `facing = -1` 会让 `facing * dx` 与 `-dx` **数值相同**，
  于是「`vx < 0` 支被删」与「兜底支忽略 facing」两条变异互相掩护。
  必须**同时**给 `facing = 1`（区分 `-dx`）与 `facing = -1`（区分 `dx`）两组零速度场景。

## 33. 切片 5：`state/CharacterState_Burning`

`native/lfw/state/character_state_burning.{h,cpp}`（对应 `src/LFW/state/CharacterState_Burning.ts`）。
第一个同时覆写 `enter` / `update` / `leave` / `on_landing` 四个位置的状态。

### 33.1 单元边界（`native/lfw/state/state_base.h` 的补充）
`IStateEntity` 补 8 个**带默认实现的虚函数**：
`bounced` / `set_bounced` / `has_catcher` / `catcher_drop_catching` / `set_facing` /
`world_dataset(key)` / `data_indexes_bouncing` / `data_indexes_lying`。

其中 `world.dataset.*` 用「按 key 取值」的缝（`world_dataset("cha_bc_tst_spd_y")` 等），
而不是给每个字段开一个成员。

### 33.2 保真要点
1. **`enter` 里调的是 `super.update(e)` 而不是 `super.enter`** —— 这是 TS 原文的
   怪异之处（`override enter(...) { super.update(e); ... }`），端口**照抄**：
   安装的 lambda 里调 `this->CharacterState_Base::update(e)`。
   注意 `CharacterState_Base::update` 不是静态成员，自由函数无法直接调用，
   所以这个钩子必须写成**捕获 `this` 的 lambda**。
   实际重启时 `enter` 只做三件事：驱动基类 update（⇒ 地面速度衰减）、
   `bounced = false`、有 catcher 就 `drop_catching()`。
2. **`update`**：`super.update(e)` 之后
   `vx = e.velocity.x; if (vx) e.facing = vx > 0 ? -1 : 1` ——
   即「朝速度方向的反面转」。`if (vx)` 是真值判定，
   **`0` 不转头**（所以 `> 0` 写成 `>= 0` 是不可观测的，已写进规格头注）。
3. **`leave`**：`super.leave(e, next_frame)` 之后 `bounced = false`。
   `super.leave` 落在 `State_Base::leave` 的 `HealSelf` 分支上（`Burning` 不可能命中）
   ⇒ 删掉这次调用**不可观测**（已写进规格头注）。
4. **`on_landing`**：
   ```text
   if (e.frame.on_landing) { enter_frame(on_landing); return }
   if (!e.bounced && (vy <= world.dataset.cha_bc_tst_spd_y
                      || abs(vx) > world.dataset.cha_bc_tst_spd_x)) {
     enter_frame_by_id(indexes?.bouncing?.[-1][1])
     set_velocity(null, world.dataset.cha_bc_spd)      // 只写 y
     e.bounced = true
   } else {
     enter_frame_by_id(indexes?.lying?.[-1])
   }
   ```
   - **`||` 的短路会影响 `dataset` 的读取条数**：`vy` 达标时右侧的
     `cha_bc_tst_spd_x` **不会被读**，日志也就少一条。端口保持同样的 `||` 顺序。
   - `bouncing` 是 `{"-1": [...]}` 形状（取 `[-1][1]`，第二个元素），
     而 `lying` 是 `{"-1": id, "1": id}` 形状（`[-1]` 直接给帧 id）。
   - `set_velocity(null, spd)` 只给 y，x 是 `null`、z 省略。
5. 构造函数**没有**默认参数（TS 里就是 `constructor() { super(StateEnum.Burning) }`）。

### 33.3 harness 观察点
- 一对 harness `character_state_burning.{cpp,ts}`。
- 输入：`env victim s "V1"`、`env catcher 0|1`（**裸数字**）、
  `env indexes|wdata|facing|bounced|velx|vely|velz|landingvel|onlanding`（值字面量）。
  `landingvel` 是传给 `on_landing` 的 `{x, y}` 速度对象。
- `run make|enter|update|leave|landing`。
- 状态文本：`bounced=` / `facing=` / `vel=[x:y:z]` / `catcher=`。
- ⚠️ **TS 侧的 `world.dataset` 用 `Proxy` 打日志**：`e.world.dataset.X` 是**属性读**，
   而端口是 `world_dataset("X")` **函数调用**；用 `Proxy` 的 `get` 陷阱才能让两边
   的日志条数与顺序（含短路少读）完全一致。
- ⚠️ **`bounced` 会被 `on_landing` 自己置真**（弹跳分支里 `e.bounced = true`），
   于是**后续所有 `landing` 场景都会被 `!e.bounced` 挡住**。
   本单元首轮 4 条存活（全在 `||` 右侧）就是因为它：
   「缓落弹跳」场景把 `bounced` 置真后，后面两条本想观测右侧条件的场景直接走了 else。
   ⇒ **每个 `landing` 场景都要显式 `env bounced n 0`。**

## 34. 切片 5：`state/CharacterState_Teleport2*`

`native/lfw/state/character_state_teleport.{h,cpp}`（对应
`src/LFW/state/CharacterState_Teleport2NearestEnemy.ts` 与
`CharacterState_Teleport2FarthestAlly.ts`）。
两个 TS 类**镜像**实现：除「同盟极性」与「接受更优者的谓词」外逐字节相同。

### 34.1 单元边界（`native/lfw/state/state_base.h` 的补充）
`IStateEntity` 补 9 个**带默认实现的虚函数**：
`world_entities()`（世界实体列表）、
`is_fighter_ref(o)` / `is_self_ref(o)` / `is_ally_ref(o)` / `ref_hp(o)` /
`ref_position_x(o)` / `ref_position_z(o)`、
`ground_segment(x, z)` / `ground_y(segment, x, z)`。

**静默 vs 打日志**这条界线在本单元最关键：
`is_*_ref` / `ref_*` 对应 TS 的**属性读**（`o.hp`、`o.position.x`、`o === m`、
`o.is_ally(m)`）⇒ 端口里**静默**，不打日志；
而 `world.ground.segment(x, z)` 与 `world.ground.y(seg, x, z)` 在 TS 里是
**方法调用** ⇒ 端口里**打日志**，两个 harness 必须给出一致的条数与顺序。

### 34.2 保真要点
1. **`abs(o.position.z - o.position.z)` 是 TS 原文的 bug**（恒为 0），端口**照抄**成
   `abs(e.ref_position_z(o) - e.ref_position_z(o))`。
   规格里用「第二个操作数换成 `mz` / 第一个换成 `mx`」两条变异证明用例能看见这个死项。
2. **两个类的谓词不对称**：近敌是 `if (_dis < 0 || dis < _dis)`（首个候选无条件接受）；
   远盟是 `if (dis > _dis)`（靠 `_dis` 初值 `-1` 接受首个候选）。
   端口写成一条三元：`nearest_enemy ? (best < 0 || dis < best) : (dis > best)`。
3. **两个类合并成两个自由函数**
   （`pick_target(e, bool nearest_enemy)` + `teleport_to(e, tar)`）：
   两份镜像体逐字节相同，直接复制会让**每一条变异锚点都二义**（同 §25.2 的理由）。
   这个合并**没有引入**原对里不存在的分支 —— 那个布尔开关正是「同盟极性 + 比较谓词」本身。
4. **`let { x, z } = m.position` 在 `if (_tar)` 之前** ⇒ 端口**先无条件**读一次
   `e.position(x, y, z)`。TS 只解构 `x` / `z`，端口多读了一个 `y`（不用它）；
   因为读是静默的，`my` / `mz` 这两个槽位**不可观测**（已写进规格头注）。
5. `x = round(_tar.position.x - m.facing * 120)`：**只有 x 带偏移**，
   `z = round(_tar.position.z)` 没有；`round` 是 `utils/math/base` 的版本。
6. 无目标时**保留自身坐标**（`x` / `z` 来自 `m.position`），
   但 `y` 仍然来自 `world.ground.y(...)` —— 所以「无目标」不是「什么都不写」。
7. 构造函数带默认实参（`StateEnum.TeleportToNearestEnemy = 400` /
   `TeleportToFarthestAlly = 401`），端口保留；harness 用 `run default` 观测它。

### 34.3 harness 观察点
- 一对 harness `character_state_teleport.{cpp,ts}`。
- 输入：`env cls s "nearest"|"farthest"`、`env state`（值字面量）、
  `env victim s "M1"`、`env facing|seg`（值字面量）、`env gy <裸数字>`、
  `env pos o 3 x n .. y n .. z n ..`、
  `env ent o 6 id s "E1" fighter b .. ally b .. hp n .. x n .. z n ..`
  （**按 id 覆盖或追加**，保留插入序，插入序本身是搜索序）。
- `run make|default|enter`。输出 `run <op> || <日志> | <状态>`；
  状态文本 `id= pos=[x:y:z] face= seg= gy=`。
- ⚠️ **自身实体必须是同一个对象**：TS 侧 `env ent` 的 id 若等于 `victim` 的 id，
  必须把 `victim` **本身**放进实体列表（而不是包一层代理对象），否则 `o === m`
  恒假，「跳过自身」永远不会触发。本单元首轮差分就是在这里漂移的
  （TS 选中了自身 ⇒ `x = -119`，端口选中 `E5` ⇒ `-117`）。
  相应地 TS 的 `FakeEnt` 要暴露 `data.type = 8` 与 `hp`，否则会在
  `is_fighter(o)` 这步就被过滤掉。
- ⚠️ **切换 `env cls` 之后必须重新 `run make`**：状态对象是 `make` 时按 `cls`
  构造的，`env cls` 本身不会换对象。本单元首轮**唯一**存活的变异
  （「谓词忽略搜索方向」）就是远盟那一段忘了 `run make`：
  结果用近敌对象跑了「只有同盟」的场景，两边都退化成自身位置，无法区分。

## 35. 切片 5：`state/WeaponState_Base` + `defines/weapon_bounce.h`

`native/lfw/state/weapon_state_base.{h,cpp}`（对应 `src/LFW/state/WeaponState_Base.ts`）
与新的手写常量表 `native/lfw/defines/weapon_bounce.h`
（对应 `src/LFW/defines/defines.ts` 里 `Defines.WT_*` 九张表）。

### 35.1 常量表（`lfw/defines/weapon_bounce.h`）
`Defines.WT_BOUNCE_MIN_Y/X/Z`、`WT_BOUNCE_Y/X/Z`、`WT_FAST_Y/X/Z` 都是
`Record<WT, number>`，即「以武器类型为键的对象」。TS 里用一个**不在范围内的键**
（`undefined`、`9`、`2.5`）取值会得到 `undefined`，从而让调用点的 `??` 兜底生效。
所以端口不能返回一个「随便什么样的 double」，而是：

- `weapon_bounce_index(wt)` 把 `Value` 映射成 `0..5` 的整数下标，**否则返回 `-1`**；
- 九个 `wt_*()` 访问器在下标为 `-1` 时返回 `Value()`（缺失），由调用点 `coalesce2` 兜底。

⚠️ **`-1` 必须被拦住**：表是定长 `std::array<double,6>`，负下标是 UB 而不是可观测分歧
（规格头注里写明了这条，避免以后有人把「范围检查」当成死代码删掉）。

### 35.2 保真要点
1. **三个钩子在构造函数里安装**（`get_auto_frame` / `on_landing` / `on_leave_ground`），
   `update` 是 `override`（TS 里 `WeaponState_Base.update` 只调
   `e.handle_ground_velocity_decay()`，**不调 `super.update`**）。
2. `get_auto_frame` 的 `if (!indexes) return void 0;` 是**真守卫**：
   端口用新缝 `has_data_indexes()` 表达（TS 是属性读 ⇒ 静默）。
   它只有在「`indexes` 是假值、但 `frames` 里恰好有键 `"undefined"`」时才可观测 —— 用例里
   专门造了这个组合（`env indexes n 0` + `frames` 的 `undefined` 键）。
3. `on_landing` 走 `frame.on_landing` 优先、否则 `enter_frame_by_id(indexes?.on_ground)`
   （**没有** truthy 判定，与 `CharacterState_Base` 的 `landing_2` 形成对照）。
4. `on_leave_ground` 是 `enter_frame(Defines.NEXT_FRAME_AUTO)` ——
   实参本身是**帧对象**，端口用文件内的 `next_frame(Value(u"auto"))` 复刻。
5. `hit_ground_rebouncing(e, nf, velocity)` 是**类的新方法**（不是钩子）：
   - 三个反弹系数与三个阈值都用 `base.* ?? Defines.WT_*[wt] ?? 默认` 三级兜底
     （端口用文件内 `nullish` + `coalesce2`，因为 `??` 对 `null` 也兜底，
     而端口的 `missing()` 只认 `undefined`）。
   - **`is_bounce` 的第 4 项是 `dvx >= bounce_min_z || dvx < -bounce_min_z`**
     —— 左右两边都写的是 `dvx`（TS 原文），端口**照抄**，并专门为它造了两条反向用例。
   - `if (!e.drop_hurted) { e.drop_hurted = true; if (base.drop_hurt) {...} }`：
     标志位与伤害是**两层**守卫，「falsy 的 `drop_hurt`」只翻转标志。
   - `e.hp = e.hp - base.drop_hurt` 与 `e.hp_r = e.hp_r - base.drop_hurt` 是**属性写** ⇒
     端口的 `set_hp` / `set_hp_r` / `set_drop_hurted` 三个缝必须**静默**，
     靠状态文本观测。
   - 反弹分支里的 `const nf = e.find_align_frame(...)` **遮蔽了参数 `nf`**
     （TS 原文如此），端口用不同名字（`align`）但保持同一作用域。
   - `e.state == SE.Weapon_Throwing` 是**宽松** `==` ⇒ 端口用 `equals`。
   - `fast_*` 四个比较都是**开区间**（`> -fast` 且 `< fast`）。
6. **`Heavy` 的三个 `fast_*` 在表里都是 `1`，而它的阈值是 `min_y=min_x=2`、
   `min_z=99`** ⇒ 反弹条件成立时 `|dvy| >= 2` 或 `|dvx| >= 2`，
   与 `|dvy| < 1 且 |dvx| < 1` 矛盾 ⇒ **Heavy 永远进不了 align 分支**。
   这条推理写进了变异规格头注：改这三个表项是不可观测的，不要硬造用例。
7. `Defines.WT_BOUNCE_Z` 与 `Defines.WT_BOUNCE_X` **逐项相同** ⇒
   「深度反弹走 x 表」这类变异不可观测（同样写进规格头注）。

### 35.3 harness 观察点
- 一对 harness `weapon_state_base.{cpp,ts}`。
- 输入：`env state|indexes|ionground|ithrow|isky|frames|onlanding|base|wt|dh|vstate|fid|vel|nf|align`
  （值字面量）、`env hp|hpr <裸数字>`、`env onground <真值>`。
- `run make|auto|landing|update|leaveground|rebound`。
- 状态文本 `hp= hpr= dh=`；`auto` 额外打印 `fid=`，`rebound` 靠日志看
  `set_velocity` / `leave_ground` / `find_align_frame` / `enter_frame`。
- ⚠️ **`frames` 的键必须写成裸 token**：`o 3 4 s "F4" 5 s "F5" undefined s "FU"`
  （`o` 的计数是**键值对**个数，不是 token 数）。第一次就是因为把键写成了 `s "4"` 才被拒。
- ⚠️ 被 `hit_ground_rebouncing` 写回的字段（`dh`、`hp`、`hp_r`）**每段场景开头都要重置**。

## 36. 切片 5：`state/StateBase_Proxy` + `State_15` + `State_Frozen`

`native/lfw/state/state_base_proxy.{h,cpp}`（对应 `StateBase_Proxy.ts` / `State_15.ts` /
`State_Frozen.ts` 三个小类，按 `state_misc` 的先例合并成一个单元）。

### 36.1 单元边界
`IStateEntity` 补两个带默认实现的虚函数：`play_sound(const Value& sounds)`、
`apply_opoints(const std::vector<Value>& opoints)`。

### 36.2 保真要点
1. **`leave` / `update` / `on_restrict` 是虚函数，不是可选钩子**：`State_Base` 里只有
   `pre_update` / `enter` / `on_dead` / `on_landing` / `get_*` / `find_frame_by_id` /
   `on_leave_ground` 是 `std::function` 成员。端口第一版把 `leave` 当成钩子成员写，
   编译直接报 `C3867`（`leave` 解析成了成员函数）——**看到 `?.()` 不要想当然，先看它在
   基类里是方法还是可选属性**。
2. **四个代理按值持有**（`_character_proxy` / `_weapon_proxy` / `_ball_proxy` / `_proxy`），
   构造函数里 `State_Base(state)` 与四个成员**都用拷贝**：如果基类初始化列表写
   `std::move(state)`，四个成员就会拿到被掏空的值（基类先于成员初始化）。
3. **TS 的四个可选构造参数没有人传过**（只有 `State_15` 传 `void 0` × 4，等价于默认值）
   ⇒ 端口只保留 `StateBase_Proxy(Value state)`，四个成员一律用 `state` 构造。
   这是一处**有意简化**，已写进变异规格头注。
4. **枚举判定要用 `is_*_data`**：`entity::is_fighter(v)` 内部读的是 `v.data`，即它期望
   一个**实体形对象**；代理手上只有 `e.data()`（就是那个 `data` 对象），所以要用
   `entity::is_fighter_data(data)` 等三个函数。用错会静默落到 `_proxy`（首轮差分就是这么漂的）。
5. `StateBase_Proxy` 的 11 个转发里，`get_*` 四兄弟与 `find_frame_by_id` 在钩子为空时
   返回 `Value()`（对应 TS 的 `?.()` → `undefined`）；其余返回 void。
6. `State_15` 就是 `StateBase_Proxy(StateEnum.Normal)`，四个代理全默认。
7. `State_Frozen`：
   - `enter` **包装**（不是覆写）代理装好的 `enter`：先调 `super_enter`（转发给目标类型），
     再 `if (e.catcher) e.catcher.drop_catching()`、`if (e.holding?.base_type == Heavy)
     e.drop_holding()`（**宽松** `==`）、最后 `play_sound(["data/065.wav.mp3"])`。
   - `leave` 是**虚函数覆写**：`super.leave` → `play_sound(["data/066.wav.mp3"])` →
     `apply_opoints(ice_piece_opoints)` → **再调一次 `super.leave`**（TS 原文如此）。
     这条「调两次」正是本单元最好的变异目标：`State_Base::leave` 在 `HealSelf` 状态下会
     `grant_buff`，于是 buf 会被授予两次。
     ⚠️ **必须在 `env state n 1700` 之后重新 `run make`**：状态对象是用构造时的
     `state` 建的，改 `env state` 不会改已有对象（首轮 5 条存活就是这个原因）。
   - `on_landing` **完全不调 super**（与 `CharacterState_Base`/`WeaponState_Base` 的钩子
     无关）：`frame.on_landing` 优先，否则
     `vy <= world.dataset.cha_bc_tst_spd_y * 2` 时进 `bouncing[-1][0]`、
     `set_velocity(null, cha_bc_spd)`、`hp -= 10`。
8. `sound_list()` 是文件内 helper：把单个路径包成一个数组（TS 传的是 `["..."]`）。

### 36.3 harness 观察点
- 一对 harness `state_base_proxy.{cpp,ts}`。
- 输入：`env cls s "proxy"|"15"|"frozen"`、`env dstate`（构造参数，用 `state`）、
  `env data|indexes|frames|onlanding|wdata|hbtype|vx|vz|vel|rid|pos|rxyz`（值字面量）、
  `env vstate`（实体状态）、`env hp <裸数字>`、`env catcher|onground <真值>`。
- `run make|default|update|leave|restrict|preupdate|enter|dead|landing|leaveground|gravity|auto|sdf|cef|ffbi`，
  返回型钩子打印 `r=`，其余打印日志。
- ⚠️ **TS 侧 `set_shaking` / `set_motionless` 是属性写**（`e.shaking = 0`），
  所以假实体要用 **setter**（`set shaking(v)`）而不是方法，才能与端口缝的日志对齐。
- ⚠️ **`set_position` 两侧都必须静默**：TS 原文写的是 `e.position.x/y/z`（三次属性写），
  端口是 `set_position(x,y,z)` 一次调用；把位置放进状态文本比较。
  为此 TS 假实体的 `position` getter 必须返回**同一个持久对象**，否则写丢失。
- ⚠️ TS 侧真 `grant_buff` 需要 `victim.lfw.factory.create_buff` 与 `victim.world.buffs`；
  假实体要提供 `world.buffs = new Map()` 与一个打日志并返回 `undefined` 的 `create_buff`，
  才能与 C++ 侧 `BuffEnv::create_buff` 对齐。

## 37. 切片 5：`state/WeaponState_OnGround` / `_OnHand` / `_Throwing` / `_InTheSky`

`native/lfw/state/weapon_state_misc.{h,cpp}`（对应
`src/LFW/state/WeaponState_OnGround.ts` / `_OnHand.ts` / `_Throwing.ts` / `_InTheSky.ts`）。
四个类都直接继承 `WeaponState_Base`，合并成一个单元。

### 37.1 单元边界（`native/lfw/state/state_base.h` 的补充）
`IStateEntity` 补 9 个**带默认实现的虚函数**：
`motionless` / `has_bearer` / `bearer_motionless` / `set_bearer_motionless` /
`lfw_new_team` / `frame_behavior` / `set_dropping` /
`data_indexes_throw_on_ground` / `data_indexes_just_on_ground`。
分别对应 TS 的 `e.motionless`、`e.bearer.motionless`、`e.lfw.new_team`、
`e.frame.behavior`、`e.dropping`、`indexes?.throw_on_ground` / `?.just_on_ground`。

### 37.2 保真要点
1. 四个类在 TS 里都**没有构造函数默认实参**（继承来的 `state` 是必填），
   端口保持 `explicit X(Value state)`；`ENTITY_STATES` 对普通/重型武器的多次实例化
   都靠显式传值。
2. **`WeaponState_OnGround.enter`** 只做 `e.team = e.lfw.new_team`；
   `update` 只有 `handle_ground_velocity_decay`。`e.team` 是属性写 ⇒
   端口的 `set_team` 静默，靠状态文本 `team=` 观测。
3. **`WeaponState_OnHand.pre_update`**：`if (e.motionless && e.bearer)` 是**两层**
   真值判定；`max` 是 `Math.max`（NaN 传播、`-0/+0` 规则）⇒ 端口用 `lfw::max`。
   `e.bearer.motionless` 是「另一个实体上的属性」，端口开成
   `has_bearer` / `bearer_motionless` / `set_bearer_motionless` 三个静默缝。
4. **`WeaponState_Throwing.get_gravity`**：`e.frame.behavior == FrameBehavior.Boomerang`
   是**宽松**比较（`"3"` 也算）⇒ 端口 `equals`；boomerang 走
   `round_float(dataset / 4)`，否则原样返回 dataset（缺失 ⇒ `undefined/4 = NaN`）。
5. **`WeaponState_Throwing.enter`** 的顺序是 `leave_ground()` →
   `drop_hurted = false` → `if (boomerang) set_velocity(e.velocity.x * 0.6)`
   （**单实参**，y/z 缺省 ⇒ 端口传 `Value(), Value()`）；乘 0.6 **不取整**
   （真正的取整在 `Entity.set_velocity` 内部，本单元被 fake 缝掉）。
6. **`WeaponState_Throwing.on_landing`** 的
   `indexes?.throw_on_ground || indexes?.just_on_ground` 需要一个
   「取第一个真值」的复刻；两个读取都静默，但**选择顺序**决定传给
   `hit_ground_rebouncing` 的 `nf`。
7. **两处 `find_align_frame` 的实参顺序不同**：`WeaponState_Base.hit_ground_rebouncing`
   是 `(id, throwings, in_the_skys)`，`WeaponState_InTheSky.update` 是
   `(id, in_the_skys, throwings)`（TS 原文如此）。端口各自照抄，
   用例用同一组 `isky/ithrow` 数组同时锁住两条路径。
8. `wt != WT.Heavy` 是**宽松** `!=`（`"2"` 也算重型）；`fast_*` 是
   `base ?? 表 ?? 99` 三层兜底，端口用 `coalesce2`（`null` 与 `undefined` 都兜底，
   这一点由 `fast_vx z` 的用例锁住）。
9. **不可观测的三处**（写进变异规格头注）：
   - Heavy 的 `fast_*` 表值全是 1，而重型被守卫排除；
   - 其余类型的 `fast_y` / `fast_z` 表值全是 99，与最终兜底相同
     ⇒ 这两条中间兜底删掉不可观测（`fast_x` 的 4.5（Baseball）可观测）；
   - `set_drop_hurted` 与 `leave_ground` 的先后顺序（`dh` 无日志）。
10. `nf` 为真时先 `e.dropping = false`（属性写 ⇒ 静默）再 `enter_frame(nf)`。

### 37.3 harness 观察点
- 一对 harness `weapon_state_misc.{cpp,ts}`；`env cls` 选类，构造参数走 `env state`。
- 输入与覆盖面见 `PROTOCOL.md` §6.9.89。
- 状态文本 `team= dh= hp= hpr= motionless= bmotion= dropping=`；
  `run make` 额外打 `s=`，`run gravity` 额外打 `r=`。
- ⚠️ **`base` 必须一直是有键对象**（TS 读 `base.fast_vx` 没有 `?.`）；
  `o` 的计数是键值对个数。
- ⚠️ `nf` 为 `undefined` 的落地组合**故意不覆盖**：
  `enter_frame_by_id` 在 C++ 是 `std::u16string` 形参，会把 `undefined` 渲染成
  字符串 `"undefined"`，与 TS 的 `u` 不同（属工具边界，不是语义差异）。

## 38. 切片 5：`state/CharacterState_Drink` + `state/State_Burning`

`native/lfw/state/character_state_drink.{h,cpp}`（对应
`src/LFW/state/CharacterState_Drink.ts`）与 `native/lfw/state/state_burning.{h,cpp}`
（对应 `src/LFW/state/State_Burning.ts`）。

### 38.1 单元边界（`StateBase_Proxy` 的代理注入）
TS 的 `StateBase_Proxy` 构造器可以注入四个代理，`State_Burning` 注入了
`new CharacterState_Burning()` 作为 character 代理（其余三个用默认值）。
§36 的端口当时把四个成员**按值**持有并统一用 `state` 构造，无法表达这个需求，
所以本片给 `StateBase_Proxy` 补了一个重载：

```cpp
StateBase_Proxy(Value state, std::unique_ptr<CharacterState_Base> character_proxy);
```

`_character_proxy` 由 `CharacterState_Base` 值成员改成 `std::unique_ptr`，
以保住多态（`CharacterState_Burning` 覆写了 `update`/`leave`）。
单参构造器委托给双参版本（传 `nullptr` 时按 `state` 默认构造）。
`State_15` / `State_Frozen` / 既有 `state_base_proxy` 用例的语义不受影响。

`IStateEntity` 补 5 个带默认实现的虚函数：
`hp_max` / `holding_drink`（返回 `DrinkInfo*`）/ `holding_set_hp` /
`holding_set_hp_r` / `holding_set_velocity` / `holding_mt_range`
（`has_holding` 复用 6u 的缝）。

### 38.2 保真要点
1. **`State_Burning` 只做三件事**：默认 state 取 `StateEnum.Burning`（18）、
   把 `CharacterState_Burning` 注入代理、其余三个代理留给基类默认。
   `state` 参数仍可覆写（构造函数默认实参）。
2. **分派即代理的全部行为**：Fighter（`data.type = 8`）→ `CharacterState_Burning`、
   Weapon（16）→ `WeaponState_Base(state)`、Ball（32）→ `BallState_Base(state)`、
   其它 → 裸 `State_Base(state)`。端口用 `entity::is_*_data(data)`（读 `e.data`），
   与 `is_fighter(e)`（读 `v.data`）的区别见 §36.2 第 4 条。
3. **`CharacterState_Drink.update` 照抄原文的四段结构**：
   - `super.update`（地面速度衰减）永远先跑；
   - `holding` / `drink` 两层守卫，缺一即返回；
   - 三个 `*_empty` 是**先解构再分支**（`hp_h_empty = hp_h >= hp_h_total || !hp_h_value`）；
   - 三段恢复各自的 `add()` 门控 + `min(…, current + value)` 钳制，
     随后累加 `drink.*_h`；
   - **三段都空**才掉罐子：`drop_holding` → `enter_frame(NEXT_FRAME_AUTO)` →
     `holding.hp_r = 1`、`holding.hp = 1`（赋值链从右到左）→
     `mt.range(-6, 6) / 2` → `holding.set_velocity(vx, 6, 0)`。
4. **hp_r 的钳制也用 `e.hp_max`**（TS 原文如此，没有独立的 `hp_r_max`），端口照抄。
5. **类型边界**：`drink.*_value` 在 TS 是 `number`，`e.hp + value` 的 `+` 在
   字符串输入下会变成拼接；端口按数值相加（`to_number`），
   用例只喂数字（已写进变异规格头注）。
6. **两处有意不移植**（写进变异规格头注）：
   - `holding.lfw.mt.mark = "drink_drop"` 是 MT 的调试探针，C++ MT 端口没有 `mark`；
   - `holding.hp = holding.hp_r = 1` 的**赋值顺序**不可观测（两个缝都静默，
     终值又相同）。
7. `mt.range(-6, 6)` 的结果由缝注入（`holding_mt_range` 打日志并返回注入值）；
   真 MT 流由 `mersenne_twister` 单元负责，本片只锁参数与后续算术。

### 38.3 harness 观察点
- 一对 harness `burning_drink.{cpp,ts}`；`env cls` 选类。
- 输入与覆盖面见 `PROTOCOL.md` §6.9.90。
- 状态文本 `hp= hpr= hpmax= mp= mpmax= state= bounced= facing= holding= hhp= hhpr= drink=<快照> pos=[…]`；
  `run make|default` 额外打 `s=`。
- ⚠️ `indexes.bouncing` 的索引形状是 `{"-1":[_, id]}`、`lying` 是 `{"-1": id}`，
  参与 `enter_frame_by_id` 的值必须是**字符串**（否则撞 §37.3 的 `to_string` 差异）。
- ⚠️ `env drink` 直接构造真 `DrinkInfo`，状态文本打 `to_snapshot()` ——
  三个 `Times` 的内部状态因此全部可观测（`add()` 是否被调用也不例外）。

## 39. 切片 5：`state/CharacterState_Jump`

`native/lfw/state/character_state_jump.{h,cpp}`（对应 `src/LFW/state/CharacterState_Jump.ts`）。

### 39.1 单元边界（`IStateEntity` 补 12 个缝）
`ctrl_is_bot()` / `ctrl_is_end(const std::u16string&)`、`jumping_x/y/z/t()` 与
`set_jumping_x/y/z/t()`（8 个）、`prev_frame()`、`update_velocity(const Value&)`。
其余用既有缝：`position`、`set_position`、`ground_y`、`dataset`、`world_dataset`、
`ctrl_lr/ud`、`handle_ground_velocity_decay`、`frame_on_landing`、
`data_indexes_landing_1`、`enter_frame`、`enter_frame_by_id`、`set_velocity`。

### 39.2 保真要点
1. **钩子装配**：构造函数里只装 `enter` 与 `on_landing`（`update` 是重写的虚函数，
   不是钩子）。默认 state 取 `StateEnum.Jump`（4），`state` 参数仍可覆写。
2. **`enter` 只清四个 `jumping` 字段**（x→y→z→t 各写一次 0），不碰位置、速度、帧。
3. **`update` 的骨架是「衰减 → 落点判断 → 计步 → 起跳」**：
   - `handle_ground_velocity_decay()` 无条件先跑；
   - `if (!float_equal(position.y, ground_y)) return;`（**空中不记步**，也不清
     `jumping`；`x`/`z` 不参与判断）；
   - `const { jump_flag } = e.get_prev_frame()` 在**分支之前**读取，
     但只在计步结束后才用；
   - 计步分两条路：机器人 `t`、`y` 各加一次 `atom_time`；人类 `t` 一定加，
     然后 `R`→`+x`、`L`→`-x`、`U`→`-z`、`D`→`+z`、`j`→`+y`，每步都是
     `round_float` 后的**读-改-写**，条件是 `!is_end(键)`（即该键被按住）；
   - `if (!jump_flag) return;` 是**宽松真值**（字符串 `"0"` 也为真）。
4. **`round_float` 每个计步都用一次**：`atom_time = 1.2345` 的用例把
   「累加后取整」锁死（连续两次 1.2345 会累积到 3 而不是 2.469）。
5. **起跳参数按 TS 的书写顺序逐条读取**：`jump_height` → `jump_h_f`
   （`vy` 初值）→ `jump_distancez` → `jump_z_f`（`vz`）→ `jump_distance` →
   `jump_x_f`（`vx`）。端口把每条 `e.dataset(...)` 拆成独立语句，日志顺序即原文求值顺序。
6. **`vx = LR * (distance * x_f - abs(vz / 4))`**：`abs` 是 JS 语义（NaN 传播），
   `/ 4` 不是取整；`vz = distancez * UD * z_f` 中间不取整；
   `vy = height * h_f` 中间也不取整。
7. **`min = 4` 是数字常量**：`vy = e.jumping.t ? min + (vy - min) * e.jumping.y / e.jumping.t : min`
   照抄，注意 `jumping.t` 是在**计步之后**读的。
8. **`set_velocity(vx, vy, vz)`** 三个参数全写，没有条件分支。
9. **`on_landing`**：`e.frame.on_landing` 有值 → `enter_frame(帧)` 并返回；
   否则 `enter_frame_by_id(to_string(e.data.indexes?.landing_1))`，随后
   `update_velocity({ dvz: 4, ctrl_z: SpeedCtrl.Control })`。

### 39.3 有意不覆盖 / 不可观测项
- 机器人分支里 `t`、`y` 加的是同一个 `atom_time`，**交换两次调用不可观测**
  （变异规格头注已写明）；少加一次 `y` 是可观测的（已列入变异）。
- `e.jumping.*` / `e.position.y` 在 TS 是静默属性读写；端口走静默取值/写入缝，
  只由状态文本观察终值，因此「读 `x` 还是读 `y`」这类差异靠后续行为与文本区分。
- ⚠️ **`atom_time > 0` 时 `jumping.t` 在插值处恒为真**（计步刚加过），
  `else` 分支只有 `atom_time = 0` 才可达。用例专门保留了这一档
  （`atom_time n 0` + `jy n 2`），否则「条件读成 `jumping.y`」的变异会存活——
  本片实测第一次跑变异就是这一条 SURVIVED。
- ⚠️ 与 §37.3 同源：`landing_1` 若是 `undefined`，C++ 的 `to_string` 渲染成
  `"undefined"` 而 TS 打 `u`，故用例里的 `landing_1` 只喂字符串，这一档有意不覆盖。

### 39.4 harness 观察点
- 一对 harness `character_state_jump.{cpp,ts}`；TS 侧用真 `CharacterState_Jump`，
  fake Entity 提供 `jumping`（逐字段 getter/setter）、`ctrl`（`__is_bot_ctrl__`
  与 `is_end` 打日志）、`world`（Proxy，打 `world_dataset:<键>`）、
  `get_prev_frame()`（打日志）。
- 输入与覆盖面见 `PROTOCOL.md` §6.9.91。
- 状态文本 `pos=[x:y:z] gy= jx= jy= jz= jt=`；`run make|default` 额外打 `s=`。

## 40. 切片 5：`state/CharacterState_Falling`

`native/lfw/state/character_state_falling.{h,cpp}`（对应
`src/LFW/state/CharacterState_Falling.ts`）。

### 40.1 单元边界（`IStateEntity` 补 13 个缝）
`ctrl_reset_key_list()`、`data_id()`、`shaking()`、
`handle_ground_velocity_decay(double factor)`（**重载**：无参版本代表默认 `1`）、
`fuse_bys()`、`ref_set_velocity(who, x, y, z)`、`dismiss_fusion(frame_id)`、
`defend_value_max()` / `set_defend_value(v)`、`resting_max()` / `set_resting(v)`、
`set_throwinjury(v)`、`data_indexes_critical_hit()`。
其余用既有缝：`bounced` / `set_bounced`、`fall_value` / `set_fall_value` /
`fall_value_max`、`has_catcher` / `catcher_drop_catching`、`drop_holding`、
`leave_ground`、`frame_id`、`frame_info`、`frame_on_landing`、`facing`、`hp`、
`wait`、`velocity_x/y/z`、`set_velocity`、`enter_frame`、`enter_frame_by_id`、
`world_dataset`、`data_indexes_bouncing` / `_falling` / `_lying`、`find_direction`。

⚠️ 本片是**第一个带实例状态**的单元：`_bouncing_frames_map` 必须跨实体保留，
所以 `enter` 用捕获 `this` 的 lambda 装配（`on_landing` 不需要实例状态，
仍是 `&csf_on_landing` 自由函数）。另外 `on_landing` 在端口里是 `State_Base`
的 `std::function` 成员而**不是**虚函数，写 `override` 会编译失败（C3668）。

### 40.2 保真要点
1. `_bouncing_frames_map` 是 `Map<dataId, Set<frameId>>`：`enter` 只在
   `!map.has(e.data.id) && e.data.indexes?.bouncing` 成立时建表，并且
   **短路顺序要保真**——`indexes.bouncing` 只有在「还没缓存」时才被读。
   端口把这次读放进同一个条件里（先算 `need_cache`，成立才读缝）。
2. `enter` 顺序：`bounced = false` → `ctrl.reset_key_list()` → 建表 →
   `catcher?.drop_catching()` → `drop_holding()` → 融合 → `leave_ground()`。
3. 融合（`hp <= 0 && fuse_bys?.length`）：解构 `e.velocity` 后
   `next_vx` 每轮 `* -1`（第 1 个 −vx、第 2 个回 vx），逐个
   `fighter.set_velocity(next_vx, vy, vz)`，最后 `dismiss_fusion(e.frame.id)`。
   `vx = 0` 时 `0 * -1` 得 `-0`，端口与原文一致（trace 里 `-0` 与 `0` 可区分）。
4. `update`：`e.shaking > 0` 直接返回；否则按 `is_bouncing_frame(e)` 分流
   （`update_bouncing` = `handle_ground_velocity_decay(0.7)`，注意**带参**；
   `update_falling` 见下）。**没有** `super.update` 调用。
5. `update_falling`：`wait <= 0` 才进；帧序号从 `1` 起，`y > 3` → 0、
   `y < -3` → 2（严格比较，`±3` 落在中间档）；方向
   `x / facing > 0 ? 1 : -1`（`0` 与 `NaN` 都归 −1）；
   最后 `enter_frame({ id: indexes?.falling?.[direction][idx] })`——
   传的是**局部对象**（只有 `id`），不是帧表里的帧对象。
6. `leave`：先 `super.leave`（`State_Base` 的 HealSelf 补 buff），再
   `bounced = false`、`fall_value = fall_value_max`、
   `defend_value = defend_value_max`、`resting = resting_max`、
   `fallinjury = 0`、`throwinjury = 0`。
7. `on_landing`：`frame.on_landing` 为真就直接 `enter_frame` 并**返回**；
   否则
   `d = find_direction(frame, indexes?.bouncing) || find_direction(frame, indexes?.falling) || find_direction(frame, indexes?.critical_hit) || facing`
   ——`||` 短路，只有前一个返回 `0` 才继续，最后才落到 `facing`。
   然后 `!bounced && (vy <= cha_bc_tst_spd_y || abs(vx) > cha_bc_tst_spd_x)`
   为真 → `enter_frame_by_id(indexes.bouncing[d][1])` +
   `set_velocity(null, cha_bc_spd)`（只有 y 有新值）+ `bounced = true`；
   否则 `enter_frame_by_id(indexes.lying[d])`。`&&` 与 `||` 都短路，所以
   `world_dataset` 的读取条数本身也是可观测的。
8. **两种索引形状**：`bouncing` / `falling` 是 `{"-1": [...], "1": [...]}`，
   `lying` / `critical_hit` 是 `{"-1": id}`。端口用两个小助手复刻 JS 取值：
   `js_at(holder, key)`（对象按键，其它给 `undefined`）与
   `js_at_index(holder, i)`（数组按下标且越界给 `undefined`，非数组回落 `js_at`）。
   用例里两种形状都喂到了。

### 40.3 有意不覆盖 / 不可观测项
- `new Set([...bouncing[1], ...bouncing[-1]])` 的**展开顺序**不可观测（集合）；
  可观测的是成员，已用「只加一个方向」「每个方向只加第一个」等变异覆盖。
- `e.data.id` / `e.frame.id` / `e.data.indexes` / `e.velocity` / `e.bounced` /
  `e.hp` / `e.wait` / `e.shaking` / `e.facing` 在原文都是**属性读**，两侧 harness
  都保持静默：它们的取值只通过行为与状态文本观察（变异打在缝实现上）。
- `leave` 里四次赋值的**先后顺序**不可观测（四个静默 setter、目标互不相同）。
- `super.leave` 的 HealSelf 分支（`State_Base` 里补 `Buff_Healing`）不在本片覆盖：
  本片用例都用 state = 12，该分支由 `state_base/main` 负责。
- ⚠️ 与 §37.3 同源：索引里的帧 id 只喂**字符串**，
  `enter_frame_by_id(undefined)` 的 `"undefined"` vs `u` 渲染差异有意不覆盖。
- ⚠️ 原理上不可杀（不是漏测）：`x / facing`、`x * facing`、`facing / x` 三者的
  符号永远相同，所以「除法换成乘法」这类变异无法用黑盒杀；可杀的同类变异是
  「丢掉 `facing`」「`>` 写成 `>=`」「`? 1 : -1` 互换」，都已覆盖。

### 40.4 harness 观察点
- 一对 harness `character_state_falling.{cpp,ts}`；TS 侧的 `fuse_bys` 返回
  `FakeFighter` 实例，其 `set_velocity` 打 `ref_set_velocity` 日志，
  对象渲染成 `{"key":"G1"}`，与 C++ 侧日志逐字相同。
- 输入与覆盖面见 `PROTOCOL.md` §6.9.92。
- 状态文本
  `pos=[x,y,z] vel=[x,y,z] dataid= frameid= hp= facing= bounced= fall=/<max> defend=/<max> rest=/<max> finj= tinj=`；
  `run make|default` 额外打 `s=`。

## 41. 切片 5：`state/CharacterState_Lying`

`native/lfw/state/character_state_lying.{h,cpp}`（对应
`src/LFW/state/CharacterState_Lying.ts`）。这是**最后一个未移植的 character state**。

### 41.1 单元边界（`IStateEntity` 补 22 个缝）
计数器 `lying_a_count` / `lying_d_count` / `lying_c_count`（读+写 6 缝）、
`toughness_max()` / `set_toughness_resting(v)`、`set_hp_max(v)`、
`dead_join()` / `set_dead_join(v)` / `dead_gone()`、`reserve()` / `set_reserve(v)`、
`wakeup_invuln()` / `set_wakeup_invuln(v)`、`set_invulnerable(v)`、`set_blinking(v)`、
`blink_and_respawn(v)` / `blink_and_gone(v)`、`world_puppets()`、
`world_etc(x, y, z, kind)`。
其余用既有缝：`has_holding` / `holding_base_type` / `holding_set_team` /
`drop_holding`、`toughness`（写，来自 `IBuffEntity` `set_toughness`）、
`hp` / `hp_r` / `hp_max` / `set_hp` / `set_hp_r`、`team` / `set_team`、`wait` /
`set_wait`、`motionless`（写）、`set_outline_color`、`state()`、`position`、
`ground_y`、`frame_info`、`ctrl_is_end`、`ctrl_reset_key_list`、`world_dataset`。

⚠️ 与 Falling 同构：`on_dead` / `find_frame_by_id` 在端口里是 `State_Base` 的
`std::function` 钩子（**不是**虚函数），`enter` 用捕获 `this` 的 lambda 调
`on_dead`；写 `override` 会编译失败。

### 41.2 保真要点
1. `enter`：三个计数器归零 → `ctrl.reset_key_list()` →
   `const holding = e.holding; if (holding) e.drop_holding();
   if (holding?.base_type === WeaponEnum.Heavy) holding.team = e.team;`
   （端口用 `has_holding()` + `strict_equals(holding_base_type(), Heavy)`）→
   `toughness = toughness_max`、`toughness_resting = 0` →
   `hp <= 0` 时调 `on_dead(e)`。
2. `on_dead`：先把 `e.world.puppets` 里每个傀儡的 `team` 收进 `Set`，
   再 `if (e.reserve) --e.reserve`（`--` 是数值语义：字符串 `"3"` → `2`），
   然后三段排他分支：`reserve && player_teams.has(team)` →
   `blink_and_respawn(gone_blink_time)`；否则 `dead_join` → **空分支**；
   否则 `dead_gone` → `blink_and_gone(gone_blink_time)`。
   端口用 `std::vector<Value>` + `strict_equals` 复刻 `Set<string>.has`。
3. `update`：先 `super.update`（= `handle_ground_velocity_decay()` 无参，factor 1）；
   `count_c` / `count_a` 先读，`is_end(GK.a)` 取反得到「按住」；随后
   `lying_a_count = count_a + 1` —— 这里是 **JS `+`**（字符串会拼接），端口用 `js_add`。
   攻击分支 `count_a && count_a % 2 && pressing_a && wait > 0`（`%` 用 `std::fmod`：
   负奇数 `-1` 与小数 `1.5` 都是真值；`truthy(count_a)` 那半边在计数为 `0` 时等价于
   `0 % 2 = 0`），成立则 `lying_c_count = count_c + 1`、
   `wait = round_float(wait - atom_time)` 并**提前返回**。
   否则继续读 `count_d`、`pressing_d`，`lying_d_count = count_d + 1`，
   `count_d && count_d % 2 && pressing_d` 成立则
   `lying_c_count = count_c + 1`（用的是**开头读到的** `count_c`，不是当前值）、
   `wait = round_float(wait + atom_time)`。
4. `leave`：**不调** `super.leave`（所以 `State_Base` 的 HealSelf 补 buff 分支
   在 Lying 上永远不跑）。`dead_join && hp <= 0` 时依次：
   `motionless = 30`、`invulnerable = 30`、
   `hp = hp_r = hp_max = dead_join.hp ?? hp_max`（赋值链从右往左，`??` 只看
   `null`/`undefined`——`0` 与 `NaN` 都会胜出）、
   `team = dead_join.team ?? Team_1`、`reserve = dead_join.reserve ?? 0`、
   `lfw.world.etc(position.x, position.y, position.z, "6")`、`outline_color = ""`、
   `dead_join = null`、`wakeup_invuln = 1`；最后 `if (wakeup_invuln)` →
   `blinking = lying_blink_time`、`invulnerable = lying_blink_time`。
5. `find_frame_by_id` 钩子：
   `hp <= 0 && position.y <= ground_y && state === StateEnum.Lying && !dead_join`
   成立时返回 `e.frame`，否则 `undefined`。

### 41.3 有意不覆盖 / 不可观测项
- `set_invulnerable(30)` 必定被紧随其后的 wakeup 块覆盖（同一个块把
  `wakeup_invuln` 置 `1`，所以那个 `if` 必跑），因此这次写入的**取值**不可观测。
- `set_hp_max` / `set_hp_r` / `set_hp` 三次写同一个值且都静默 → 写序不可观测
  （变异打在各自的**值**上）。
- `player_teams` 是 `Set`：插入顺序与去重不可观测；`Set.has` 的 SameValueZero
  与 `strict_equals` 只在 `NaN` 上不同（用例不喂 NaN team）。
- `truthy(count_a) && count_a % 2` 的前半边在计数为 `0` 时与后半边等价
  （`0 % 2 === 0`）→ 原理上不可杀。
- `on_dead` 里空的 `else if (dead_join)` 分支没有可变异实体，只变异了**分支顺序**。
- 两个 harness 的 fake 都是**原样存储**：不做真 Entity 的
  `round_float` / `max(0, v)` 归一化（`blinking` / `invulnerable` / `reserve` /
  `toughness`），那部分归 Entity 切片。
- ⚠️ **用例教训**（本片实测踩过）：`pressing_d` 这类「按键松开」场景必须让
  **另一个**键的计数为偶数，否则攻击分支会先 `return`、把后续判断整段短路，
  变异就永远看不到差异（第一版因此漏杀 3 条）。
- ⚠️ `round_float` 是否取整只有在 `atom_time` 非整数时才可观测，
  用例专门保留 `atom_time n 1.2345` 一档。

### 41.4 harness 观察点
- 一对 harness `character_state_lying.{cpp,ts}`；`env state` 是构造参数，
  `env estate` 是**实体自身**的 state（`find_frame_by_id` 要比它）；
  `env held s "..."` 给出按住的键（`is_end(key)` 为真表示**未**按住）。
- 输入与覆盖面见 `PROTOCOL.md` §6.9.93。
- 状态文本很长，一次锁住所有静默写入：
  `pos= hp=/<hp_r>/<hp_max> team= tough=/<max> trest= la= ld= lc= wait= holding= holdteam=
   deadjoin= deadgone= reserve= wakeup= motionless= invul= blink= outline= frameid= gy=`；
  `run make|default` 额外打 `s=`；`run findframe` 额外打 `r=`。

## 42. 切片 8：`state/States` 注册表 + `ENTITY_STATES` 装配

`native/lfw/state/states.{h,cpp}`（对应 `src/LFW/state/States.ts`）、
`native/lfw/state/entity_states.{h,cpp}`（对应 `src/LFW/state/ENTITY_STATES.ts`）与
`native/lfw/state/state_names.h`（C++ 侧的 `constructor.name` 替身）。

### 42.1 单元边界（为什么需要 `state_names.h`）
TS 把每个状态按 `value.state` 塞进 `Map`，靠 `constructor.name` 区分实现类。
端口这边 **RTTI 是关掉的**（编译参数 `/GR-`），`typeid` 不可用，所以：

- `state_name_of<T>` 为每个可注册类给出**显式名字**（主模板故意不定义，
  漏一个类就编译不过）；
- `state_name_u16<T>()` 把纯 ASCII 名字逐字节加宽成 `std::u16string`；
- `States::add<T>(args...)` / `States::make<T>(key, args...)` 在构造时用**模板实参**
  取名字，所以「名字」与「真正 new 出来的类」不可能对不上（不存在手写字符串写错类的事）。

`States` 的条目是 `{Value key, unique_ptr<State_Base> state, u16string class_name}`，
另有一张 `map<u16string, size_t>` 做索引。

### 42.2 保真要点
1. **JS `Map` 的键语义**：数字键与字符串键**互不相同**（`0` ≠ `"0"`），
   端口把键编码成 `"n:<值>"` / `"s:<值>"` 再查表；
   `-0` 与 `0` 在 JS 里是同一个键（SameValueZero），`to_string(-0)` 也是 `"0"`，
   所以这一点天然一致。
2. **插入顺序即遍历顺序**：`set` 命中已有键时**只换值、不挪位置**
   （JS `Map` 语义；TS 原文在这里打 `debugger`，端口留注释说明这是接线 bug 的信号，
   行为上仍然覆盖并保留原位置）。`entries()` 按插入顺序返回，
   harness 的 `head`/`tail`/`dump` 直接观察顺序。
3. `add<T>(args...)` 用 **`value.state` 当键**（对应 TS `add(...values)`）；
   `make<T>(key, args...)` 是显式键版本，供 `set_in_range` / `set_all_of` / `fallback` 使用
   ——这条区分是必须的：`fallback` 的缓存键是**字符串** `` `${type}_${code}` ``，
   不是状态自身的数值。
4. `set_in_range(from, to)` **闭区间**（`for (key = from; key <= to; ++key)`），
   `set_all_of(keys)` 逐个建；两者的 `state` 都取当前键。
5. `fallback(type, code)`：先按字符串键查缓存，命中即返回**同一实例**；
   否则按 `switch (type)`（`===` 严格比较）分派——
   Fighter(8) → `CharacterState_Base`、Weapon(16) → `WeaponState_Base`、
   Ball(32) → `BallState_Base`、其它 → `State_Base`，状态值取 `code`，
   并以那个字符串键缓存（后续同键调用直接命中）。
6. `ENTITY_STATES` 的装配顺序与 TS 逐条对应：
   `TransformTo_Min..Max`（8001..8999，**999** 条）→ 7 个球的 `StateBase_Proxy` →
   34 条具名条目（武器 7 + 重型武器 4 + 三个基类 + 21 个角色/杂项状态）→ 共 **1040** 条。

### 42.3 有意不覆盖 / 不可观测项
- TS 的 `debugger;`：只有挂了调试器才有副作用，端口按注释处理，不改语义。
- `States.get` 的键类型只支持 `Value`；JS 允许对象当键（按引用相等），
  游戏代码里没有这种用法，端口不做（harness 只探 `n/s/b/z/u` 五种键形状）。
- `States.has` 目前**还没被游戏代码调用**，所以 harness 专门加了 `run has`
  一处观察点，否则它的变异不可杀。
- `class_name` 只是**观察点**（给差分测试比对「哪个类接了这个状态号」），
  游戏逻辑不读它。

### 42.4 harness 观察点
- 一对 harness `entity_states.{cpp,ts}`；TS 侧直接用真 `ENTITY_STATES`
  （`map` 是公开字段）与真 `States`。
- 输入与覆盖面见 `PROTOCOL.md` §6.9.94。
- 输出：`run size` → `n=<条数>`；`run dump|head|tail` → 逐条
  `k=<键> cls=<类名> s=<状态值>`；`run get <键> | has <键>` → `r=`/`has=`；
  `run fallback|fallback2` → `r=`/`s=` 与 `same=b0|b1`（同一实例判定）。

## 43. 切片 8b：`WorldDataset`（全局调参表 + 变更通知）

`native/lfw/world_dataset.{h,cpp}`（对应 `src/LFW/WorldDataset.ts`）。

### 43.1 单元边界（为什么是「键值表」而不是 103 个 C++ 字段）
TS 侧 `WorldDataset` 有 103 个字段，但**所有已经移植过的调用点都按键取值**
（如 `e.world_dataset(u"atom_time")`，见 `world_data.h` 的缝），并没有按 C++ 成员访问的用法。
所以端口把实例建模成「有序键表」：

- `_keys`：own 属性名顺序（构造顺序 = 默认字段声明顺序 + `__is_world_dataset__`）；
- `_values`：`map<u16string, Value>`；
- `_tracked`：被托管（装了 getter/setter）的键集合；
- `_hooks`：`set_field_hook(key, fn)` 注册的 `on_<key>_change` 槽。

`world_dataset_fields()`（`native/lfw/defines/fields_gen.h`）就是 TS 的
`world_dataset_fields`（103 键），只用来判定托管关系与 `dump_dataset` 的键序。

### 43.2 保真要点
1. **默认值来源与顺序**：103 条按 TS 类字段**声明顺序**逐条抄写（顺序本身可观测：
   `pure` 数据集与 `run keys` 直接按这个顺序打印）。其中
   `screen_w = Defines.MODERN_SCREEN_WIDTH = 794`、`screen_h = MODERN_SCREEN_HEIGHT = 450`、
   `sync_render = SyncRenderEnum.FPS_60 = 3`、`difficulty = Difficulty.Difficult = 3`、
   `[CheatEnum.*]` 三个键分别是 `GIM_INK` / `HERO_FT` / `LF2_NET`（数值 0）。
2. **`make_private_properties` 的语义**（TS `utils/make_private_properties.ts`）：
   对每个「存在且非 `on_` / `_` 前缀、不是函数、且在 `world_dataset_fields` 里」的 own 字段，
   用 `Object.defineProperty` 装 getter/setter（**默认非枚举**，所以 `Object.keys` 看不到它们）；
   setter 里 `if (v === prev) return`（**严格相等**，故 `0` 与 `-0` 也等价），
   否则先存值、再调 `on_<key>_change?.(v, prev)`、最后调
   `on_dataset_change?.(key, v, prev)`——**顺序是先键钩子后整体回调**，两条都拿到 `(curr, prev)`。
   端口用 `tracked(key) = !_pure && _tracked.find(key)` 复刻判定，
   `set` 里对应 `strict_equals` 早退 → 存值 → 键钩子 → 整体回调。
3. **`pure` 分支**：`new WorldDataset(true)` 整个跳过表驱动安装 → 字段是普通**可枚举**属性、
   赋值不发通知，而且**没有 `__is_world_dataset__`**（`Object.assign(this, wdataset)` 也在 `!pure` 里）。
4. **`__is_world_dataset__` 的顺序与身份**：TS 先装 getter/setter、后 `Object.assign`，
   所以这个标记是**普通 own 属性**、位于 own 属性顺序**最后**、
   并且**不在** `world_dataset_fields` 里（因此永远不被托管）；
   端口在托管集合算完之后才 `emplace_back`，注释里写明了理由。
5. **键的存在性两段语义**：`set` 分两条路——
   键**根本不存在**（`_values` 里没有）→ 普通属性创建：追加到键序末尾、**不触发任何回调**；
   键**存在但未被托管**（`__is_world_dataset__`、`pure` 数据集里的所有字段）→ 存值但不通知。
   这两条都由 `run set` 的日志空白与随后 `run keys`/`run get` 交叉锁住。
6. **`dump_dataset()`**：遍历 `world_dataset_fields()` 的键并**按 UTF-16 码元排序**
   （JS 默认 `Array.prototype.sort` 就是字典序，与 `std::u16string` 的 `operator<` 等价，
   所以 `GIM_INK`/`UPS` 这类大写键排在前面），逐键取当前值（表里有、实例没有则为 `undefined`）。
   `renderValue` 按插入顺序渲染对象键，因此这一条同时锁死「键集合」与「排序」。
   注意 `dump` **不包含** `zz` 这类运行期新键，也不包含 `__is_world_dataset__`。
7. **默认实例**：`DEFAULT` 是懒加载单例（`new WorldDataset()`，非纯），
   跨 `run default` 可写且可观察：先 `set` 再 `default` 仍能读回新值。

### 43.3 有意不覆盖 / 不可观测项
- `make_private_properties` 的 `_$_<key>` 备份属性、`on_<key>_change` 属性槽、
  以及 `on_dataset_change` 本身都是**实例上的 own 属性**（会出现在 `Object.keys` 里）。
  它们是该工具的实现细节（将来单独移植 `make_private_properties` 时再逐条锁），
  端口把「字段集 + 两个回调槽」建模成数据，因此 harness 在 TS 侧把 `Object.keys`
  归一化（丢掉 `on_*` 与 `_$_*` 前缀）。**这是 harness 约定，不是行为差异**，
  已在变异表注释里记录。
- 103 个默认字段**全部**存在于 `world_dataset_fields()`，
  所以「构造时省略表归属判断」这一变异**与原实现等价**、原理上不可杀，
  变异表里没有列它（托管判定仍由 `tracked`/`run tracked` 观察）。
- `keys()`（完整 own 属性顺序）目前只服务 harness；游戏代码不枚举数据集实例，
  正式导出路径是 `dump_dataset()`。
- `pure()` 这类 C++ 侧便利访问器**没有**保留：TS 侧没有对应可观察属性，
  留着只会变成不可观测的 API。

### 43.4 harness 观察点
- 一对 harness `world_dataset.{cpp,ts}`；TS 侧用真 `WorldDataset` 类与真 `WorldDataset.DEFAULT`。
- 输入与覆盖面见 `PROTOCOL.md` §6.9.95。
- 输出：`run make|default` → `pure=`/`same=`/`keys=`；
  `run keys|dump` → 键列表 / 整张表；`run get|has|tracked` → `v=`/`has=`/`tracked=`；
  `run set` → `field_change:<键>:<新值>:<旧值>` 与 `dataset_change:<键>:<新值>:<旧值>`（无通知则为空）。

## 44. 切片 9a：`Entity` 的构造 / `reset` + 数值通知层

`native/lfw/entity/entity.{h,cpp}`（对应 `src/LFW/entity/Entity.ts` 的 2716 行中的
「构造 + `reset()` + 统计量访问器」这一段）。这是**强连通块的第一刀**：
`Entity` / `World` / `LFW` 互相依赖，只能按可验证的切面逐块搬。

### 44.1 单元边界（为什么先搬这一段）
1. `Entity.ts` 里最底层、被后续所有行为（物理、帧、碰撞、快照）依赖的是
   **统计量槽位 + 通知层**：`hp/mp/hp_r/hp_max/mp_max/toughness*/fall_value*/defend_value*/`
   `resting*/catch_time_max/reserve/blinking/invisible/invulnerable/arest/outline_*/mix_*/greyscale`，
   以及 `reset()` 对它们的初始化。先把这一层逐字节锁死，后面的物理切片才有可信地基。
2. **宿主边界**：TS 里这段代码读 `this.world` / `this.lfw`（`world.dataset`、
   `world.bg.data.dataset`、`world.mark_players_alive`、`lfw.new_id`、`lfw.new_team`、
   `lfw.factory.acquire_ctrl/release_ctrl`）以及 `this.enter_frame` / `this.apply_opoints` /
   `this.play_sound`。`World` / `LFW` 还没搬，所以端口把这些**注入**成 `IEntityHost`
   （每个虚函数注释里都写着它镜像的那句 TS）。harness 的双侧桩世界与此一一对应。
3. **数据形态**：`_data` / `frame` / `armor` / `dead_join` / `transforms` / `group` 等都是
   `Value`（和其余端口一致）；统计量本身是 C++ 数值成员（TS 里也只是数值）。
4. `_state` 在完整端口里是 `state::State_Base*`；本切片只用到两个可选钩子
   （`_state?.on_dead?.(this)` / `_state?.get_gravity?.(this)`），所以先暴露成
   `std::function<void()> state_on_dead` 与 `std::function<Value()> state_get_gravity`，
   等状态接线切片落地再换成真指针（`reset()` 里 `this._state = null` 对应「两个钩子都清空」）。

### 44.2 保真要点
1. **`??` 与 `||` 分得很清**：`resting_max` 家族是 `_x ?? world.dataset.x`（只有
   `undefined`/`null` 才回退，`0` 不回退），`armor = armor || null`、`name` 的
   `ctrl.player.name || \`Player ${id}\``、`variant = Number(team) || 0` 都是**真值**语义
   （`0`/`""`/`NaN` 会回退）。端口分别用 `opt_num`（`nullish` 判定）与 `truthy` 复刻。
2. **严格相等才静默**：`reserve/resting/fall_value/toughness*/arest/catch_time` 的 setter 都是
   `if (o === v) return;`（`aste` 用的是宽松 `==`，数值上等价），所以 `0` 与 `-0`、
   `20` 与 `20` 都不发通知；`fall_value`/`defend_value` 更细：**比较用的是未取整的入参**，
   存的是 `round_float(v)`，因此 `fall_value = 20.00049`（当前 20）会「值没变但通知照发」。
3. **通知顺序与载荷**：键钩子式的 `on_<字段>_changed` 在本层就是
   `callbacks.call(name, this, v, o)`；`hp/mp/hp_max/mp_max/hp_r` 是把**赋值写进实参**
   （`callbacks.call("on_mp_max_changed", this, (this._mp_max = v), o)`），所以载荷永远是
   存下来的值。回调里的 `this` 在端口是 `ref()`（`Value` 视图，至少含 `id`），harness 双侧都
   用「参数里的 id == 当前实体 id ? self : ?」归一。
4. **钳制与取整恒定**：`max(0, v)` + `round_float(v)` 是所有计数类 setter 的固定组合；
   `blinking/invisible/invulnerable` 只有这两个运算、**没有**通知。
5. **`fall_value` / `defend_value` 下降时的连锁恢复**：`v < o` 时先
   `resting = resting_max`、`toughness_resting = toughness_resting_max`，**再**发自己的通知；
   这两条各自的通知（`on_resting_changed` 有、`on_toughness_resting_changed` **不存在**）
   顺序被差分锁住。
6. **`hp` 的死亡分支**：`on_hp_changed` →（人类控制器且跨过 0）`mark_players_alive(v > 0)` →
   `o > 0 && v <= 0` 时依次 `on_dead` → 状态钩子 → 三个守卫
   （`state !== Gone`、`frame.id !== "gone"`、`data.base.brokens?.length`）通过才
   `apply_opoints(brokens)` + `play_sound(dead_sounds)` → `frame.on_dead ?? data.on_dead`
   有值就 `enter_frame` → 最后 `v > _hp_r` 时抬 `hp_r`。守卫是**短路**的，
   所以「0 血但 frame 是 gone」这类场景不会去打碎件。
7. **`mp` 的耗竭分支**：结构相同（`on_mp_changed` → 摘要 → `frame.on_exhaustion ?? data.on_exhaustion`），
   区别是 mp 只做摘要不做死亡。摘要用 `summary_mgr`（真端口），
   `v < o` 时按 `id` 记 `hp_lost`/`mp_usage`，`!is_independent(team)` 时**再**记一份到队伍键。
8. **控制器是「值 + 真对象」双层**：类型判定 `is_human_ctrl(v)` 读的是 `v.__is_human_ctrl__`
   （TS 侧真控制器就是有这些标记的对象），而 `gravity` 要调 `ctrl.is_end(GK.Defend)`，
   所以端口持有真 `BaseController*`，并用 `is_human()`/`is_bot()` 做等价判定；
   只有回调载荷需要 `Value` 视图时才由 `ctrl_ref()` 现搭（带 `__is_base_ctrl__` 等标记、
   `player`、`player_id`）。
9. **`name` 是「值」不是「字符串」**：TS 的 `get name(): string` 在 `_name === undefined`
   时真的会返回 `undefined`（类型谎），`_name = null` 才回落到 `data.base.name ?? ''`。
   端口把 `_name` 存成 `Value`、`name()` 也返回 `Value`，才能把 `undefined` / `null` /
   空串三种情况区分开（差分里有 `set name u` 的用例）。
10. **`team` 的 `variant` 解析**用 JS `Number()`（`"12"`→12、`"abc"`→NaN→`0`、`"-0"`→`-0`→`0`），
    端口复用 `to_number` + `truthy`；同时 `++_render_effect_time` 也在这一步。
11. **`reset()` 的顺序**：清 marks/buffs → 复位地形/数据/新 id → 基础槽位 →
    `set_catching(null)`/`catcher = null` → `callbacks.clear()` → 团队 →
    恢复 tick 与 `hp_max/mp_max` 快照 → 控制器释放+获取 → `reset_armor()` →
    `fall_value = fall_value_max`、`defend_value = defend_value_max`、
    `_hp = _hp_r = hp_max`、`_mp = mp_max`、`set_catch_time(catch_time_max)` →
    隐身/闪烁/死亡槽位 → `outline_*` / `mix_*` / `greyscale` → `auto_key_role()`。
    端口逐行照抄，只有三处**有意不同**（见 44.3）。
12. **`reset_armor()`** 的赋值链 `this.toughness = this.toughness_max = armor?.toughness ?? 0`
    在 JS 里是**先 `toughness_max` 后 `toughness`**（左值引用先求、右侧赋值先做），
    所以两条通知的顺序是 max → value，端口按这个顺序写（harness 的 `run armor` 用带监听器的
    重跑把顺序锁住）。

### 44.3 有意不覆盖 / 不可观测项
- **TS 需要 `data.base` 存在**：`reset()` 直接读 `data.base.resting_max`，
  数据里没有 `base` 会抛；端口用 `field_or` 一路读穿，这不构成差异（用例都会给 `base`）。
- `copies` / `vrests` / `blockers` / `superpunchs` / `collision_list` / `collided_list` /
  `lastest_collided` / `opoints` 在 `reset()` 里被清空，但本切片**没有任何代码往里面写**，
  所以「清空」这个动作不可观测（等碰撞/持有物切片落地后再纳入）。
- `terrain` / `_atom_time` / `prev_position` / `position` / `velocity` / `prev_velocity` /
  `_motionless_ticks` 被 `reset()` 写入，但**没人读**（物理切片才读），故不可观测。
- `add_catch_time(0)` 与 `set_catch_time(_catch_time)` 是同一件事，
  所以「`!value` 提前返回」在 DSL 能表达的值域里等价（已在变异表注释里记录）。
- 「没有状态」与「有状态但没有那个钩子」在 TS 里都表现为不调用，
  端口把两者合并成「空 `std::function`」（不可观测）。
- `_state?.leave/enter`、`set_state`、`enter_frame` / `apply_opoints` / `play_sound` 的真实实现
  属于后续切片；harness 在 TS 侧把这三个方法换成**实例间谍**（C++ 侧对应宿主虚函数），
  两边打印同一份日志。
- `bare` 值形状的边界（`group` 是字符串而不是数组等）会让 TS 侧 `group?.some` 抛异常，
  这类输入不做用例。

### 44.4 harness 观察点
- 一对 harness `entity.{cpp,ts}`；TS 侧构造**真** `Entity`，宿主侧给桩
  `world`（真 `WorldDataset` 当 `dataset`、`bg.data.dataset` 是普通层、`mark_players_alive` 打日志）
  与桩 `lfw`（`new_id`/`new_team`/`factory`），并打桩 `Ditto.vec3`。
- 控制器：`base`/`human`/`bot` 三种（对应 `__is_*_ctrl__` 标记 / `set_kind`），
  另有 `human_bare`（玩家没有名字，走 `Player <id>`）、`base_released`（`d` 键
  `_d_time > _u_time`，让 `is_end('d')` 为假以覆盖 `gravity` 的另一支）、`same`（重设同一控制器）。
- 输入与覆盖面见 `PROTOCOL.md` §6.9.96。
- `Entity::stat_slots()` 是**给 harness/宿主看的窥视口**：把 TS 侧的私有槽位
  （`_catch_time`、`_toughness_r_value`、五个恢复 tick 的 `max`）一次导出成数据，
  TS 侧用 `as any` 读同样的字段拼同一个对象，两边逐字节比对。

## 45. 切片 9b：`Entity` 的速度 / 摩擦 / 重力层

`entity.{h,cpp}` 的第二刀：`get dvx/dvy/dvz`、`set_velocity`、`leave_ground`、
`handle_ground_velocity_decay`、`handle_velocity_decay`、`handle_gravity`、
`update_velocity`。这一层是后续 `update()` / 状态机落地判定（`is_on_ground`、
`velocity.y`）的地基，输入只有「数值 + 帧 + 控制器」三样，所以在 `World` / `LFW` 缺席时
也能独立锁死。

### 45.1 单元边界（为什么是这七支）
1. **不碰位置积分**：`update_position` / `set_position` 需要 `world.restrict` 与
   `world.ground.y`（真 `World`），留到 World 切片；`set_velocity` 的 `prev_velocity`
   镜像与 `velocity.y > 0 → leave_ground` 已经能独立观察。
2. **`leave_ground` 与 `set_velocity` 一起搬**：`leave_ground` 只依赖
   `position.y` / `ground_y` / `is_on_ground`，而 `set_velocity` 依赖它；`eqlt`
   （`round_float(a - b) <= 0`）直接复用已搬的 `utils/math/float_equal.h`。
3. **`update_velocity(vinfo)` 的 `vinfo` 收 `Value`**：TS 里它就是帧对象
   （`update()` 里的 `this.update_velocity(this.frame)`），端口按 JS 解构语义用
   `field_or` 逐键读——缺失键与 `undefined` 等价，于是 9a 的 `run frame …` 探针
   可以直接驱动它。
4. **`calc_v` 复用**：速度模式计算交给已搬的 `entity/calc_v.*`，本层只负责
   `SpeedMode` / `SpeedCtrl` 的分派与 `acc_*` 的补默认。

### 45.2 保真要点
1. **`set_velocity` 跳过 `null`/`undefined` 但写入 `NaN`**：TS 是
   `_x !== null && _x !== void 0`，所以端口签名收 `Value`（`monostate` = `undefined`、
   `NullTag` = `null`），只有 `nullish` 才跳过；差分里 `run setvel u z u` 与
   `run setvel n 0.1235 u u`（`round_float` 千分位）分别锁这两半。
2. **`dvx/dvy/dvz` 的「原样返回 vs 乘因子」二分**：帧值假（`0`/`null`/`undefined`/`NaN`/`""`）
   就**原样返回**，真才 `v * dataset("fvx_f")`；数据集缺失时 `v * undefined` → `NaN`，
   所以差分里有「帧没有 `dvx` → `undefined`」「有 4 而 `fvx_f` 缺失 → `NaN`」
   「`fvz_f = 0.25` 时 `8 → 2`」「`dvz: null` → `null`」四条。
3. **`handle_velocity_decay` 的钳位是「先改值再回夹」**，且**两支的钳位目标不同**：
   `x > dvx` → `x -= accx`，若 `x < dvx` 则 `x = dvx`；`x < -dvx` → `x += accx`，
   若 `x > -dvx` 则 `x = -dvx`。`x = dvx` 这一支在 `dvx` 为 `null` 时把 **null 写回**
   （随后 `set_velocity` 跳过该轴！），而 `x = -dvx` 支写的是数值 `-0`——差分用
   `"dvx": z` + `vdecay` 与「`x == dvx` 时 `accx` 为 `NaN`」两条分别锁死。
4. **`ctrl_x && !LR` → `dvx = 0`**：只有「帧要求受控 + 控制器 `LR` 为 0」才把目标速度清零；
   `LR` 非 0 时保留帧上的 `dvx`（此时 `calc_v` 用 `LR` 当方向）。`ctrl_z/UD` 同理。
5. **落地区分是对象同一性**：`this._landing_frame === this.frame`，端口用 `strict_equals`
   （`shared_ptr` 指针比较）；`landing` 决定 `land_friction_*` 还是 `friction_*` 三件套，
   且 `factor *= …` 在数据集缺失时变成 `NaN`（TS 的 `factor *= undefined`）。
6. **`fz` 为 `undefined` 时 `accz` 走默认参数**：TS 是
   `handle_velocity_decay(accx, accz = accx, factor = 1)`；端口把「`undefined`」映射成
   `std::nullopt` 才触发默认，`null` 不触发（会被 `to_number` 成 `0`）。
7. **`handle_gravity` 的 `gravity_enabled = true` 只吃 `undefined`**：`null` 是真值语义下的假
   → 直接返回；`0`/`""` 同理；只有缺失才默认开。`position.y <= ground_y`、
   `bearer`/`catcher`/`shaking`/`motionless` 五个守卫全部短路照抄。
8. **`update_velocity` 的流水线**：`dvx/dvy/dvz` 先按 `fv*_f` 缩放（只对真值）→ 补
   `vxm/vym/vzm`（默认 `Default`/`AccTo`/`Default`）与 `ctrl_x/y/z`（默认 0）→
   `acc_*`（`AccTo|FixedAccTo` 且 `acc == void 0` 且 `dv` 为真时取 `dv`）→
   按 `nullish(dv)`、`!ctrl`、`LR|UD|jd` 四路分派调 `calc_v`。
   - `acc == void 0` 是**宽松比较**：`null` 也吃默认，`0` 不吃。
   - `dvx == void 0` 同理：`null`/`undefined` 直接 noop，`0`/`""` 会进 `calc_v`。
   - `vxm == SpeedMode.AccTo` 也是宽松比较：`"4"` 这样的字符串同样命中
     （差分用 `vxm s "4"` 锁 `equals` 而非 `strict_equals`）。
   - `LR != 0 && Control` 才把 `LR` 当方向；`Enable`/`Disable` 一律用 `1`。`jd`/`UD` 同理。
9. **`update_velocity` 直接写 `velocity.x/y/z`**（`round_float`），**不碰 `prev_velocity`、
   也不调 `leave_ground`**：差分先 `run set is_on_ground n 1`，正 `vy` 之后
   `is_on_ground` 必须仍是 `b1`；同时「0.1 + 0.2」这类和必须在写回时重新取整。

### 45.3 有意不覆盖 / 不可观测项
- `update_position` / `set_position` / `check_velocity` 一类的调用点属于 World 切片。
- `WorldDataset` 给 `friction_factor = land_friction_factor = 1`、
  `friction_x = friction_z = 0.25`、`land_friction_x = 1`、`land_friction_z = 0.5` 全部**带默认值**，
  所以 `fz` 为 `undefined` 的分支在真实数据下不可达 →「`fz` 的 `nullopt` 处理」这条
  等价，已在变异表头记录，不列条目。
- `vxm`/`vym`/`vzm`/`ctrl_*` 的**默认赋值本身**不可观测：`vxm` 默认 `Default` 与
  `calc_v` 的兜底分支是同一条路径；`ctrl_x = 0` 与 `undefined` 在 `truthy`/`equals` 下同真同假
  → 相关候选撤回（表头记录）。
- `acc_*` 的 `truthy(dvx)` 守卫在 DSL 能表达的值域下与去掉等价（真值→同值，假值→同假）；
  「`nullish` 改成严格 `undefined`」则**可**观测（`acc_x: z` 用例），故保留。
- 「`truthy(dv)` 改成 `!nullish(dv)`」等价（`0`/`""`/`NaN` 乘/不乘都得到同一个值）。

### 45.4 harness 观察点
- 新增 op：`run setvel <x> <y> <z>`（`Value` 字面量，`u`/`z` 就是 `undefined`/`null`）、
  `run leaveground`、`run pos <x> <y> <z>` 与 `run ground <数字>`（窥视写 `position` / `_ground_y`，
  因为 `set_position` 未搬）、`run link bearer|catcher|holding|catching self|none`、
  `run gravity`、`run gdecay <factor>`、`run vdecay <accx> <accz> <factor>`、
  `run velocity <vinfo 字面量>`、`run land <帧字面量>|self`、
  `run keys <LR> <UD> <jd>`（新建 base 控制器并按下对应键）。
- 新 `get` 字段：`velocity`/`prev_velocity`/`position`/`prev_position`（`{x,y,z}`）、
  `dvx`/`dvy`/`dvz`、`atom_time`、`landing_frame`。
- TS 侧 `Ditto.vec3` 桩把 `set` 定义成**不可枚举**属性，`Object.keys`（即渲染）才只看到
  `x/y/z`——真 `IVector3` 的方法在原型上，同理。
- `run keys` 用 `hit(1)` 让 `_d_time = 1 > _u_time = 0`（`is_end()` 为假 → 键算「按下」），
  两侧同一个字面量；`lr/ud/jd` 由控制器 getter 打印，顺带验证键位映射。
- `run land self` 把 `_landing_frame` 指向**当前帧对象**，才能覆盖 `=== this.frame` 的
  同一性分支（另一支用等值但不同一的对象）。

## 46. 切片 9c：`Entity` 的帧查找 / 标志处理

`entity.{h,cpp}` 的第三刀：`find_frame_by_id`、`find_auto_frame`、`find_align_frame`、
`get_prev_frame`、`get_sudden_death_frame`、`get_caught_end_frame`、`handle_facing_flag`、
`handle_wait_flag`、`get_frame_wait`。这一层的输入只有「帧表 + 状态回调 + 控制器/关系」
三样，所以在 `set_frame` / `enter_frame`（需要 `__judger` 与 `lfw.mt` 缝）之前就能独立锁死。

### 46.1 单元边界（为什么是这九支）
1. **不碰 `set_frame` / `enter_frame` / `get_next_frame`**：这三支要 `lfw.mt.pick`、
   `preprocess_next_frame`（端口仍是桩）和 `__judger` 表达式求值，属于下一刀。
2. **不碰 `set_state`**：它要完整的 `IStateEntity` 实现与 `_states` 注册表注入，放在
   `Entity` 实现 `state::IStateEntity` 的那一刀一起做。本刀只把 `_state?.xxx?.(this)` 的
   六个可选回调保留成注入钩子（9a/9b 已建立的缝）。
3. **`get_prev_frame` 顺手搬**：它只是 `_prev_frame` 的读取口，同时把 9a 的窥视口
   从 `prev_frame()` 改名成 `get_prev_frame()`，避免与 TS 只有方法无属性的事实混淆。
4. **`find_align_frame` 依赖 `find_auto_frame`**：后者的兜底链要被前者复用，所以两支同刀。

### 46.2 保真要点
1. **`find_frame_by_id` 的 `switch` 是严格比较**：`case void 0:` 只吃 `undefined`，
   所以 `null` 的 id **不会**返回当前帧，而是落到 `this._data.frames["null"]`（键名是
   `String(null)`）→ 查不到 → `find_auto_frame()`。端口因此只对 `monostate` 短路，
   `NullTag` 继续走查表；差分里 `run findframe z` 与 `run findframe u` 的结果不同即锁此点。
2. **状态钩子的返回值用 `truthy` 判定**：`const r = this._state?.find_frame_by_id?.(this, id); if (r) return r;`
   ——`0`/`""`/`null` 都算「没找到」，继续走 switch。端口用 `truthy(r)`；差分用
   `run hook frameid s "hookf"`（真值命中）与 `run hook frameid n 0`（假值穿透）两面覆盖。
   钩子的**第一个参数是实体自身**（TS `(this, id)`），端口目前只传 id——见 §46.3。
3. **`find_auto_frame` 是 nullish 链**：`_state?.get_auto_frame?.() ?? frames["0"] ?? this.frame`。
   注意 `frames["0"]` 只要非 nullish 就赢，哪怕它是 `0`（假值）——差分用
   `run frames o 1 "0" n 0` + `run autoframe` 锁「返回 0 而不是当前帧」。
4. **`find_align_frame` 的取模对齐**：`dst?.length && src?.length` 时
   `(src.indexOf(id) + 1) % dst.length`，**未命中 `indexOf` 得 -1 → `dst[0]`**；
   只有 `dst` 有长度时也是 `dst[0]`；都没有才是 `find_auto_frame()`。
   端口按 `(idx + 1) % d_len` 复刻（`idx` 初值 `s_len` 表示 -1），返回 `{ id: … }` 新对象。
   另外 `src` / `dst` 只认数组（`as_array`），空数组与 `null` 同路。
5. **`get_sudden_death_frame` / `get_caught_end_frame` 的兜底是 `||`**：
   `_state?.xxx?.(this) || Defines.NEXT_FRAME_AUTO`——钩子返回假值同样落到 `NEXT_FRAME_AUTO`，
   与 `??` 不同；差分用 `run hook sudden n 0` 锁住穿透。
6. **`get_caught_end_frame` 会就地抬升 `position.y` 且不取整**：
   `if (this.position.y < this.ground_y) this.position.y = this.ground_y + 1;`
   端口原样照抄（**不经过 `round_float`**）；差分把 `ground_y` 设成 `0.1235`，抬升后
   `position.y` 必须打出 `1.1235`。
7. **`handle_facing_flag` 的分派**：严格 `switch`，非数字（`undefined`/`null`/字符串）
   一律走 default `this.facing`。各分支细节：
   - `Ctrl` 用 `ctrl?.LR || this.facing`（`LR` 为 0 时回退），`AntiCtrl` 用
     `ctrl?.LR ? turn_face(LR) : this.facing`——注意 `turn_face` 收的是 **LR** 而不是 facing。
   - `SameAsCatcher` / `OpposingCatcher` 走 `catcher?.facing || this.facing`（`||` 真值语义），
     `Opposing` 支对 `turn_face` 的结果再判真值（`turn_face(undefined)` 是 `NaN` → 假 → 回退）。
   - `Trend` 里 TS 写的是 `const { LR } = this.ctrl;` **没有可选链**，端口对应取
     `ctrl_` 的 `LR()`；`LR` 为 0 才看 `velocity.x`。
   - `VX` / `AntiVX` 在 `velocity.x == 0`（含 `-0`）时回退 `this.facing`。
   - **`SameAsBearer` / `OpposingBearer` 的 case 不可达**：`FacingFlag` 里
     `SameAsBearer == SameAsCatcher == 4`、`OpposingBearer == OpposingCatcher == 5`，
     JS `switch` 取**第一个**匹配 `case`，所以 4/5 永远走 catcher 支（bearer 支是死代码）。
     端口照抄顺序（catcher 在前），差分用两组场景锁死：①`link catcher buddy` + buddy 朝向
     `-1/0/2`，4/5 必须跟着 **catcher** 变；②`link catcher none` + `link bearer buddy`，
     4/5 必须**忽略 bearer** 回退到 `this.facing`。把两条 case 的顺序对调就会被杀。
8. **`handle_wait_flag` 的四级判定**：
   `wait == void 0 && frame` → `get_frame_wait(frame)`（宽松比较，`null` 也命中）；
   `is_positive(wait)` → `wait`；`wait === "i" || !frame` → `this.wait`；
   `wait === "d"` → `max(0, frame.wait - this.frame.wait + this.wait)`；否则
   `get_frame_wait(frame)`。
   - `frame` 是可选参数，**存在但假值**（`0`/`null`/`""`）一律算「没有帧」：
     端口用 `has_frame = frame_value.has_value() && truthy(*frame_value)`。
   - `is_positive` 只对 `number` 且 `> 0` 为真（`"3"` 这种字符串不算），差分有
     `run waitflag s "3"` 对照。
   - `"d"` 支的 `frame.wait` 缺失时是 `undefined` → `max(0, NaN)` = `NaN`（端口 `to_number`
     同值），差分用「帧没有 `wait` 键」覆盖。
9. **`get_frame_wait` 的两半**：`frame.wait + world.dataset.wait_offset`（数据集缺失 →
   `NaN`），再在 `_from_wait_block` 时减去 `_atom_time`。端口额外加了
   `from_wait_block()` / `set_from_wait_block()` 窥视口，差分把 `atom_time` 设成 2 后
   用 `run waitblock 1` 锁「减的是 `_atom_time` 而不是常量 1」。

### 46.3 有意不覆盖 / 不可观测项
- `this._state?.find_frame_by_id?.(this, id)` 的**第一个参数**（实体自身）在端口里没有对应
  类型可传，钩子签名收缩成 `std::function<Value(const Value&)>`（只收 id）。这是明确的
  **缝简化**，等 `Entity` 实现 `state::IStateEntity` 时统一改回 `(Entity&, …)`；在
  harness 里用 `(_e, id) => id` 的 echo 钩子把「第二参数是查找用的 id」钉住，
  所以「端口误传实体 id」这条变异是可杀的（见变异表 `find_frame_by_id passes the entity id`）。
- `Ditto.warn("Entity::find_frame_by_id", "frame not find! id:", id)` 只是控制台警告，
  无任何可观测副作用，端口直接不调用（已在实现处注释）。
- `find_frame_by_id` 的 `Gone` 支返回 `GONE_FRAME_INFO` 这个**常量对象**，其内容由
  `Defines` 数据决定；差分只断言「与 `Defines` 里的同名字段一致」，不重复锁常量形状。
- `find_align_frame` 的 `dst` 里含非字符串项、`src` 含重复 id 等情形属于数据约束
  （帧表由 `IData` 保证），不在 DSL 里构造。
- `handle_facing_flag` 在 `ctrl` 为 `null` 时 TS 的 `Trend` 支会抛异常（没有可选链），
  端口同样不防；这是上游行为，不构造该输入。

### 46.4 harness 观察点
- 新增 op：`run prev`、`run findframe <id>`、`run autoframe`、`run align <id> <src> <dst>`、
  `run suddenframe`、`run caughtframe`、`run facingflag <flag>`、`run waitflag <wait> <frame>`、
  `run framewait <frame>`、`run waitblock <数字>`（裸数字，不用 `Value` 字面量）、
  `run buddy` / `run buddyset <字段> <值>`（第二个实体，用来让 catcher 与 bearer 有不同朝向）。
- `run link … buddy` 把第二个实体接到 `bearer` 上，于是「catcher 与 bearer 朝向不同」成为
  可构造输入（4/5 别名的判别力全靠它）。
- `run hook` 扩展 `frameid|autoframe|sudden|caught` 四个子命令，`frameid` 多一个
  `echo` 形式（把 id 原样返回，锁钩子的参数）；`run hook none` 现在清空全部六个钩子。
- 新 `get` 字段：`from_wait_block`（`b0`/`b1`）。
- `run findframe` 的 id 直接收 `Value` 字面量，`s "gone"` / `z` / `u` / `n 7` 分别覆盖
  `Gone` 常量支、`null` 查表支、`undefined` 当前帧支、数字键支。
## 47. 切片 9d：`Entity` 的快照读写层

`entity.{h,cpp}` 的第四刀：`to_snapshot(nums, strs)` / `read_snapshot(nums, strs)`，
外加它需要的 `copies`（TS 的 `Set<string>`）、两个新宿主缝
（`find_data` = `lfw.datas.find`、`find_entity` = `world.entity_map.get`）。
这一层是存档 / 回放的地基：`NSlot` / `SSlot` 表在切片 4 就搬完了，所以本刀只做
「逐槽位搬运 + 类型转换」，可以在没有 `World` / `LFW` 的情况下整片锁死。

### 47.1 单元边界（为什么是这两支）
1. **不碰 `set_frame` / `enter_frame` / `get_next_frame`**：它们要
   `__judger`（`base/expression.h` + `lfw.mt`）与 `preprocess_next_frame`（端口仍是桩）。
2. **不碰 `set_state`**：需要 `Entity` 实现 `state::IStateEntity`，与本刀无关。
3. **`copies` 顺手补上**：`to_snapshot` 读它、`read_snapshot` 写它、`reset()` 清它，
   而且它出现的第三条路径（`create_copy`）属于后续切片，所以本刀只给
   `Set.add` 语义（插入序 + 去重）留一个 `add_copy` 缝。
4. **两个宿主缝**：`read_snapshot` 是端口里第一处「按 id 找数据 / 找实体」的需求
   （`lfw.datas.find`、`world.entity_map.get`），做成 `IEntityHost` 的虚函数，
   默认实现是「找不到」。

### 47.2 保真要点
1. **端口签名收 `Value` 而不是 `double`**：TS 的 `nums` 是 `number[]`，但槽位里
   可以出现 `null`（`nums[MP_MAX] = this._mp_max` 没有 `?? NaN` 兜底），而 `null`
   与 `NaN` 在 `read_snapshot` 之后的 `mp_max()` 上**可观测地不同**，所以端口用
   `std::vector<Value>`：数字、`NaN`、`null`、`undefined` 四态都能原样表达。
   `Times` 的 5 槽块（`write_nums` / `read_nums`）走一个 `std::vector<double>` 暂存数组，
   `utils/times.*` 一行未改。差分里 `run snappoke MP_MAX z` 与 `n 7.5` 分别验这两半。
2. **`?? NaN` 槽位与裸槽位是两类**：`RESTING_MAX` / `FALL_VALUE_MAX` / `DEFEND_VALUE_MAX` /
   `DEFEND_RATIO` / `CATCH_TIME_MAX` / `DISMISS_TIME` 写的是 `x ?? NaN`
   （端口 `or_nan`），而 `MP_MAX` / `HP_MAX` 写的是裸值。`read` 侧前者走
   `num_or_null(…)`（NaN → `null` → `nullopt`），后者只走 `opt_num`：
   于是「NaN 进 → null 出 → NaN 再写出」对前一组成立，对后一组 `null` 会留着。
   差分给六个槽位各喂一次 `n NaN`（再 `run get` 看它回落到数据集）与一次普通数字。
3. **`num_or_null` 是严格 `Number.isNaN`**：`null` / `undefined` / 字符串都原样穿过，
   只有真 NaN 变 `null`。端口复用切片 4 的 `entity::num_or_null(const Value&)`，
   再套 `opt_num`（nullish → `nullopt`，NaN 保持 NaN）。
4. **布尔四件套是 `!== 0`，不是 `truthy`**：`bounced` / `drop_hurted` / `dropping` /
   `is_on_ground` 的读侧是 `nums[SLOT] !== 0`，所以 `null` / `undefined` / `"0"`
   都算 **true**。端口用 `not_zero`（`strict_equals(v, 0)` 取反）；差分用两组
   「0 / 2 / z / -1」不同真值模式把「取反」与「收成 `truthy`」两种变异都杀掉。
   写侧是 `x ? 1 : 0`（`false` 落 0）。
5. **字符串槽位的三种「压平」**：实体引用（`catching` / `catcher` / `bearer` /
   `holding` / `landing_frame`）写 `?? ''`；`_name` / `_after_blink` /
   `dismiss_data.id` 同理；而读侧的 `_name = strs[NAME] || null` 与
   `_after_blink = strs[AFTER_BLINK] || null` 是**真值语义**——空串进去 → null 出来。
6. **`copies` 是插入序 + 去重的集合**：端口用 `std::vector<std::u16string>` + `add_copy`
   （`Set.add` 语义：重复不插）。写侧把所有 id 用逗号连起来再 `slice(0, -1)` 去掉尾逗号
   （空集合写 `''`）；读侧 `split(',')` 会把 `"x,"` 变成 `["x", ""]`——空串也是成员。
   差分有「`c1 / c2 / c1` 三次 `run copy`」（去重）、「`COPIES = "b,a,b"`」（去重 + 顺序）、
   「`COPIES = "x,"`」（尾逗号产生空成员）三段。
7. **`transforms` 的双重条件**：读侧要 `t0 && t1`（**两个 id 都非空**）且 `find` 到的
   两份数据都真值，否则 `null`；写侧是 `transforms?.[0]?.id ?? ''` / `[1]`，
   端口按 `as_array` + `size() > 0` / `> 1` 复刻。差分有一条 1 元素数组
   （`TRANSFORM_0` 有值、`TRANSFORM_1` 必须为空）与三条「只有一个 id 能查到」的场景。
8. **`dead_join` 走 JSON**：写 `JSON.stringify`（端口 `core/json.h` 的 `json_stringify`），
   读 `JSON.parse`（`json_parse`）。空串 → `null`。TS 在 JSON 非法时**抛异常**，
   属于端口模型外：端口保留旧值，差分只喂合法 JSON（`{"d":5}`）。
9. **`read_snapshot` 不触发任何通知**：它直接写私有字段（`_hp` / `_mp` / `_reserve` …），
   所以每段 `run snapapply` 的回调日志都必须是空的——这也是差分的一部分。
10. **帧查找的 `?? this.frame` 是死代码**：`find_frame_by_id` 永远不会返回
    nullish（未命中会回落到 auto 帧），所以 TS 的 `?? this.frame` / 端口
    `if (!nullish(...))` 两边同路；`LANDING_FRAME_ID` 则真的分「空串 → null」与
    「非空 → 查表结果（查不到是 auto 帧对象）」两支。
11. **赋值顺序**：`id` 与 `_data`（`lfw.datas.find`）先落，再做帧查找，所以数据里
    自带的 `frames` 表参与查找。差分的 `env data` 记录里放了一张 `frames` 表
    （`"f-7"`），并用 `DATA_ID → "d1"` + `FRAME_ID → "f-7"` 把这条顺序钉住。

### 47.3 有意不覆盖 / 不可观测项
- `to_tri` / `from_tri` 不在 `Entity` 的快照路径上（属于别的调用点），本刀不涉及。
- `Times::to_snapshot` / `read_snapshot`（`array<double,5>`）不属于本刀。
- `_after_blink` 的「空串 vs nullopt」在**本刀不可观测**：它只喂给尚未搬的闪烁逻辑，
  写侧两种表示都打印成 `""` → 相关候选撤回（表头记录）。
- 往 `double` 槽位 poke 非数字（例如 `s "7"`）超出端口模型：TS 会把字符串原样存进
  `_hp`，端口按 `to_number` 折算；差分只 poke 数字 / `null` / `NaN`。
- `JSON.parse` 失败时 TS 抛异常、端口保留旧值 → 不做差分（不是可比较的行为）。
- `read_snapshot` 里 `frame = find_frame_by_id(...) ?? frame` 的兜底（见 47.2-10）等价，
  与 `_landing_frame` 的 `?? null` 同类，均已在变异表头记录，不列条目。
- `nums[STAT_BAR_TYPE] = this.stat_bar ?? NaN` 的 `??` 在端口不可达：TS 把 `stat_bar`
  声明成 `number`（初值 0），端口同样是 `double`，所以写侧直接 `Value(stat_bar)`。

### 47.4 harness 观察点
- 新增 op：`run snap`（当前快照）、`run snapbuf`（把当前快照存进缓冲区）、
  `run snappoke <槽位名> <值字面量>`、`run snappokestr <槽位名> <值字面量>`、
  `run snappokeid <槽位名> self|buddy`（把**活实体的 id** 塞进某个字符串槽位，
  这样 `read_snapshot` 的按 id 解析才真的能命中，且不会把 id 写死在用例里）、
  `run snapapply`（把缓冲区喂回 `read_snapshot` 并打印回读后的快照）、
  `run copy <字符串>`（`copies.add`，打印 `added=b0|b1`）、
  `env data <id 字面量> <数据字面量>`（`lfw.datas.find` 的表）。
- 输出：`run snap|snapbuf|snapapply || <日志> | n=<全部 num 槽位> s=<全部 str 槽位>`，
  槽位用**枚举顺序**、逗号分隔、两侧共用 `render`（数字带位模式），任何一个槽位错位都会露。
- 槽位名 → 下标走 `nslot_entries()` / `sslot_entries()`（切片 4 的表），
  顺带把「枚举表与实际赋值」绑在一起。
- 新 `get` 字段：`dismiss_time`（`number | null`）、`dismiss_data`（对象或 null）、
  `catching` / `catcher` / `bearer` / `holding`（渲染成 `{id}` 或 `null`，
  避免把整只实体打出来）；`transforms` / `dead_join` / `landing_frame` 同时可 `set`。
- 「每个槽位都 poke 一个互不相同的值再 `snapapply`」这一段（`snapbuf` → 105 次
  `snappoke` + 17 次 `snappokestr` → `snapapply` → `snap` + 60 多个 `get`）
  是杀掉「写错槽位 / 漏写槽位 / 读错槽位」三类变异的骨干。
- TS 侧的 `world.entity_map.get` 用**活实体**现算（`reset` 会换 id，用表会过期），
  `lfw.datas.find` 用 `env data` 填的表；端口侧同义。

## 48. 切片 9e：`Entity` 的每 tick 恢复层

`entity.{h,cpp}` 的第五刀：`toughness_recovering`、`fall_value_recovering`、
`defend_value_recovering`、`stat_recovering`、`hp_recovering`、`mp_recovering`。
这六支是 `update()` 每个 tick 都会调的「回血 / 回气 / 回韧性」逻辑，输入只有
「帧标志 + 数据集 + `Times` + `_atom_time`」，所以仍然可以在 `World` / 状态机缺席时整片锁死。

### 48.1 单元边界（为什么是这六支）
1. **不碰 `update()`**：它是这些函数的调用者，还要状态机、碰撞与 `lfw.mt`，属于后面的切片。
2. **`Times` 早已搬完**：`add(_atom_time)` 的「到上限才返回 true + 回绕 + `_remains` 递减」
   是上一批 `Times` 切片的既有行为，本刀只负责**门控**（不通过就 return）。
3. **`clamp_add` 已存在**（`utils/math/clamp_add.h`）：本刀第一次用它，
   与 `round_float(value + offset)` 再夹的语义一致（先取整再夹）。

### 48.2 保真要点
1. **两段式分支**：`toughness_recovering` 与 `stat_recovering` 都是
   「先看 *_resting 是否 > 0 → 走排空支；否则走恢复支」，两支都以 `return` 结尾。
   端口逐字照抄，其中排空支靠 `frame.toughness_recover` / `frame.stat_recover` 开关。
2. **排空支按 `_atom_time` 排**：`clamp_add(resting, -_atom_time, 0, max)`，
   所以 `run set atom_time n 2` 之后一次调用就排 2；差值的上下界分别是
   `_toughness_resting_max`（韧性）与 `resting_max()`（休息值）。
3. **恢复支的门是 `Times`**：只有 `add(_atom_time)` 返回真才写值。
   `hp_recovering` / `mp_recovering` 每次调用都会先用 `dataset("hp_r_ticks")` /
   `dataset("mp_r_ticks")` 覆盖 tick 的上限，而 `toughness` / `fall` / `defend`
   用的是 `reset()` / `reset_armor()` 装好的区间——差分把四个 interval 设成
   互不相同的值（3 / 2 / 2 / 5 / 4），任何「读了别的数据集键 / 用了别的 tick」都会露。
4. **`fall_value_max` / `defend_value_max` 是 getter**（`_x ?? world.dataset.x`）：
   数据集缺键时比较与夹取都会遇到 `NaN`，而 `clamp_add` 的 `value > NaN` 恒假 → 保留原值。
   差分有 `env dataset mp_r_ratio u` 这类「缺键 → NaN 传染」的场景。
5. **`hp_recovering` 夹的是私有 `_hp_r`**（不是 `hp_max`），增量是 `dataset("hp_r_value")`：
   `set_hp(min(_hp_r, _hp + v))`。因为 `set hp` 自己还会夹一次 `hp_max`，
   差分专门把 `HP_R` 通过快照窥视口设成 `30 < hp_max = 40`，才能把「不夹 `_hp_r`」这条变异杀掉。
6. **`mp_recovering` 的比率公式**：`a = hp_max()`、`b = _hp`，两者先各自夹到 500，
   然后 `value = 1 + round_float((a - min(r_ratio * b, a)) / 100)`，最后
   `set_mp(min(mp_max(), _mp + value))`。
   - 三个守卫（`_hp <= 0`、`_mp >= mp_max()`、`_blinking`、`_invisible`）中前两个是数值比较、
     后两个走 `truthy`（所以 `NaN` 也算假）。
   - 500 的两处夹取用 `520 / 600` 的场景才能区分（不夹时 `min(1 * 520, 600) = 520` 与
     夹后 `min(500, 500) = 500` 给出不同的增量）。
   - `round_float` 是**三位小数**取整，差分用 `mp_r_ratio = 0.33333333333` 造出
     `1.9600000000004 → 1.96` 的差异（harness 打印数字的位模式，能分辨）。

### 48.3 有意不覆盖 / 不可观测项
- `update()`（调用方）与 `mouse` / `world` 相关的旁路不在本刀。
- `get toughness_max()` 就是私有 `_toughness_max`（没有数据集兜底），所以
  「夹取用 getter 还是字段」在韧性这一支不可观测 → 不列条目。
- `hp_recovering` 的 `_hp_r_tick.set_max(...)` 传的是 `to_number(dataset(...))`：
  数据集缺键时是 `NaN`，`Times` 的区间比较随之恒假；这条与 `Times` 自身的行为重合，
  只在 `hp_r_value` 缺键的场景里顺带覆盖。
- 各恢复函数的返回值是 `void`，没有任何「是否恢复」的外部信号，所以只能用状态读数判断。

### 48.4 harness 观察点
- 新增 op `run rec <stat|hp|mp|toughness|fall|defend>`，一次调用同时打印
  `hp / hpr / mp / mpmax / r / t / tr / fv / dv` 与回调日志——恢复函数的每个写入
  都走 setter，所以 `on_hp_changed` 这类日志本身就是判别力来源。
- `run set atom_time <数字>` 补了一个**窥视写入**（TS 侧写 `_atom_time`、端口加
  `set_atom_time`），否则只能用「改数据集 + 重新 `run make`」来造 `_atom_time` 的变化。
- `Times` 的内部状态（`VALUE/MIN/MAX/LIFES/REMAINS` 五个槽位）直接用 9d 的快照窥视口
  `snappoke` 精确摆位（例如把 `HP_R_TICK_REMAINS` 设成 0 验证「耗尽后永不再触发」），
  再用 `run snap` 读回——9d 的快照能力在这里第一次被别的切片当工具用。
## 49. 切片 9f：`Entity` 的标记 / 发射者 / 出弹点速度

`entity.{h,cpp}` 的第六刀：`set_mark` / `del_mark` / `is_ally` / `get_emitter` /
`get_opoint_speed_z`。这五支是「不含状态机、不含 World」的最后一批纯逻辑：
前四支只读自身的 `marks` / `_team` / `emitters`，最后一支只读 `frame.state` 与
emitter 的 `data.type`，所以仍然可以整片锁死。

### 49.1 单元边界（为什么是这五支）
1. **`marks` 是 `Map<string, string>`**：端口用 `std::map`（键唯一 + 按 key 有序）。
   JS `Map` 是插入序，端口是字典序，所以 harness 两侧都把转储**排序**后再打印
   （`k:v` 逗号拼接），让「顺序」不再是差异来源，只留「内容」。
2. **`set_mark` / `del_mark` 的 `prev == void 0` 是宽松比较**：
   `null` 也算「没有期望值」；`marks.get(key) == prev` 同样是 `==`，
   所以存了 `"3"` 的标记能被 `prev = 3`（数字）命中。端口这两处都用 `equals`（JS `==` 语义），
   而不是别处常用的 `strict_equals`。
3. **`del_mark` 返回的是 `Map.delete` 的返回值**（删到了才 true），
   而不是「条件成立」；条件成立但键不存在时是 `false`。
4. **`is_ally` 是严格 `===`**：团队字段是字符串领域，宽松与严格在字符串上不可区分，
   所以这条只能靠「比较对象写错 / 取反 / 恒真」这类变异检验。
5. **`get_emitter(idx)`**：`this.emitters[idx]` 是 JS 数组下标（分数 / 负数 / 越界
   一律 `undefined`），随后 `if (!id) return;` 把**空串**（唯一的假值字符串）也拦掉，
   最后 `world.entity_map.get(id)` 查不到就是 `undefined`。
6. **`get_opoint_speed_z(emitter, opoint)`**：`speedz !== void 0` 先胜出
   （`null` 原样返回，`undefined` 才落到默认），然后 `is_fighter(emitter)`
   只读 `emitter.data`（`v?.data` 让缺失的 emitter 也走「非 fighter」），
   最后 `switch (this.state)` 是**严格数字开关**：只有
   `Ball_Flying(3000)` / `Ball_3006(3006)` / `Weapon_Throwing(1002)` /
   `HeavyWeapon_InTheSky(2000)` 四个状态给 `Defines.DEFAULT_OPOINT_SPEED_Z`，
   其余（含字符串 `"1002"`、小数 `1002.5`、`null`、`undefined`）一律 `0`。

### 49.2 保真要点
1. **端口签名收 `const Entity*`**：TS 里 emitter 可以是 `undefined`（调用方
   `get_emitter` 的返回值），`is_fighter(undefined)` 走 `v?.data` → false → `0`。
   差分有 `run opointz null …` 场景锁这条路径（`speedz` 存在时仍然先返回 `speedz`）。
2. **`emitter->data()` 与 `data()` 不是一回事**：一个是发射者的数据（决定
   `is_fighter`），一个是自己的数据（决定 `frame.state` 的宿主）。harness 故意让
   buddy 的 `type = 8`（fighter）而自动生成的那个实体是 `type = 1`，
   于是「读错对象」的变异立刻在两端的 `opointz` 读数上分叉。
3. **空串 emitter id 必须真的拦掉**：`run emit 0 s ""` 之后 `get_emitter`
   必须返回 `undefined`。为了让「去掉空串检查」这条变异可杀，差分用 9d 的快照窥视口
   把实体的 id 改成 `""`（`snappokestr ID` + `snapapply`），此时宿主
   `find_entity("")` 是能查到实体的——只有真的判断了空串才会返回 `undefined`。
4. **`state` 不做数值化**：端口先 `std::get_if<double>` 再比较，
   没有 `to_number` 那一层；差分对同一个实体跑
   `n 3000 / n 1002 / n 3006 / n 2000 / n 3009 / s "1002" / n 1002.5 / z / u`
   九个状态，任何一种「提前转成数字」都会在字符串与小数两行露出来。

### 49.3 有意不覆盖 / 不可观测项
- `if (!(idx >= 0) || …)` 里去掉 `!(idx >= 0)` 那一半是**等价变异**：
  `-1` 转成 `size_t` 是巨大值，仍会被越界检查挡住并返回 `undefined`，不列条目。
- `i >= emitters.size()` 改成 `i > emitters.size()` 会越界读（UB），不作为变异条目。
- `del_mark` 里「缺键读成 `""` 而不是 `undefined`」不可观测：缺键时
  `Map.delete` 无论如何都返回 `false`，转储也不会变，所以同一处只对 `set_mark` 列条目。
- `is_ally` 的严格 / 宽松在字符串团队上同义，没有对应变异条目。
- `speedz` 是对象 / 数组这类非数字时，两端都原样返回（`field_or` 交回 `Value`），
  由「原样返回」那条覆盖，不再单列。

### 49.4 harness 观察点
- 新增 op：`run mark <键> <值> [prev]`、`run delmark <键> [值]`
  （都打印 `v=b0|b1` 与排序后的 marks 转储）、`run ally self|buddy`
  （同时打印双方的 `team`）、`run emit <下标> <id 字面量>`、
  `run emitid <下标> self|buddy`（把活实体的 id 写进发射者数组，避免写死）、
  `run getemitter <下标>`（渲染解析到的实体，`u` 表示解析不到）、
  `run opointz <self|buddy|null> <opoint 字面量>`（同时打印 `state`）。
- 场景矩阵：宽松 `==`（`n 3` 命中 `"3"`）、`z` 作 prev / value、缺键 vs `""`、
  下标 `0 / 1 / 2 / 3 / -1 / 1.5 / 0.5` 与「同一 id 的空串」、
  四个命中状态 + 未命中状态 + 字符串 / 小数 / `null` / `undefined` 状态。

## 50. 切片 9g：`Entity` 的状态接线（`set_state` + `_state`）

`entity.{h,cpp}` 的第七刀，也是 Entity 块里第一刀**结构接线**：新增
`set_state`、`_state` 字段与 `EntityStateView` 适配器，并把此前注入的六个状态钩子
（`state_on_dead` / `state_get_gravity` / `state_find_frame_by_id` /
`state_get_auto_frame` / `state_get_sudden_death_frame` / `state_get_caught_end_frame`）
全部改成从**活动状态对象**上取——TS 里这些钩子本来就在 `State_Base` 上，
注入只是"状态机到位前的临时接法"。

### 50.1 单元边界（为什么是这一刀）
1. **`set_state` 是唯一入口**：TS 里 `this._state` 只在 `set_state` / `reset` 里被赋值，
   而 `set_state` 的调用点（`set_frame` / `attach` / `update` 系列）都还没搬，
   所以这一刀能**自洽地**把状态接线锁死：harness 直接调 `set_state` 即可。
2. **六个钩子调用点已存在**（9a 的 `on_dead` / `get_gravity`，9c 的 `find_frame_by_id` /
   `find_auto_frame` / `get_sudden_death_frame` / `get_caught_end_frame`），
   这一刀只换"从哪取钩子"，不改调用语义（`truthy` / `??` / `nullish` 的判定保持原样）。
3. **`States` / `State_Base` 早在状态块里搬完**：本刀第一次让它们与真实实体对接，
   于是 `States.fallback` 的键格式（`\`${type}_${code}\``）、类型分派
   （Fighter → `CharacterState_Base`、Weapon → `WeaponState_Base`、
   Ball → `BallState_Base`、其余 → `State_Base`）与**缓存**（命中就不新建）
   都第一次有了外部观察点。
4. **`reset` 里的 `this._states = states` 第一次变得可观测**：9a 就照抄了这一行，
   但在此之前没有任何观察点（`set_state` 还不存在）。
   差分用 `run resetstates`（`reset(data, &g_states)`）与 `run reset`（默认注册表）
   两次对照把它钉住：同一批 `setstate` 在两个注册表里的结果不同。

### 50.2 保真要点
1. **注册表查找与兜底**：`states.get(state_code) || states.fallback(_data.type, state_code)`。
   `get` 用的是**数字键**，`fallback` 用的是字符串键 `"${type}_${code}"`——
   两套键在端口里由 `States::encode_key` 的 `n:` / `s:` 前缀区分，
   差分专门注册一个字符串键 `"20"` 再 `setstate 20`，证明它**不会**命中数字 20。
2. **类型是 `_data.type` 原值**：`field_or(_data, u"type")` 直接交给 `fallback`，
   `States` 内部用**严格**比较分派，所以 `type: "8"`（字符串）落到
   `default` → `State_Base` 而不是 `CharacterState_Base`；差分对这一条有专门场景。
3. **同一性早退**：`if (this._state === v) return;` —— 对**对象**比较（TS 的 `===`），
   所以"同一个状态再设一次"必须完全静默（`run setstate 20` 连打两次，第二次日志为空）。
4. **leave / enter 的帧参数**：`leave(this, this.frame)` 拿**当前帧**，
   `enter(this, this.get_prev_frame())` 拿**上一帧**；差分把两者设成不同的帧 id，
   任何"传错帧 / 传反"都会在日志里露。
5. **`v || null` 的写法**：`fallback` 永远返回对象，所以这一句在 TS 里不会产生 `null`；
   端口写成赋值本身，不再加多余判断（`states_` 永不为空指针）。
6. **`reset` 清 `_state`**：TS 在 `_blinking = 0` 之后即 `this._state = null`，
   所以 `reset` 之后钩子必须全部失灵（差分在 reset 前打开 `on_dead` 钩子、
   reset 后打死血，日志必须为空）。

### 50.3 `EntityStateView`（为什么需要适配器）
- TS 把实体**本身**交给钩子（`on_dead?.(this)`），而端口的 `state::IStateEntity`
  把同一批成员写成了 `Value` 签名（`Value hp_max()`），与 `Entity` 的
  `double hp_max()` 冲突，所以用 `EntityStateView` 做桥：`Entity` 持有一个
  `std::unique_ptr<EntityStateView>`（头文件里只有前置声明，析构写在 `.cpp`）。
- **已转发的成员**（差分全部覆盖）：`id` / `position` / `velocity_x|y|z` /
  `hp` / `hp_r` / `set_hp_r` / `hp_max` / `mp` / `motionless` / `set_motionless` /
  `shaking` / `state` / `frame_info` / `prev_frame` / `is_on_ground` / `team` /
  `data_type` / `jumping_x`。harness 的假状态在 `enter` 里把这些全部读出来打日志，
  于是每条转发都有一条变异能被杀。
- **有意留空**（`IBuffEntity` / `IStateEntity` 的默认值，属于后续切片）：
  `set_position`（World 拥有位置，等地形/限制切片）、`frame_centery` / `frame_height` /
  `frame_pic_h` / `set_frame` / `enter_frame_by_id`（进入帧链）、`attach`（要
  `world.add_entities`），以及 `IStateEntity` 里那批还没搬的成员
  （`holding_*` / `data_indexes_*` / `world_*` / `ground_*` …）。
- **只能转发的两个**：`buffs_set` / `buffs_delete` 已接到 `Entity.buffs` 上，
  但本轮差分没有"读回 buff 表"的观察点（buff 接线切片负责），
  同理 `set_outline_alpha|width|color` 只是照抄转发。

### 50.4 生命周期：TS 的 `Map` 与端口的裸指针
TS 的 `States` 是 `Map`，`set` 覆盖一个键时旧对象仍被 `this._state` 引用着（GC 保命），
端口 `States::set` 会销毁被替换的 `unique_ptr`。真实游戏的注册表在启动时**一次性**建好
（`entity_states()`），此后不再替换，所以裸指针是安全的；差分也遵守这条：
场景里覆盖一个键时，该键一定**不是**当前活动状态（`reg 50`/50 活动 → 切到 51 再覆盖 50）。
这条写进 DESIGN 而不是"悄悄绕过"，因为它是端口与 TS 之间唯一无法用代码表达成一致的差异。

### 50.5 harness 观察点
- `run reg <数字>` / `run regbare <数字>` / `run regkey <键字面量>`：往 harness 自己的
  注册表里放"会打日志的假状态" / "没有钩子的裸 `State_Base`" / "任意键的假状态"。
- `run setstate <数字>` / `run setstateb <数字>`（后者作用于 buddy，用来验证
  "同一个状态对象被两个实体共享"），打印**注册表大小**与"当前是否有活动状态"。
- `run statesdump`：按插入序打印 `键:类名`，于是 `fallback` 的键、类名与缓存都能直接看。
- `run reset` / `run resetstates`：前者走默认注册表、后者带 harness 注册表，
  同一条 `setstate` 在两个注册表里的结果不同，正好把 `this._states = states` 钉住。
- `run hook …` 语义不变（只是改成配置"假状态的钩子开不开"），
  假状态的 `enter` / `leave` 日志自带**自己的状态键**，
  于是"leave 的是旧状态还是新状态"这种顺序错误也能被杀。

### 50.6 有意不覆盖 / 不可观测项
- 上面 50.3 列出的留空成员与两个"只转发未观察"的成员：本轮没有观察点，
  不列变异条目（各自的切片负责）。
- `States::set` 覆盖**活动**键的场景不可测（见 50.4 的生命周期差异），差分不写。
- `pre_update` / `update` / `on_landing` / `on_leave_ground` / `on_restrict` 的调用点
  分别在更新循环与地形限制里，尚未搬运，本轮不涉及。

## 51. 切片 9h：`Entity` 的 v_rest 三段 / 关系清理 / 闪烁标记 / 落地请求

第四刀之后的又一把「小刀」：这一层全部是**短函数 + 一个隐藏的语义分歧点**，
输入只有实体自己的容器与字段，外加一次宿主「请求进入某帧」的回调。

搬运的十支：`add_v_rest` / `get_v_rest` / `del_v_rest`、
`get_flag`、`clean_holding` / `clean_catching` / `drop_catching`、
`blink_and_gone` / `blink_and_respawn`、`update_itr_bdy_hit_ground`。

### 51.1 单元边界（为什么是这十支）

1. **不碰帧链**：`set_frame` / `enter_frame` / `get_next_frame` /
   `handle_next_frame_result` 要 `preprocess_next_frame` 的 `__judger`
   （端口仍是桩）、`lfw.mt.pick`、以及 `world.dataset.infinity_mp` 这类 World 数据。
2. **不碰位置**：`set_position` / `update_position` / `spark_point` 要
   `world.restrict`（含 `get_bound` 与武器/球的越界判定）与 `world.ground.y`。
3. **不碰跨实体动作**：`drop_holding` / `pick` / `follow_bearer` / `follow_catcher`
   会对**另一个实体**调 `enter_frame` / `set_position` / 克隆 v_rest，等帧链落地。
4. **不碰生命周期**：`attach` / `spawn` / `on_spawn` / `release` / `transform` /
   `transfrom_to_another` 要 `world.add_entities` / `factory.create_ctrl` /
   `world.del_entity`。
5. **不碰更新循环**：`update_caught` / `update_catching` 内含 `follow_catcher` 与
   `transfrom_to_another`；`update_ghost` / `check_fusion_dismissing` 要 Actor 与 `ctrl` 的按键判定。

### 51.2 保真要点

1. **`add_v_rest` 写三张表，但从不删旧镜像**：`vrests` 一定写；
   `blockers` / `superpunchs` 只在 `itr.kind` **严格等于** `ItrKind.Block`(14) /
   `SuperPunchMe`(6) 时写。所以「先以 kind=6 登记 w2、再以 kind=14 覆盖」之后，
   `superpunchs` 里的 w2 仍是**旧的**那条（TS 行为如此，端口照抄）——
   用例用 `run vrest s "w2" n 6 n 4` → `run vrest s "w2" n 14 n 9` → `run vrestdump`
   前后对照锁死（`b` 里是新值、`s` 里还是旧值）。
2. **kind 读取是 `field_or`，不是解引用**：TS 写 `c.itr.kind`，`itr` 为 `undefined`
   时**抛错**；端口用 `field_or(c.itr, u"kind")`（`u`/`z`/字符串 kind 一律只是「不镜像」）。
   非法输入不写进差分，这条差异记在这里。
3. **容器值语义**：端口三张表都是 `std::map<std::u16string, Collision>`，**按值存**；
   TS 三张 `Map` 共享同一个 `Collision` 对象引用。于是：
   - TS 的 `Map` 插入序端口不保留 ⇒ `run vrestdump` 两侧都**按键排序**输出；
   - TS 的别名（同一条 collision 同时出现在三张表里，改一处三处都变）端口没有对应物，
     本轮用例不覆盖别名，等碰撞层真的按引用传递时再定。
4. **`get_v_rest` 的 `|| 0`**：缺失、`0`、`NaN`（还有 `-0`）都读回 `0`；
   端口因此写 `truthy(Value(rest)) ? rest : 0`。用例覆盖 `n 0`、`n NaN` 与未知 id。
5. **key 会被字符串化**：`Map<string, …>` 的 key 是字符串，端口用 `std::u16string`；
   harness 的 `text_of`（= TS `String(v)`）把数字 token 变成 `"5"`，
   所以 `run vrestget n 5` 与 `run vrestget s "5"` 命中同一条（用例锁）。
6. **`get_flag` 是「队伍位 + Dead + 类型」的位组合**：
   `team` 相等给 `Ally`(2) 否则 `Enemy`(1)；`hp <= 0` 时 `|= Dead`(0x80)；
   最后 `| this.type`。最后一步是 JS 位运算 ⇒ 端口先 `js_to_int32(type())`：
   `type = 2.7` 先截成 2（`2|2 = 2` 之上再或队伍位）、`type = "8"` 先 `ToInt32` 成 8、
   `type = undefined` 当 0、`type = -1` 因符号位把整个结果压成 -1。
   `hp` 的边界用 `0 / -5 / 0.5 / 10` 四点覆盖（注意 `set hp n -5` 会被 setter 钳到 0）。
7. **`clean_holding` / `clean_catching` 只清「对方回指自己」**：
   先判自己这侧为空立刻返回；`holding.bearer === this` 才写 `null`，
   然后再清自己。用例用 `run linkb bearer self`（回指自己）与
   `run linkb bearer buddy`（回指别人）两支覆盖，`!= this` 与「不清自己这侧」都能被杀。
8. **`drop_catching` 恒返回 `true`**（只要原来有 `catching`）：清对方回指 →
   `set_catching(null)`（走 9a 的同一性短路）→ `enter_frame(Defines.NEXT_FRAME_AUTO)`
   → `return true`。`enter_frame` 是**宿主缝**：帧链未移植，harness 两侧都只记
   `enter_frame:{"id":s"auto"}`。用例覆盖「没有 catching」（`b0`）、
   「回指是自己」「回指是别人」「连打两次」。
9. **`blink_and_gone` / `blink_and_respawn` 不走 setter**：它们**直接**写 `_blinking`，
   而 `set blinking`（9a 移植）是 `round_float(max(0, v))`。用例因此成对写
   `run blinkgone n -3`（得 `-3`）与 `run set blinking n -3`（得 `0`）把这条锁死。
   `_after_blink` 只会是 `"gone"` / `"respawn"`（`FrameId.Gone` / `FrameId.Respawn`
   字面量），`run snap` 把 `AFTER_BLINK` 槽位一并纳入快照覆盖，
   `run blinkgone u` / `run blinkrespawn n -3` / `run blinkgone n 4` 顺带锁「时长是
   NaN / 负数 / 正数都照写」（`n -3` 是关键：换 `set_blinking` 会被它杀）。
10. **`update_itr_bdy_hit_ground` 的四个判定**：
    `if (!itrs?.length) return;`（端口 `as_array` 判空/非数组）、
    `if (!itr.on_hit_ground) continue;`（**真值**门，`b0`/`u`/`""` 都跳过）、
    `const { y = 0, h = 0 } = itr`（默认值只对 `undefined` 生效；`null` 在减法里也是 0，
    所以端口统一写 `nullish ? 0 : to_number`）、
    `(position.y + frame.centery - y - h) > _ground_y` 是**严格大于**（相等要进帧），
    命中后 `enter_frame` 并**继续走完整个列表**（`continue` 不是 `break`）。
    用例覆盖：缺 `on_hit_ground`、假值 `on_hit_ground`、`y` 为 `z`、
    `y/h` 为字符串数字、`y` 为负、边界相等（`fG`）、`centery` 缺失（NaN 比较为假 ⇒ 仍进帧）、
    `centery` 为字符串（`to_number` 后照算）、`ground_y = NaN`、
    以及「前一条被跳过、后一条仍进帧」（`fM`/`fN`）；另外三条「假值 `on_hit_ground`
    （`b0` / `u` / 缺键）但箱子已经压到地面线」的场景把
    `if (!truthy(target)) continue;` 这条守卫单独钉住（少了它就会请求一个假帧）。

### 51.3 harness 扩充

- 新 op：`run vrest <aid> <kind> <rest>` / `run vrestget <aid>` / `run vrestdel <aid>` /
  `run vrestdump` / `run flag <self|buddy>` / `run cleanhold` / `run cleancatch` /
  `run dropcatch` / `run blinkgone <数>` / `run blinkrespawn <数>` /
  `run itrground <数组字面量>` / `run linkb <字段> <self|buddy|null>`。
- 端口侧直接构造 `collision::Collision` 结构体（只填被读的 `aid` / `itr` / `rest`），
  TS 侧给一个同形状的字面量对象 —— 两侧都只关心这三个字段。
- `run link` 只管 self，`run linkb` 是它的镜像（回指必须能指向自己/对方），
  两者都打印八位关系探针 `sh sb sc sr bh bb bc br`（self/buddy 的
  holding/bearer/catching/catcher 是否非空），于是「回指被清/没被清」在日志里可见。
- `_after_blink` 端口通过窥视口 `after_blink()` 读回（TS 直接读 `_after_blink`），
  `null` 与字符串两侧渲染一致。

### 51.4 有意不覆盖 / 不可观测项

- TS 的 `Map` 插入序与三表别名（见 51.2.3）：差分两侧不可比 / 端口无对应物，不写场景。
- `add_v_rest` 收到没有 `itr` 的 collision：TS 抛错、端口静默，属于非法输入，不写场景。
- `update_itr_bdy_hit_ground` 的非数组入参：TS 对字符串会按 `length` 逐字符迭代
  （字符上没有 `on_hit_ground`，什么也不做），端口 `as_array` 直接返回 ——
  两条路都**不产生日志**，因此不可观测（用例仍写了 `n 5` / `s "ab"` 两组，
  作用是把「不崩」钉住）。
- `drop_catching` / `update_itr_bdy_hit_ground` 里的 `enter_frame` 在端口只是
  **请求**（`IEntityHost::enter_frame` 缝），harness 记录 `enter_frame:<帧>`；
  真正「进入帧」的语义归帧链切片。
