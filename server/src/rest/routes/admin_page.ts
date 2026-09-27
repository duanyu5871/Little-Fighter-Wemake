import html from '../../admin/index.html';
import type { Rest } from '../index';

/**
 * 内置的服务器管理页面（`server/src/admin/index.html`，构建时作为字符串内联进 bundle）。
 *
 * 页面本身是公开的（否则浏览器拿不到它），它请求的 `/api/system`、`/api/clients`、
 * `/api/auth/tokens`、`POST /api/rooms/:key/close|kick` 等接口才是 admin 权限，
 * 需要把管理员 token 填进页面（或从托盘「打开管理页面」带 `?token=` 打开）。
 */
export function register_admin_page_routes(rest: Rest) {
  const send = (c: { res: { text(data: string, content_type?: string): void } }) => {
    c.res.text(html, 'text/html; charset=utf-8');
  };
  rest.router.get('/admin', send);
}
