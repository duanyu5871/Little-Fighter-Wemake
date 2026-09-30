import { abs } from '../../utils/math/base';
import { float_equal } from '../../utils/math/float_equal';

export type SmoothNumberMode = 'linear' | 'exponential';

export class SmoothNumber {
  private _v: number = 0;
  private _t: number = 0;
  private _c: (self: this) => void = () => { };
  private _mode: SmoothNumberMode = 'linear';
  private _speed = 20;
  private _factor = 0.3;
  private _min_diff = 1;
  private done: boolean = false;

  get value(): number { return this._v; }
  set value(v: number) { if (this._t == v) return; this._v = this._t = v; this.done = true; }
  get target(): number { return this._t }
  set target(v: number) { if (this._t == v) return; this._t = v; this.done = false; }

  mode(v: SmoothNumberMode): this { this._mode = v; return this; }
  speed(v: number): this { this._speed = v; return this; }
  factor(v: number): this { this._factor = v; return this; }
  min_diff(v: number): this { this._min_diff = v; return this; }

  handler(v: (self: this) => void) {
    this._c = v;
    return this;
  }
  handle() { this._c(this); }

  update() {
    if (this.done) return;

    if (float_equal(this._t, this._v)) {
      this.done = true;
      this._c(this);
      return;
    }

    if (this._mode === 'linear') {
      const diff = this._t - this._v;
      this._v += abs(diff) > this._speed ? this._speed * Math.sign(diff) : diff;
      if (this._v === this._t) this.done = true;
    } else {
      this._v = this._v + this._factor * (this._t - this._v);
      if (abs(this._v - this._t) < this._min_diff) {
        this.done = true;
        this._v = this._t;
      }
    }
    this._c(this);
  }
}
