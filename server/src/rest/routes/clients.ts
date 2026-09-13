import type { Rest } from '../index';
import { query_bool } from '../utils';

export function register_client_routes(rest: Rest) {
  rest.router.get('/api/clients', { access: 'admin' }, (c) => {
    const detail = query_bool(c.req.query, 'detail');
    return {
      clients: Array.from(c.ctx.client_mgr.all).map(v => detail ? v.full_info : v.info ?? {}),
    };
  });
}
