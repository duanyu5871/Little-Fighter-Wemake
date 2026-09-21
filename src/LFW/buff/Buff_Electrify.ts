import { Buff } from "./Buff";
import type { Entity } from "../entity/Entity";

export class Buff_Electrify extends Buff {
  static override readonly KIND = "Electrify";
  protected override get effect_oid(): string { return "fx"; }
  protected override get effect_frame_id(): string { return "32"; }
  protected override place_effect(effect: Entity, victim: Entity): void {
    this.place_effect_center(effect, victim);
  }

  override mount(): void {
    super.mount();
    for (const vid of this.victims) {
      const victim = this.world.find_entity(vid);
      if (!victim) continue;
      victim.set_mark(Buff_Electrify.KIND, this.id);
    }
  }
  override unmount(): void {
    for (const vid of this.victims) {
      const victim = this.world.find_entity(vid);
      if (!victim) continue;
      victim.del_mark(Buff_Electrify.KIND, this.id);
    }
    super.unmount();
  }
}
