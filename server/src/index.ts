import http from "http";
import https from "https";
import { WebSocketServer } from 'ws';
import { Client } from './Client.js';
import { ClientMgr } from './ClientMgr.js';
import { Context } from './Context.js';
import { RoomMgr } from './RoomMgr.js';
import "./init.js";
import { read_file } from "./read_file.js";
import { AuthMgr } from './rest/AuthMgr.js';
import { attach_rest } from './rest/index.js';
import arg from "../node_modules/arg"
import info from "../package.json"

const args = arg({
  '--help': Boolean, '-h': '--help',
  '--port': Number, '-p': '--port',
  '--ssl-key-path': String,
  '--ssl-cer-parh': String,
  '--admin-token': String,
})
function handle_help() {
  console.log(`
Little Fighter Wemake Multiplayer Server v${info.version}
Options:
  -h, --help
  -p, --port
  --ssl-key-path 
  --ssl-cer-parh
  --admin-token   管理员 token（同 ADMIN_TOKEN，逗号分隔多个）

Environment variables (.env is supported):
  HTTPS_PORT
  HTTP_PORT
  SSL_KEY_FILE_PATH
  SSL_CER_FILE_PATH
  ADMIN_TOKEN / ADMIN_PWD
  RANKS_FILE_PATH

REST API (与 ws 共用端口):
  GET  /api                接口清单
  GET  /api/stats          服务器统计
  GET  /api/rooms          房间列表
  GET  /api/rooms/:key     房间详情
  GET  /api/clients        客户端列表（admin）
  POST /api/auth/token     申请玩家 token
  POST /api/ranks          提交排行分数
  GET  /api/ranks/:type    查询排行
`.trim())
}
async function main() {
  if (args[`--help`]) {
    handle_help();
    return;
  }
  console.log(`Little Fighter Wemake Multiplayer Server v${info.version}`)
  const ssl_key = (args['--ssl-key-path'] as string) || await read_file(process.env.SSL_KEY_FILE_PATH);
  const ssl_cer = (args['--ssl-cer-parh'] as string) || await read_file(process.env.SSL_CER_FILE_PATH);
  const https_port = (args['--port'] as Number) || Number(process.env.HTTPS_PORT) || 443
  const http_port = (args['--port'] as Number) || Number(process.env.HTTP_PORT) || 80
  const is_https = ssl_key && ssl_cer;
  const port = is_https ? https_port : http_port;
  const server = !is_https ? http.createServer() : https.createServer({
    key: ssl_key,
    cert: ssl_cer,
  })

  const wss = new WebSocketServer({ server });
  const auth = new AuthMgr();
  const ctx = new Context(
    wss,
    new RoomMgr(),
    new ClientMgr(),
    auth,
  );
  attach_rest(server, ctx, {
    admin_tokens: [args['--admin-token'] as string, process.env.ADMIN_TOKEN, process.env.ADMIN_PWD],
    info: { ssl: is_https, port, http_port, https_port },
  });
  wss.on('connection', (ws, req) => {
    const client = new Client(ctx, ws, req);
    auth.bind_from_req(client, req);
  });
  wss.on('error', e => console.error('WebSocket error:', e));
  server.listen(port)
  console.log(`${is_https ? 'wss' : 'ws'} server start, port: ${port}`);
  console.log(`${is_https ? 'https' : 'http'} rest api start, port: ${port}`);
}
main();