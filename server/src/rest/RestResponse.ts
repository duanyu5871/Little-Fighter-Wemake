import type { ServerResponse } from 'node:http';
import type { IRestRes } from './types';

export class RestResponse implements IRestRes {
  readonly raw: ServerResponse;
  protected _status: number = 200;
  protected _headers: Record<string, string> = {};
  protected _responded: boolean = false;

  constructor(raw: ServerResponse) {
    this.raw = raw;
    this.header('access-control-allow-origin', '*');
    this.header('access-control-allow-methods', 'GET, POST, PUT, PATCH, DELETE, OPTIONS');
    this.header('access-control-allow-headers', 'content-type, authorization, x-token');
    this.header('access-control-max-age', '86400');
    this.header('cache-control', 'no-store');
  }

  get responded(): boolean { return this._responded }
  get status_code(): number { return this._status }

  status(code: number): IRestRes {
    this._status = code;
    return this;
  }

  header(name: string, value: string): IRestRes {
    this._headers[name.toLowerCase()] = value;
    return this;
  }

  json(data: unknown): void {
    this.send(`${JSON.stringify(data ?? null)}`, 'application/json; charset=utf-8');
  }

  text(data: string, content_type: string = 'text/plain; charset=utf-8'): void {
    this.send(`${data}`, content_type);
  }

  end(): void {
    this.send('');
  }

  protected send(body: string, content_type?: string): void {
    if (this._responded) return;
    this._responded = true;
    const headers = { ...this._headers };
    if (content_type) headers['content-type'] = content_type;
    const empty = this._status === 204 || this._status === 304;
    if (!empty) headers['content-length'] = `${Buffer.byteLength(body)}`;
    this.raw.writeHead(this._status, headers);
    this.raw.end(empty ? void 0 : body);
  }
}
