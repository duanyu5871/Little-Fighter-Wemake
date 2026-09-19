import { SE, type IEntityData } from "../defines";
import type { Entity } from "../entity/Entity";
import { is_fighter } from "../entity/type_check";
import { round_float } from "../utils";
import { Buff } from "./Buff";

const ELECTROSHOCK_OID = "electroshock";
const EFFECT_FRAME = "0";

export class Buff_Electroshock extends Buff {
  static override readonly KIND = "Electroshock";
  private _data?: IEntityData;
  private readonly _effects = new Map<string, Entity>();
  override init() {
    this._ticker.max = 3;
  }
  override reset(id: string): this {
    this.clear_effects();
    return super.reset(id);
  }
  private effect_data(): IEntityData | undefined {
    return this._data ??= this.lfw.datas.find(ELECTROSHOCK_OID);
  }
  private clear_effects(): void {
    for (const [, effect] of this._effects) {
      if (this.world.find_entity(effect.id)) this.world.del_entity(effect);
    }
    this._effects.clear();
  }
  private del_effect(vid: string): void {
    const effect = this._effects.get(vid);
    if (!effect) return;
    if (this.world.find_entity(effect.id)) this.world.del_entity(effect);
    this._effects.delete(vid);
  }
  override del_victims(...victims: (string | Entity)[]): this {
    for (const victim of victims)
      this.del_effect(typeof victim === 'string' ? victim : victim.id);
    return super.del_victims(...victims);
  }
  private victim_center(victim: Entity): [number, number, number] {
    const { centery = 0, height = 0, pic } = victim.frame;
    const h = height || pic?.h || 0;
    return [
      victim.position.x,
      victim.position.y + centery - h / 2,
      victim.position.z,
    ];
  }
  private show_effect(victim: Entity): void {
    let effect = this._effects.get(victim.id);
    if (effect && !this.world.find_entity(effect.id)) effect = void 0;
    if (!effect) {
      const data = this.effect_data();
      if (!data) return;
      effect = this.lfw.factory.create_entity(this.world, data);
      if (!effect) return;
      effect.outline_alpha = 0;
      effect.outline_width = 0;
      effect.outline_color = "";
      effect.set_position(...this.victim_center(victim));
      effect.enter_frame_by_id(EFFECT_FRAME);
      effect.attach(true);
      this._effects.set(victim.id, effect);
    }
    effect.set_position(...this.victim_center(victim));
  }
  override on_update(_?: Entity, victim?: Entity): 'keep' | 'del' {
    for (const vid of this._effects.keys()) {
      if (!this.world.find_entity(vid)) this.del_effect(vid);
    }
    if (!victim) return 'del';
    if (is_fighter(victim)) this.show_effect(victim);
    else this.del_effect(victim.id);
    return 'keep';
  }
  override on_tick(_?: Entity, victim?: Entity): 'keep' | 'del' {
    if (!victim || !is_fighter(victim)) return 'del';
    if (
      victim.state === SE.Falling ||
      victim.state === SE.Injured ||
      victim.state === SE.Lying
    ) return 'keep'
    victim.wait += 1;
    return 'keep'
  }
  override on_end(): 'keep' | 'del' {
    this.clear_effects();
    return 'keep';
  }
  override apply(): void {
    for (const vid of this.victims) {
      const victim = this.world.find_entity(vid)
      if (!victim) continue;
      if (victim.state == SE.Injured) continue;
      if (victim.state == SE.Falling) continue;
      this.duration = round_float(this.duration / 2);
    }
  }
}
