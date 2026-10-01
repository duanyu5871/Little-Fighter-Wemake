import { todo, bool, fields, int, str } from "../fields";
import { EntityEnum, type TEntityEnum } from "./EntityEnum";
import type { IBdyInfo } from "./IBdyInfo";
import { entity_info_new, type IEntityInfo } from "./IEntityInfo";
import type { IFrameIndexes } from "./IFrameIndexes";
import type { IFrameInfo } from "./IFrameInfo";
import type { IItrInfo } from "./IItrInfo";
import type { TNextFrame } from "./INextFrame";
export type TItrPrefabs = {
  [x in string]?: IItrInfo;
}
export type TBdyPrefabs = {
  [x in string]?: IBdyInfo;
}
export type TFramePrefabs = {
  [x in string]?: IFrameInfo;
}
export interface IEntityData {
  id: string;
  type: TEntityEnum;
  alias_id?: string;
  base: IEntityInfo;

  on_dead?: TNextFrame;
  on_exhaustion?: TNextFrame;
  indexes?: IFrameIndexes;
  bdy_prefabs?: TBdyPrefabs;
  itr_prefabs?: TItrPrefabs;
  frame_prefabs?: TFramePrefabs;
  pre_hitkeys?: { [x in string]?: TNextFrame; };
  post_hitkeys?: { [x in string]?: TNextFrame; };
  frames: Record<string, IFrameInfo>;

  /**
   * 数据是否已处理
   *
   * 存在processed为false时 
   * 
   * 加载后时会对数据进行额外处理
   * 见函数 preprocess_entity_data
   * 
   * @type {?boolean} 默认值: true
   */
  processed?: boolean;

  __pics?: number;
  __pre_hitkeys_map?: Map<string, TNextFrame>;
  __post_hitkeys_map?: Map<string, TNextFrame>;
}

export function entity_data_new(): IEntityData {
  return {
    id: "",
    type: EntityEnum.Entity,
    frames: {},
    base: entity_info_new()
  }
}
export const entity_data_fields = fields<IEntityData>({
  id: str({ nullable: true }),
  type: int({ nullable: true }),
  alias_id: str({ nullable: true }),
  base: str({ nullable: true }),
  on_dead: todo({ nullable: true }),
  on_exhaustion: todo({ nullable: true }),
  indexes: todo({ nullable: true }),
  bdy_prefabs: todo({ nullable: true }),
  itr_prefabs: todo({ nullable: true }),
  frame_prefabs: todo({ nullable: true }),
  pre_hitkeys: todo({ nullable: true }),
  post_hitkeys: todo({ nullable: true }),
  frames: todo({ nullable: true }),
  processed: bool({ nullable: true }),
  __pics: todo({ nullable: true }),
  __pre_hitkeys_map: todo({ nullable: true }),
  __post_hitkeys_map: todo({ nullable: true }),
})
