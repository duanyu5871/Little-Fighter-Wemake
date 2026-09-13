import type { SurvivalRankItem, SurvivalRankPeriod } from "./LFW/LFW";

const PERIODS: SurvivalRankPeriod[] = ['all', 'month', 'week', 'day'];
const TYPE_OF_PERIOD: Record<SurvivalRankPeriod, string> = {
  all: 'survival_all',
  month: 'survival_month',
  week: 'survival_week',
  day: 'survival_day',
};
const SUBMITTED_KEY = 'rank_api_submitted_max';
const LIST_LIMIT = 100;
const BILI_TYPE_OF_PERIOD: Record<SurvivalRankPeriod, string> = {
  all: 'survival_bili_all',
  month: 'survival_bili_month',
  week: 'survival_bili_week',
  day: 'survival_bili_day',
};

function query_param(name: string): string {
  const params = new URLSearchParams(location.search);
  const qi = location.hash.indexOf('?');
  if (qi >= 0) {
    for (const [k, v] of new URLSearchParams(location.hash.slice(qi + 1)))
      if (!params.has(k)) params.set(k, v);
  }
  return params.get(name)?.trim() ?? '';
}

export function rank_api_base(): string {
  const base = (query_param('RANK_API') || RANK_API_URL || '').trim();
  return base.replace(/\/+$/, '');
}

export function rank_api_available(): boolean {
  return !!rank_api_base();
}

export function rank_api_type(period: SurvivalRankPeriod): string {
  return TYPE_OF_PERIOD[period] ?? TYPE_OF_PERIOD.all;
}

export function rank_api_bili_type(period: SurvivalRankPeriod): string {
  return BILI_TYPE_OF_PERIOD[period] ?? BILI_TYPE_OF_PERIOD.all;
}

function api(path: string, init?: RequestInit): Promise<Response> {
  return fetch(`${rank_api_base()}${path}`, init);
}

interface IRankEntry {
  name?: string;
  score?: number;
  extra?: { fighter?: string };
}

async function post_score(type: string, name: string, score: number, fighter: string): Promise<void> {
  const resp = await api('/api/ranks', {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify({ type, name, score, extra: { fighter } }),
  });
  if (!resp.ok) throw new Error(`[rank_api] 提交失败: ${resp.status}`);
}

function submitted_max(): number {
  try {
    const value = Number(localStorage.getItem(SUBMITTED_KEY));
    return Number.isFinite(value) ? value : -1;
  } catch {
    return -1;
  }
}

function set_submitted_max(score: number) {
  try {
    localStorage.setItem(SUBMITTED_KEY, `${score}`);
  } catch {
    //
  }
}

export async function submit_rank_score(score: number, name: string, fighter: string): Promise<void> {
  if (score <= submitted_max()) return;
  set_submitted_max(score);
  await Promise.all(PERIODS.map(period => post_score(rank_api_type(period), name, score, fighter)));
}

export async function get_rank_list(period: SurvivalRankPeriod): Promise<SurvivalRankItem[]> {
  const resp = await api(`/api/ranks/${rank_api_type(period)}?limit=${LIST_LIMIT}`);
  if (!resp.ok) throw new Error(`[rank_api] 拉取榜单失败: ${resp.status}`);
  const data = await resp.json() as { scores?: IRankEntry[] };
  const entries = Array.isArray(data?.scores) ? data.scores : [];
  const best = new Map<string, { score: number; fighter?: string }>();
  for (const entry of entries) {
    const nickname = `${entry?.name ?? ''}`;
    const score = Number(entry?.score);
    if (!nickname || !Number.isFinite(score)) continue;
    if (!best.has(nickname)) best.set(nickname, { score, fighter: entry?.extra?.fighter });
  }
  return Array.from(best, ([nickname, v], i) => ({ rank: i + 1, score: v.score, nickname, fighter: v.fighter }));
}

export async function get_my_rank(period: SurvivalRankPeriod, name: string): Promise<{ rank: number; score: number } | null> {
  if (!name) return null;
  const resp = await api(`/api/ranks/${rank_api_type(period)}?name=${encodeURIComponent(name)}&limit=1`);
  if (!resp.ok) throw new Error(`[rank_api] 查询我的排名失败: ${resp.status}`);
  const data = await resp.json() as { best?: { rank?: number; score?: { score?: number } } | null };
  const rank = Number(data?.best?.rank);
  const score = Number(data?.best?.score?.score);
  if (!Number.isFinite(rank) || !Number.isFinite(score)) return null;
  return { rank, score };
}

export async function submit_bili_record(score: number, name: string, fighter: string): Promise<void> {
  if (!rank_api_available() || !name) return;
  await Promise.all(PERIODS.map(period => post_score(rank_api_bili_type(period), name, score, fighter)));
}

export async function lookup_fighters(period: SurvivalRankPeriod, names: string[]): Promise<Map<string, string>> {
  const ret = new Map<string, string>();
  const wanted = names.filter(Boolean);
  if (!rank_api_available() || !wanted.length) return ret;
  const resp = await api('/api/ranks/lookup', {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify({ type: rank_api_bili_type(period), names: wanted }),
  });
  if (!resp.ok) return ret;
  const data = await resp.json() as { chars?: { name?: string; fighter?: string }[] };
  for (const v of data?.chars ?? []) {
    const name = `${v?.name ?? ''}`;
    const fighter = `${v?.fighter ?? ''}`;
    if (name && fighter) ret.set(name, fighter);
  }
  return ret;
}
