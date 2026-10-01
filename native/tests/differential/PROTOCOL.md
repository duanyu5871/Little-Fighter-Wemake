# 差分测试协议

目标：**同一份用例，TS 侧与 C++ 侧输出逐字节相同。**

TS 侧必须导入 `src/LFW` 下的**真实实现**（不是副本），C++ 侧是移植版。
任何一处语义偏差都会在文本 diff 里暴露出来。

---

## 1. case 文件（DSL）

位置：`cases/*.txt`

```
# 以 # 开头到行尾是注释；空行忽略
# token 之间用任意空白分隔

seed  <number>                          # mt.reset(seed)；同时输出一行 run
int   <count>                           # 取 count 个原始 32 位整数
float <count>                           # 取 count 个 [0,1) 浮点（已量化到 1/1000）
range <min> <max> <count>               # 取 count 个 [min,max) 内的值
pick  <item> <item> ...                 # 用 items 建数组，pick 一次（不改数组）
take  <item> <item> ...                 # 用 items 建数组，take 一次（会移除元素）
state                                   # 输出当前 MT 状态哈希
```

`pick` / `take` 的 items 在每次出现时**重新构建数组**，所以可以连续写多行。

---

## 2. 输出 trace

每行一个 token 序列，token 之间**单个空格**，行末**无空格**。
浮点一律输出 **IEEE-754 位模式的 16 位小写十六进制**（不做十进制格式化 ——
格式化本身就会引入差异）。

### 2.1 判据：量化后再比（**不是**原始位模式）

**跨语言不要求原始浮点位级相同。** 判据是：

> 经过 `round_float(v)` 之后一致 —— 也就是 `Math.round(v * 1000) / 1000`。

两侧各自调用**自己那份真实实现**：

| | 实现 |
|---|---|
| TS | `src/LFW/utils/math/round_float.ts` |
| C++ | `lfw::round_float()`（`native/lfw/core/js_num.{h,cpp}`） |

理由：`round_float` 是**游戏自己的可观测精度**。两个值量化到同一个格点，
对游戏而言就是同一个值；量化后不同，才是真的不同。
另外 `Math.round` 的半值规则（平局朝 +∞，且 `\|x\| < 0.5` 返回带符号 0）
本来就与 `std::round` 不同，所以这一步本身也在校验 `js_round`。

**不量化的是**：

- `int` / `range` 的 min、max —— 都是整数或输入，本来就精确
- `state` —— 内部状态是整数 + 种子，必须**逐位精确**，它是最后一道防线
- `pick` / `take` 的剩余长度 —— 整数

**注意**：`MersenneTwister.float()` 返回的是 `floor(n * 1000) / 1000`，
已经落在 1/1000 格点上，而 `round_float` 在该格点上是幂等的 ——
所以对当前用例集来说，量化前后**输出完全相同**，这一条是给真实浮点预留的规则。

| 行 | 含义 |
|---|---|
| `run <bits16>` | `reset(seed)` 之后的 seed 值（位模式）。用位模式输出是为了连"解析差异"都能抓到 |
| `state <hex16>` | MT 整状态哈希，见下 |
| `int <decimal>` | `next_int()` 的返回值，无符号十进制 |
| `float <bits16>` | `next_float()` 的位模式 |
| `range <bits16min> <bits16max> <bits16result>` | min / max / 结果 的位模式 |
| `pick <bits16\|-> <remaining>` | 选中值的位模式（`undefined` 输出 `-`）与剩余长度 |
| `take <bits16\|-> <remaining>` | 同上 |

`-` 表示 JS 的 `undefined`（C++ 侧是 `std::nullopt`）。

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
u32 mt[1]
...
u32 mt[623]
```

改这个顺序 = **两侧同时改**，否则差分测试会报"状态不同"。
实现：C++ 见 `native/lfw/core/state_hash.{h,cpp}` + `mersenne_twister.cpp`
的 `state_hash()`；TS 见 `mt_trace.ts`。

---

## 4. 怎么跑

```
node native/tools/native.mjs all
```

产物落在 `native/build/gen/`：

```
trace.cpp.txt   C++ 侧输出
trace.ts.txt    TS 侧输出
```

diff 失败时脚本会打印**第一处不同的行号**与两侧内容 —— 那就是漂移点。

---

## 5. 为什么先测 MT19937

因为它是 `src/LFW` 里**唯一一个两侧都已存在、且输出可以逐位对比**的东西：

- 它自带完整的 32 位整数运算（`>>>`、`<<`、`^`、`&`、隐式 int32 截断）
- 还带一段**非标准的双精度种子初始化**（见 `mersenne_twister.cpp` 顶部注释）
- 还有 `floor_float` 那层浮点量化

如果这 1000 个数对不上，说明 `core/` 的地基就有问题 ——
**那比在 `World.step` 里发现第 300 帧漂移便宜一万倍。**
