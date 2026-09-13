
import type { WebSocketServer } from "ws";
import type { IMsgRespMap, IResp, MsgEnum, TInfo } from "./Net";
import type { Client } from "./Client";
import type { ClientMgr } from './ClientMgr';
import type { Room } from "./Room";
import type { RoomMgr } from './RoomMgr';
import type { AuthMgr } from './rest/AuthMgr';

export class Context {
  readonly room_mgr: RoomMgr;
  readonly client_mgr: ClientMgr;
  readonly wss: WebSocketServer;
  readonly auth: AuthMgr;
  constructor(wss: WebSocketServer, room_mgr: RoomMgr, client_mgr: ClientMgr, auth: AuthMgr) {
    this.wss = wss;
    this.room_mgr = room_mgr;
    this.client_mgr = client_mgr;
    this.auth = auth;
  }
  broadcast<T extends MsgEnum, Resp extends IResp = IMsgRespMap[T]>(type: T, resp: TInfo<Resp>, ...excludes: Client[]) {
    for (const c of this.client_mgr.all)
      if (!excludes.some(v => v === c))
        c.resp(type, '', resp).catch(e => { })
  }
  room(id: string): Room | null {
    for (const r of this.room_mgr.all)
      if (r.id === id)
        return r;
    return null
  }
}
