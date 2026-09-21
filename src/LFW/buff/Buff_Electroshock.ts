import { SE } from "../defines";
import type { Entity } from "../entity/Entity";
import { is_fighter } from "../entity/type_check";
import { round_float } from "../utils";
import { Buff } from "./Buff";

export class Buff_Electroshock extends Buff {
  static override readonly KIND = "Electroshock";
  protected override get effect_oid(): string { return "fx"; }
  protected override place_effect(effect: Entity, victim: Entity): void {
    this.place_effect_center(effect, victim);
  }
  override init() {
    this._ticker.max = 3;
  }
  override on_tick(_?: Entity, victim?: Entity): void {
    if (!victim || !is_fighter(victim)) return;
    if (
      victim.state === SE.Falling ||
      victim.state === SE.Lying
    ) return
    victim.wait += 1;
    return
  }
  override mount(): void {
    super.mount()
    for (const vid of this.victims) {
      const victim = this.world.find_entity(vid)
      if (!victim) continue;
      if (victim.state == SE.Injured) continue;
      if (victim.state == SE.Falling) continue;
      this.duration = round_float(this.duration / 2);
    }
  }
}
