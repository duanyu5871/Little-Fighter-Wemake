import { GameKey as GK, type IValGetter, type IValGetterGetter } from "../defines";
import { S_Val } from "../defines/StageVal";
import { Stage } from "../stage";
import { get_val_from_world } from "./get_val_from_world";

export const stage_world_val_getters = new Map<string, undefined | IValGetter<Stage>>();
export const stage_val_getters: Record<S_Val, IValGetter<Stage>> = {
  [S_Val.EnemiesCleared]: (e) => e.all_fighter_dead() ? 1 : 0,
  [S_Val.DialogCleared]: (e) => e.dialog_cleared() ? 1 : 0,
  [S_Val.CurPhaseTime]: (e) => e.phase_time,
  [S_Val.CurDialogTime]: (e) => e.dialog_time,
  [S_Val.PressAttack] /**/: (e) => e.lfw.keys[GK.a].is_start() ? 1 : 0,
  [S_Val.PressJump]   /**/: (e) => e.lfw.keys[GK.j].is_start() ? 1 : 0,
  [S_Val.PressDefend] /**/: (e) => e.lfw.keys[GK.d].is_start() ? 1 : 0,
  [S_Val.PressUp]     /**/: (e) => e.lfw.keys[GK.U].is_start() ? 1 : 0,
  [S_Val.PressDown]   /**/: (e) => e.lfw.keys[GK.D].is_start() ? 1 : 0,
  [S_Val.PressLeft]   /**/: (e) => e.lfw.keys[GK.L].is_start() ? 1 : 0,
  [S_Val.PressRight]  /**/: (e) => e.lfw.keys[GK.R].is_start() ? 1 : 0,
  [S_Val.Broadcast]: (e) => e.lfw.broadcasts,
}

export const get_val_getter_from_stage: IValGetterGetter<Stage> = (word: string): IValGetter<Stage> | undefined => {
  const val_getter = stage_val_getters[word as S_Val];
  if (val_getter) return val_getter;

  const world_val_getter = get_val_from_world(word);
  if (!world_val_getter) {
    stage_world_val_getters.set(word, world_val_getter);
    return void 0;
  }
  const fallback: IValGetter<Stage> = (e, ...arg) => world_val_getter(e.world, ...arg);
  stage_world_val_getters.set(word, fallback);
  return fallback;
};
