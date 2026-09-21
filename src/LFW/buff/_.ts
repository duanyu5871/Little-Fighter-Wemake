import { Factory } from "../Factory";
import { Buff_Electrify } from "./Buff_Electrify";
import { Buff_Electroshock } from "./Buff_Electroshock";
import { Buff_GroupAttack } from "./Buff_GroupAttack";
import { Buff_Healing } from "./Buff_Healing";
import { Buff_MagicFlute } from "./Buff_MagicFlute";
import { Buff_MagicFlute2 } from "./Buff_MagicFlute2";
import { Buff_MpHealing } from "./Buff_MpHealing";

let _registed = false
export function regist_buffs() {
  if (_registed) return;
  _registed = true;
  Factory.register_buff(Buff_MagicFlute)
  Factory.register_buff(Buff_MagicFlute2)
  Factory.register_buff(Buff_Electroshock)
  Factory.register_buff(Buff_GroupAttack)
  Factory.register_buff(Buff_Healing)
  Factory.register_buff(Buff_MpHealing)
  Factory.register_buff(Buff_Electrify)
}