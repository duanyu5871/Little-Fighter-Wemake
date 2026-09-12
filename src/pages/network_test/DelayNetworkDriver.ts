import { MsgEnum, type IRespTick } from "@/Net";
import type { IRespKeyTick } from "@/Net/IMsg_KeyTick";
import { LFWNetworkDriver } from "./LFWNetworkDriver";

export class DelayNetworkDriver extends LFWNetworkDriver {
  readonly input_delay: number;
  protected _seq: number = 0;
  protected _starved: boolean = false;
  protected readonly _inputs = new Map<number, IRespTick | IRespKeyTick>();

  constructor(input_delay: number = 2) {
    super();
    this.input_delay = input_delay;
  }

  get lead(): number { return Math.max(1, Math.floor(this.input_delay)); }

  protected override on_start(): void {
    this._seq = 0;
    this._starved = false;
    this._inputs.clear();
    const { conn } = this;
    if (!conn) return;
    for (let seq = 1; seq < this.lead; seq++)
      conn.send_nowait(MsgEnum.Tick, { seq });
  }

  protected on_tick_data(resp: IRespTick | IRespKeyTick): void {
    const seq = resp.seq!;
    this._inputs.set(seq, resp);
    if (this._starved && seq === this._seq) {
      this._starved = false;
      this.lf2?.world.awake();
    }
  }

  before_update = () => {
    const { lf2 } = this;
    if (!lf2) return;
    const resp = this._inputs.get(this._seq);
    if (!resp) {
      this._starved = true;
      return lf2.world.sleep();
    }
    this._inputs.delete(this._seq);
    this.run_tick(this._seq, resp);
  };

  after_update = () => {
    const { lf2 } = this;
    if (!lf2) return;
    if (this.debugging) this._snapshot2?.capture(lf2.world.entities);
    this._seq++;
    if (!this._inputs.has(this._seq)) {
      this._starved = true;
      lf2.world.sleep();
    }
  };
}
