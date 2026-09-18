# 碰撞测试表达式

为了日后实现更多的碰撞判定……为了避免过于“hard-code”，
LFW引入了“碰撞测试表达式”，
很多碰撞判定实现是通过此功能实现的。

## 写法

`bdy.test` / `itr.test` 是一个字符串表达式：

- 单项形如 `变量 运算符 值`，例如 `itr_kind==0`、`victim_state==1004`；
- 运算符：`==`、`!=`、`>`、`>=`、`<`、`<=`；
- 多项用 `&&`（且）、`||`（或）连接，也可以写成 `&` / `|`；
- `!(...)` 取反，圆括号用于分组。

优先级与常见语言一致：`&&` 比 `||` 结合得更紧，从左到右求值、支持短路。
（早期版本里两者同级，必须自己加括号来决定顺序，这个坑已经修掉了；不过按需加括号读起来还是更清楚。）

变量完整清单见 `src/LFW/defines/CollisionVal.ts`（带注释）。攻击方以 `attacker_*` 开头、受击方以 `victim_*` 开头。

## 生效机制

- 碰撞盒相交、hit_flag 通过后，分别对 `bdy.test` 与 `itr.test` 求值；任意一边为假，该次碰撞不生效。
- 数据里没写 `test` 时，`preprocess_itr` / `preprocess_bdy` 会按 `kind`、`effect` 填上默认判定；写了就完全按数据的来（`??=` 覆盖，不会叠加）。
- `itr.actions` / `bdy.actions` 里的 `test` 也复用同一套语法。
- 开发模式（`lfw.dev`）下会打印求值过程，便于排查。

## 例

- 角色拾取武器（配合hitflag，`ItrKind.Pick` 的默认判定）：
  `itr.test = "attacker_has_holding==0&&victim_has_holder==0&&(victim_state==1004||victim_state==2004)"`
- 滚动时按A才能偷偷拾取武器（`ItrKind.PickSecretly` 的默认判定）：
  `itr.test = "attacker_has_holder==0&&victim_has_holder==0&&victim_state==1004&&a_hit_attack==1"`
- 气功波被摧毁（配合hitflag）：
  `bdy.test = "itr_kind==0&&(attacker_type==32||attacker_type==4||(attacker_type==16&&attacker_state!=1001)||(attacker_type==8&&attacker_state==19))"`
- 气功波被反弹（配合hitflag）：
  `bdy.test = "itr_kind==9||((itr_kind==0||itr_kind==4||itr_kind==5)&&((same_team==0&&(attacker_type==8||(attacker_type==16&&attacker_state==1001)))||(same_team==1&&same_facing==0&&(attacker_type==8||(attacker_type==16&&attacker_state==1001)))))"`

后两条由 `cook_ball_bdy_get_hit_to_frame_20` / `cook_ball_bdy_get_hit_to_frame_30` 生成：气功波被打后，分别跳转帧 20、帧 30。

TODO: 更细致的说明，这恐怕量有点大
