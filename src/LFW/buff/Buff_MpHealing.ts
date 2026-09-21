import { ceil, max, min } from "../utils";
import { Buff } from "./Buff";
import type { Entity } from "../entity/Entity";

export class Buff_MpHealing extends Buff {
  static override readonly KIND = "MpHealing";

  static duration_of(e: Entity, mp: number): number {
    const value = max(1, e.dataset("mp_healing_value"));
    const ticks = max(1, e.dataset("mp_healing_ticks"));
    return ceil(mp / value) * ticks;
  }

  override mount(): void {
    super.mount();
    for (const vid of this.victims) {
      const victim = this.world.find_entity(vid);
      if (!victim) continue;
      victim.set_mark(Buff_MpHealing.KIND, this.id);
      this.ticks = victim.dataset("mp_healing_ticks");
    }
  }
  override on_tick(_?: Entity, victim?: Entity): void {
    if (!victim) return;
    // if (victim.mp >= victim.mp_max) {
    //   this.lifetime = this.duration;
    //   return;
    // }
    victim.mp = min(victim.mp_max, victim.mp + victim.dataset("mp_healing_value"));
  }
  override unmount(): void {
    for (const vid of this.victims) {
      const victim = this.world.find_entity(vid);
      if (!victim) continue;
      victim.del_mark(Buff_MpHealing.KIND, this.id);
    }
    super.unmount();
  }
}
