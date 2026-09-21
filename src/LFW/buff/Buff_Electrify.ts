import { Buff } from "./Buff";
import type { Entity } from "../entity/Entity";

export class Buff_Electrify extends Buff {
  static override readonly KIND = "Electrify";

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
