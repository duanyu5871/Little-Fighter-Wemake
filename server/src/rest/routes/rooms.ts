import { MsgEnum } from '../../Net';
import type { Rest } from '../index';
import { RestError } from '../RestError';
import { body_of, str_of } from '../read_body';
import { query_bool, require_room } from '../utils';

export function register_room_routes(rest: Rest) {
  const { router } = rest;

  router.get('/api/rooms', (c) => {
    const all = Array.from(c.ctx.room_mgr.all).map(v => v.room_info);
    return { rooms: query_bool(c.req.query, 'show_all') ? all : all.filter(v => v.sync_mode !== 'delay') };
  });

  router.get('/api/rooms/:key', (c) => ({
    room: require_room(c.ctx, c.req.params.key).room_info,
  }));

  router.post('/api/rooms/:key/close', { access: 'admin' }, (c) => {
    const room = require_room(c.ctx, c.req.params.key);
    const room_info = room.room_info;
    room.close(room.owner);
    return { room: room_info };
  });

  router.post('/api/rooms/:key/kick', { access: 'admin' }, (c) => {
    const room = require_room(c.ctx, c.req.params.key);
    const client_id = str_of(body_of(c.req.body)?.client_id);
    if (!client_id) throw RestError.bad_request('缺少 client_id');
    const exists = Array.from(room.clients).some(v => v.id === client_id);
    if (!exists) throw RestError.not_found(`房间内没有客户端：${client_id}`);
    room.kick({ type: MsgEnum.Kick, is_req: true, pid: '', client_id });
    return { room: room.room_info };
  });
}
