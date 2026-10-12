import type { LFW } from "..";
import type { IBdyInfo, IEntityData, IFrameInfo, IItrInfo, IOpointInfo } from "../defines";

export interface IEntityDataContext {
  lfw: LFW;
  data: IEntityData;
  jobs: Promise<any>[];
  errors: string[];
}
export interface IFrameInfoContext extends IEntityDataContext {
  frame: IFrameInfo;
}
export interface IBdyInfoContext extends IEntityDataContext {
  index?: number;
  frame?: IFrameInfo;
  bdy: IBdyInfo;
}
export interface IItrInfoContext extends IEntityDataContext {
  index?: number;
  frame?: IFrameInfo;
  itr: IItrInfo;
}

export interface IOpointInfoContext extends IEntityDataContext {
  index?: number;
  frame?: IFrameInfo;
  opoint: IOpointInfo;
}
