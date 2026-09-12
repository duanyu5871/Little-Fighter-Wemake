import type { IReq, IResp } from "./_Base";
import type { MsgEnum } from "./MsgEnum";

export type NetSyncMode = 'lockstep' | 'delay';

export interface IReqRoomStart extends IReq<MsgEnum.RoomStart> { }
export interface IRespRoomStart extends IResp<MsgEnum.RoomStart> {
  seed?: number;
  sync_mode?: NetSyncMode;
  input_delay?: number;
}

