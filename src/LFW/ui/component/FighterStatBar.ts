import { Buff_Healing } from "../../buff/Buff_Healing";
import { Buff_MpHealing } from "../../buff/Buff_MpHealing";
import { Defines, T_E, type IPropsMeta } from "../../defines";
import { Entity, type IEntityCallbacks } from "../../entity";
import { clamp } from "../../utils/math/clamp";
import { UINode } from "../UINode";
import { Picture } from "./Picture";
import { SmoothNumber } from "./SmoothNumber";
import { UIComponent } from "./UIComponent";


interface IFighterStatBarProps {
  dark_hp_bar?: UINode
  hp_bar?: UINode;
  dark_mp_bar?: UINode;
  mp_bar?: UINode;
  fall_value_bar?: UINode;
  defend_value_bar?: UINode;
  toughness_bar?: UINode;
  head_img?: Picture;
  name_txt?: UINode;
}
export class FighterStatBar extends UIComponent<IFighterStatBarProps> {
  static override readonly TAGS: string[] = ["FighterStatBar"];
  static override readonly PROPS: IPropsMeta<IFighterStatBarProps> = {
    dark_hp_bar: UINode,
    hp_bar: UINode,
    dark_mp_bar: UINode,
    mp_bar: UINode,
    fall_value_bar: UINode,
    defend_value_bar: UINode,
    toughness_bar: UINode,
    head_img: Picture,
    name_txt: UINode,
  };
  protected entity?: Entity;
  protected defend_value_max = new SmoothNumber().handler(() => this.update_defend_value())
  protected defend_value = new SmoothNumber().handler(() => this.update_defend_value())
  protected fall_value_max = new SmoothNumber().handler(() => this.update_fall_value())
  protected fall_value = new SmoothNumber().handler(() => this.update_fall_value())
  protected toughness = new SmoothNumber().handler(() => this.update_toughness())
  protected toughness_max = new SmoothNumber().handler(() => this.update_toughness())
  protected hp_max = new SmoothNumber().handler(() => { this.update_hp(); this.update_hp_r(); })
  protected hp_r = new SmoothNumber().handler(() => this.update_hp_r())
  protected hp = new SmoothNumber().handler(() => this.update_hp())
  protected mp_max = new SmoothNumber().handler(() => { this.update_mp_max(); this.update_mp() })
  protected mp = new SmoothNumber().handler(() => this.update_mp())
  protected healing: boolean = false;
  protected mp_healing: boolean = false;
  protected dead: boolean = false;
  protected cbs: IEntityCallbacks = {
    on_hp_changed: (_, v) => { this.update_hp_target(v); },
    on_hp_max_changed: (_, v) => { this.hp_max.target = this.dead ? 0 : v; },
    on_hp_r_changed: (_, v) => { this.hp_r.target = this.dead ? 0 : v; },
    on_mp_max_changed: (_, v) => { this.mp_max.target = this.dead ? 0 : v; },
    on_mp_changed: (_, v) => { this.mp.target = this.dead ? 0 : v; },
    on_defend_value_max_changed: (_, v) => { this.defend_value_max.target = this.dead ? 0 : v; },
    on_defend_value_changed: (_, v) => { this.defend_value.target = this.dead ? 0 : v; },
    on_fall_value_max_changed: (_, v) => { this.fall_value_max.target = this.dead ? 0 : v; },
    on_fall_value_changed: (_, v) => { this.fall_value.target = this.dead ? 0 : v; },
    on_toughness_max_changed: (_, v) => { this.toughness_max.target = this.dead ? 0 : v; },
    on_toughness_changed: (_, v) => { this.toughness.target = this.dead ? 0 : v; },
    on_data_changed: () => this.update_head()
  }
  protected direction: string = '';
  private _eid: string | undefined;
  update_hp_target(v: number) {
    if (v <= 0) {
      this.dead = true;
      this.hp_max.target           /**/ = 0
      this.hp_r.target             /**/ = 0
      this.mp_max.target           /**/ = 0
      this.mp.target               /**/ = 0
      this.defend_value_max.target /**/ = 0
      this.defend_value.target     /**/ = 0
      this.fall_value_max.target   /**/ = 0
      this.fall_value.target       /**/ = 0
      this.toughness_max.target    /**/ = 0
      this.toughness.target        /**/ = 0
      this.update_bars();
    } else if (this.dead) {
      this.dead = false;
      const { entity } = this;
      if (entity) {
        this.hp_max.target           /**/ = entity.hp_max
        this.hp_r.target             /**/ = entity.hp_r
        this.mp_max.target           /**/ = entity.mp_max
        this.mp.target               /**/ = entity.mp
        this.defend_value_max.target /**/ = entity.defend_value_max
        this.defend_value.target     /**/ = entity.defend_value
        this.fall_value_max.target   /**/ = entity.fall_value_max
        this.fall_value.target       /**/ = entity.fall_value
        this.toughness_max.target    /**/ = entity.toughness_max || 1
        this.toughness.target        /**/ = entity.toughness;
      }
      this.update_bars();
    }
    this.hp.target = v;
  }
  set_entity(entity: Entity | undefined) {
    if (this.entity === entity && this._eid == entity?.id) return;
    if (this.entity) this.entity.callbacks.del(this.cbs)

    this._eid = this.entity?.id;
    this.entity = entity
    if (entity) {
      this.hp_max.target           /**/ = entity.hp_max
      this.hp_r.target             /**/ = entity.hp_r
      this.mp_max.target           /**/ = entity.mp_max
      this.mp.target               /**/ = entity.mp
      this.defend_value_max.target /**/ = entity.defend_value_max
      this.defend_value.target     /**/ = entity.defend_value
      this.fall_value_max.target   /**/ = entity.fall_value_max
      this.fall_value.target       /**/ = entity.fall_value
      this.toughness_max.target    /**/ = entity.toughness_max || 1
      this.toughness.target        /**/ = entity.toughness;
      this.update_hp_target(entity.hp);
      this.update_bars();
      entity.callbacks.add(this.cbs)
    }
    this.update_head();
  }
  override on_show(): void {
    this.direction = this.props_holder.str('direction') ?? ''
  }
  protected update_bars() {
    this.update_defend_value();
    this.update_mp();
    this.update_mp_max();
    this.update_fall_value();
    this.update_toughness();
    this.update_hp_r();
    this.update_hp();
  }
  update_defend_value(val = this.defend_value.value, max = this.defend_value_max.value) {
    const node = this.props.defend_value_bar;
    if (!node) return;
    if (this.dead) { node.set_scale(0, 1, 1); return; }
    if (max === 0) return;
    node.set_scale(val / max, 1, 1);
  }
  update_fall_value(val = this.fall_value.value, max = this.fall_value_max.value) {
    const node = this.props.fall_value_bar;
    if (!node) return;
    if (this.dead) { node.set_scale(0, 1, 1); return; }
    if (max === 0) return;
    node.set_scale(val / max, 1, 1)
  }
  update_toughness(val = this.toughness.value, max = this.toughness_max.value) {
    const node = this.props.toughness_bar;
    if (!node) return;
    if (this.dead) { node.set_scale(0, 1, 1); return; }
    if (max === 0) return;
    node.set_scale(val / max, 1, 1);
  }
  protected _tier_colors?: string[];
  protected _tier_flashing?: boolean;
  protected update_hp_tier_colors(node: UINode): void {
    if (this._tier_flashing === this.healing) return;
    this._tier_flashing = this.healing;
    const tiers = node.children;
    const colors = this._tier_colors ??= tiers.map(v => v.color);
    for (let i = 0; i < tiers.length; i++)
      tiers[i].color = this.healing ? 'rgb(255,130,130)' : colors[i];
  }
  update_hp(val = this.hp.value, max = this.hp_max.value): void {
    this.update_tiers(this.props.hp_bar, val, max);
  }
  update_hp_r(val = this.hp_r.value, max = this.hp_max.value): void {
    this.update_tiers(this.props.dark_hp_bar, val, max);
  }
  protected update_tiers(node: UINode | undefined, val: number, max: number): void {
    if (!node) return;
    const tiers = node.children;
    if (tiers.length < 2) return;
    for (let i = 0; i < tiers.length; i++) {
      const tier  /**/ = tiers[i];
      const vis = clamp(val - i * 500, 0, 500);
      const inner_w = tier.w;
      const visible = !this.dead && max > 0 && vis > 0 && inner_w > 0;
      if (tier.visible != visible) tier.visible = visible;
      if (!visible) continue;
      tier.set_scale(vis / 500, 1, 1);
    }
  }
  update_mp(val = this.mp.value, max = this.mp_max.value) {
    const node = this.props.mp_bar;
    if (!node) return;
    if (this.dead) { node.set_scale(0, 1, 1); return; }
    if (max === 0) return;
    node.set_scale(val / max, 1, 1);
  }
  update_mp_max() {
    const node = this.props.dark_mp_bar;
    if (!node) return;
    node.set_scale(this.dead ? 0 : 1, 1, 1);
  }
  update_head(): void {
    const { entity } = this;
    if (entity) {
      const { head } = entity.data.base;
      this.props.head_img?.set_src(typeof head === 'string' ? head : '')
    } else {
      this.props.head_img?.set_src('')
    }
  }
  protected _last_name?: string;
  protected _last_name_version: number = -1;
  protected _last_name_outline?: string;
  update_name() {
    const { name_txt } = this.props;
    if (!name_txt) return;
    const name0 = this.entity?.name.trim() ?? 'NONE'
    const name1 = this.entity?.data.base.name.trim() ?? ''
    let name = name0 || name1;
    if (name0 !== name1 && name0 && name1)
      name = `${name1} (${name0})`
    const version = name_txt.style.version;
    const outline = name_txt.outlineColor;
    if (name === this._last_name && version === this._last_name_version && outline === this._last_name_outline) return;
    this._last_name = name;
    this._last_name_version = version;
    this._last_name_outline = outline;
    name_txt.set_text(name)
  }
  update_team() {
    const { name_txt } = this.props;
    if (!name_txt) return;
    const team = this.entity?.team ?? 0;
    const { txt_color, txt_outline_color } = Defines.TeamInfoMap[team] || Defines.TeamInfoMap[T_E.Independent]
    if (name_txt.outlineColor === txt_outline_color && name_txt.style.fill_style === txt_color) return;
    name_txt.outlineColor = txt_outline_color;
    name_txt.style.fill_style = txt_color;
    name_txt.style.touch();
  }
  override update(): void {
    this.update_team();
    this.update_name();
    const { entity, props: { hp_bar, mp_bar } } = this
    if (hp_bar) {
      this.healing = !!entity?.marks.has(Buff_Healing.KIND) && (entity.lifetime % 8) < 4;
      this.update_hp_tier_colors(hp_bar)
    }
    if (mp_bar) {
      this.mp_healing = !!entity?.marks.has(Buff_MpHealing.KIND) && (entity.lifetime % 8) < 4;
      mp_bar.children[0].color = this.mp_healing ? 'rgb(130,130,255)' : 'rgb(0,0,255)'
    }
    this.defend_value_max.update()
    this.defend_value.update()
    this.fall_value_max.update()
    this.fall_value.update()
    this.toughness.update()
    this.toughness_max.update()
    this.hp_max.update()
    this.hp_r.update()
    this.hp.update()
    this.mp_max.update()
    this.mp.update()
  }
}
