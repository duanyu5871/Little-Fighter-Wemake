import type { IReq, IResp } from './_Base';
import type { MsgEnum } from './MsgEnum';

export interface IReqRoomContinue extends IReq<MsgEnum.RoomContinue> {
}
export interface IRespRoomContinue extends IResp<MsgEnum.RoomContinue> {
}
