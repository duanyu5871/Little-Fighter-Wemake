// `cmds/` 家族第二批（4Y：作弊码族 + SPAWN / SET_PUPPET / F8）的变异档。
//
// 用例：`cases/cmds/{cheat,spawn,set_puppet,f8}.txt`（任一锁住即可）。
//
// 有意不覆盖（不可观察 / 不建形）：
//   * 小写命令词（`gim_ink`）走不进 `is_cheat_type` 的**真分支**：`true` 那个方向只会多写
//     一个没人读的 dataset 键（`gim_ink`），既无音效也无回调；反方向的 `return` 变体已被
//     用例全线杀死。`!cmd.has_value()` 这一半也不可达（能派发到本 handler 就一定
//     `positionals[0] == words[0]`）。
//   * `cheat.sound` 缺失分支（`if (truthy(sound))`）：三条作弊码的 `CheatInfos` 都带 sound
//     ⇒ 假值路径不可达。
//   * `SPAWN` 的 `y ?? e.position.y` 缺省分支：`create_entity*` 出的新实体 `position.y` 恒 0
//     ⇒ 与 `?? 0` 同观（缺省值本身不建形）。
//   * `f.data !== data` 的「总是 transform」方向：同对象 transform 无日志、无 dump 差异
//     （`did` 不变）；「永不 transform」方向已杀。
//   * `cmd_cheat.cpp` 的三条 help 文案：`helps` 表没有读取口（不建形）。
export default {
  subject: "cmds",
  cases: ["cheat", "spawn", "set_puppet", "f8", "fs", "kill", "puppet", "handle", "parse"],
  mutations: [
    // ------------------------------------------------------------ cheat_code_handler
    {
      note: "作弊：整条守卫短路返回（全都别想生效）",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  if (!cmd.has_value() || !defines::is_cheat_type(*cmd)) return;`,
      to: `  if (true) return;`,
    },
    {
      note: "作弊：enabled 不看 num 的真值（有参就算 1）",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  const double enabled = (num.has_value() && truthy(Value(*num))) ? 1.0 : 0.0;`,
      to: `  const double enabled = (num.has_value()) ? 1.0 : 0.0;`,
    },
    {
      note: "作弊：enabled 的 1/0 互换",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  const double enabled = (num.has_value() && truthy(Value(*num))) ? 1.0 : 0.0;`,
      to: `  const double enabled = (num.has_value() && truthy(Value(*num))) ? 0.0 : 1.0;`,
    },
    {
      note: "作弊：先写 dataset 再读 prev（prev 恒等于 enabled ⇒ 永远早退）",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  const Value prev = world.dataset.get(*cmd);
  const std::optional<double> num = ctx.num(1);
  const double enabled = (num.has_value() && truthy(Value(*num))) ? 1.0 : 0.0;
  world.dataset.set(*cmd, Value(enabled));
  if (equals(prev, Value(enabled))) return;`,
      to: `  const std::optional<double> num = ctx.num(1);
  const double enabled = (num.has_value() && truthy(Value(*num))) ? 1.0 : 0.0;
  world.dataset.set(*cmd, Value(enabled));
  const Value prev = world.dataset.get(*cmd);
  if (equals(prev, Value(enabled))) return;`,
    },
    {
      note: "作弊：不回写 dataset（后续 prev 全错）",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  world.dataset.set(*cmd, Value(enabled));
  if (equals(prev, Value(enabled))) return;`,
      to: `  if (equals(prev, Value(enabled))) return;`,
    },
    {
      note: "作弊：回写恒为 0",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  world.dataset.set(*cmd, Value(enabled));`,
      to: `  world.dataset.set(*cmd, Value(0.0));`,
    },
    {
      note: "作弊：prev == enabled 改成严格比较",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  if (equals(prev, Value(enabled))) return;`,
      to: `  if (strict_equals(prev, Value(enabled))) return;`,
    },
    {
      note: "作弊：查错表（Defines.CheatInfos → Defines.Sounds）",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  const Value* const infos = defines::find(u"Defines.CheatInfos");`,
      to: `  const Value* const infos = defines::find(u"Defines.Sounds");`,
    },
    {
      note: "作弊：拿错条目的 info（cmd → HERO_FT）",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  const Value cheat = infos != nullptr ? field_or_key(*infos, *cmd) : Value();`,
      to: `  const Value cheat = infos != nullptr ? field_or_key(*infos, u"HERO_FT") : Value();`,
    },
    {
      note: "作弊：播的是命令名不是 sound 路径",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  if (truthy(sound)) world.lfw().sounds_play_with_load(sound);`,
      to: `  if (truthy(sound)) world.lfw().sounds_play_with_load(Value(*cmd));`,
    },
    {
      note: "作弊：回调参数恒 true",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  world.lfw().cheat_changed(*cmd, enabled != 0.0);`,
      to: `  world.lfw().cheat_changed(*cmd, true);`,
    },
    {
      note: "作弊：回调命令名换成 CFG 里的（cmd → LF2_NET）",
      file: "native/lfw/cmds/cheat_code_handler.cpp",
      from: `  world.lfw().cheat_changed(*cmd, enabled != 0.0);`,
      to: `  world.lfw().cheat_changed(u"LF2_NET", enabled != 0.0);`,
    },
    // ------------------------------------------------------------ cmd_spawn
    {
      note: "SPAWN：count 缺省 1 → 2",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `  const double count_value = count.has_value() ? *count : 1.0;`,
      to: `  const double count_value = count.has_value() ? *count : 2.0;`,
    },
    {
      note: "SPAWN：count 缺省 1 → 0（啥都不生成）",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `  const double count_value = count.has_value() ? *count : 1.0;`,
      to: `  const double count_value = count.has_value() ? *count : 0.0;`,
    },
    {
      note: "SPAWN：循环条件改成 <=（count=2 生成 3 个）",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `  for (double i = 0; i < count_value; i += 1) {`,
      to: `  for (double i = 0; i <= count_value; i += 1) {`,
    },
    {
      note: "SPAWN：--oid 空串不走告警（漏掉 empty 判定）",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `  if (!oid.has_value() || oid->empty()) {`,
      to: `  if (!oid.has_value()) {`,
    },
    {
      note: "SPAWN：缺参告警文案变体",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    world.lfw().warn(u"SPAWN failed, must \\"SPAWN --oid=xxx [--team=...] [--count=n]\\", got: " +`,
      to: `    world.lfw().warn(u"SPAWN failed, usage: SPAWN --oid=xxx, got: " +`,
    },
    {
      note: "SPAWN：not found 告警不带 oid",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    world.lfw().warn(u"SPAWN failed, oid not found: " + *oid);`,
      to: `    world.lfw().warn(u"SPAWN failed, oid not found.");`,
    },
    {
      note: "SPAWN：--player_id= 空串也走玩家分支",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `  const bool use_player = player_id.has_value() && !player_id->empty();`,
      to: `  const bool use_player = player_id.has_value();`,
    },
    {
      note: "SPAWN：玩家分支参数写错（传空 id）",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    Entity* const e = use_player ? world.lfw().create_entity_with_player(*player_id, world, data)
                                 : world.lfw().create_entity_with_bot(u"", world, data);`,
      to: `    Entity* const e = use_player ? world.lfw().create_entity_with_player(u"", world, data)
                                 : world.lfw().create_entity_with_bot(u"", world, data);`,
    },
    {
      note: "SPAWN：team 空串也照用（不走 new_team）",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    e->set_team(team.has_value() && !team->empty() ? *team : world.lfw().new_team());`,
      to: `    e->set_team(team.has_value() ? *team : world.lfw().new_team());`,
    },
    {
      note: "SPAWN：name 空串也照写（漏 truthy 判定）",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    if (name.has_value() && !name->empty()) e->set_name(Value(*name));`,
      to: `    if (name.has_value()) e->set_name(Value(*name));`,
    },
    {
      note: "SPAWN：facing 只认 1（丢 -1）",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    if (facing.has_value() && (*facing == 1.0 || *facing == -1.0)) e->facing = *facing;`,
      to: `    if (facing.has_value() && (*facing == 1.0)) e->facing = *facing;`,
    },
    {
      note: "SPAWN：facing 写死 1",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    if (facing.has_value() && (*facing == 1.0 || *facing == -1.0)) e->facing = *facing;`,
      to: `    if (facing.has_value() && (*facing == 1.0 || *facing == -1.0)) e->facing = 1.0;`,
    },
    {
      note: "SPAWN：给了 --x 也不设位置（永远随机落位）",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    if (x.has_value()) {
      e->position.set(*x, y.has_value() ? *y : e->position.y,
                      z.has_value() ? *z : e->position.z);
    } else {
      world.lfw().random_entity_info(*e);
    }`,
      to: `    if (false) {
      e->position.set(*x, y.has_value() ? *y : e->position.y,
                      z.has_value() ? *z : e->position.z);
    } else {
      world.lfw().random_entity_info(*e);
    }`,
    },
    {
      note: "SPAWN：坐标三轴顺序写乱（x,y,z → y,z,x）",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `      e->position.set(*x, y.has_value() ? *y : e->position.y,
                      z.has_value() ? *z : e->position.z);`,
      to: `      e->position.set(y.has_value() ? *y : e->position.y, z.has_value() ? *z : e->position.z,
                      *x);`,
    },
    {
      note: "SPAWN：不写 hp",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    if (hp.has_value()) e->set_hp(*hp);
    if (mp.has_value()) e->set_mp(*mp);`,
      to: `    if (mp.has_value()) e->set_mp(*mp);`,
    },
    {
      note: "SPAWN：不写 mp",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    if (hp.has_value()) e->set_hp(*hp);
    if (mp.has_value()) e->set_mp(*mp);`,
      to: `    if (hp.has_value()) e->set_hp(*hp);`,
    },
    {
      note: "SPAWN：不 attach（不进世界）",
      file: "native/lfw/cmds/cmd_spawn.cpp",
      from: `    e->attach();
  }
}`,
      to: `  }
}`,
    },
    // ------------------------------------------------------------ cmd_set_puppet
    {
      note: "SET_PUPPET：--player_id 空串漏判",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `  if (!player_id.has_value() || player_id->empty() || !oid.has_value() || oid->empty()) {`,
      to: `  if (!player_id.has_value() || !oid.has_value() || oid->empty()) {`,
    },
    {
      note: "SET_PUPPET：--oid 空串漏判",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `  if (!player_id.has_value() || player_id->empty() || !oid.has_value() || oid->empty()) {`,
      to: `  if (!player_id.has_value() || player_id->empty() || !oid.has_value()) {`,
    },
    {
      note: "SET_PUPPET：player not found 文案不带 id",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `    world.lfw().warn(u"SET_PUPPET failed, player not found: " + *player_id);`,
      to: `    world.lfw().warn(u"SET_PUPPET failed, player not found.");`,
    },
    {
      note: "SET_PUPPET：fighter oid not found 文案不带 oid",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `    world.lfw().warn(u"SET_PUPPET failed, fighter oid not found: " + *oid);`,
      to: `    world.lfw().warn(u"SET_PUPPET failed, fighter oid not found.");`,
    },
    {
      note: "SET_PUPPET：新傀儡落点 y 用 0（不是 450）",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `    f->set_position(Value(middle.x), Value(450.0), Value(middle.z));`,
      to: `    f->set_position(Value(middle.x), Value(0.0), Value(middle.z));`,
    },
    {
      note: "SET_PUPPET：落点 x/z 互换",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `    f->set_position(Value(middle.x), Value(450.0), Value(middle.z));`,
      to: `    f->set_position(Value(middle.z), Value(450.0), Value(middle.x));`,
    },
    {
      note: "SET_PUPPET：不写玩家名",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `  f->set_name(player_info->name());`,
      to: `  (void)player_info;`,
    },
    {
      note: "SET_PUPPET：team 空串也照写",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `  if (team.has_value() && !team->empty()) f->set_team(*team);`,
      to: `  if (team.has_value()) f->set_team(*team);`,
    },
    {
      note: "SET_PUPPET：team 干脆不写",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `  if (team.has_value() && !team->empty()) f->set_team(*team);`,
      to: `  (void)team;`,
    },
    {
      note: "SET_PUPPET：f.data 判定恒 false（永不 transform）",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `  if (as_object(f->data()) != as_object(data)) f->transform(data);`,
      to: `  if (false) f->transform(data);`,
    },
    {
      note: "SET_PUPPET：data 判定的另一半（as_object 只留一边）",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `  if (as_object(f->data()) != as_object(data)) f->transform(data);`,
      to: `  if (as_object(f->data()) != nullptr) f->transform(data);`,
    },
    {
      note: "SET_PUPPET：换控判定里丢了 player_id 比较",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `  if (ctrl == nullptr || !ctrl->is_human() || ctrl->player_id != *player_id) {`,
      to: `  if (ctrl == nullptr || !ctrl->is_human()) {`,
    },
    {
      note: "SET_PUPPET：is_human 判定丢弃（非人控也不换控）",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `  if (ctrl == nullptr || !ctrl->is_human() || ctrl->player_id != *player_id) {`,
      to: `  if (false) {`,
    },
    {
      note: "SET_PUPPET：换控时传错玩家 id",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `    f->set_ctrl(world.lfw().acquire_local_ctrl(*player_id, *f));`,
      to: `    f->set_ctrl(world.lfw().acquire_local_ctrl(u"", *f));`,
    },
    {
      note: "SET_PUPPET：不 attach",
      file: "native/lfw/cmds/cmd_set_puppet.cpp",
      from: `  f->attach();
}`,
      to: `}`,
    },
    // ------------------------------------------------------------ cmd_f8
    {
      note: "F8：两条守卫次序互换",
      file: "native/lfw/cmds/cmd_f.cpp",
      from: `  if (world.fn_locked()) {
    world.lfw().debug(u"F8 failed, Fn Locked.");
    return;
  }
  if (world.stage_limit()) {
    world.lfw().debug(u"F8 failed, Stage Limited.");
    return;
  }`,
      to: `  if (world.stage_limit()) {
    world.lfw().debug(u"F8 failed, Stage Limited.");
    return;
  }
  if (world.fn_locked()) {
    world.lfw().debug(u"F8 failed, Fn Locked.");
    return;
  }`,
    },
    {
      note: "F8：Fn Locked 守卫失效",
      file: "native/lfw/cmds/cmd_f.cpp",
      from: `  if (world.fn_locked()) {
    world.lfw().debug(u"F8 failed, Fn Locked.");
    return;
  }
  if (world.stage_limit()) {`,
      to: `  if (false) {
    world.lfw().debug(u"F8 failed, Fn Locked.");
    return;
  }
  if (world.stage_limit()) {`,
    },
    {
      note: "F8：Stage Limited 守卫失效",
      file: "native/lfw/cmds/cmd_f.cpp",
      from: `  if (world.stage_limit()) {
    world.lfw().debug(u"F8 failed, Stage Limited.");
    return;
  }
  world.add_count(u"f8", 1.0);`,
      to: `  if (false) {
    world.lfw().debug(u"F8 failed, Stage Limited.");
    return;
  }
  world.add_count(u"f8", 1.0);`,
    },
    {
      note: "F8：计数键写错（f8 → f9）",
      file: "native/lfw/cmds/cmd_f.cpp",
      from: `  world.add_count(u"f8", 1.0);`,
      to: `  world.add_count(u"f9", 1.0);`,
    },
    {
      note: "F8：is_stage 恒 true",
      file: "native/lfw/cmds/cmd_f.cpp",
      from: `  const bool is_stage = world.stage()->id() != std::u16string(u"VOID_STAGE");`,
      to: `  const bool is_stage = true;`,
    },
    {
      note: "F8：is_stage 比较反向",
      file: "native/lfw/cmds/cmd_f.cpp",
      from: `  const bool is_stage = world.stage()->id() != std::u16string(u"VOID_STAGE");`,
      to: `  const bool is_stage = world.stage()->id() == std::u16string(u"VOID_STAGE");`,
    },
    {
      note: "F8：两个武器组互换",
      file: "native/lfw/cmds/cmd_f.cpp",
      from: `      is_stage ? entity_group::kStageWeapon : entity_group::kVsWeapon)));`,
      to: `      is_stage ? entity_group::kVsWeapon : entity_group::kStageWeapon)));`,
    },
    {
      note: "F8：entities_add 的数量参数写死 2",
      file: "native/lfw/cmds/cmd_f.cpp",
      from: `  for (size_t i = 0; i < list->size(); ++i) world.lfw().entities_add(list->at(i), 1.0);`,
      to: `  for (size_t i = 0; i < list->size(); ++i) world.lfw().entities_add(list->at(i), 2.0);`,
    },
    {
      note: "F8：武器列表整个丢掉",
      file: "native/lfw/cmds/cmd_f.cpp",
      from: `  const Array* const list = as_array(weapon_datas);
  if (list == nullptr) return;
  for (size_t i = 0; i < list->size(); ++i) world.lfw().entities_add(list->at(i), 1.0);`,
      to: `  const Array* const list = as_array(weapon_datas);
  if (list == nullptr) return;`,
    },
    // ------------------------------------------------------------ 注册表
    {
      note: "注册表：F8 登记到 F9 的键上",
      file: "native/lfw/cmds/cmds.cpp",
      from: `  put(cmd::kF8, cmd_f8_help(), cmd_f8);`,
      to: `  put(cmd::kF9, cmd_f8_help(), cmd_f8);`,
    },
    {
      note: "注册表：GIM_INK 键换成 HERO_FT",
      file: "native/lfw/cmds/cmds.cpp",
      from: `  put(cmd::kGIM_INK, cmd_gim_ink_help(), cheat_code_handler);`,
      to: `  put(cmd::kHERO_FT, cmd_gim_ink_help(), cheat_code_handler);`,
    },
    {
      note: "注册表：作弊码的 handler 换成 F9",
      file: "native/lfw/cmds/cmds.cpp",
      from: `  put(cmd::kLF2_NET, cmd_lf2_net_help(), cheat_code_handler);`,
      to: `  put(cmd::kLF2_NET, cmd_lf2_net_help(), cmd_f9);`,
    },
    {
      note: "注册表：SPAWN 没登记",
      file: "native/lfw/cmds/cmds.cpp",
      from: `  put(cmd::kSPAWN, cmd_spawn_help(), cmd_spawn);`,
      to: `  // SPAWN 未登记`,
    },
    {
      note: "注册表：SET_PUPPET 登记成 SPAWN 的实现",
      file: "native/lfw/cmds/cmds.cpp",
      from: `  put(cmd::kSET_PUPPET, cmd_set_puppet_help(), cmd_set_puppet);`,
      to: `  put(cmd::kSET_PUPPET, cmd_set_puppet_help(), cmd_spawn);`,
    },
  ],
};
