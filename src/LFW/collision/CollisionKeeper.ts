
import { ALL_ENTITY_ENUM, ALL_STATES, BdyKind, EntityEnum, ItrKind, StateEnum, type TEntityEnum } from "../defines";
import { Ditto } from "../ditto";
import { collision_action_handlers } from "../entity/collision_action_handlers";
import { NestedMap } from "../utils/container_help/nested_map";
import type { Collision, ICollisionFunc } from "./Collision";
import { handle_ball_frozen } from "./handle_ball_frozen";
import { handle_ball_hit_other } from "./handle_ball_hit_other";
import { handle_ball_is_hit_a, handle_ball_is_hit_b } from "./handle_ball_is_hit";
import { handle_body_goto as handle_criminal_hit } from "./handle_body_goto";
import { handle_healing } from "./handle_healing";
import { handle_itr_catch } from "./handle_itr_catch";
import { handle_itr_kind_freeze } from "./handle_itr_kind_freeze";
import { handle_itr_kind_magic_flute } from "./handle_itr_kind_magic_flute";
import { handle_itr_kind_whirlwind } from "./handle_itr_kind_whirlwind";
import { handle_itr_normal_bdy_defend } from "./handle_itr_normal_bdy_defend";
import { handle_itr_normal_bdy_normal } from "./handle_itr_normal_bdy_normal";
import { handle_john_shield_hit_other_ball } from "./handle_john_shield_hit_other_ball";
import { handle_rest } from "./handle_rest";
import { handle_super_punch_me } from "./handle_super_punch_me";
import { handle_weapon_hit_other } from "./handle_weapon_hit_other";
import { handle_weapon_is_hit } from "./handle_weapon_is_hit";
import { handle_weapon_picked } from "./handle_weapon_picked";
import type { ICollisionHandler } from "./ICollisionHandler";

export interface IHandlerConfig {
  /** 攻击方实体类型 */
  a_type: TEntityEnum[];
  /** 攻击框类型 */
  itr: ItrKind[];
  /** 受击方实体类型 */
  v_type: TEntityEnum[];
  /** 受击框类型 */
  bdy: BdyKind[];
  /** 碰撞处理器 */
  handler: ICollisionFunc;
  /** 攻击方帧状态（默认全状态） */
  a_state?: StateEnum[];
  /** 受击方帧状态（默认全状态） */
  v_state?: StateEnum[];
  /** Description of handler */
  desc?: string;
}

function pack_a(a_type: number, itr_kind: number): number {
  return (a_type << 12) | itr_kind;
}

function pack_b(v_type: number, bdy_kind: number): number {
  return (v_type << 16) | bdy_kind;
}

const is_u8 = (v: number) => Number.isInteger(v) && v >= 0 && v < 256;
const is_u12 = (v: number) => Number.isInteger(v) && v >= 0 && v < 4096;
const is_u16 = (v: number) => Number.isInteger(v) && v >= 0 && v < 65536;

interface IHandlerEntry {
  fn: ICollisionFunc;
  a_state?: StateEnum[];
  v_state?: StateEnum[];
}

export class CollisionKeeper {
  protected pair_map = new NestedMap<number, number, IHandlerEntry[]>();

  private add(
    a_type_list: TEntityEnum[],
    itr_kind_list: ItrKind[],
    v_type_list: TEntityEnum[],
    bdy_kind_list: BdyKind[],
    fn: ICollisionFunc,
    a_state_list: StateEnum[] = ALL_STATES,
    v_state_list: StateEnum[] = ALL_STATES,
  ) {
    const entry: IHandlerEntry = { fn };
    if (a_state_list !== ALL_STATES) entry.a_state = a_state_list;
    if (v_state_list !== ALL_STATES) entry.v_state = v_state_list;
    for (const a_type of a_type_list) {
      for (const itr_kind of itr_kind_list) {
        const k1 = pack_a(a_type, itr_kind);
        for (const v_type of v_type_list) {
          for (const bdy_kind of bdy_kind_list) {
            const k2 = pack_b(v_type, bdy_kind);
            const list = this.pair_map.get(k1, k2);
            list ? list.push(entry) : this.pair_map.set(k1, k2, [entry]);
          }
        }
      }
    }
  }

  register(configs: IHandlerConfig[]): this {
    for (const cfg of configs) {
      this.add(cfg.a_type, cfg.itr, cfg.v_type, cfg.bdy, cfg.handler, cfg.a_state, cfg.v_state);
    }
    return this;
  }

  adds(...list: ICollisionHandler[]) {
    for (const i of list) {
      this.add(i.a_type, i.itr, i.v_type, i.bdy, i.run.bind(i));
    }
  }

  load_handlers(collision: Collision): boolean {
    collision.handlers.length = 0;
    const a_type = collision.attacker.data.type
    const itr_kind = collision.itr.kind
    const v_type = collision.victim.data.type
    const bdy_kind = collision.bdy.kind
    const a_state = collision.attacker.state
    const b_state = collision.victim.state
    if (!is_u8(a_type) || !is_u12(itr_kind) || !is_u8(v_type) || !is_u16(bdy_kind)) return false;
    const list = this.pair_map.get(pack_a(a_type, itr_kind), pack_b(v_type, bdy_kind));
    if (!list?.length) return false;
    for (let i = 0; i < list.length; i++) {
      const e = list[i];
      if (e.a_state && !e.a_state.includes(a_state)) continue;
      if (e.v_state && !e.v_state.includes(b_state)) continue;
      collision.handlers.push(e.fn);
    }
    return collision.handlers.length > 0;
  }

  handle(collision: Collision) {
    const { handlers } = collision;
    if (Ditto.DEV && handlers) {
      const collision_desc =
        `[${collision.attacker.data.type}]#${ItrKind[collision.itr.kind]} => ` +
        `[${collision.victim.data.type}]#${BdyKind[collision.bdy.kind]}`;
      Ditto.debug(` collision: ${collision_desc} \nhandlers: ${handlers?.map(v => v.name) ?? 'none'}`)
    }

    let ball_hit = false;
    const { itr, bdy, victim, attacker } = collision;
    const itr_tests = itr.actions?.map(v => v.pretest && v.tester?.run(collision) !== false)
    const bdy_tests = bdy.actions?.map(v => v.pretest && v.tester?.run(collision) !== false)

    handlers?.forEach(fn => {
      ball_hit = ball_hit ||
        fn === handle_ball_is_hit_a ||
        fn === handle_ball_is_hit_b;
      return fn(collision)
    })
    if (!handle_ball_frozen(victim, attacker, itr)) {
      itr.actions?.forEach((action, idx) => {
        const test_result = action.pretest ? itr_tests?.[idx] : action.tester?.run(collision);
        if (test_result === false) return;
        collision_action_handlers[action.type](action as any, collision)
      })
      bdy.actions?.forEach((action, idx) => {
        const test_result = action.pretest ? bdy_tests?.[idx] : action.tester?.run(collision);
        if (test_result === false) return;
        collision_action_handlers[action.type](action as any, collision)
      })
    }

    victim.collided_list.push((victim.lastest_collided = collision));
    attacker.collision_list.push(collision);
    /*  
    FIXME: 
      总而言之要尽量少点这么硬的东西，但目前要改似乎代价太大
        -Gim
    */
    if (
      itr.kind !== ItrKind.Block &&
      itr.kind !== ItrKind.Whirlwind &&
      itr.kind !== ItrKind.MagicFlute &&
      itr.kind !== ItrKind.MagicFlute2 &&
      itr.kind !== ItrKind.Pick &&
      itr.kind !== ItrKind.PickSecretly
    ) {
      const sounds = victim.data.base.hit_sounds;
      victim.play_sound(sounds);
    }
  }
}
export const collisions_keeper = new CollisionKeeper();


const HANDLER_CONFIGS: IHandlerConfig[] = [
  // ── 抓取类 ──
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.Catch, ItrKind.ForceCatch],
    v_type: [EntityEnum.Fighter],
    bdy: [BdyKind.Normal, BdyKind.Defend],
    handler: handle_itr_catch,
    desc: ` 
能否抓起的判定通过itr.test实现
`.trim()
  },
  // ── 特殊效果类 ──
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.Whirlwind],
    v_type: [EntityEnum.Fighter, EntityEnum.Weapon],
    bdy: [BdyKind.Normal, BdyKind.Defend],
    handler: handle_itr_kind_whirlwind,
  },
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.Freeze],
    v_type: [EntityEnum.Fighter],
    bdy: [BdyKind.Normal, BdyKind.Defend],
    handler: handle_itr_kind_freeze,
  },
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.MagicFlute, ItrKind.MagicFlute2],
    v_type: [EntityEnum.Fighter, EntityEnum.Weapon],
    bdy: [BdyKind.Normal, BdyKind.Defend],
    handler: handle_itr_kind_magic_flute,
  },
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.Heal],
    v_type: [EntityEnum.Fighter],
    bdy: [BdyKind.Normal],
    handler: handle_healing,
  },
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.SuperPunchMe],
    v_type: [EntityEnum.Fighter],
    bdy: [BdyKind.Normal],
    handler: handle_super_punch_me,
  },

  // ── 普通攻击 → 角色 ──
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.JohnShield, ItrKind.Normal, ItrKind.WeaponSwing, ItrKind.CharacterThrew],
    v_type: [EntityEnum.Fighter],
    bdy: [BdyKind.Normal],
    handler: handle_itr_normal_bdy_normal,
  },
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.JohnShield, ItrKind.Normal, ItrKind.WeaponSwing, ItrKind.CharacterThrew],
    v_type: [EntityEnum.Fighter],
    bdy: [BdyKind.Defend],
    handler: handle_itr_normal_bdy_defend,
  },

  // ── 拾取武器 ──
  {
    a_type: [EntityEnum.Fighter],
    itr: [ItrKind.Pick, ItrKind.PickSecretly],
    v_type: [EntityEnum.Weapon],
    v_state: [StateEnum.Weapon_OnGround, StateEnum.HeavyWeapon_OnGround],
    bdy: [BdyKind.Normal],
    handler: handle_weapon_picked,
  },

  // ── 武器被攻击（飞行中/空中） ──
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.JohnShield, ItrKind.Normal, ItrKind.WeaponSwing, ItrKind.CharacterThrew],
    v_type: [EntityEnum.Weapon],
    bdy: [BdyKind.Normal],
    handler: handle_weapon_is_hit,
    v_state: [
      StateEnum.HeavyWeapon_OnGround,
      StateEnum.HeavyWeapon_InTheSky,
      StateEnum.HeavyWeapon_JustOnGround,
      StateEnum.Weapon_Throwing,
      StateEnum.Weapon_InTheSky,
      StateEnum.Weapon_Rebounding,
    ],
  },
  {
    a_type: [EntityEnum.Weapon, EntityEnum.Ball],
    itr: [ItrKind.Normal],
    v_type: [EntityEnum.Weapon],
    bdy: [BdyKind.Normal],
    handler: handle_weapon_is_hit,
    v_state: [StateEnum.Weapon_OnGround],
  },

  // ── 格挡/硬直 ──
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.Block],
    v_type: [EntityEnum.Fighter],
    bdy: [BdyKind.Normal],
    handler: handle_rest,
  },

  // ── 气功波相关 ──
  {
    a_type: [EntityEnum.Ball],
    itr: [ItrKind.Normal],
    v_type: ALL_ENTITY_ENUM,
    bdy: [BdyKind.Normal, BdyKind.Defend],
    handler: handle_ball_hit_other,
  },
  {
    a_type: [EntityEnum.Fighter],
    itr: [ItrKind.Normal, ItrKind.WeaponSwing, ItrKind.CharacterThrew],
    v_type: [EntityEnum.Ball],
    bdy: [BdyKind.Normal],
    handler: handle_ball_is_hit_a,
  },
  {
    a_type: [EntityEnum.Weapon],
    itr: [ItrKind.WeaponSwing],
    v_type: [EntityEnum.Ball],
    bdy: [BdyKind.Normal],
    handler: handle_ball_is_hit_a,
  },
  {
    a_type: [EntityEnum.Weapon, EntityEnum.Ball],
    itr: [ItrKind.Normal],
    v_type: [EntityEnum.Ball],
    bdy: [BdyKind.Normal],
    handler: handle_ball_is_hit_b,
  },
  {
    a_type: ALL_ENTITY_ENUM,
    itr: [ItrKind.JohnShield],
    v_type: [EntityEnum.Ball],
    bdy: [BdyKind.Normal],
    handler: handle_john_shield_hit_other_ball,
  },

  // ── 武器 / 特殊 ──
  {
    a_type: [EntityEnum.Weapon],
    itr: [ItrKind.Normal],
    v_type: ALL_ENTITY_ENUM,
    bdy: [BdyKind.Normal, BdyKind.Defend],
    handler: handle_weapon_hit_other,
  },
  {
    a_type: [EntityEnum.Weapon, EntityEnum.Fighter],
    itr: [ItrKind.WeaponSwing, ItrKind.Normal],
    v_type: ALL_ENTITY_ENUM,
    bdy: [BdyKind.Criminal],
    handler: handle_criminal_hit,
  },
];

collisions_keeper.register(HANDLER_CONFIGS);

