import type { INextFrame } from "../defines";
import { EntityVal as EV, } from "../defines/EntityVal";
import { FacingFlag as FF } from "../defines/FacingFlag";
import type { IFrameInfo } from "../defines/IFrameInfo";
import { WeaponEnum as WT } from "../defines/WeaponType";
import { assign } from "../utils/container_help/assign";
import { CondMaker } from "./CondMaker";
export function hit_next_frame_drink(): INextFrame[] {
  return [{
    id: "55",
    desc: 'drink',
    mp_mode: 1,
    expression: new CondMaker<EV>()
      .one_of(EV.Holding_W_Type, WT.Drink)
      .done(),
  }];
}
export function hit_next_frame_super_punch(): INextFrame[] {
  return [{
    id: "70",
    mp_mode: 1,
    facing: FF.Ctrl,
    desc: "super_punch",
    expression: new CondMaker<EV>()
      .add(EV.RequireSuperPunch, ">", 0 )
      .done(),
  }]
}
export function hit_next_frame_punch(): INextFrame[] {
  return [{
    mp_mode: 1,
    id: ["60", "65"],
    facing: FF.Ctrl,
    desc: "punch",
  }]
}
export function hit_next_frame_turn_back(frame: IFrameInfo, back_frame?: string) {
  if (back_frame == void 0) {
    frame.facing = FF.Ctrl;
    return;
  }
  frame.key_down = assign(frame.key_down, {
    B: {
      id: back_frame,
      wait: "i",
      facing: FF.Backward
    }
  });
  frame.hit = assign(frame.hit, {
    B: {
      id: back_frame,
      wait: "i",
      facing: FF.Backward
    }
  });
}
export function hit_next_frame_jump(): INextFrame[] {
  return [{
    id: "210",
    facing: FF.Ctrl,
    mp_mode: 1,
  }]
}
export function hit_next_frame_defend(): INextFrame[] {
  return [{
    id: "110",
    facing: FF.Ctrl,
    mp_mode: 1,
  }]
}
export function hit_next_frame_weapon_atk(): INextFrame[] {
  return [{
    mp_mode: 1,
    id: "45", facing: FF.Ctrl,
    expression: new CondMaker<EV>()
      .add(EV.Holding_W_Type, "==", WT.Baseball)
      .or((v) => v
        .add(EV.Holding_W_Type, "==", WT.Knife)
        .and(EV.PressFB, "!=", 0),
      ).done(),
  }, {
    mp_mode: 1,
    id: ["20", "25"], facing: FF.Ctrl,
    expression: new CondMaker<EV>()
      .one_of(EV.Holding_W_Type, WT.Knife, WT.Stick)
      .done(),
  }]
}
export function hit_next_frame_jump_atk(): INextFrame[] {
  return [{
    id: "52", // 角色跳跃丢出武器
    facing: FF.Ctrl,
    desc: "空中丢出武器",
    mp_mode: 1,
    expression: new CondMaker<EV>()
      .one_of(
        EV.Holding_W_Type,
        WT.Baseball,
        WT.Drink
      )
      .or((v) => v
        .add(EV.PressFB, "!=", 0)
        .and(EV.Holding_W_Type, "!=", WT.None)
      )
      .done(),
  }, {
    id: "30", // 角色跳跃用武器攻击
    facing: FF.Ctrl,
    desc: "空中武器攻击",
    mp_mode: 1,
    expression: new CondMaker<EV>()
      .one_of(
        EV.Holding_W_Type,
        WT.Knife,
        WT.Stick
      )
      .done(),
  }, {
    id: "80", // 角色跳跃攻击
    desc: "跳跃攻击",
    facing: FF.Ctrl,
  }]
}

