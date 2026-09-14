import { Defines, GK, O_ID, TE, type IWpointInfo } from "../../../../defines";
import { type Entity, StatBarType } from "../../../../entity";
import { ActionDirector } from "../ActionDirector";
import { TestCase } from "../TestCase";

class Dropping_Base extends TestCase {
  protected a?: Entity;
  protected b?: Entity;
  protected c?: Entity;
  protected stone?: Entity;

  override enter(): void {
    this.director.reset();
    this.setup();
  }

  protected setup(): void {
    const { world } = this;
    world.clear();
    this.lfw.change_bg('pixel_ruler');

    const x = this.midX;
    const z = this.midZ;

    const a = this.a = this.spawn(O_ID.Mark) ?? void 0;
    if (!a) return;
    a.name = 'A: 捡起石头(稍后被攻击)';
    a.team = TE.Team_1;
    a.facing = 1;
    a.set_position(x, 0, z);
    a.attach();

    const stone = this.stone = this.spawn(O_ID.Weapon_Stone) ?? void 0;
    if (!stone) return;
    stone.set_position(x - 8, 0, z);
    stone.attach();

    const c = this.c = this.spawn(O_ID.Deep) ?? void 0;
    if (!c) return;
    c.name = 'C: 被石头砸到(不应受伤)';
    c.team = TE.Team_2;
    c.facing = 1;
    c.set_position(x - 150, 0, z);
    c.attach();

    const b = this.b = this.spawn(O_ID.Davis) ?? void 0;
    if (!b) return;
    b.name = 'B: 攻击A';
    b.team = TE.Team_2;
    b.facing = -1;
    b.set_position(x + 40, 0, z);
    b.attach();

    this.fighters = [a, b, c];
    for (const f of this.fighters) {
      f.stat_bar_type = StatBarType.None;
      f.key_role = true;
    }
  }

  protected a_pick_up(): void {
    this.a?.ctrl.click(GK.Attack);
  }

  /** B 攻击 A → 石头被击落 */
  protected b_attack(): void {
    this.b?.ctrl.click(GK.Attack);
  }

  protected place_c_under_weapon(): void {
    const { c, stone } = this;
    if (!c || !stone?.bearer) return;
    c.set_position(stone.position.x - 8, 0, stone.position.z);
  }

  protected c_pick_up_weapon(): void {
    const { c, stone } = this;
    if (!c || !stone || c.holding) return;
    c.pick(stone);
    c.enter_frame(Defines.NEXT_FRAME_AUTO);
  }

  protected c_throw_at_a(): void {
    const { c, a } = this;
    if (!c) return;
    if (a) c.facing = a.position.x < c.position.x ? -1 : 1;
    c.ctrl.click(GK.Attack);
  }

  protected place_a_at_throw_spot(): void {
    const { a, c } = this;
    if (!a || !c) return;
    let dx = 86;
    for (const f of Object.values(c.data.frames ?? {})) {
      const wp = f.wpoint as IWpointInfo | IWpointInfo[] | undefined;
      if (!wp || Array.isArray(wp)) continue;
      if (wp.dvx === void 0 && wp.dvy === void 0 && wp.dvz === void 0) continue;
      dx = (wp.x ?? 0) - (f.centerx ?? 0);
      break;
    }
    a.facing = -1; // 面向 C：石头从 C 手中飞向 A
    a.set_position(c.position.x + dx, 0, a.position.z);
  }
}

export class Dropping_1 extends Dropping_Base {
  override name: string = 'Dropped Weapon: No Hurt';

  override readonly director = new ActionDirector().repeat(
    9999, 1000,
    () => this.a_pick_up(),      // A 捡起石头
    () => this.place_c_under_weapon(),
    () => this.b_attack(),       // B 攻击 A → 石头被击落
    () => { },                   // 观察: 石头砸向 C
    () => { },                   // 观察: C 不受伤
    () => this.setup(),          // 重来
  );
}

export class Dropping_2 extends Dropping_Base {
  override name: string = 'Dropped Weapon: Pick Up & Throw Still Hurts';

  override readonly director = new ActionDirector().repeat(
    9999, 1000,
    () => this.a_pick_up(),      // A 捡起石头
    () => this.place_c_under_weapon(),
    () => this.b_attack(),       // B 攻击 A → 石头被击落
    () => { },                   // 观察: C 不受伤
    () => this.c_pick_up_weapon(), // C 捡起石头
    () => { },                   // 观察: C 举起石头
    () => this.place_a_at_throw_spot(), // A 站到投掷生成点上(保证被砸中)
    () => this.c_throw_at_a(),   // C 转身投掷 → 砸中 A
    () => { },                   // 观察: A 被砸倒(投掷依然伤人)
    () => { },
    () => this.setup(),          // 重来
  );
}
