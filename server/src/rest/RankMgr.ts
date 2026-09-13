import { existsSync, mkdirSync, readFileSync, renameSync, writeFileSync } from 'node:fs';
import { dirname } from 'node:path';

export interface IRankScore {
  type: string;
  name: string;
  score: number;
  extra?: unknown;
  date: number;
  client_id?: string;
  address?: string;
}

export interface IRankSubmit {
  type: string;
  name: string;
  score: number;
  extra?: unknown;
  client_id?: string;
  address?: string;
}

export interface IRankResult {
  rank: number;
  total: number;
  kept: boolean;
  score: IRankScore;
}

export interface IRankOptions {
  max_per_type?: number;
  allowed_types?: string[];
}

export const DEFAULT_RANKS_FILE = 'ranks.json';
export const MAX_PER_TYPE = 10000;
export const SAVE_DELAY = 500;

function sort_scores(list: IRankScore[]) {
  list.sort((a, b) => b.score - a.score || a.date - b.date);
}

export class RankMgr {
  static readonly TAG = 'RankMgr';
  readonly path: string;
  readonly max_per_type: number;
  readonly allowed_types?: string[];
  protected _scores = new Map<string, IRankScore[]>();
  protected _timer?: ReturnType<typeof setTimeout>;
  protected _dirty = false;

  constructor(path: string, options: IRankOptions = {}) {
    this.path = path;
    this.max_per_type = options.max_per_type ?? MAX_PER_TYPE;
    const allowed = options.allowed_types?.map(v => `${v}`.trim()).filter(Boolean);
    this.allowed_types = allowed?.length ? Array.from(new Set(allowed)) : void 0;
    this.load();
    process.on('exit', () => this.flush());
  }

  get types(): string[] { return Array.from(this._scores.keys()) }

  allows(type: string): boolean {
    return !this.allowed_types || this.allowed_types.includes(type);
  }

  total(type: string): number { return this._scores.get(type)?.length ?? 0 }

  scores(type: string, limit: number = 50, name?: string): { total: number; scores: IRankScore[] } {
    const list = this._scores.get(type) ?? [];
    const filtered = name ? list.filter(v => v.name === name) : list;
    return { total: filtered.length, scores: filtered.slice(0, limit) };
  }

  submit(info: IRankSubmit): IRankResult {
    const list = this.list_of(info.type);
    const score: IRankScore = { ...info, date: Date.now() };
    list.push(score);
    sort_scores(list);
    const rank = list.indexOf(score) + 1;
    const total = list.length;
    if (list.length > this.max_per_type) list.length = this.max_per_type;
    this._dirty = true;
    this.save();
    return { rank, total, kept: rank <= this.max_per_type, score };
  }

  flush() {
    if (!this._dirty) return;
    if (this._timer) { clearTimeout(this._timer); this._timer = void 0 }
    try {
      const scores: Record<string, IRankScore[]> = {};
      for (const [type, list] of this._scores) scores[type] = list;
      const tmp = `${this.path}.tmp`;
      mkdirSync(dirname(this.path), { recursive: true });
      writeFileSync(tmp, JSON.stringify({ version: 1, saved_at: Date.now(), scores }));
      renameSync(tmp, this.path);
      this._dirty = false;
    } catch (error) {
      console.error(`[${RankMgr.TAG}] 写入失败: ${this.path}`, error);
    }
  }

  protected save() {
    if (this._timer) return;
    this._timer = setTimeout(() => { this._timer = void 0; this.flush() }, SAVE_DELAY);
    this._timer.unref?.();
  }

  protected load() {
    if (!existsSync(this.path)) return;
    try {
      const raw = JSON.parse(readFileSync(this.path, 'utf8')) as { scores?: Record<string, IRankScore[]> };
      for (const [type, list] of Object.entries(raw?.scores ?? {})) {
        if (!Array.isArray(list)) continue;
        const target = this.list_of(type);
        for (const v of list)
          if (v && typeof v.name === 'string' && Number.isFinite(v.score)) target.push(v);
        sort_scores(target);
        if (target.length > this.max_per_type) target.length = this.max_per_type;
      }
      console.log(`[${RankMgr.TAG}] ${this.path} 已加载（${this.types.length} 个排行类型）`);
    } catch (error) {
      console.error(`[${RankMgr.TAG}] 读取失败: ${this.path}`, error);
    }
  }

  protected list_of(type: string): IRankScore[] {
    let list = this._scores.get(type);
    if (!list) this._scores.set(type, list = []);
    return list;
  }
}
