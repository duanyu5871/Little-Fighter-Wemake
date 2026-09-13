import { RestError } from './RestError';
import type { IRoute, IRouteOptions, TRestHandler } from './types';

function split_path(path: string): string[] {
  return path.split('/').filter(v => v.length > 0);
}

function match_segments(pattern: readonly string[], parts: readonly string[]): Record<string, string> | null {
  const params: Record<string, string> = {};
  for (let i = 0; i < pattern.length; i++) {
    const p = pattern[i]!;
    if (p === '*') return params;
    const part = parts[i];
    if (part === undefined || part === '') return null;
    if (p.startsWith(':')) {
      try { params[p.slice(1)] = decodeURIComponent(part) }
      catch { params[p.slice(1)] = part }
    } else if (p !== part) {
      return null;
    }
  }
  return pattern.length === parts.length ? params : null;
}

export class Router {
  static readonly TAG = 'Router';
  protected _routes: IRoute[] = [];

  get routes(): readonly IRoute[] { return this._routes }

  add<Path extends string>(method: string, path: Path, handler: TRestHandler<Path>): this;
  add<Path extends string>(method: string, path: Path, options: IRouteOptions, handler: TRestHandler<Path>): this;
  add<Path extends string>(
    method: string,
    path: Path,
    options_or_handler: IRouteOptions | TRestHandler<Path>,
    handler?: TRestHandler<Path>
  ): this {
    const options: IRouteOptions = typeof options_or_handler === 'function' ? {} : options_or_handler;
    const fn = typeof options_or_handler === 'function' ? options_or_handler : handler;
    if (typeof fn !== 'function')
      throw new RestError(500, `路由 ${method} ${path} 缺少处理器`);
    const route: IRoute = {
      method: method.toUpperCase(),
      path,
      segments: split_path(path),
      access: options.access ?? 'public',
      handler: fn as IRoute['handler'],
    };
    const exists = this._routes.some(v => v.method === route.method && v.path === route.path);
    if (exists)
      throw new RestError(500, `路由重复注册：${route.method} ${route.path}`);
    this._routes.push(route);
    return this;
  }

  get<Path extends string>(path: Path, handler: TRestHandler<Path>): this;
  get<Path extends string>(path: Path, options: IRouteOptions, handler: TRestHandler<Path>): this;
  get<Path extends string>(path: Path, options_or_handler: IRouteOptions | TRestHandler<Path>, handler?: TRestHandler<Path>): this {
    return typeof options_or_handler === 'function'
      ? this.add('GET', path, options_or_handler)
      : this.add('GET', path, options_or_handler, handler as TRestHandler<Path>);
  }

  post<Path extends string>(path: Path, handler: TRestHandler<Path>): this;
  post<Path extends string>(path: Path, options: IRouteOptions, handler: TRestHandler<Path>): this;
  post<Path extends string>(path: Path, options_or_handler: IRouteOptions | TRestHandler<Path>, handler?: TRestHandler<Path>): this {
    return typeof options_or_handler === 'function'
      ? this.add('POST', path, options_or_handler)
      : this.add('POST', path, options_or_handler, handler as TRestHandler<Path>);
  }

  put<Path extends string>(path: Path, handler: TRestHandler<Path>): this;
  put<Path extends string>(path: Path, options: IRouteOptions, handler: TRestHandler<Path>): this;
  put<Path extends string>(path: Path, options_or_handler: IRouteOptions | TRestHandler<Path>, handler?: TRestHandler<Path>): this {
    return typeof options_or_handler === 'function'
      ? this.add('PUT', path, options_or_handler)
      : this.add('PUT', path, options_or_handler, handler as TRestHandler<Path>);
  }

  patch<Path extends string>(path: Path, handler: TRestHandler<Path>): this;
  patch<Path extends string>(path: Path, options: IRouteOptions, handler: TRestHandler<Path>): this;
  patch<Path extends string>(path: Path, options_or_handler: IRouteOptions | TRestHandler<Path>, handler?: TRestHandler<Path>): this {
    return typeof options_or_handler === 'function'
      ? this.add('PATCH', path, options_or_handler)
      : this.add('PATCH', path, options_or_handler, handler as TRestHandler<Path>);
  }

  delete<Path extends string>(path: Path, handler: TRestHandler<Path>): this;
  delete<Path extends string>(path: Path, options: IRouteOptions, handler: TRestHandler<Path>): this;
  delete<Path extends string>(path: Path, options_or_handler: IRouteOptions | TRestHandler<Path>, handler?: TRestHandler<Path>): this {
    return typeof options_or_handler === 'function'
      ? this.add('DELETE', path, options_or_handler)
      : this.add('DELETE', path, options_or_handler, handler as TRestHandler<Path>);
  }

  match(method: string, path: string): { route?: IRoute; params: Record<string, string>; allowed: string[] } {
    const parts = split_path(path);
    const allowed = new Set<string>();
    for (const route of this._routes) {
      const params = match_segments(route.segments, parts);
      if (!params) continue;
      if (route.method !== method) { allowed.add(route.method); continue }
      return { route, params, allowed: [] };
    }
    return { params: {}, allowed: Array.from(allowed) };
  }
}
