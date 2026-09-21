import { O_ID, TeamEnum } from "../../../../defines";
import type { Entity } from "../../../../entity";
import { round_float } from "../../../../utils/math/round_float";
import { TestCase } from "../TestCase";

export class GreenBook_Electrify extends TestCase {
  override name: string = "Green Book: Electrify";
  john_top: Entity | null = null;
  john_bottom: Entity | null = null;
  bandit_top: Entity | null = null;
  bandit_bottom: Entity | null = null;
  book: Entity | null = null;

  override enter(): void {
    super.enter();
    const z_top = round_float(this.midZ + (this.far - this.midZ) * 0.6);
    const z_bottom = round_float(this.midZ + (this.near - this.midZ) * 0.6);
    const john_x = this.midX - 210;
    const bandit_x = this.midX - 30;

    this.john_top = this.place_john(john_x, z_top);
    this.john_bottom = this.place_john(john_x, z_bottom);
    this.bandit_top = this.place_bandit(bandit_x, z_top);
    this.bandit_bottom = this.place_bandit(bandit_x, z_bottom);

    this.book = this.place_weapon("book_green", john_x + 24, z_top);
  }

  protected place_john(x: number, z: number): Entity | null {
    const john = this.spawn(O_ID.John);
    if (!john) return null;
    john.team = TeamEnum.Team_1;
    john.facing = 1;
    john.key_role = true;
    john.set_position(x, 0, z);
    john.attach();
    return john;
  }

  protected place_bandit(x: number, z: number): Entity | null {
    const bandit = this.spawn(O_ID.Bandit);
    if (!bandit) return null;
    bandit.team = TeamEnum.Team_2;
    bandit.facing = -1;
    bandit.key_role = true;
    bandit.set_position(x, 0, z);
    bandit.attach();
    return bandit;
  }

  protected place_weapon(oid: string, x: number, z: number): Entity | null {
    const weapon = this.spawn(oid);
    if (!weapon) return null;
    weapon.set_position(x, 0, z);
    weapon.attach();
    return weapon;
  }
}
