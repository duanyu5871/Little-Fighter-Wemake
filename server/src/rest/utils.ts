import { URLSearchParams } from 'node:url';
import type { Client } from '../Client';
import type { Context } from '../Context';
import type { Room } from '../Room';
import { RestError } from './RestError';

export function split_url(url: string | undefined): { path: string; query: URLSearchParams } {
  const raw = url ?? '/';
  const q_index = raw.indexOf('?');
  let path = q_index < 0 ? raw : raw.slice(0, q_index);
  const hash_index = path.indexOf('#');
  if (hash_index >= 0) path = path.slice(0, hash_index);
  if (!path.startsWith('/')) path = `/${path}`;
  path = path.replace(/\/+$/, '') || '/';
  return { path, query: new URLSearchParams(q_index < 0 ? '' : raw.slice(q_index + 1)) };
}

export function query_bool(query: URLSearchParams, name: string): boolean {
  const value = query.get(name);
  if (value === null) return false;
  return !/^(0|false|no|off)$/i.test(value.trim());
}

export function query_str(query: URLSearchParams, name: string, default_value?: string): string | undefined {
  const value = query.get(name)?.trim();
  return value || default_value;
}

export function query_int(query: URLSearchParams, name: string, default_value: number, min: number, max: number): number {
  const raw = query.get(name)?.trim();
  if (!raw) return default_value;
  const value = Number(raw);
  if (!Number.isFinite(value)) return default_value;
  return Math.min(Math.max(Math.trunc(value), min), max);
}

export function find_room(ctx: Context, key: string | undefined): Room | undefined {
  const k = `${key ?? ''}`.trim();
  if (!k) return void 0;
  for (const room of ctx.room_mgr.all)
    if (room.id === k || room.code === k) return room;
  return void 0;
}

export function require_room(ctx: Context, key: string | undefined): Room {
  const room = find_room(ctx, key);
  if (!room) throw RestError.not_found(`房间不存在：${key ?? ''}`);
  return room;
}

export function require_client(c: { client: Client | undefined }): Client {
  if (!c.client) throw RestError.conflict('token 未绑定在线客户端：WebSocket 连接地址加上 ?token=<token>');
  return c.client;
}
