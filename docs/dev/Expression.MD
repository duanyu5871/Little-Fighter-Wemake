# “表达式”的应用

我不太确定“‘表达式’的应用”这个名字是否直观，但暂时先这么叫吧。

已知 LF2 的碰撞是否生效（`itr` 是否与 `bdy` 反应）明显遵循代码里固定的逻辑，甚至还与 `OID` 有关。

而我希望这个检测能更“自由”、更“原子化”，让游戏数据的开发者能自己通过数据来决定碰撞是否发生。

举个例子：在原版中，角色的 `itr.kind = 2` 可以拾取地面上的武器，跳转至固定的帧。

LFW 是通过预处理（`preprocess_itr`）让 `itr` 拥有一个 `itr.test`：碰撞时用 `test` 判断是否生效，随后通过 `itr.actions` 驱动角色帧跳转来实现的。

此时 `itr.test` 大概相当于：`角色未持有武器 且 武器处于“在地面”状态 且 武器未被持有`

（当然，在 LF2 中，地面上的武器几乎不会被持有……）

``` typescript
  switch(itr.kind) {
    // ...
    case ItrKind.Pick: {
      set_hit_flag(itr, itr.hit_flag ?? HitFlag.AllBoth)
      itr.motionless = itr.motionless ?? 0;
      itr.shaking = itr.shaking ?? 0;
      itr.test ??= new CondMaker<C_Val>()
        .add(C_Val.AttackerHasHolder, "==", 0)
        .and(C_Val.VictimHasHolder, "==", 0)
        .and_one_of(
          C_Val.VictimState,
          StateEnum.Weapon_OnGround,
          StateEnum.HeavyWeapon_OnGround,
        )
        .done();
      const test_1 = new CondMaker<C_Val>()
        .and(C_Val.VictimBaseType, "!=", WeaponEnum.Heavy)
        .done()
      const test_2 = new CondMaker<C_Val>()
        .and(C_Val.VictimBaseType, "==", WeaponEnum.Heavy)
        .done()
      itr.actions = ensure(itr.actions, {
        type: ActionType.A_NEXT_FRAME,
        desc: "picking_light 捡起轻型武器",
        pretest: true,
        test: test_1,
        data: { id: "115" },
      }, {
        type: ActionType.A_NEXT_FRAME,
        pretest: true,
        test: test_2,
        desc: "picking_heavy 捡起重型武器",
        data: { id: "117" },
      })
      break;
    }
    // ...
  }
```

`itr.test` 命中后，碰撞执行函数负责让角色拾取武器。

``` typescript
  // 碰撞生效的判定与test的职责可能有所重复，但大概无碍。
  collisions_keeper.register([{
    a_type: [EntityEnum.Fighter],
    itr: [ItrKind.Pick, ItrKind.PickSecretly],
    v_type: [EntityEnum.Weapon],
    v_state: [StateEnum.Weapon_OnGround, StateEnum.HeavyWeapon_OnGround],
    bdy: [BdyKind.Normal],
    handler: handle_weapon_picked,
  }])
  // 
  function handle_weapon_picked(collision: Collision): void {
    const { victim, attacker } = collision;
    attacker.pick(victim)
  }
```

随后 `itr.actions` 决定角色进入哪一帧（`itr.actions` 我觉得也可以单独说说）。

哪个 `action` 该被执行，同样由 `test` 来决定。于是有了拾取轻武器去 115、拾取重武器去 117。

这样 LFW 就允许了拾取武器去往其他帧，甚至用其他类型的 `action` 干更有想象力的事情。
没必要再用各种技巧“抓来抓去”来实现跳帧了。（虽说对某些硬核改档者来说，这也失去了某种改档的魅力，但原谅我还是固执己见。）

## 正在纠结

在早期版本中，`preprocess_itr` 在数据打包时就执行了，如此把 `test` 固化下来，在功能与逻辑稳定之后其实没有什么问题。

但 LFW 尚未稳定，各种判断还需要变来变去，我不希望每次变动都要改数据包。

于是我把 `preprocess_itr` 埋在了加载数据的阶段，用 `processed` 粗暴地判断是否该执行 `preprocess_itr`。

代价是：碰撞判定变得不那么透明了。

PS: 想要更自由，那就得自己想办法改代码了。
