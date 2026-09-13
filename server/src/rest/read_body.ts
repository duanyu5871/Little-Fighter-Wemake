import type { IncomingMessage } from 'node:http';
import { URLSearchParams } from 'node:url';
import { RestError } from './RestError';

export function read_body(raw: IncomingMessage, max_size: number): Promise<string> {
  return new Promise<string>((resolve, reject) => {
    const chunks: Buffer[] = [];
    let size = 0;
    let done = false;
    raw.on('data', (chunk: Buffer | string) => {
      if (done) return;
      const buf = Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk);
      size += buf.length;
      if (size > max_size) {
        done = true;
        raw.resume();
        reject(RestError.too_large(`请求体过大（上限 ${max_size} 字节）`));
        return;
      }
      chunks.push(buf);
    });
    raw.on('end', () => {
      if (done) return;
      done = true;
      resolve(Buffer.concat(chunks).toString('utf8'));
    });
    raw.on('error', (error) => {
      if (done) return;
      done = true;
      reject(error);
    });
  });
}

export function parse_body(text: string, content_type: string | undefined): unknown {
  if (!text) return void 0;
  const ct = `${content_type ?? ''}`.split(';')[0]!.trim().toLowerCase();
  if (ct === 'application/json' || ct.endsWith('+json') || /^\s*[\[{]/.test(text)) {
    try {
      return JSON.parse(text);
    } catch (error) {
      throw RestError.bad_request(`请求体 JSON 解析失败: ${(error as Error).message}`);
    }
  }
  if (ct === 'application/x-www-form-urlencoded') {
    const params: Record<string, string> = {};
    for (const [k, v] of new URLSearchParams(text)) params[k] = v;
    return params;
  }
  return text;
}

export function body_of(body: unknown): Record<string, unknown> | undefined {
  return body && typeof body === 'object' && !Array.isArray(body) ? body as Record<string, unknown> : void 0;
}

export function str_of(value: unknown): string | undefined {
  if (value === undefined || value === null) return void 0;
  const v = `${value}`.trim();
  return v || void 0;
}

export function strs_of(value: unknown): string[] | undefined {
  if (!Array.isArray(value)) return void 0;
  return value.map(v => `${v}`);
}
