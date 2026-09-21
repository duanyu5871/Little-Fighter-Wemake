import { ceil, max, min } from "../utils";
import { Buff } from "./Buff";
import type { Entity } from "../entity/Entity";

export class Buff_Healing extends Buff {
  static override readonly KIND = "Healing";

  static duration_of(e: Entity, hp: number): number {
    const value = max(1, e.dataset("hp_healing_value"));
    const ticks = max(1, e.dataset("hp_healing_ticks"));
    return ceil(hp / value) * ticks;
  }

  override mount(): void {
    super.mount();
    for (const vid of this.victims) {
      const victim = this.world.find_entity(vid);
      if (!victim) continue;
      victim.set_mark(Buff_Healing.KIND, this.id);
      this.ticks = victim.dataset("hp_healing_ticks");
    }
  }
  override on_tick(_?: Entity, victim?: Entity): void {
    if (!victim) return;
    if (victim.hp >= victim.hp_r) {
      this.lifetime = this.duration;
      return;
    }
    victim.hp = min(victim.hp_r, victim.hp + victim.dataset("hp_healing_value"));
    return;
  }
  override unmount(): void {
    for (const vid of this.victims) {
      const victim = this.world.find_entity(vid);
      if (!victim) continue;
      victim.del_mark(Buff_Healing.KIND, this.id);
    }
    super.unmount();
  }
}
