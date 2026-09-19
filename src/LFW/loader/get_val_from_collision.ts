import type { Collision } from "../collision/Collision";
import { is_armor_work } from "../collision/is_armor_work";
import { CheatEnum, EntityGroup, GK, HitFlag, SE } from "../defines";
import { CollisionVal } from "../defines/CollisionVal";
import type { IValGetter, IValGetterGetter } from "../defines/IExpression";
import { is_ball_ctrl, is_fighter } from "../entity";
import { abs, round } from "../utils";

const map: Record<CollisionVal, IValGetter<Collision>> = {
  [CollisionVal.AttackerType]: c => c.attacker.data.type,
  [CollisionVal.VictimType]: c => c.victim.data.type,
  [CollisionVal.AttackerBaseType]: c => c.attacker.data.base.type,
  [CollisionVal.VictimBaseType]: c => c.victim.data.base.type,
  [CollisionVal.ItrKind]: c => c.itr.kind,
  [CollisionVal.ItrEffect]: c => c.itr.effect,
  [CollisionVal.SameTeam]: c => c.attacker.is_ally(c.victim) ? 1 : 0,
  [CollisionVal.SameFacing]: c => c.attacker.facing === c.victim.facing ? 1 : 0,
  [CollisionVal.AttackerState]: c => c.aframe.state,
  [CollisionVal.VictimState]: c => c.bframe.state,
  [CollisionVal.AttackerHasHolder]: c => c.attacker.bearer ? 1 : 0,
  [CollisionVal.VictimHasHolder]: c => c.victim.bearer ? 1 : 0,
  [CollisionVal.AttackerHasHolding]: c => c.attacker.holding ? 1 : 0,
  [CollisionVal.VictimHasHolding]: c => c.victim.holding ? 1 : 0,
  [CollisionVal.AttackerOID]: c => c.attacker.data.id,
  [CollisionVal.VictimOID]: c => c.victim.data.id,
  [CollisionVal.BdyKind]: c => c.bdy.kind,
  [CollisionVal.VictimFrameId]: c => c.bframe.id,
  [CollisionVal.VictimFrameIndex_ICE]: c => c.victim.data.indexes?.ice,
  [CollisionVal.ItrFall]: c => c.itr.fall,
  [CollisionVal.AttackerThrew]: c => c.attacker.throwinjury ? 1 : 0,
  [CollisionVal.VictimThrew]: c => c.victim.throwinjury ? 1 : 0,
  [CollisionVal.VictimIsChasing]: c => {
    return is_ball_ctrl(c.attacker.ctrl) && c.victim === c.attacker.ctrl.chasing ? 1 : 0;
  },
  [CollisionVal.VictimIsFreezableBall]: c => c.victim.group?.some(v => v === EntityGroup.FreezableBall) ? 1 : 0,
  [CollisionVal.AttackerIsFreezableBall]: c => c.attacker.group?.some(v => v === EntityGroup.FreezableBall) ? 1 : 0,
  [CollisionVal.ArmorWork]: (collision: Collision) => is_armor_work(collision) ? 1 : 0,
  [CollisionVal.V_FrameBehavior]: c => c.victim.frame.behavior,
  [CollisionVal.NoItrEffect]: c => c.itr.effect === void 0 ? 1 : 0,
  [CollisionVal.A_HP_P]: c => round(100 * c.attacker.hp / c.attacker.hp_max),
  [CollisionVal.V_HP_P]: c => round(100 * c.victim.hp / c.victim.hp_max),
  [CollisionVal.LF2_NET_ON]: c => c.attacker.lfw.is_cheat(CheatEnum.LF2_NET) ? 1 : 0,
  [CollisionVal.BdyHitFlag]: c => c.bdy.hit_flag ?? HitFlag.AllEnemy,
  [CollisionVal.ItrHitFlag]: c => c.itr.hit_flag ?? HitFlag.AllEnemy,
  [CollisionVal.BdyCode]: c => c.bdy.code,
  [CollisionVal.ItrCode]: c => c.itr.code,
  [CollisionVal.VToughness]: c => c.victim.toughness,
  [CollisionVal.AToughness]: c => c.attacker.toughness,
  [CollisionVal.AClosingSpeedX]: c => {
    const v = c.attacker.velocity.x;
    const p1 = c.attacker.position.x;
    const p2 = c.victim.position.x;
    if (p1 > p2) return -v;
    if (p1 < p2) return v;
    return abs(-v);
  },
  [CollisionVal.AClosingSpeedY]: c => {
    const v = c.attacker.velocity.y;
    const p1 = c.attacker.position.y;
    const p2 = c.victim.position.y;
    if (p1 > p2) return -v;
    if (p1 < p2) return v;
    return abs(-v);
  },
  [CollisionVal.AClosingSpeedZ]: c => {
    const v = c.attacker.velocity.z;
    const p1 = c.attacker.position.z;
    const p2 = c.victim.position.z;
    if (p1 > p2) return -v;
    if (p1 < p2) return v;
    return abs(-v);
  },
  [CollisionVal.AEmitter]: c => c.attacker.emitter ?? '',
  [CollisionVal.VEmitter]: c => c.victim.emitter ?? '',
  [CollisionVal.AHitAttack /**    */]: c => c.attacker.ctrl.is_hit(GK.a),
  [CollisionVal.AHitJump /**      */]: c => c.attacker.ctrl.is_hit(GK.j),
  [CollisionVal.AHitDefend /**    */]: c => c.attacker.ctrl.is_hit(GK.d),
  [CollisionVal.AHitUp /**        */]: c => c.attacker.ctrl.is_hit(GK.U),
  [CollisionVal.AHitDown /**      */]: c => c.attacker.ctrl.is_hit(GK.D),
  [CollisionVal.AHitLeft /**      */]: c => c.attacker.ctrl.is_hit(GK.L),
  [CollisionVal.AHitRight /**     */]: c => c.attacker.ctrl.is_hit(GK.R),
  [CollisionVal.VHitAttack /**    */]: c => c.victim.ctrl.is_hit(GK.a),
  [CollisionVal.VHitJump /**      */]: c => c.victim.ctrl.is_hit(GK.j),
  [CollisionVal.VHitDefend /**    */]: c => c.victim.ctrl.is_hit(GK.d),
  [CollisionVal.VHitUp /**        */]: c => c.victim.ctrl.is_hit(GK.U),
  [CollisionVal.VHitDown /**      */]: c => c.victim.ctrl.is_hit(GK.D),
  [CollisionVal.VHitLeft /**      */]: c => c.victim.ctrl.is_hit(GK.L),
  [CollisionVal.VHitRight /**     */]: c => c.victim.ctrl.is_hit(GK.R),
  [CollisionVal.AClickAttack /**  */]: c => c.attacker.ctrl.is_start(GK.a),
  [CollisionVal.AClickJump /**    */]: c => c.attacker.ctrl.is_start(GK.j),
  [CollisionVal.AClickDefend /**  */]: c => c.attacker.ctrl.is_start(GK.d),
  [CollisionVal.AClickUp /**      */]: c => c.attacker.ctrl.is_start(GK.U),
  [CollisionVal.AClickDown /**    */]: c => c.attacker.ctrl.is_start(GK.D),
  [CollisionVal.AClickLeft /**    */]: c => c.attacker.ctrl.is_start(GK.L),
  [CollisionVal.AClickRight /**   */]: c => c.attacker.ctrl.is_start(GK.R),
  [CollisionVal.VClickAttack /**  */]: c => c.victim.ctrl.is_start(GK.a),
  [CollisionVal.VClickJump /**    */]: c => c.victim.ctrl.is_start(GK.j),
  [CollisionVal.VClickDefend /**  */]: c => c.victim.ctrl.is_start(GK.d),
  [CollisionVal.VClickUp /**      */]: c => c.victim.ctrl.is_start(GK.U),
  [CollisionVal.VClickDown /**    */]: c => c.victim.ctrl.is_start(GK.D),
  [CollisionVal.VClickLeft /**    */]: c => c.victim.ctrl.is_start(GK.L),
  [CollisionVal.VClickRight /**   */]: c => c.victim.ctrl.is_start(GK.R),
  [CollisionVal.ADbcAttack /**    */]: c => c.attacker.ctrl.is_db_hit(GK.a),
  [CollisionVal.ADbcJump /**      */]: c => c.attacker.ctrl.is_db_hit(GK.j),
  [CollisionVal.ADbcDefend /**    */]: c => c.attacker.ctrl.is_db_hit(GK.d),
  [CollisionVal.ADbcUp /**        */]: c => c.attacker.ctrl.is_db_hit(GK.U),
  [CollisionVal.ADbcDown /**      */]: c => c.attacker.ctrl.is_db_hit(GK.D),
  [CollisionVal.ADbcLeft /**      */]: c => c.attacker.ctrl.is_db_hit(GK.L),
  [CollisionVal.ADbcRight /**     */]: c => c.attacker.ctrl.is_db_hit(GK.R),
  [CollisionVal.VDbcAttack /**    */]: c => c.victim.ctrl.is_db_hit(GK.a),
  [CollisionVal.VDbcJump /**      */]: c => c.victim.ctrl.is_db_hit(GK.j),
  [CollisionVal.VDbcDefend /**    */]: c => c.victim.ctrl.is_db_hit(GK.d),
  [CollisionVal.VDbcUp /**        */]: c => c.victim.ctrl.is_db_hit(GK.U),
  [CollisionVal.VDbcDown /**      */]: c => c.victim.ctrl.is_db_hit(GK.D),
  [CollisionVal.VDbcLeft /**      */]: c => c.victim.ctrl.is_db_hit(GK.L),
  [CollisionVal.VDbcRight /**     */]: c => c.victim.ctrl.is_db_hit(GK.R),
  [CollisionVal.AFALLING /**      */]: c => (is_fighter(c.attacker) && c.attacker.state == SE.Falling) ? 1 : 0,
  [CollisionVal.VFALLING /**      */]: c => (is_fighter(c.victim) && c.victim.state == SE.Falling) ? 1 : 0,
};
export const get_val_geter_from_collision: IValGetterGetter<Collision> = (
  word: string,
): IValGetter<Collision> | undefined => {
  return (map as any)[word];
};
