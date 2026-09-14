import type { Collision } from "../collision/Collision";
import { Defines, OID, SparkEnum, WT } from "../defines";
import { calc_itr_velocity } from "./calc_itr_velocity";
import { handle_injury } from "./handle_injury";
import { handle_rest } from "./handle_rest";
import { handle_stiffness } from "./handle_stiffness";

export function handle_weapon_is_hit(collision: Collision): void {
  handle_rest(collision)
  handle_stiffness(collision)
  handle_injury(collision)
  const { itr, attacker, victim, a_cube, b_cube } = collision;

  victim.dropping = false;
  if (itr.bdefend && itr.bdefend >= Defines.DEFAULT_FORCE_BREAK_DEFEND_VALUE) {
    victim.hp = victim.hp_r = 0;
  }

  const is_fly = itr.fall && itr.fall >= Defines.DEFAULT_FALL_VALUE_CRITICAL;
  victim.world.spark(
    ...collision.victim.spark_point(a_cube, b_cube),
    is_fly ? SparkEnum.SilentCriticalHit : SparkEnum.SilentHit
  );

  let [vx, vy, vz] = calc_itr_velocity(collision)
  const is_base_ball =
    victim.base_type === WT.Baseball ||
    victim.base_type === WT.Drink;

  if (victim.base_type !== WT.Heavy || is_fly) {
    victim.set_velocity(vx, vy, vz);
    victim.leave_ground();
    victim.lfw.mt.mark = 'hwih_1';
    let nid: string | undefined = void 0
    if (is_base_ball && (vx >= 6 || vx <= -6))
      nid = victim.lfw.mt.pick(victim.data.indexes?.throwings)
    else
      nid = victim.lfw.mt.pick(victim.data.indexes?.in_the_skys)
    victim.enter_frame_by_id(nid);
  }

  if (
    attacker.data.id === OID.Weapon_Stick &&
    is_base_ball
  ) {
    vx = attacker.facing * 2 // fast!
    victim.lfw.mt.mark = 'hwih_2'
    victim.enter_frame_by_id(victim.data.indexes?.throwings?.[0])
    victim.set_velocity(vx)
  }


  if (is_fly && !victim.bearer) {
    /* 
    武器被单方面命中，直接修改队伍就好
    武器谁先判定就变谁的队
    */
    victim.team = attacker.team;
  }
}
