export interface IEntrant {
  uid: string;
  name: string;
  oid?: string;
}

export class JoinQueue {
  protected readonly _items: IEntrant[] = [];
  protected readonly _uids = new Set<string>();
  constructor(readonly cap: number = 50) { }

  get size(): number { return this._items.length; }
  get all(): readonly IEntrant[] { return this._items; }
  has(uid: string): boolean { return this._uids.has(uid); }

  enqueue(entrant: IEntrant): boolean {
    if (!entrant.uid) return false;
    if (this._uids.has(entrant.uid)) return false;
    if (this._items.length >= this.cap) return false;
    this._uids.add(entrant.uid);
    this._items.push(entrant);
    return true;
  }
  dequeue(): IEntrant | undefined {
    const ret = this._items.shift();
    if (ret) this._uids.delete(ret.uid);
    return ret;
  }
  remove(uid: string): boolean {
    const idx = this._items.findIndex((v) => v.uid === uid);
    if (idx < 0) return false;
    this._items.splice(idx, 1);
    this._uids.delete(uid);
    return true;
  }
  clear(): void {
    this._items.length = 0;
    this._uids.clear();
  }
}

export function pick_join_team(
  counts: ReadonlyMap<string, number>,
  caps: ReadonlyMap<string, number>,
  fallen: string | null | undefined,
  order: readonly string[],
): string | undefined {
  const candidates: string[] = [];
  let least = Infinity;
  for (const team of order) {
    const alive = counts.get(team) ?? 0;
    if (alive <= 0) continue;
    if ((caps.get(team) ?? 0) - alive <= 0) continue;
    if (alive < least) {
      least = alive;
      candidates.length = 0;
      candidates.push(team);
    } else if (alive === least) {
      candidates.push(team);
    }
  }
  if (!candidates.length) return void 0;
  if (fallen && candidates.includes(fallen)) return fallen;
  return candidates[0];
}
