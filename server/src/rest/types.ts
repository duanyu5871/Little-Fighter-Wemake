import type { IncomingMessage, ServerResponse } from 'node:http';
import type { URLSearchParams } from 'node:url';
import type { Client } from '../Client';
import type { Context } from '../Context';
import type { IAuth } from './AuthMgr';

export type TAccess = 'public' | 'player' | 'client' | 'admin';

export type TParamKeys<Path extends string> =
  Path extends `${string}:${infer K}/${infer Rest}`
  ? K | TParamKeys<`/${Rest}`>
  : Path extends `${string}:${infer K}`
  ? K
  : never;

export type TParams<Path extends string> = { [K in TParamKeys<Path>]: string };

export interface IRestReq {
  readonly raw: IncomingMessage;
  readonly method: string;
  readonly path: string;
  readonly query: URLSearchParams;
  readonly params: Record<string, string>;
  readonly text: string;
  readonly body: unknown;
}

export interface IRestRes {
  readonly responded: boolean;
  readonly status_code: number;
  readonly raw: ServerResponse;
  status(code: number): IRestRes;
  header(name: string, value: string): IRestRes;
  json(data: unknown): void;
  text(data: string, content_type?: string): void;
  end(): void;
}

export interface IRestCall<Params extends Record<string, string> = Record<string, string>> {
  readonly ctx: Context;
  readonly req: IRestReq & { readonly params: Params };
  readonly res: IRestRes;
  readonly auth: IAuth;
  readonly client: Client | undefined;
}

export type TRestHandler<Path extends string = string> =
  (c: IRestCall<TParams<Path>>) => unknown | Promise<unknown>;

export interface IRouteOptions {
  access?: TAccess;
}

export interface IRoute {
  readonly method: string;
  readonly path: string;
  readonly segments: readonly string[];
  readonly access: TAccess;
  readonly handler: (c: IRestCall<any>) => unknown | Promise<unknown>;
}
