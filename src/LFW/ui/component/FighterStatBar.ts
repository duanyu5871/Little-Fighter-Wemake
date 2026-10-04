import { Buff_Healing } from "../../buff/Buff_Healing";
import { Buff_MpHealing } from "../../buff/Buff_MpHealing";
import { Defines, T_E, type IPropsMeta } from "../../defines";
import { Entity, is_bot_ctrl } from "../../entity";
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
  protected hp_flashing: boolean = false;
  protected mp_healing: boolean = false;
  protected dead: boolean = false;
  protected direction: string = '';
  protected _eid: string | undefined;
  protected _head?: string;
  protected _name?: string;
  protected _name_version: number = -1;
  protected _name_outline?: string;
  protected _tier_colors?: string[];

  protected update_hp_tier_colors(tiers: readonly UINode[], flashing: boolean): void {
    if (flashing === this.hp_flashing) return;
    this.hp_flashing = flashing;
    const colors = this._tier_colors ??= tiers.map(v => v.color);
    for (let i = tiers.length - 1; i >= 0; --i) {
      const tier = tiers[i];
      if (!tier.visible) tier.color = colors[i];
      else tiers[i].color = flashing ? 'rgb(255,130,130)' : colors[i];
    }
  }

  set_entity(entity: Entity | undefined) {
    if (this.entity === entity && this._eid == entity?.id) return;
    this._eid = this.entity?.id;
    this.entity = entity
  }
  override on_show(): void {
    this.direction = this.props_holder.str('direction') ?? '';
  }
  update_defend_value(val = this.defend_value.value, max = this.defend_value_max.value) {
    this.update_bar(this.props.defend_value_bar, val, max)
  }
  update_fall_value(val = this.fall_value.value, max = this.fall_value_max.value) {
    this.update_bar(this.props.fall_value_bar, val, max)
  }
  update_toughness(val = this.toughness.value, max = this.toughness_max.value) {
    this.update_bar(this.props.toughness_bar, val, max)
  }
  update_hp(val = this.hp.value): void {
    this.update_tiers(this.props.hp_bar, val);
  }
  update_hp_r(val = this.hp_r.value): void {
    this.update_tiers(this.props.dark_hp_bar, val);
  }
  protected update_tiers(node: UINode | undefined, val: number): void {
    if (!node) return;
    const tiers = node.children;
    if (tiers.length < 2) return;
    for (let i = 0; i < tiers.length; i++) {
      const tier  /**/ = tiers[i];
      const vis = clamp(val - i * 500, 0, 500);
      const visible = !this.dead;
      if (tier.visible != visible) tier.visible = visible;
      this.update_bar(tier, vis, 500)
    }
  }
  update_bar(node: UINode | undefined, val: number = 0, max: number = 0) {
    if (!node) return;
    if (max == 0) { node.set_scale(0); }
    else { node.set_scale(val / max); }
  }
  update_mp(val = this.mp.value, max = this.mp_max.value) {
    this.update_bar(this.props.mp_bar, val, max)
  }
  update_mp_max() {
    const node = this.props.dark_mp_bar;
    if (!node) return;
    node.set_scale(this.dead ? 0 : 1, 1, 1);
  }
  update_head(): void {
    const head = this.entity?.data.base.head ?? '';
    if (head === this._head) return;
    this.props.head_img?.set_src(head)
    this._head = head;
  }
  update_name() {
    const { name_txt } = this.props;
    if (!name_txt) return;
    const { entity } = this;
    // 被电脑接管的玩家（非 COM 槽）仅显示上加 (bot) 后缀，不动玩家名本身，交回后自动恢复
    const ctrl = entity?.ctrl;
    const player = ctrl ? this.lfw.players.get(ctrl.player_id) : void 0;
    const taken_over = !!entity && is_bot_ctrl(ctrl) && !!player && !player.is_com;
    const name0 = (entity?.name.trim() ?? 'NONE') + (taken_over ? '(bot)' : '');
    const name1 = entity?.data.base.name.trim() ?? ''
    let name = name0 || name1;
    if (name0 !== name1 && name0 && name1)
      name = `${name1} [${name0}]`
    const version = name_txt.style.version;
    const outline = name_txt.outlineColor;
    if (name === this._name && version === this._name_version && outline === this._name_outline) return;
    this._name = name;
    this._name_version = version;
    this._name_outline = outline;
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
    this.update_head();
    this.defend_value.update()
    this.defend_value_max.update()
    this.fall_value.update()
    this.fall_value_max.update()
    this.toughness.update()
    this.toughness_max.update()
    this.mp.update()
    this.mp_max.update()
    this.hp.update()
    this.hp_r.update()
    this.hp_max.update()

    const { entity, props: { hp_bar, mp_bar } } = this
    if (!entity) return;

    if (hp_bar) {
      const flashing = !!entity.marks.has(Buff_Healing.KIND) && (entity.lifetime % 8) < 4;
      this.update_hp_tier_colors(hp_bar.children, flashing)
    }
    if (mp_bar) {
      this.mp_healing = !!entity.marks.has(Buff_MpHealing.KIND) && (entity.lifetime % 8) < 4;
      mp_bar.children[0].color = this.mp_healing ? 'rgb(130,130,255)' : 'rgb(0,0,255)'
    }
    this.dead = entity.hp <= 0;
    this.defend_value.target = this.dead ? 0 : entity.defend_value;
    this.defend_value_max.target = this.dead ? 1 : entity.defend_value_max;
    this.fall_value.target = this.dead ? 0 : entity.fall_value;
    this.fall_value_max.target = this.dead ? 1 : entity.fall_value_max;
    this.toughness.target = this.dead ? 0 : entity.toughness;
    this.toughness_max.target = this.dead ? 1 : entity.toughness_max;
    this.mp.target = this.dead ? 0 : entity.mp;
    this.mp_max.target = this.dead ? 1 : entity.mp_max;
    this.hp.target = this.dead ? 0 : entity.hp;
    this.hp_r.target = this.dead ? 0 : entity.hp_r;
    this.hp_max.target = this.dead ? 1 : entity.hp_max;
  }

}
