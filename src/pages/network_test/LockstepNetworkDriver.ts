import type { IRespTick } from "@/Net";
import type { IRespKeyTick } from "@/Net/IMsg_KeyTick";
import { LFWNetworkDriver } from "./LFWNetworkDriver";

export class LockstepNetworkDriver extends LFWNetworkDriver {
  protected _resp?: IRespTick | IRespKeyTick;

  get lead(): number { return 1; }

  protected on_tick_data(resp: IRespTick | IRespKeyTick): void {
    this._resp = resp;
    this.lf2?.world.awake();
  }

  before_update = () => {
    const { lf2 } = this;
    if (!lf2) return;
    const resp = this._resp;
    this._resp = void 0;
    if (!resp) return lf2.world.sleep();
    this.run_tick(resp.seq!, resp);
  };

  after_update = () => {
    const { lf2 } = this;
    if (!lf2) return;
    if (this.debugging) this._snapshot2?.capture(lf2.world.entities);
    lf2.world.sleep();
  };
}
