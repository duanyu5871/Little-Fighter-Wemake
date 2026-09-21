import type { Entity } from "../entity/Entity";
import type { Buff } from "./Buff";

export function grant_buff(
  kind: string,
  attacker: Entity | undefined,
  victim: Entity,
  duration: number = 0,
): Buff | undefined {
  const { lfw, world } = victim;
  const id = kind + '_' + victim.id;
  const buf = world.buffs.get(id) ?? lfw.factory.create_buff(kind, lfw, id);
  if (!buf) return void 0;
  buf.lifetime = 0;
  buf.duration = duration;
  buf.level += 1;
  if (attacker) buf.set_attacker(attacker);
  buf.set_victim(victim);
  buf.mount();
  return buf;
}
