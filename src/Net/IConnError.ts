import type { ErrCode } from "./ErrCode";
import type { MsgEnum } from "./MsgEnum";

export interface IConnError extends Error {
  lfw: {
    type: MsgEnum | string;
    code: ErrCode | number;
    error: string;
  }
};
