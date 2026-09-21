import { GK, O_ID, TeamEnum } from "../../../../defines";
import type { Entity } from "../../../../entity";
import { round_float } from "../../../../utils/math/round_float";
import { ActionDirector } from "../ActionDirector";
import { TestCase } from "../TestCase";

export class BlueBook_MpHealing extends TestCase {
  override name: string = "Blue Book: MpHealing";
  dennis_top: Entity | null = null;
  dennis_bottom: Entity | null = null;
  book: Entity | null = null;
  override director: ActionDirector = new ActionDirector().offset(1000, () => {
    this.dennis_top?.ctrl.click(GK.a)
  }).offset(1000, () => {
    this.dennis_top?.ctrl.click(GK.d, GK.R, GK.a)
    this.dennis_bottom?.ctrl.click(GK.d, GK.R, GK.a)
  }).repeat(2000, 100, () => {
    this.dennis_top?.ctrl.click(GK.a)
    this.dennis_bottom?.ctrl.click(GK.a)
  })
  override enter(): void {
    super.enter();
    const z_top = round_float(this.far);
    const z_bottom = round_float(this.near);
    const dennis_x = this.midX - 30;

    this.dennis_top = this.place_dennis(dennis_x, z_top);
    this.dennis_bottom = this.place_dennis(dennis_x, z_bottom);

    this.book = this.place_weapon("book_blue", dennis_x + 20, z_top);
  }

  protected place_dennis(x: number, z: number): Entity | null {
    const dennis = this.spawn(O_ID.Dennis);
    if (!dennis) return null;
    dennis.team = TeamEnum.Team_1;
    dennis.facing = 1;
    dennis.key_role = true;
    dennis.set_position(x, 0, z);
    dennis.attach();
    return dennis;
  }

  protected place_weapon(oid: string, x: number, z: number): Entity | null {
    const weapon = this.spawn(oid);
    if (!weapon) return null;
    weapon.set_position(x, 0, z);
    weapon.attach();
    return weapon;
  }
}
