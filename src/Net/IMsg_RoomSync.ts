import type { IReq, IResp } from "./_Base";
import type { IRoomInfo } from "./IRoomInfo";
import type { MsgEnum } from "./MsgEnum";
import type { RoomSyncMode } from "./net_sync";

export interface IReqRoomSync extends IReq<MsgEnum.RoomSync> {
  sync_mode?: RoomSyncMode;
}
export interface IRespRoomSync extends IResp<MsgEnum.RoomSync> {
  room?: IRoomInfo;
}
