import { existsSync, mkdirSync, readFileSync, readdirSync, renameSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';

export interface IRankScore {
  type: string;
  name: string;
  score: number;
  extra?: unknown;
  date: number;
  uid?: string;
  client_id?: string;
  address?: string;
}

export interface IRankSubmit {
  type: string;
  name: string;
  score: number;
  extra?: unknown;
  uid?: string;
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
  dir?: string;
}

export const DEFAULT_RANKS_FILE = 'ranks.json';
export const RANK_FILE_EXT = '.json';
export const MAX_PER_TYPE = 10000;
export const SAVE_DELAY = 500;
export const RANK_TYPE_PATTERN = /^[A-Za-z0-9_.-]{1,64}$/;
export const PERIOD_SUFFIXES = ['_all', '_month', '_week', '_day'] as const;

function sort_scores(list: IRankScore[]) {
  list.sort((a, b) => b.score - a.score || a.date - b.date);
}

function owner_key(v: { uid?: string; name: string }): string {
  return v.uid ? `uid:${v.uid}` : `name:${v.name}`;
}

function family_of(type: string): string[] {
  for (const suffix of PERIOD_SUFFIXES) {
    if (!type.endsWith(suffix)) continue;
    const base = type.slice(0, -suffix.length);
    if (!base) break;
    return PERIOD_SUFFIXES.map(v => `${base}${v}`);
  }
  return [];
}

function write_atomic(file: string, text: string) {
  const tmp = `${file}.tmp`;
  mkdirSync(dirname(file), { recursive: true });
  writeFileSync(tmp, text);
  renameSync(tmp, file);
}

export class RankMgr {
  static readonly TAG = 'RankMgr';
  readonly path: string;
  readonly dir?: string;
  readonly max_per_type: number;
  readonly allowed_types?: string[];
  protected _scores = new Map<string, IRankScore[]>();
  protected _timer?: ReturnType<typeof setTimeout>;
  protected _dirty = new Set<string>();

  constructor(path: string, options: IRankOptions = {}) {
    this.path = path;
    this.dir = options.dir?.trim() || void 0;
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

  file_of(type: string): string {
    return this.dir ? join(this.dir, `${type}${RANK_FILE_EXT}`) : this.path;
  }

  total(type: string): number { return this._scores.get(type)?.length ?? 0 }

  find(type: string, opts: { name?: string; uid?: string }): { rank: number; score: IRankScore } | undefined {
    const list = this._scores.get(type);
    if (!list) return void 0;
    for (let i = 0; i < list.length; i++) {
      const score = list[i]!;
      if (opts.uid) {
        if (score.uid === opts.uid) return { rank: i + 1, score };
      } else if (opts.name && score.name === opts.name) {
        return { rank: i + 1, score };
      }
    }
    return void 0;
  }

  scores(type: string, limit: number = 50, name?: string): { total: number; scores: IRankScore[] } {
    const list = this._scores.get(type) ?? [];
    const filtered = name ? list.filter(v => v.name === name) : list;
    return { total: filtered.length, scores: filtered.slice(0, limit) };
  }

  extra_fighter(extra: unknown): string | undefined {
    const fighter = (extra as { fighter?: unknown } | undefined)?.fighter;
    return typeof fighter === 'string' && fighter ? fighter : void 0;
  }

  char_lookup(type: string, names: string[]): { name: string; score: number; fighter: string }[] {
    const list = this._scores.get(type);
    if (!list?.length || !names.length) return [];
    const want = new Set(names);
    const found = new Map<string, { name: string; score: number; fighter: string }>();
    for (const v of list) {
      if (!want.has(v.name) || found.has(v.name)) continue;
      const fighter = this.extra_fighter(v.extra);
      if (fighter) found.set(v.name, { name: v.name, score: v.score, fighter });
    }
    return Array.from(found.values());
  }

  submit(info: IRankSubmit): IRankResult {
    const result = this.store(info);
    for (const type of family_of(info.type)) {
      if (type !== info.type && this.allows(type)) this.store({ ...info, type });
    }
    return result;
  }

  protected store(info: IRankSubmit): IRankResult {
    const list = this.list_of(info.type);
    const key = owner_key(info);
    const index = list.findIndex(v => owner_key(v) === key);
    if (index >= 0 && list[index]!.score >= info.score) {
      const rank = index + 1;
      return { rank, total: list.length, kept: rank <= this.max_per_type, score: list[index]! };
    }
    const score: IRankScore = { ...info, date: Date.now() };
    if (index >= 0) list.splice(index, 1, score);
    else list.push(score);
    sort_scores(list);
    const rank = list.indexOf(score) + 1;
    const total = list.length;
    if (list.length > this.max_per_type) list.length = this.max_per_type;
    this._dirty.add(info.type);
    this.save();
    return { rank, total, kept: rank <= this.max_per_type, score };
  }

  flush() {
    if (!this._dirty.size) return;
    if (this._timer) { clearTimeout(this._timer); this._timer = void 0 }
    try {
      if (this.dir) {
        for (const type of this._dirty) {
          const list = this._scores.get(type) ?? [];
          write_atomic(this.file_of(type), JSON.stringify({ version: 1, type, saved_at: Date.now(), scores: list }));
        }
      } else {
        const scores: Record<string, IRankScore[]> = {};
        for (const [type, list] of this._scores) scores[type] = list;
        write_atomic(this.path, JSON.stringify({ version: 1, saved_at: Date.now(), scores }));
      }
      this._dirty.clear();
    } catch (error) {
      console.error(`[${RankMgr.TAG}] 写入失败: ${this.dir ?? this.path}`, error);
    }
  }

  protected save() {
    if (this._timer) return;
    this._timer = setTimeout(() => { this._timer = void 0; this.flush() }, SAVE_DELAY);
    this._timer.unref?.();
  }

  protected load() {
    if (this.dir) {
      if (!existsSync(this.dir)) return;
      try {
        for (const name of readdirSync(this.dir)) {
          if (!name.endsWith(RANK_FILE_EXT)) continue;
          this.load_file(join(this.dir, name), name.slice(0, -RANK_FILE_EXT.length));
        }
      } catch (error) {
        console.error(`[${RankMgr.TAG}] 读取失败: ${this.dir}`, error);
      }
      return;
    }
    this.load_file(this.path);
  }

  protected load_file(file: string, type?: string) {
    if (!existsSync(file)) return;
    try {
      const raw = JSON.parse(readFileSync(file, 'utf8')) as { type?: unknown; scores?: unknown };
      if (Array.isArray(raw.scores)) {
        const file_type = type ?? (typeof raw.type === 'string' ? raw.type : void 0);
        if (file_type) this.load_scores(file_type, raw.scores);
      } else if (raw.scores && typeof raw.scores === 'object') {
        for (const [type_name, list] of Object.entries(raw.scores))
          if (Array.isArray(list)) this.load_scores(type_name, list);
      }
      console.log(`[${RankMgr.TAG}] ${file} 已加载`);
    } catch (error) {
      console.error(`[${RankMgr.TAG}] 读取失败: ${file}`, error);
    }
  }

  protected load_scores(type: string, list: unknown[]) {
    const target = this.list_of(type);
    const parsed: IRankScore[] = [];
    for (const v of list) {
      const score = v as IRankScore;
      if (score && typeof score.name === 'string' && Number.isFinite(score.score)) parsed.push(score);
    }
    sort_scores(parsed);
    const kept = new Set<string>();
    for (const score of parsed) {
      const key = owner_key(score);
      if (kept.has(key)) continue;
      kept.add(key);
      target.push(score);
    }
    sort_scores(target);
    if (target.length > this.max_per_type) target.length = this.max_per_type;
  }

  protected list_of(type: string): IRankScore[] {
    let list = this._scores.get(type);
    if (!list) this._scores.set(type, list = []);
    return list;
  }
}
