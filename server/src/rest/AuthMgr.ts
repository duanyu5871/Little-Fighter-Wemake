import { randomBytes } from 'node:crypto';
import type { IncomingMessage } from 'node:http';
import { URLSearchParams } from 'node:url';
import type { Client } from '../Client';

export type TAuthLevel = 'guest' | 'player' | 'admin';

export interface IAuth {
  level: TAuthLevel;
  token: string;
  name?: string;
  client?: Client;
}

export interface IPlayerToken {
  readonly token: string;
  name: string;
  players: string[];
  readonly created_at: number;
  last_seen: number;
  client?: Client;
}

function safe_equal(a: string, b: string): boolean {
  if (a.length !== b.length) return false;
  let diff = 0;
  for (let i = 0; i < a.length; i++) diff |= a.charCodeAt(i) ^ b.charCodeAt(i);
  return diff === 0;
}

export class AuthMgr {
  static readonly TAG = 'AuthMgr';
  protected _admin_tokens: string[] = [];
  protected _players = new Map<string, IPlayerToken>();
  protected _bindings = new Map<string, string>();

  get admin_tokens(): readonly string[] { return this._admin_tokens }
  get players(): ReadonlyMap<string, IPlayerToken> { return this._players }

  add_admin_token(...tokens: (string | undefined | null)[]) {
    for (const token of tokens) {
      for (const part of `${token ?? ''}`.split(',')) {
        const v = part.trim();
        if (v && !this._admin_tokens.includes(v)) this._admin_tokens.push(v);
      }
    }
  }

  is_admin_token(token: string | undefined): boolean {
    if (!token) return false;
    return this._admin_tokens.some(v => safe_equal(v, token));
  }

  create_player_token(name?: string, players?: string[], token?: string): IPlayerToken {
    const info: IPlayerToken = {
      token: token?.trim() || randomBytes(24).toString('hex'),
      name: name?.trim() || '',
      players: (players ?? []).map(v => `${v}`),
      created_at: Date.now(),
      last_seen: Date.now(),
    };
    if (!info.name) info.name = `player_${info.token.slice(0, 8)}`;
    this._players.set(info.token, info);
    return info;
  }

  get_player_token(token: string | undefined): IPlayerToken | undefined {
    return token ? this._players.get(token) : void 0;
  }

  del_player_token(token: string): boolean {
    const info = this._players.get(token);
    if (!info) return false;
    if (info.client) this._bindings.delete(info.client.id);
    return this._players.delete(token);
  }

  bind_client(token: string, client: Client): IAuth {
    const auth = this.resolve(token);
    if (auth.level === 'admin') {
      client.is_admin = true;
      return auth;
    }
    let info = this._players.get(token);
    if (!info) info = this.create_player_token(void 0, void 0, token);
    if (info.client && info.client !== client) this._bindings.delete(info.client.id);
    info.client = client;
    info.last_seen = Date.now();
    if (client.client_info) {
      if (client.client_info.name) info.name = client.client_info.name;
      if (client.client_info.players) info.players = client.client_info.players;
    }
    this._bindings.set(client.id, token);
    return this.resolve(token);
  }

  bind_from_req(client: Client, req: IncomingMessage): IAuth {
    const token = token_of_url(req.url);
    if (!token) return { level: 'guest', token: '' };
    const auth = this.bind_client(token, client);
    console.log(`[${AuthMgr.TAG}::bind_from_req] ${client.id} -> ${auth.level}`);
    return auth;
  }

  unbind_client(client: Client) {
    const token = this._bindings.get(client.id);
    if (!token) return;
    this._bindings.delete(client.id);
    const info = this._players.get(token);
    if (info && info.client === client) delete info.client;
  }

  resolve(token: string | undefined): IAuth {
    if (!token) return { level: 'guest', token: '' };
    if (this.is_admin_token(token)) return { level: 'admin', token };
    const info = this._players.get(token);
    if (info) return { level: 'player', token, name: info.name, client: info.client };
    return { level: 'guest', token };
  }

  token_of(req: IncomingMessage, query: URLSearchParams): string {
    const authorization = `${req.headers.authorization ?? ''}`.trim();
    const bearer = /^bearer\s+(.+)$/i.exec(authorization);
    if (bearer?.[1]) return bearer[1].trim();
    const header = req.headers['x-token'];
    const x_token = Array.isArray(header) ? header[0] : header;
    if (x_token?.trim()) return x_token.trim();
    return query.get('token')?.trim() ?? '';
  }
}

export function token_of_url(url: string | undefined): string {
  if (!url) return '';
  const index = url.indexOf('?');
  if (index < 0) return '';
  const params = new URLSearchParams(url.slice(index + 1));
  return params.get('token')?.trim() ?? '';
}
