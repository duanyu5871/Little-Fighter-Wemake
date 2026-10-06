// `World::step` 的碰撞配对（`native/lfw/world.cpp`）与 `WorldCollisionHost` 的 Env 接线
// （`native/lfw/world_collision.cpp`）的变异档。用例是 `cases/world/collision.txt`
// （一个世界、三组实体：x=0 单向、x=3000 双向、x=6000 `hit_flag` 不匹配）。
//
// 有意不覆盖（不可观察 / 按构造等价 / 要等「dump 扩展 + 更多 handler 路径用例」那一刀）：
//
// 【配对循环】
//   * `if (frame_is_gone(*b)) continue;`：三组实体都不是幽灵（帧 id 都是 `"0"`）⇒ 够不着。
//   * z 轴的 AABB 剔除（`a_max_z < b->aabb_min_z || b->aabb_max_z < a_min_z`）：这一刀的实体
//     AABB 全是 0 ⇒ `0 < 0` 两侧都假 ⇒ `||` 与 `&&` 等价。
//   * `collision_host().reset_collisions()`：`acquire_collision` 每次给新对象 ⇒ 清不清池子
//     一样（这一句只是防内存涨）。
//   * `c2 = collision_get(*b, *a)` 写成 `collision_get(*a, *b)`：两条碰撞的 `id` 都是攻击方 id
//     （`rest == 0`），`add_collision` 对同 id 同距离的记录会直接返回 ⇒ 观察不到。
//   * `p1` / `p2` 的 `?? Infinity`：三组实体的类型（8）在 `ENTITY_PRIORITY_MAP` 里**同进同出**
//     ⇒ 两侧要么都有值、要么都是 `undefined`；后者经这条规则都变成 `Infinity` ⇒ 与「不补
//     Infinity」在 `<=` 下等价（`undefined <= undefined` 与 `Infinity <= Infinity` 都假，
//     而只有一侧为 `undefined` 时才分岔 —— 那要一组跨类型的实体，留给下一刀）。
//
// 【宿主缝】这一刀的用例只观测三件东西：`pc=`（配对比次）、`col=`（本帧加入的碰撞数）、
// 受击方 `hp`。所以只保留「改了就会让这三者变」的缝；其余一律等观测量扩展：
//   * `bot_ignore`（只在 Pick 系的 itr kind 上读）、`is_bot_ctrl`、`team` / `emitter` /
//     `spawn_time`（用例里三者的组合本来就放行）、`catcher_hurtable`（`has_catcher` 为假时不读）、
//     `marks_group_attack` / `itr_prefabs` / `bear_wpoint_attacking`（`rest` 仍是 0）、
//     `px/py/pz` 与 `data_id`（只进快照与 `m_distance`，用例里不影响判定）；
//   * `get_bounding` 的每个字段：用例的两颗判定框**完全重合**（左右上下远近各自相等）⇒ 交换
//     左右一类的改动两侧同向，不产生差异 —— 要锁住得加一组「部分重叠」的实体；
//   * `dataset` / `victim_get_v_rest` / `attacker_is_ally` / `new_id` / `log` / `dev` /
//     `find_entity` / `find_object_data` / `load_handlers`：这一条路径上要么不读（`rest` 为 0、
//     `dev` 为假），要么结果与改动无关；
//   * `KeeperEnv` 的 `attacker_state` / `victim_state`（配置里没有 `a_state` / `v_state` 过滤）、
//     `ball_frozen`（`handle_ball_frozen` 是给气功波的）、`run_action`（用例的 `itr` / `bdy`
//     没有 `actions`）、`victim_push_collided` / `attacker_push_collision`（写进实体的
//     `collided_list` / `collision_list`，dump 里没有）、`victim_play_sound` 与 `victim_data`
//     （台面的 `play_sound` 不记日志）；
//   * `HandlersEnv` 的 12 条（`victim_add_v_rest` / `attacker_pick_victim` / `attacker_set_arest`
//     是 `rest` / `Pick` 那一支的；`attacker_itr_motionless` / `attacker_set_motionless` /
//     `victim_set_shaking` 的结果 dump 里没有；`buff_*` 要 `magic_flute` 或 `Electrify` 才走）；
//   * `ActionEnv` / `Handlers2-4Env` 里没被这条路径调用的那些（`Handlers2Env` 只走了
//     `find_entity` / `hp_recoverability` / `summary_apply_damage` / `is_fighter` / `calc_velocity`
//     / `buff_env`，其中 `hp_recoverability` 与 `calc_velocity` 的结果进了 `hp_r` / 速度，
//     前者 dump 里没有、后者被 `handle_fall` 的 `set_velocity` 吃掉 ⇒ 也够不着）；
//   * 三个视图的转发（`EntityHandlerView::hp/set_hp/...`）：改坏了都会让 `hp` 变，但那样的
//     `from` 串在视图里出现多次（三个类各一份），单点替换会打错类 ⇒ 留给视图自己的用例。
//   * `step: 宿主不再置当前那一对（collision_get 的包装器）`：**实测**（手工去掉两行 + `build`
//     + 对比输出：零差异）—— `collision_get` 期间的 `collision_test` 只在这条缝上读
//     `attacker_is_ally`，而用例里没有任何一对能在这一步分岔（场景 4 试过「同队」：
//     `went G team t1` / `went H team t1`，但两者的判定在此之前就没走到那一行）。要锁住它得先
//     找到一对「只在 ally 位上分岔」的实体（`is_ally` 的专属用例，或 `LFW` 那一刀的真玩家）。
export default {
  subject: "world",
  cases: ["collision"],
  mutations: [
    // ───────────────────────── 配对循环 ─────────────────────────
    {
      note: "step: AABB 的 x 方向用 <= 提前 break（相等的 a_max_x 也断）",
      file: "native/lfw/world.cpp",
      from: "      if (a_max_x < b->aabb_min_x) break;",
      to: "      if (a_max_x <= b->aabb_min_x) break;",
    },
    {
      note: "step: pairs_compared 加二",
      file: "native/lfw/world.cpp",
      from: "      pairs_compared += 1.0;",
      to: "      pairs_compared += 2.0;",
    },
    {
      note: "step: c1 取反方向（(b,a) 而不是 (a,b)）",
      file: "native/lfw/world.cpp",
      from: "      collision::Collision* const c1 = collision_host().collision_get(*a, *b);",
      to: "      collision::Collision* const c1 = collision_host().collision_get(*b, *a);",
    },
    {
      note: "step: 不 add c1",
      file: "native/lfw/world.cpp",
      from: "      if (c1 != nullptr && le(p1, p2)) add_collision(*c1);",
      to: "      if (c1 == nullptr && le(p1, p2)) add_collision(*c1);",
    },
    {
      note: "step: 不 add c2",
      file: "native/lfw/world.cpp",
      from: "      if (c2 != nullptr && le(p2, p1)) add_collision(*c2);",
      to: "      if (c2 == nullptr && le(p2, p1)) add_collision(*c2);",
    },
    {
      note: "step: c2 也用 (a,b)（双向那一组只剩一条）",
      file: "native/lfw/world.cpp",
      from: "      collision::Collision* const c2 = collision_host().collision_get(*b, *a);",
      to: "      collision::Collision* const c2 = collision_host().collision_get(*a, *b);",
    },
    // ─────────────────────── `CollisionActor` 投影 ───────────────────────
    {
      note: "actor_of: data_type 恒 0（hit_flag 判定全灭）",
      file: "native/lfw/world_collision.cpp",
      from: "  a.data_type = to_number(field_or(e.data(), u\"type\"));",
      to: "  a.data_type = 0;",
    },
    {
      note: "actor_of: frame 恒 undefined（itr / bdy 都取不到）",
      file: "native/lfw/world_collision.cpp",
      from: "  a.frame = e.frame;",
      to: "  a.frame = Value();",
    },
    {
      note: "actor_of: dropping 恒真",
      file: "native/lfw/world_collision.cpp",
      from: "  a.dropping = e.dropping;",
      to: "  a.dropping = true;",
    },
    {
      note: "actor_of: arest 恒 1（无 rest 时直接拦掉）",
      file: "native/lfw/world_collision.cpp",
      from: "  a.arest = Value(e.arest());",
      to: "  a.arest = Value(1.0);",
    },
    {
      note: "actor_of: invulnerable 恒 1",
      file: "native/lfw/world_collision.cpp",
      from: "  a.invulnerable = Value(e.invulnerable());",
      to: "  a.invulnerable = Value(1.0);",
    },
    {
      note: "actor_of: has_catcher 恒真",
      file: "native/lfw/world_collision.cpp",
      from: "  a.has_catcher = e.catcher != nullptr;",
      to: "  a.has_catcher = true;",
    },
    // ─────────────────────── handler 分发 ───────────────────────
    {
      note: "keeper: 丢掉 normal_bdy_normal 这一支",
      file: "native/lfw/world_collision.cpp",
      from: "    } else if (fn == u\"handle_itr_normal_bdy_normal\") {",
      to: "    } else if (false && fn == u\"handle_itr_normal_bdy_normal\") {",
    },
    {
      note: "keeper: normal_bdy_normal 换成分掉血的 handle_rest",
      file: "native/lfw/world_collision.cpp",
      from: "      collision::handle_itr_normal_bdy_normal(c);",
      to: "      collision::handle_rest(c);",
    },
  ],
};
