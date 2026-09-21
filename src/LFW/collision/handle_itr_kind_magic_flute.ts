import type { Collision } from "./Collision";
import { handle_rest } from "./handle_rest";
/** 
 * @todo 要利用itr/bdy命中后上BUFF的数据支持机制，而不是这样写死的。
 * @see IAction_ABuff
 * @see IAction_VBuff
 * @deprecated 
 */
export function handle_itr_kind_magic_flute(collision: Collision): void {
  handle_rest(collision)
  const { victim, attacker, world, lfw, itr } = collision;
  const bid = `magic_flute_to_${victim.id}`
  let buf = world.buffs.get(bid)

  if (buf) {
    buf.lifetime = 0;
    return;
  }

  buf = lfw.factory.create_buff(itr.kind, lfw, bid)
  if (!buf) return;
  buf.set_attacker(attacker);
  buf.set_victim(victim);
  buf.mount();
}
