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
