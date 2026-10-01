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
class Array;   // vector<Value>
class Object;  // 维持 JS 键顺序

using Value = std::variant<
    std::monostate,                 // Undefined
    NullTag,                        // Null（与 Undefined 区分：?? 与 null 的语义点）
    bool,
    double,
    std::u16string,                 // JS 字符串 = UTF-16 code unit 序列
    std::shared_ptr<Array>,
    std::shared_ptr<Object>>;
```

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
| V1 | `Value` 基本类型 + 真值 / `typeof` / `Array.isArray` | 待做 |
| V2 | **`string_to_number`**（`ToNumber(string)`） | **已完成**（146 + 81 行差分全过） |
| V3 | **`number_to_string`**（最短往返 + JS 指数阈值） | **已完成**（77 + 10558 行差分全过） |
| V4 | **宽松 `==`** 完整规则表 | 待做 |
| V5 | 关系比较 `< > <= >=` | 待做 |
| V6 | `Object` + `Object.keys` 顺序 | 待做 |
| V7 | `Expression` 跑在 `Value` 上 | 待做 |

### 4.1 V2 `string_to_number` 的实现要点

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

### 4.1.1 一条必须记住的隐含前提

`from_chars` 的 `result_out_of_range` 只用于**超出 double 范围**的判定，而次正规数（`4.9e-324`）**不算越界**。
若某个实现把次正规也报成 `result_out_of_range`，按十进制指数 `E < 0` 就会返回 `+0`，而 V8 返回 `5e-324`。
用例里的 `"4.9e-324"` / `"5e-324"` 正是钉住这一点的，**实测 MSVC 的 `from_chars` 行为正确**。

### 4.2 V3 `number_to_string`

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

### 4.2.1 不在 V3 范围内：`toFixed`

`get_short_file_size_txt.ts` 和 `dat_translator/fixed_float.ts` 用 `Number(n.toFixed(d))`，
那是**另一套舍入规则**（`toFixed` 有自成的舍入与补零逻辑）。
它们只被 `cook_frames` / `make_itr_prefabs` 调用 ⇒ **构建期 cook**，不在运行期，运行期移植不需要。

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
