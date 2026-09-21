import { Buff_Healing } from "../buff/Buff_Healing";
import { grant_buff } from "../buff/grant_buff";
import type { Collision } from "./Collision";

export function handle_healing(collision: Collision): void {
  const { itr, attacker, victim } = collision;
  if (!itr.injury) return;
  grant_buff(
    Buff_Healing.KIND,
    attacker,
    victim,
    Buff_Healing.duration_of(victim, itr.injury),
  );
}
