import type { IReq, IResp } from './_Base';
import type { TRejoinTick } from './IMsg_Rejoin';
import type { MsgEnum } from './MsgEnum';

/** 重连归队后分批拉取要回放的帧（一次一包，避免消息过大） */
export interface IReqRejoinFrames extends IReq<MsgEnum.RejoinFrames> {
  /** 从第几帧开始取 */
  from_seq?: number;
  /** 本次最多取多少帧，缺省由服务端决定 */
  count?: number;
}
export interface IRespRejoinFrames extends IResp<MsgEnum.RejoinFrames> {
  /** 本包起始帧号 */
  from_seq?: number;
  /** 服务端当前最新帧号（追到它就追平了） */
  next_seq?: number;
  /** 是否已经发完所有需要回放的帧 */
  done?: boolean;
  resps?: TRejoinTick[];
}
