import { O_ID, TeamEnum } from "../../../../defines";
import type { Entity } from "../../../../entity";
import { round_float } from "../../../../utils/math/round_float";
import { TestCase } from "../TestCase";

export class RedBook_GroupAttack extends TestCase {
  override name: string = "Red Book: GroupAttack";
  davis_top: Entity | null = null;
  davis_bottom: Entity | null = null;
  bandits_top: Entity[] = [];
  bandits_bottom: Entity[] = [];
  book: Entity | null = null;

  override enter(): void {
    super.enter();
    const z_top = round_float(this.midZ + (this.far - this.midZ) * 0.6);
    const z_bottom = round_float(this.midZ + (this.near - this.midZ) * 0.6);
    const davis_x = this.midX - 210;
    const bandit_x = this.midX - 30;

    this.davis_top = this.place_davis(davis_x, z_top);
    this.davis_bottom = this.place_davis(davis_x, z_bottom);

    this.bandits_top = [];
    this.bandits_bottom = [];
    for (let i = 0; i < 3; i++) {
      const x = bandit_x + i * 30;
      const top = this.place_bandit(x, z_top);
      if (top) this.bandits_top.push(top);
      const bottom = this.place_bandit(x, z_bottom);
      if (bottom) this.bandits_bottom.push(bottom);
    }

    this.book = this.place_weapon("book_red", davis_x + 24, z_top);
  }

  protected place_davis(x: number, z: number): Entity | null {
    const davis = this.spawn(O_ID.Davis);
    if (!davis) return null;
    davis.team = TeamEnum.Team_1;
    davis.facing = 1;
    davis.key_role = false;
    davis.set_position(x, 0, z);
    davis.attach();
    return davis;
  }

  protected place_bandit(x: number, z: number): Entity | null {
    const bandit = this.spawn(O_ID.Bandit);
    if (!bandit) return null;
    bandit.team = TeamEnum.Team_2;
    bandit.facing = -1;
    bandit.key_role = false;
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
