import type { IEntityData } from "../defines";
import type { IBuffRenderer } from "../ditto/render/IBuffRenderer";
import type { Entity } from "../entity";
import type { LFW } from "../LFW";
import { Times } from "../utils/Times";
import { World } from "../World";
import type { IBuffSnapshot } from "./IBuffSnapshot";

export abstract class Buff {
  static readonly KIND: string | number = '';
  static readonly GROUPS: string[] = [];
  readonly lfw: LFW;
  readonly world: World;
  protected _id: string;
  readonly kind: string | number;
  protected _attacker: Entity | undefined;
  protected _attacker_id: string = '';
  level: number = 0;
  protected _mounted = false;
  renderer?: IBuffRenderer;
  /** 
   * 此BUFF需处理的victims
   */
  protected readonly _victims: string[] = [];
  protected readonly _ticker = new Times();
  protected readonly _lifetime = new Times(0, 1).set_lifes(1);
  protected readonly _effects = new Map<string, Entity>();
  protected _effect_data?: IEntityData;

  get id(): string { return this._id }
  get victims(): ReadonlyArray<string> { return this._victims; }
  get dead() { return this._lifetime.remains == 0 }
  get lifetime() { return this._lifetime.value }
  set lifetime(v) { this._lifetime.value = v }
  get duration() { return this._lifetime.max }
  set duration(v) { this._lifetime.max = v }
  get ticks() { return this._ticker.max }
  set ticks(v) { this._ticker.max = v }
  get attacter(): Entity | undefined {
    if (!this._attacker_id)
      return this._attacker = void 0;
    if (this._attacker?.id == this._attacker_id)
      return this._attacker = this.world.find_entity(this._attacker_id);
    return this._attacker;
  }
  constructor(lfw: LFW, id: string, kind: string | number) {
    this.lfw = lfw;
    this.kind = kind
    this.world = lfw.world;
    this._id = id;
  }
  init(): void { };
  reset(id: string): this {
    this.clear_effects();
    const prev = this._id;
    if (prev !== id)
      for (const vid of this._victims)
        this.world.find_entity(vid)?.buffs.delete(prev);
    this._id = id;
    this._attacker = void 0;
    this._attacker_id = '';
    this.level = 0;
    this._mounted = false;
    this.renderer = void 0;
    this._victims.length = 0;
    this._ticker.reborn();
    this._lifetime.set_range(0, 1).set_lifes(1);
    return this;
  }
  on_tick?(attacker?: Entity, victim?: Entity): void;
  on_update?(attacker?: Entity, victim?: Entity): void;
  on_end?(attacker?: Entity, victim?: Entity): void;
  set_attacker(attacker: string | Entity) {
    if (typeof attacker === 'string') {
      this._attacker = this.world.find_entity(attacker);
      this._attacker_id = attacker;
    } else {
      this._attacker = attacker;
      this._attacker_id = attacker.id;
    }
  }
  set_victim(victim: Entity): this {
    for (const vid of this._victims)
      this.world.find_entity(vid)?.buffs.set(this.id, this);
    this._victims.length = 0;
    return this.add_victim(victim);
  }
  add_victim(victim: Entity): this {
    if (this._victims.includes(victim.id)) return this;
    this._victims.push(victim.id);
    victim.buffs.set(this.id, this);
    return this;
  }
  del_victim(victim: Entity): this {
    this.del_effect(victim.id);
    this._del(victim.id);
    victim.buffs.delete(this.id);
    return this;
  }

  /** 特效实体使用的数据 oid，空字符串 = 不使用特效实体 */
  protected get effect_oid(): string { return ''; }
  /** 特效实体进入的帧ID，由子类选择同一数据里的"哪个"特效 */
  protected get effect_frame_id(): string { return "0"; }
  protected effect_data(): IEntityData | undefined {
    const oid = this.effect_oid;
    if (!oid) return void 0;
    return this._effect_data ??= this.lfw.datas.find(oid);
  }
  /** 特效实体的位置（缺省 = 受击者位置） */
  protected effect_anchor(victim: Entity): [number, number, number] {
    return [victim.position.x, victim.position.y, victim.position.z];
  }
  protected del_effect(vid: string): void {
    const effect = this._effects.get(vid);
    if (!effect) return;
    if (this.world.find_entity(effect.id)) this.world.del_entity(effect);
    this._effects.delete(vid);
  }
  protected clear_effects(): void {
    for (const [, effect] of this._effects)
      if (this.world.find_entity(effect.id)) this.world.del_entity(effect);
    this._effects.clear();
  }
  /** 为受击者生成/跟随后特效实体（每个受击者一个，跟随受击者） */
  protected show_effect(victim: Entity): void {
    let effect = this._effects.get(victim.id);
    if (effect && !this.world.find_entity(effect.id)) effect = void 0;
    if (!effect) {
      const data = this.effect_data();
      if (!data) return;
      effect = this.lfw.factory.create_entity(this.world, data);
      if (!effect) return;
      effect.outline_alpha = 0;
      effect.outline_width = 0;
      effect.outline_color = '';
      effect.set_position(...this.effect_anchor(victim));
      effect.enter_frame_by_id(this.effect_frame_id);
      effect.attach(true);
      this._effects.set(victim.id, effect);
    }
    effect.set_position(...this.effect_anchor(victim));
  }
  protected update_effects(): void {
    if (!this._effects.size && !this.effect_oid) return;
    for (const [vid, effect] of this._effects) {
      if (
        this._victims.includes(vid) &&
        this.world.find_entity(vid) &&
        this.world.find_entity(effect.id)
      ) continue;
      if (this.world.find_entity(effect.id)) this.world.del_entity(effect);
      this._effects.delete(vid);
    }
    for (const vid of this._victims) {
      const victim = this.world.find_entity(vid);
      if (victim) this.show_effect(victim);
    }
  }

  private _del(id: string): boolean {
    let fast = 0, slow = 0
    let len = this._victims.length
    for (; fast < len; ++fast) {
      if (slow < fast) this._victims[slow] = this._victims[fast]
      if (this._victims[fast] === id) continue;
      ++slow;
    }
    this._victims.length = slow;
    return slow !== fast
  }


  update(d: number) {
    const { on_update, on_tick, on_end } = this;
    let attacker: Entity | undefined | 0 = 0
    if (on_update) {
      if (attacker === 0) attacker = this.attacter;
      this.loop(on_update, attacker);
    }
    if (this._ticker.add(d) && on_tick) {
      if (attacker === 0) attacker = this.attacter;
      this.loop(on_tick, attacker);
    }
    if (this._lifetime.add() && on_end) {
      if (attacker === 0) attacker = this.attacter;
      this.loop(on_end, attacker);
    }
    this.update_effects();
  }

  private loop(fn: (attacker?: Entity, victim?: Entity) => void, attacker: Entity | undefined) {
    for (const vid of this._victims)
      fn.call(this, attacker, this.world.find_entity(vid));
  }

  to_snapshot(): IBuffSnapshot {
    return {
      attacker_id: this._attacker_id,
      level: this.level,
      mounted: this._mounted,
      victims: [...this._victims],
      ticker: this._ticker.to_snapshot(),
      lifetime: this._lifetime.to_snapshot(),
    };
  }
  read_snapshot(s: IBuffSnapshot): this {
    this._attacker_id = s.attacker_id;
    this._attacker = s.attacker_id ? this.world.find_entity(s.attacker_id) : void 0;
    this.level = s.level;
    this._mounted = s.mounted;
    this._victims.length = 0;
    this._victims.push(...s.victims);
    this._ticker.read_snapshot(s.ticker);
    this._lifetime.read_snapshot(s.lifetime);
    return this;
  }
  /** pls super.mount() */
  mount(): void {
    if (this._mounted) return;
    this._mounted = true;
    this.world.buffs.set(this.id, this);
  }
  unmount() {
    if (!this._mounted) return;
    this._mounted = false;
    while (this._victims.length) {
      const vid = this._victims[0];
      this.del_effect(vid);
      this._del(vid);
      this.world.find_entity(vid)?.buffs.delete(this.id);
    }
  }
}