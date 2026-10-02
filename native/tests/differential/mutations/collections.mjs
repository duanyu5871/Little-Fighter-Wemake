/**
 * collections 变异规格
 *
 * 覆盖 subject: collections
 *   base/graves.h
 *   utils/array/{loop_arr,make_arr,map_arr}.h
 *   utils/container_help/{filter,find,fisrt,ensure,loop_offset,map_no_void,nested_map,nested_multi_map}.h
 *
 * 已删除 / 未注入的变异（不算覆盖缺口，理由记在此处以免以后重复尝试）：
 *  1. graves.h `add()` 的 `_l[--_i] = t;` 改成 `_l[_i--] = t;` —— 当 `_i == _l.size()` 时
 *     （例如 basic.txt 里 `graves_add 3` 紧跟在把 `_i` 推到最后一次 take 之后）会写越界 ⇒ UB，
 *     按规矩不作为覆盖证据。
 *  2. graves.h `take()` 的 `_i >= _l.size()` 改成 `_i > _l.size()`、`add()` 的 `_i == 0`
 *     改成 `_i != 0`、成员初值 `size_t _i = 0;` 改成 `1` —— 三者都会让 `_i` 在越界状态下
 *     继续被 `_l[_i]` / `_l[--_i]`（回绕成 SIZE_MAX）使用 ⇒ UB。
 *  3. ensure.h 模板版 `if (!output.has_value()) return items;` 取反 —— `ensure | 1 2 3`
 *     这类"目标为空"的用例会让取反后的代码走到 `output->insert`（对 nullopt 解引用）⇒ UB。
 *     该行的正方向（目标为空时返回 items）由 `ensure | 1 2 3` / `ensure | 7` 差分覆盖。
 *  4. map_arr.h `if (!list.has_value()) return ret;` —— 取反或删除后紧跟 `*list` 对空
 *     optional 解引用 ⇒ UB。空输入的正方向改由 `map_arr_nil` 用例差分覆盖（两侧都返回空数组，
 *     行为上无法与"空 vector"区分，因此无法用变异区分）。
 *  5. loop_offset.h `if (idx >= static_cast<double>(len)) return std::nullopt;` —— `idx` 来自
 *     `fmod(x, len)`，恒 `< len`；NaN 在上一行 `!(idx >= 0.0)` 已被拦下 ⇒ 该行不可达，
 *     改成 `>` 恒等。
 *  6. nested_map.h `clear()` 里把回收对象塞进对象池的 `_graves.add(kv.second);` 删掉 ——
 *     池只影响性能：回收的 map 已经被上一行 `kv.second.clear()` 清空，等价于 `ref()` 里新建的
 *     `Inner{}` ⇒ 恒等。（反过来说 `kv.second.clear();` 是必须的，已单独变异。）
 *  7. nested_map.h `ref()` 里 `pooled.has_value() ? *pooled : Inner{}` 只取 `Inner{}`（不复用池）
 *     —— 与第 6 条同源，恒等。
 *  8. nested_map.h `clear()` 的 `if (_map.empty()) return;` 删掉 —— 空循环 ⇒ 恒等。
 *  9. nested_multi_map.h `first()` 的 `if (!ret.has_value() || ret->empty())` 去掉 `|| ret->empty()`
 *     —— `add()` 保证每个内层 vector 至少一个元素，不会出现空 vector ⇒ 恒等；而把
 *     `!ret.has_value() ||` 改成 `ret.has_value() ||` 会在 `ret` 为空时对 `ret->empty()` 解引用 ⇒ UB。
 * 10. filter.h / find.h / fisrt.h 里把 range-for 换成 `begin()/end()` 循环之类的重写 ——
 *     同序同元素，不可观测。
 * 11. find.h 的 `find_value_index`（`const Array*` 版）不属于本 subject 的输入种类，
 *     已有 make_ball_special subject 覆盖。
 * 12. map_no_void.h `if (r.has_value()) ret.push_back(*r);` 去掉 `if` —— 对 nullopt 取 `*r` ⇒ UB；
 *     已改为把 `*r` 换成 `R{}`（保留守卫，变异取值）。
 * 13. intersection 的 `ret.push_back(c1)` 换成 `push_back(c2)` 在只传默认 `equal_to` 时恒等
 *     （能匹配上说明 c1 == c2），因此新增 `intersection_lt`（`std::less<double>`）用例使其可区分，
 *     见下面 d8。
 * 14. nested_map.h `clear()` 末尾的 `_map.clear();` 删掉 —— 恒等：循环里每个内层 map 都已
 *     `kv.second.clear()` 清空，残留下来的空内层 map 让 `get` / `has` / `remove` 全部返回"未命中"，
 *     与"外层键不存在"完全同观；之后 `set(k1,k2,v)` 无论走 `ref()` 的新建分支（从对象池取一个
 *     空 map）还是走"已有 k1"分支（就地写那个空 map），结果都一样。唯一差别是对象池里空 map
 *     的条数，而池是私有的、只影响性能 ⇒ 不可观测。（首次跑出的 1 个幸存者就是它，已按此理由删除。）
 */
export default {
  subject: "collections",
  mutations: [
    // ---- base/graves.h ----
    {
      note: "graves: take 固定读 _l[0]",
      file: "native/lfw/base/graves.h",
      from: "    const std::optional<T> ret = _l[_i];",
      to: "    const std::optional<T> ret = _l[0];",
    },
    {
      note: "graves: take 空槽守卫取反",
      file: "native/lfw/base/graves.h",
      from: "    if (!ret.has_value()) return std::nullopt;",
      to: "    if (ret.has_value()) return std::nullopt;",
    },
    {
      note: "graves: take 不清空槽位",
      file: "native/lfw/base/graves.h",
      from: "    _l[_i] = std::nullopt;\n    ++_i;",
      to: "    ++_i;",
    },
    {
      note: "graves: take 后 _i 前进 2",
      file: "native/lfw/base/graves.h",
      from: "    ++_i;\n    return ret;",
      to: "    _i += 2;\n    return ret;",
    },
    {
      note: "graves: add 扩容时写入空值",
      file: "native/lfw/base/graves.h",
      from: "      _l.push_back(t);",
      to: "      _l.push_back(std::nullopt);",
    },

    // ---- utils/array/make_arr.h ----
    {
      note: "make_arr: 循环上界 <= size",
      file: "native/lfw/utils/array/make_arr.h",
      from: "  for (double i = 0.0; i < size; i += 1.0) ret.push_back(fn(i));",
      to: "  for (double i = 0.0; i <= size; i += 1.0) ret.push_back(fn(i));",
    },
    {
      note: "make_arr: 步长 2",
      file: "native/lfw/utils/array/make_arr.h",
      from: "  for (double i = 0.0; i < size; i += 1.0) ret.push_back(fn(i));",
      to: "  for (double i = 0.0; i < size; i += 2.0) ret.push_back(fn(i));",
    },
    {
      note: "make_arr: 回调固定收 0",
      file: "native/lfw/utils/array/make_arr.h",
      from: "  for (double i = 0.0; i < size; i += 1.0) ret.push_back(fn(i));",
      to: "  for (double i = 0.0; i < size; i += 1.0) ret.push_back(fn(0.0));",
    },
    {
      note: "make_arr: 起始下标 1",
      file: "native/lfw/utils/array/make_arr.h",
      from: "  for (double i = 0.0; i < size; i += 1.0) ret.push_back(fn(i));",
      to: "  for (double i = 1.0; i < size; i += 1.0) ret.push_back(fn(i));",
    },

    // ---- utils/array/loop_arr.h ----
    {
      note: "loop_arr: 下标恒为 0",
      file: "native/lfw/utils/array/loop_arr.h",
      from: "  for (size_t i = 0; i < list.size(); ++i) fn(list[i], i, list);",
      to: "  for (size_t i = 0; i < list.size(); ++i) fn(list[i], 0, list);",
    },
    {
      note: "loop_arr: 第三参传空 vector",
      file: "native/lfw/utils/array/loop_arr.h",
      from: "  for (size_t i = 0; i < list.size(); ++i) fn(list[i], i, list);",
      to: "  for (size_t i = 0; i < list.size(); ++i) fn(list[i], i, std::vector<double>{});",
    },
    {
      note: "loop_arr: 步长 2",
      file: "native/lfw/utils/array/loop_arr.h",
      from: "  for (size_t i = 0; i < list.size(); ++i) fn(list[i], i, list);",
      to: "  for (size_t i = 0; i < list.size(); i += 2) fn(list[i], i, list);",
    },
    {
      note: "loop_arr: 标量版本下标 1",
      file: "native/lfw/utils/array/loop_arr.h",
      from: "  fn(item, 0, std::vector<double>{item});",
      to: "  fn(item, 1, std::vector<double>{item});",
    },
    {
      note: "loop_arr: 标量版本第三参空 vector",
      file: "native/lfw/utils/array/loop_arr.h",
      from: "  fn(item, 0, std::vector<double>{item});",
      to: "  fn(item, 0, std::vector<double>{});",
    },

    // ---- utils/array/map_arr.h ----
    {
      note: "map_arr: 下标恒为 0",
      file: "native/lfw/utils/array/map_arr.h",
      from: "  for (size_t i = 0; i < v.size(); ++i) ret.push_back(fn(v[i], i, v));",
      to: "  for (size_t i = 0; i < v.size(); ++i) ret.push_back(fn(v[i], 0, v));",
    },
    {
      note: "map_arr: 第三参空 vector",
      file: "native/lfw/utils/array/map_arr.h",
      from: "  for (size_t i = 0; i < v.size(); ++i) ret.push_back(fn(v[i], i, v));",
      to: "  for (size_t i = 0; i < v.size(); ++i) ret.push_back(fn(v[i], i, std::vector<double>{}));",
    },
    {
      note: "map_arr: 恒取第 0 个元素",
      file: "native/lfw/utils/array/map_arr.h",
      from: "  for (size_t i = 0; i < v.size(); ++i) ret.push_back(fn(v[i], i, v));",
      to: "  for (size_t i = 0; i < v.size(); ++i) ret.push_back(fn(v[0], i, v));",
    },
    {
      note: "map_arr: 步长 2",
      file: "native/lfw/utils/array/map_arr.h",
      from: "  for (size_t i = 0; i < v.size(); ++i) ret.push_back(fn(v[i], i, v));",
      to: "  for (size_t i = 0; i < v.size(); i += 2) ret.push_back(fn(v[i], i, v));",
    },
    {
      note: "map_arr: 标量版本下标 1",
      file: "native/lfw/utils/array/map_arr.h",
      from: "  ret.push_back(fn(list, 0, std::vector<double>{list}));",
      to: "  ret.push_back(fn(list, 1, std::vector<double>{list}));",
    },
    {
      note: "map_arr: 标量版本元素置 0",
      file: "native/lfw/utils/array/map_arr.h",
      from: "  ret.push_back(fn(list, 0, std::vector<double>{list}));",
      to: "  ret.push_back(fn(0.0, 0, std::vector<double>{list}));",
    },
    {
      note: "map_arr: 标量版本第三参空 vector",
      file: "native/lfw/utils/array/map_arr.h",
      from: "  ret.push_back(fn(list, 0, std::vector<double>{list}));",
      to: "  ret.push_back(fn(list, 0, std::vector<double>{}));",
    },

    // ---- utils/container_help/filter.h ----
    {
      note: "filter: 谓词取反",
      file: "native/lfw/utils/container_help/filter.h",
      from: "    if (p(i)) ret.push_back(i);",
      to: "    if (!p(i)) ret.push_back(i);",
    },
    {
      note: "filter: 收集首元素而不是当前元素",
      file: "native/lfw/utils/container_help/filter.h",
      from: "    if (p(i)) ret.push_back(i);",
      to: "    if (p(i)) ret.push_back(*set.begin());",
    },

    // ---- utils/container_help/find.h ----
    {
      note: "find: 谓词取反",
      file: "native/lfw/utils/container_help/find.h",
      from: "    if (p(v)) return v;",
      to: "    if (!p(v)) return v;",
    },
    {
      note: "find: 未命中返回 0 而不是空",
      file: "native/lfw/utils/container_help/find.h",
      from: "  for (const auto& v : set) {\n    if (p(v)) return v;\n  }\n  return std::nullopt;",
      to: "  for (const auto& v : set) {\n    if (p(v)) return v;\n  }\n  return typename C::value_type{};",
    },
    {
      note: "find_last: 不检查下标 0",
      file: "native/lfw/utils/container_help/find.h",
      from: "  for (size_t i = arr.size(); i > 0; --i) {",
      to: "  for (size_t i = arr.size(); i > 1; --i) {",
    },
    {
      note: "find_last: 命中后返回首元素",
      file: "native/lfw/utils/container_help/find.h",
      from: "    if (p(arr[i - 1])) return arr[i - 1];",
      to: "    if (p(arr[i - 1])) return arr[0];",
    },
    {
      note: "find_last: 谓词取反",
      file: "native/lfw/utils/container_help/find.h",
      from: "    if (p(arr[i - 1])) return arr[i - 1];",
      to: "    if (!p(arr[i - 1])) return arr[i - 1];",
    },
    {
      note: "intersection: 谓词取反",
      file: "native/lfw/utils/container_help/find.h",
      from: "      if (fn(c1, c2)) {",
      to: "      if (!fn(c1, c2)) {",
    },
    {
      note: "intersection: 不加 break（重复命中会重复收集）",
      file: "native/lfw/utils/container_help/find.h",
      from: "        ret.push_back(c1);\n        break;",
      to: "        ret.push_back(c1);",
    },
    {
      note: "intersection: 收集 c2 而不是 c1",
      file: "native/lfw/utils/container_help/find.h",
      from: "        ret.push_back(c1);",
      to: "        ret.push_back(c2);",
    },

    // ---- utils/container_help/fisrt.h ----
    {
      note: "fisrt(1 参): 空容器返回 0 而不是空",
      file: "native/lfw/utils/container_help/fisrt.h",
      from: "  for (const auto& item : iterable) return item;\n  return std::nullopt;",
      to: "  for (const auto& item : iterable) return item;\n  return typename C::value_type{};",
    },
    {
      note: "fisrt(2 参): 命中守卫取反",
      file: "native/lfw/utils/container_help/fisrt.h",
      from: "    R r = p(item);\n    if (r.has_value()) return r;",
      to: "    R r = p(item);\n    if (!r.has_value()) return r;",
    },
    {
      note: "last(1 参): 返回首元素",
      file: "native/lfw/utils/container_help/fisrt.h",
      from: "  for (const auto& item : iterable) ret = item;",
      to: "  for (const auto& item : iterable) ret = *iterable.begin();",
    },
    {
      note: "last(2 参): 不检查下标 0",
      file: "native/lfw/utils/container_help/fisrt.h",
      from: "  for (size_t i = arr.size(); i > 0; --i) {",
      to: "  for (size_t i = arr.size(); i > 1; --i) {",
    },
    {
      note: "last(2 参): 每轮都谓词首元素",
      file: "native/lfw/utils/container_help/fisrt.h",
      from: "    R r = p(arr[i - 1]);\n    if (r.has_value()) return r;",
      to: "    R r = p(arr[0]);\n    if (r.has_value()) return r;",
    },
    {
      note: "last(2 参): 命中守卫取反",
      file: "native/lfw/utils/container_help/fisrt.h",
      from: "    R r = p(arr[i - 1]);\n    if (r.has_value()) return r;",
      to: "    R r = p(arr[i - 1]);\n    if (!r.has_value()) return r;",
    },

    // ---- utils/container_help/ensure.h ----
    {
      note: "ensure(模板): 追加到头部而不是尾部",
      file: "native/lfw/utils/container_help/ensure.h",
      from: "  output->insert(output->end(), items.begin(), items.end());",
      to: "  output->insert(output->begin(), items.begin(), items.end());",
    },
    {
      note: "ensure(模板): 返回入参而不是合并结果",
      file: "native/lfw/utils/container_help/ensure.h",
      from: "  return *output;",
      to: "  return items;",
    },
    {
      note: "ensure(Value&): 已有数组时返回空 Value",
      file: "native/lfw/utils/container_help/ensure.h",
      from: "    return output;",
      to: "    return Value();",
    },
    {
      note: "ensure(Value&): 追加时压入空值",
      file: "native/lfw/utils/container_help/ensure.h",
      from: "    for (const Value& v : items) a->push_back(v);",
      to: "    for (const Value& v : items) a->push_back(Value());",
    },
    {
      note: "ensure(Value&): 新建数组时丢掉 items",
      file: "native/lfw/utils/container_help/ensure.h",
      from: "  output = Value(std::make_shared<Array>(fresh));",
      to: "  output = Value(std::make_shared<Array>());",
    },
    {
      note: "ensure(Value&, Item): 转发时丢掉单个 item",
      file: "native/lfw/utils/container_help/ensure.h",
      from: "inline Value ensure(Value& output, const Value& item) { return ensure(output, std::vector<Value>{item}); }",
      to: "inline Value ensure(Value& output, const Value& item) { return ensure(output, std::vector<Value>{}); }",
    },

    // ---- utils/container_help/loop_offset.h ----
    {
      note: "loop_offset: idx 初值 0（找不到 current 时按 0 处理）",
      file: "native/lfw/utils/container_help/loop_offset.h",
      from: "  double idx = -1.0;",
      to: "  double idx = 0.0;",
    },
    {
      note: "loop_offset: 负偏移方向取反",
      file: "native/lfw/utils/container_help/loop_offset.h",
      from: "    idx = std::fmod(static_cast<double>(len) + idx + offset, static_cast<double>(len));",
      to: "    idx = std::fmod(static_cast<double>(len) + idx - offset, static_cast<double>(len));",
    },
    {
      note: "loop_offset: 正负分支互换",
      file: "native/lfw/utils/container_help/loop_offset.h",
      from: "  if (offset > 0.0) {",
      to: "  if (offset <= 0.0) {",
    },
    {
      note: "loop_offset: offset 为 0 走正分支",
      file: "native/lfw/utils/container_help/loop_offset.h",
      from: "  if (offset > 0.0) {",
      to: "  if (offset >= 0.0) {",
    },
    {
      note: "loop_offset: 整数守卫取反",
      file: "native/lfw/utils/container_help/loop_offset.h",
      from: "  if (idx != std::floor(idx)) return std::nullopt;",
      to: "  if (idx == std::floor(idx)) return std::nullopt;",
    },
    {
      note: "loop_offset: 恒取第 0 个元素",
      file: "native/lfw/utils/container_help/loop_offset.h",
      from: "  return list[static_cast<size_t>(idx)];",
      to: "  return list[0];",
    },
    {
      note: "loop_offset: 空表守卫取反",
      file: "native/lfw/utils/container_help/loop_offset.h",
      from: "  if (len == 0) return std::nullopt;",
      to: "  if (len != 0) return std::nullopt;",
    },
    {
      note: "loop_offset: 非负守卫取反",
      file: "native/lfw/utils/container_help/loop_offset.h",
      from: "  if (!(idx >= 0.0)) return std::nullopt;",
      to: "  if (idx <= 0.0) return std::nullopt;",
    },

    // ---- utils/container_help/map_no_void.h ----
    {
      note: "map_no_void: 收集 0 而不是映射结果",
      file: "native/lfw/utils/container_help/map_no_void.h",
      from: "    if (r.has_value()) ret.push_back(*r);",
      to: "    if (r.has_value()) ret.push_back(R{});",
    },

    // ---- utils/container_help/nested_map.h ----
    {
      note: "nested_map::get: 外层用 k2 查表",
      file: "native/lfw/utils/container_help/nested_map.h",
      from: "    const auto it = _map.find(k1);\n    if (it == _map.end()) return std::nullopt;",
      to: "    const auto it = _map.find(k2);\n    if (it == _map.end()) return std::nullopt;",
    },
    {
      note: "nested_map::get: 内层用 k1 查表",
      file: "native/lfw/utils/container_help/nested_map.h",
      from: "    const auto it2 = it->second.find(k2);",
      to: "    const auto it2 = it->second.find(k1);",
    },
    {
      note: "nested_map::get: 返回内层首项",
      file: "native/lfw/utils/container_help/nested_map.h",
      from: "    return it2->second;",
      to: "    return it->second.begin()->second;",
    },
    {
      note: "nested_map::has: 存在性判断取反",
      file: "native/lfw/utils/container_help/nested_map.h",
      from: "    return it != _map.end() && it->second.find(k2) != it->second.end();",
      to: "    return it != _map.end() && it->second.find(k2) == it->second.end();",
    },
    {
      note: "nested_map::set: 键写反",
      file: "native/lfw/utils/container_help/nested_map.h",
      from: "  void set(const K1& k1, const K2& k2, const V& value) { ref(k1, k2) = value; }",
      to: "  void set(const K1& k1, const K2& k2, const V& value) { ref(k2, k1) = value; }",
    },
    {
      note: "nested_map::remove: 删除结果取反",
      file: "native/lfw/utils/container_help/nested_map.h",
      from: "    return it != _map.end() && it->second.erase(k2) != 0;",
      to: "    return it != _map.end() && it->second.erase(k2) == 0;",
    },
    {
      note: "nested_map::remove: 内层删错键",
      file: "native/lfw/utils/container_help/nested_map.h",
      from: "    return it != _map.end() && it->second.erase(k2) != 0;",
      to: "    return it != _map.end() && it->second.erase(k1) != 0;",
    },
    {
      note: "nested_map::clear: 回收前不清空内层 map",
      file: "native/lfw/utils/container_help/nested_map.h",
      from: "      kv.second.clear();\n      _graves.add(kv.second);",
      to: "      _graves.add(kv.second);",
    },

    // ---- utils/container_help/nested_multi_map.h ----
    {
      note: "nested_multi_map::add: 键写反",
      file: "native/lfw/utils/container_help/nested_multi_map.h",
      from: "    _map.ref(k1, k2).push_back(value);",
      to: "    _map.ref(k2, k1).push_back(value);",
    },
    {
      note: "nested_multi_map::add: 存入 value + 1",
      file: "native/lfw/utils/container_help/nested_multi_map.h",
      from: "    _map.ref(k1, k2).push_back(value);",
      to: "    _map.ref(k1, k2).push_back(value + 1.0);",
    },
    {
      note: "nested_multi_map::first: 取最后一项",
      file: "native/lfw/utils/container_help/nested_multi_map.h",
      from: "    return (*ret)[0];",
      to: "    return (*ret).back();",
    },
    {
      note: "nested_multi_map::has: 键查反",
      file: "native/lfw/utils/container_help/nested_multi_map.h",
      from: "  bool has(const K1& k1, const K2& k2) const { return _map.has(k1, k2); }",
      to: "  bool has(const K1& k1, const K2& k2) const { return _map.has(k2, k1); }",
    },
    {
      note: "nested_multi_map::collect: 命中守卫取反",
      file: "native/lfw/utils/container_help/nested_multi_map.h",
      from: "    if (!ret.has_value()) return out;",
      to: "    if (ret.has_value()) return out;",
    },
    {
      note: "nested_multi_map::collect: 倒序收集",
      file: "native/lfw/utils/container_help/nested_multi_map.h",
      from: "    for (const auto& v : *ret) out.push_back(v);",
      to: "    for (const auto& v : *ret) out.insert(out.begin(), v);",
    },
    {
      note: "nested_multi_map::remove: 键删反",
      file: "native/lfw/utils/container_help/nested_multi_map.h",
      from: "  bool remove(const K1& k1, const K2& k2) { return _map.remove(k1, k2); }",
      to: "  bool remove(const K1& k1, const K2& k2) { return _map.remove(k2, k1); }",
    },
    {
      note: "nested_multi_map::clear: 不清空",
      file: "native/lfw/utils/container_help/nested_multi_map.h",
      from: "  void clear() { _map.clear(); }",
      to: "  void clear() {}",
    },
  ],
};
