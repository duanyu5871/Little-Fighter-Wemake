import type { Entity } from "../entity/Entity";
import type { PlayerInfo } from "../PlayerInfo";
import { BaseController } from "./BaseController";

export class LocalController
  extends BaseController {
  readonly __is_human_ctrl__ = true;
  override player: PlayerInfo;
  constructor(player_id: string, entity: Entity) {
    super(player_id, entity);
    this.player = this.lfw.player(player_id)
  }
  override reset(player_id: string, entity: Entity): void {
    super.reset(player_id, entity);
    this.player = this.lfw.player(player_id)
  }
}
