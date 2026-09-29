import type { IReq, IResp } from './_Base';
import type { IClientInfo } from './IClientInfo';
import type { IRespTick } from './IMsg_Tick';
import type { IRoomInfo } from './IRoomInfo';
import type { MsgEnum } from './MsgEnum';

export type TRejoinTick = Omit<IRespTick, 'pid' | 'type' | 'is_resp' | 'is_req'>;

export interface IReqRejoin extends IReq<MsgEnum.Rejoin> {
  roomid?: string;
  client_id?: string;
  secret?: string;
  from_seq?: number;
}
export interface IRespRejoin extends IResp<MsgEnum.Rejoin> {
  client?: IClientInfo;
  room?: IRoomInfo;
  next_seq?: number;
  resps?: TRejoinTick[];
}
