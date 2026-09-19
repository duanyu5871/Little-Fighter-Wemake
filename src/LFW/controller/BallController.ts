
import { ChaseStrategy, EMPTY_FRAME_INFO, FID, FrameBehavior, GK, type IChaseInfo, type IFrameInfo, type IVector3 } from "../defines";
import { ChaseLost } from "../defines/ChaseLost";
import type { Entity } from "../entity/Entity";
import { closer_one, manhattan_xz } from "../helper";
import { is_f_num, round_float } from "../utils";
import { BaseController } from "./BaseController";
import type { ControllerResult } from "./ControllerResult";
const { L, R, U, D, j, d } = GK
export class BallController extends BaseController {
  readonly __is_ball_ctrl__ = true;
  chasing: Entity | null = null;
  chase_point: IVector3 = this.entity.position.clone();
  frame: IFrameInfo = EMPTY_FRAME_INFO;
  gave_up = false;
  dir_x: 0 | 1 | -1 = 0;
  dir_y: 0 | 1 | -1 = 0;
  dir_z: 0 | 1 | -1 = 0;
  leave_dir: 0 | 1 | -1 = 0;
  
  set_chase_point(x: number, y: number, z: number) {
    if (is_f_num(x) || is_f_num(y) || is_f_num(z)) debugger;
    this.chase_point.set(
      round_float(x),
      round_float(y),
      round_float(z)
    )
  }

  aim_at(e: Entity, oy: number = 0) {
    const { x, y, z } = e.position;
    this.set_chase_point(x, y + e.frame.height * oy, z);
  }

  update_lookup(me: number, entities: Entity[]): void {
    const { chase } = this.entity.frame;
    if (!chase) return;
    const { stratedy } = chase;
    if (stratedy === ChaseStrategy.StopOnLost && this.gave_up) return;

    const current = this.chasing;
    const still_valid = this.should_chase(current) ? current : this.chasing = null;
    if (current && (stratedy === ChaseStrategy.UntilLost || (still_valid && stratedy === ChaseStrategy.StopOnLost))) {
      this.aim_at(current);
      return;
    }
    if (current && stratedy === ChaseStrategy.StopOnLost) {
      this.gave_up = true;
      this.stop_chasing();
      return;
    }
    if (stratedy === ChaseStrategy.Default)
      this.chasing = null;

    const self = this.entity;
    const x0 = self.position.x;
    let i1 = me - 1;
    let i2 = me + 1;
    let found: Entity | null = null;
    let found_d = Infinity;

    let e: Entity | null = null;
    do {
      let l: Entity | undefined = entities[i1];
      let r: Entity | undefined = entities[i2];
      if (found) {
        if (l && x0 - l.position.x >= found_d) l = void 0;
        if (r && r.position.x - x0 >= found_d) r = void 0;
      }
      e = closer_one(self, l, r);
      if (!e) break;
      if (!e.ghosted && this.should_chase(e)) {
        const d = manhattan_xz(self, e);
        if (d < found_d) {
          found = e;
          found_d = d;
        }
      }
      if (l === e) --i1;
      if (r === e) ++i2;
    } while (e)

    if (!found) return;
    this.chasing = found;
    this.aim_at(found);
  }

  should_chase(other: Entity | null): boolean {
    if (!other) return false;
    if (
      other.frame.id === FID.Gone ||
      other.frame.id === FID.None
    ) return false;
    const { chase } = this.entity.frame;
    if (!chase) return false;
    const { flag } = chase
    const target = other.get_flag(this.entity)
    return (target & flag) == target
  }

  override update(): ControllerResult {
    const { frame, facing, hp } = this.entity;
    const { chase, behavior } = frame;

    if (hp > 0 && this.frame != frame) {
      this.gave_up = false;
      if (this.frame.chase && !chase) {
        this.stop_chasing()
        this.chase_point.copy(this.entity.position);
      }
    } else if (hp <= 0 && chase) {
      this.stop_chasing()
    }

    if (behavior === FrameBehavior.JohnBiscuitLeaving) {
      const p1 = this.entity.position;
      this.key_down(facing < 0 ? L : R).key_up(facing < 0 ? R : L, U, D);
      if (p1.y > 40) this.key_down(d).key_up(j);
      else if (p1.y < 40) this.key_down(j).key_up(d);
      else this.key_up(j, d);
    }
    if (chase) this.update_chasing(chase)
    this.frame = frame;
    return super.update();
  }
  update_chasing(chase: IChaseInfo) {
    const { chasing } = this;
    const { facing, hp } = this.entity;

    const me = this.entity.position;

    if (chasing) {
      const { oy = 0.5 } = chase;
      this.aim_at(chasing, oy);
    }

    const { x, y, z } = this.chase_point;

    if (hp > 0 && (this.chasing || (chase.lost & ChaseLost.Hover))) {
      const over_x = chase.overshoot?.x ?? 0;
      const over_y = chase.overshoot?.y ?? 0;
      const over_z = chase.overshoot?.z ?? 0;

      const dx = x - me.x;
      this.dir_x = this.calc_dir(dx, over_x, this.dir_x);
      if (this.dir_x > 0) this.key_down(R).key_up(L)
      else if (this.dir_x < 0) this.key_down(L).key_up(R)
      else this.key_up(L, R)

      const dz = z - me.z;
      this.dir_z = this.calc_dir(dz, over_z, this.dir_z);
      if (this.dir_z > 0) this.key_down(D).key_up(U)
      else if (this.dir_z < 0) this.key_down(U).key_up(D)
      else this.key_up(U, D)

      const dy = y - me.y;
      this.dir_y = this.calc_dir(dy, over_y, this.dir_y);
      if (this.dir_y > 0) this.key_down(j).key_up(d)
      else if (this.dir_y < 0) this.key_down(d).key_up(j)
      else this.key_up(j, d)
    } else {
      if (!this.leave_dir)
        this.leave_dir = facing < 0 ? -1 : 1;
      if (this.leave_dir < 0) this.key_down(L).key_up(R, U, D)
      else this.key_down(R).key_up(L, U, D)

      const dy = y - me.y;
      if (dy < 0) this.key_down(d).key_up(j)
      else if (dy > 0) this.key_down(j).key_up(d)
      else this.key_up(j, d)
    }
    this.chasing = chasing;
  }
  calc_dir(delta: number, over: number, prev: 0 | 1 | -1): 0 | 1 | -1 {
    let dir = prev;
    if (!dir && delta) dir = delta > 0 ? 1 : -1;
    if (dir > 0 && delta < -over) dir = -1;
    else if (dir < 0 && delta > over) dir = 1;
    return dir;
  }

  /** 停止追击：清空追击实体，目标点保留 */
  stop_chasing() {
    this.dir_x = this.dir_y = this.dir_z = 0;
    this.chasing = null;
  }
}