# JSON5 移植

**唯一真相源**：仓库里的 `node_modules/json5`（**v2.2.3**）—— 即 Web 构建实际使用的那个包。
`src/DittoImpl/JSON5.ts` 就是 `JSON5.parse` / `JSON5.stringify` 的注入实现。

所以目标不是“按 JSON5 规范写一个”，而是**与这个包的观测行为逐一对齐**，
TS 侧差分直接用 `require("json5")` 当参照。

## 为什么不能用第三方 C++ JSON 库

1. **`JSON.stringify` 必须逐字节等于 JS** —— `DatMgr.ts:135/157` 是拿字符串比的。
   库至少在数字格式（printf 系 vs `Number::toString`）、键序（`std::map` 字母序 vs JS 整数键优先）、
   转义（多处会转义 `/` 和非 ASCII）三处不同。
2. **JSON5 本身**：装载器读的 `.json5` 是**真 JSON5** —— `data.index.json5` 里 `objects:`（无引号键）+ 尾随逗号，
   `gim.obj.json5` 里 `row: 10` / `type: 8`（裸数字）。nlohmann / RapidJSON / simdjson **都不支持 JSON5**。
3. `lfw_core` 是**被测对象**，引入库就等于把被测对象换掉了；且 `check_lfw_cpp_includes.mjs` 是包含白名单。

## `unicode.js` 的处理：生成，不手抄；**探测，不解析**

`unicode.js` 只有 3 个正则（`Space_Separator` / `ID_Start` / `ID_Continue`），是 ES5 标识符表。

**做法：`native/tools/gen_json5_unicode.mjs` 对 `0..0x10FFFF` 每个码点调 `re.test(String.fromCodePoint(cp))`，
把命中区间合并后直接生成 `lfw/core/json5_unicode.{h,cpp}`。**

```
node native/tools/gen_json5_unicode.mjs
→ kSpaceSeparator=5  kIdStart=586  kIdContinue=687   (全码点 mismatch = 0)
```

### 为什么不是“解析正则源码”（试了两次都错）

`ID_Start` 是个**顶层交替**，不是单个字符类：

```
[BMP 类] | \uD800[低代理类] | \uD801[低代理类] | ... | \uD87E[低代理类]
```

ID_Start 有 29 个分支（1 个 BMP + 28 个代理对分支），ID_Continue 有 33 个。
但它是被压缩在一行里的，肉眼很容易误读成“一个字符类里嵌了 `[`” —— 我就误读了两次：

1. **第一次**：以为 `[`/`]`/`|` 是字符类成员（其实 `u.ID_Start.source` 里有**30 个 `[`**：1 个 BMP 类 + 29 个代理对分支）。
2. **第二次**：纠正为交替后，按分支顺序把 BMP 区间追加进数组却**没全局排序**，导致二分查找失效。

探测法**在构造上不可能错**：它用的就是 V8 自己的正则引擎，不需要我对语法结构做任何假设。
代价只是 111 万次 `test()`（不到一秒）。

另外：`Space_Separator`（`\u1680 \u2000-\u200A \u202F \u205F \u3000`）**不包含 `\uFEFF`** ——
BOM 是在词法器 `lexStates.default` 的 `case` 里单独处理的，也不是 `\u200B`。

## 进度

| 文件 | 行数 | 内容 | 状态 |
|---|---|---|---|
| `lib/util.js` | 34 | `isSpaceSeparator` / `isIdStartChar` / `isIdContinueChar` / `isDigit` / `isHexDigit` | ✅ `core/json5_util.h` |
| `lib/unicode.js` 的两张表 | — | `Space_Separator`(5) / `ID_Start`(586) / `ID_Continue`(687) | ✅ `core/json5_unicode.cpp`（生成 + 全码点穷举验证） |
| `lib/parse.js` | ~1130 | 词法状态机 + 语法状态机 | ✅ `core/json5.cpp` |
| `lib/stringify.js` | 218 | `JSON5.stringify`（选项多；本作只需默认行为） | ✅ `core/json5.cpp`（仅默认参数路径） |

`JSON5.parse` 的对拍结果：`json5/parse` **304 行逐行相同**（含每一处的 `行:列`），
12 条变异**全杀**（见 `tests/differential/PROTOCOL.md` §6.2）。
TS 侧参考实现直接 `import JSON5 from "json5"` —— 用原库当真相源。
`JSON5.stringify` 的对拍结果：`json5/stringify` **111 行逐行相同**，
12 条变异**全杀**（§6.3）。只实现了默认参数路径 —— LFW 唯一的调用点
（`dat_translator/copy_itr_info.ts`）就是 `Ditto.JSON5.stringify(value)` 单参数。
`replacer` / `space` / `quote` 选项（只有 `tool/` 下的 Node 脚本在用）**未移植**。
### 移植时必须注意

- **按码点工作，不是 UTF-16 码元**：`peek()` 是 `String.fromCodePoint(source.codePointAt(pos))`，
  `read()` 用 `c.length` 推进（代理对 = 2）。列号按 `c.length` 累加，`line` 只在遇到 `\n` 时 +1。
- **`lexStates` 与 `parseStates` 是两个独立的状态机**，靠 `token` 传递。
- **无异常**：`lfw` 没有异常，错误要改成 `err` 标志 + 消息。
  上游消息形如 `JSON5: invalid character 'x' at 1:7`（`formatChar` 的替换表 + `\xHH` 分支）。
- 数字字面量最终是 `sign * Number(buffer)`；`buffer` 是原文字串（如 `0x10` / `1.5e3`），
  所以**直接复用 `lfw::string_to_number` 即可**（已验证与 V8 一致）。
- 对象键用 `Object.defineProperty`（插入序 + 整数键优先）⇒ 用 `lfw::Object::set`。

### stringify 侧的三个“反直觉”行为（实测，不是猜的）

用 `native/tools/probe_json5_stringify.mjs` 探测出来的：

1. **`util.isIdStartChar('$')` / `('_')` 都是 `true`** —— json5 的正则里确实包含
   `$` 和 `_`，而 Unicode 的 `ID_Start` 本身不含。所以 `{_a:1}` 序列化出来仍是不带引号的 `{_a:1}`。
2. **星号（代理对）出现在键的中间会被加引号**：`{a\u{1D400}:1}` → `{'a𝐀':1}`。
   原因是 `serializeKey` 的循环 `for (i = firstChar.length; ...)` 用 `codePointAt(i)`
   逐位前进，走到低代理位时 `codePointAt` 返回的是**孤立的低代理**，
   它不是 `ID_Continue` → 整个键退回去加引号。**这是上游的行为，不是 bug，且要复刻**。
3. **引号选“少数派”**：默认 `'`；若 `'` 比 `"` 多（严格满足 `nq < 1 + 2*nd`）则换成 `"`，
   然后**只转义选中的那个**（用 `replace(new RegExp(quoteChar, 'g'), ...)`）。

### C++ 侧照着搬时容易踩的三点

1. **`default` 状态里的分支要用 `continue`，不能用 `break`**。
   上游 `lexStates.default` 里 `case '/': ...; return` 是**直接返回**；
   若在 C++ 里用 `break` 跳出内层 `switch`，会继续往下走到
   `lexStates[parseState]()` 那一步，把刚设好的状态**覆盖掉**。
2. **`INFINITY` / `NAN` 用 `std::numeric_limits<double>::infinity()` / `quiet_NaN()`**，
   别用 `<cmath>` 的宏（MSVC 上要 `_USE_MATH_DEFINES`，且会污染标识符）。
3. **`\u2028` / `\u2029` 在字符串里放行（并 warn），裸 `\n` / `\r` 报错** ——
   别凭“JSON5 支持多行字符串”的印象改掉（详见 PROTOCOL.md §6.2 结尾）。