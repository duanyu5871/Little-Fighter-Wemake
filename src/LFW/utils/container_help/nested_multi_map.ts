import { NestedMap } from "./nested_map";

export class NestedMultiMap<K1, K2, V> {
  protected readonly _map = new NestedMap<K1, K2, V | V[]>();

  add(k1: K1, k2: K2, value: V): void {
    const prev = this._map.get(k1, k2);
    if (prev === void 0) this._map.set(k1, k2, value);
    else if (Array.isArray(prev)) prev.push(value);
    else this._map.set(k1, k2, [prev, value]);
  }

  first(k1: K1, k2: K2): V | undefined {
    const ret = this._map.get(k1, k2);
    return Array.isArray(ret) ? ret[0] : ret;
  }

  has(k1: K1, k2: K2): boolean {
    return this._map.has(k1, k2);
  }

  collect(k1: K1, k2: K2, out: V[] = []): V[] {
    const ret = this._map.get(k1, k2);
    if (ret === void 0) return out;
    if (Array.isArray(ret)) {
      for (let i = 0; i < ret.length; i++) out.push(ret[i]);
    } else {
      out.push(ret);
    }
    return out;
  }

  delete(k1: K1, k2: K2): boolean {
    return this._map.delete(k1, k2);
  }

  clear(): void {
    this._map.clear();
  }
}
