import { RANK_TYPE_PATTERN, type RankMgr } from '../RankMgr';
import { RestError } from '../RestError';
import type { Rest } from '../index';
import { body_of, str_of } from '../read_body';
import { query_int, query_str } from '../utils';

const MAX_LOOKUP_NAMES = 200;

function require_type(ranks: RankMgr, type: string) {
  if (!RANK_TYPE_PATTERN.test(type))
    throw RestError.bad_request(`排行类型只能包含字母、数字、下划线、点和短横线（最长 64 字符）：${type}`);
  if (ranks.allows(type)) return;
  const allowed = ranks.allowed_types;
  throw RestError.bad_request(`排行类型不允许：${type}${allowed ? `（允许的类型：${allowed.join(', ')}）` : ''}`);
}

export function register_rank_routes(rest: Rest) {
  const { router, ranks } = rest;

  router.post('/api/ranks', (c) => {
    const body = body_of(c.req.body);
    const type = str_of(body?.type)?.slice(0, 64);
    const name = str_of(body?.name)?.slice(0, 32);
    if (!type) throw RestError.bad_request('缺少 type');
    if (!name) throw RestError.bad_request('缺少 name');
    require_type(ranks, type);
    const value = body?.score;
    const score = typeof value === 'number' ? value : Number(str_of(value) ?? NaN);
    if (!Number.isFinite(score)) throw RestError.bad_request('score 必须是数字');
    const result = ranks.submit({
      type,
      name,
      score,
      extra: body?.extra,
      client_id: c.client?.id,
      address: c.req.raw.socket.remoteAddress,
    });
    return { rank: result.rank, total: result.total, kept: result.kept, ...result.score };
  });

  router.post('/api/ranks/lookup', (c) => {
    const body = body_of(c.req.body) as { type?: unknown; names?: unknown } | undefined;
    const type = str_of(body?.type);
    if (!type) throw RestError.bad_request('缺少 type');
    require_type(ranks, type);
    const raw_names = Array.isArray(body?.names) ? body.names.slice(0, MAX_LOOKUP_NAMES) : [];
    const names: string[] = [];
    for (const raw of raw_names) {
      const name = str_of(raw);
      if (name) names.push(name);
    }
    return { type, chars: ranks.char_lookup(type, names) };
  });

  router.get('/api/ranks/:type', (c) => {
    const type = c.req.params.type;
    require_type(ranks, type);
    const limit = query_int(c.req.query, 'limit', 50, 1, 500);
    const name = query_str(c.req.query, 'name');
    const result = ranks.scores(type, limit, name);
    const best = name ? ranks.best_of(type, name) ?? null : void 0;
    return { type, limit, total: result.total, scores: result.scores, best };
  });
}
