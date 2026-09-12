import { Graves } from "../../base/Graves";

export class NestedMap<K1, K2, V> {
  protected readonly _map = new Map<K1, Map<K2, V>>();
  protected readonly _graves = new Graves<Map<K2, V>>();

  get(k1: K1, k2: K2): V | undefined {
    return this._map.get(k1)?.get(k2);
  }

  has(k1: K1, k2: K2): boolean {
    const inner = this._map.get(k1);
    return inner !== void 0 && inner.has(k2);
  }

  set(k1: K1, k2: K2, value: V): void {
    let inner = this._map.get(k1);
    if (inner === void 0) {
      inner = this._graves.take() ?? new Map();
      this._map.set(k1, inner);
    }
    inner.set(k2, value);
  }

  delete(k1: K1, k2: K2): boolean {
    const inner = this._map.get(k1);
    return inner !== void 0 && inner.delete(k2);
  }

  clear(): void {
    if (!this._map.size) return;
    for (const inner of this._map.values()) {
      inner.clear();
      this._graves.add(inner);
    }
    this._map.clear();
  }
}
