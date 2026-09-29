import type { Client } from './Client';
import type { Context } from "./Context";
import {
  ErrCode,
  IBotEvent,
  IClientInfo,
  IMsgRespMap,
  IReqAbandon,
  IReqClientReady,
  IReqCloseRoom, IReqCreateRoom,
  IReqExitRoom,
  IReqJoinRoom, IReqKick,
  IReqRejoin,
  IReqRoomPwd,
  IReqRoomSync,
  IReqRoomStart,
  IReqTick,
  IResp,
  IRespAbandon,
  IRespClientReady,
  IRespCloseRoom,
  IRespExitRoom,
  IRespJoinRoom, IRespKick,
  IRespRejoin,
  IRespRoomSync,
  IRespRoomStart,
  IRespTick,
  IDataInfo, IRoomInfo, MsgEnum, NetSyncMode, RoomSyncMode, SystemPlayerInfo, TInfo,
  resolve_sync
} from "./Net";
import { random_str } from './random_str';
import { next_msg_seq } from './handle_req_chat';

let room_id = 0;

const MAX_PIPELINE = 16;
const MAX_TICK_CACHE = 4096;
const REJOIN_TTL = 5 * 60 * 1000;
export class Room {
  static TAG = 'Room';
  readonly id = '' + (++room_id);
  readonly ctx: Context;
  protected _code: string = '';
  owner: Client;
  min_players: number = 2;
  max_players: number = 4;
  title: string = `ROOM_${this.id}`;
  clients = new Set<Client>();
  tick_req_maps = new Map<number, Map<Client, IReqTick>>();
  tick_resp_cache = new Map<number, TInfo<IRespTick>>();
  rejoin_records = new Map<string, { secret: string; client_info: Required<IClientInfo>; disconnected_at: number }>();
  pending_bot_events: IBotEvent[] = [];
  bot_clients = new Set<string>();
  protected _cleanup_timer?: ReturnType<typeof setTimeout>;
  private _tick_seq = -1;
  /** 房主选择的同步模式 */
  sync_mode: RoomSyncMode = 'auto';
  /** 开局时根据成员延迟定下的实际模式 */
  protected _start_sync: { sync_mode: NetSyncMode, input_delay: number } = { sync_mode: 'lockstep', input_delay: 1 };
  seed: number;
  pwd: string = '';
  lfw_version: string = '';
  data_infos: IDataInfo[] = [];
  get code() { return this._code; }
  get started() { return this._tick_seq >= 0; }
  get room_info(): Required<IRoomInfo> {
    return {
      title: this.title,
      code: this._code,
      id: this.id,
      owner: this.owner.client_info!,
      clients: Array.from(this.clients).map(v => ({
        ...v.client_info!,
        ready: v.ready
      })),
      min_players: this.min_players,
      max_players: this.max_players,
      started: this.started,
      sync_mode: this.sync_mode,
      need_pwd: !!this.pwd,
      lfw_version: this.lfw_version,
      data_infos: this.data_infos,
    }
  }
  constructor(owner: Client, req: IReqCreateRoom) {
    const { ctx } = owner
    this.ctx = ctx
    this.sync_mode = req.sync_mode ?? 'auto';
    this.owner = owner;
    this.seed = Date.now()
    while (!this._code || ctx.room_mgr.codes.has(this._code)) {
      this._code = random_str();
    }
    ctx.room_mgr.add(this)
    this.title = req.title?.trim() || `${owner.client_info?.name}的房间`
    const { max_players = 4, min_players = 2 } = req
    if (typeof max_players === 'number')
      this.max_players = Math.max(1, Math.floor(max_players))
    if (typeof min_players === 'number')
      this.min_players = Math.max(1, Math.floor(min_players))
    const min = Math.min(this.max_players, this.min_players);
    const max = Math.max(this.max_players, this.min_players);
    this.min_players = min;
    this.max_players = max
    this.lfw_version = req.lfw_version?.trim() ?? ''
    this.data_infos = (req.data_infos ?? []).map(v => ({ ...v }))
    this.clients.add(owner);
    owner.room = this;
    owner.resp(req.type, req.pid, { room: this.room_info })
    console.log(`[${Room.TAG}::constructor] owner: ${this.owner.id}`)
  }

  ready(client: Client, req: IReqClientReady = { type: MsgEnum.ClientReady, is_req: true, pid: '' }) {
    console.log(`[${Room.TAG}::ready]`)
    const { clients: players } = this;
    const { client_info: player_info, room } = client;
    if (!players.has(client)) return false;
    if (!player_info) return false;
    if (room !== this) return false;

    client.ready = req.ready ?? client.ready;
    const resp: TInfo<IRespClientReady> = { client: player_info, ready: client.ready }
    this.broadcast(req.type, resp, client)
    client.resp(req.type, req.pid, resp)
    return true;
  }
  kick(req: IReqKick = { type: MsgEnum.Kick, is_req: true, pid: '' }) {
    const { clients: players } = this;
    let client: Client | null = null
    for (const p of players) {
      if (p.id === req.client_id) {
        client = p;
        break;
      }
    }
    if (!client) return false;
    const { client_info: player_info, room } = client;
    if (room !== this) return false;

    client.ready = false
    delete client.room
    players.delete(client)
    if (this.owner === client && players.size)
      room.owner = players.values().next().value!;
    const { room_info } = this;
    const resp: TInfo<IRespKick> = {
      client: player_info,
      room: room_info
    }
    this.broadcast(req.type, resp, client)
    client.resp(req.type, req.pid, resp).catch(() => void 0)
    this.drop_pending_reqs(client)
    this.flush_ticks()
    if (!this.clients.size && !this.rejoin_records.size)
      this.ctx.room_mgr.del(room)
  }
  exit(client: Client, req: IReqExitRoom = { type: MsgEnum.ExitRoom, is_req: true, pid: '' }) {
    console.log(`[${Room.TAG}::exit]`)
    const { clients: players } = this;
    const { client_info: player_info, room } = client;
    if (!players.has(client)) return;
    if (!player_info) return;
    if (room !== this) return;

    client.ready = false
    delete client.room
    players.delete(client)
    if (this.owner === client && players.size)
      room.owner = players.values().next().value!;
    const { room_info } = this;
    const resp: TInfo<IRespExitRoom> = {
      client: player_info,
      room: room_info
    }
    this.broadcast(req.type, resp, client)
    client.resp(req.type, req.pid, resp).catch(() => void 0)

    for (const pl of players)
      pl.resp(MsgEnum.Chat, '', { target: 'room', sender: SystemPlayerInfo, text: `玩家[${player_info.name}]退出了房间`, seq: next_msg_seq() }).catch(() => void 0)
    this.drop_pending_reqs(client)
    this.flush_ticks()
    if (!this.clients.size && !this.rejoin_records.size)
      this.ctx.room_mgr.del(this)
  }
  disconnect(client: Client) {
    console.log(`[${Room.TAG}::disconnect]`)
    const { clients: players } = this;
    const { client_info: player_info, room } = client;
    if (!players.has(client)) return;
    if (!player_info) return;
    if (room !== this) return;

    client.ready = false
    delete client.room
    players.delete(client)
    if (this.owner === client && players.size)
      room.owner = players.values().next().value!;
    const { room_info } = this;
    const resp: TInfo<IRespExitRoom> = {
      client: player_info,
      room: room_info
    }
    this.broadcast(MsgEnum.ExitRoom, resp, client)
    for (const pl of players)
      pl.resp(MsgEnum.Chat, '', { target: 'room', sender: SystemPlayerInfo, text: `玩家[${player_info.name}]掉线了，等待重连…`, seq: next_msg_seq() }).catch(() => void 0)
    this.drop_pending_reqs(client)
    this.flush_ticks()
    this.rejoin_records.set(player_info.id!, {
      secret: client.secret,
      client_info: player_info,
      disconnected_at: Date.now(),
    })
    if (!players.size)
      this.schedule_cleanup()
  }
  rejoin(client: Client, req: IReqRejoin) {
    console.log(`[${Room.TAG}::rejoin]`)
    const fail = (error: string, del_record = false) => {
      if (del_record && req.client_id) this.rejoin_records.delete(req.client_id)
      client.resp(req.type, req.pid, { code: ErrCode.RejoinFailed, error }).catch(() => void 0)
      return false
    }
    const { client_id, secret, from_seq } = req;
    if (!client_id || !secret) return fail('invalid rejoin request')
    const record = this.rejoin_records.get(client_id)
    if (!record || record.secret !== secret) return fail('rejoin record not found')
    if (Date.now() - record.disconnected_at > REJOIN_TTL) {
      this.rejoin_records.delete(client_id)
      return fail('rejoin timeout')
    }
    if (!this.started) {
      this.rejoin_records.delete(client_id)
      return fail('room not started')
    }
    const next = this._tick_seq;
    const need = typeof from_seq === 'number' ? from_seq : -1
    if (need < 0 || need > next) return fail('invalid from_seq')
    const resps: TInfo<IRespTick>[] = []
    for (let s = need; s < next; s++) {
      const r = this.tick_resp_cache.get(s)
      if (!r) {
        this.rejoin_records.delete(client_id)
        return fail('rejoin too late')
      }
      resps.push(r)
    }
    this.rejoin_records.delete(client_id)
    this.cancel_cleanup()
    if (this.bot_clients.delete(client_id))
      this.pending_bot_events.push({ client_id, to_bot: false })

    client.id = client_id
    client.secret = record.secret
    client.client_info = { ...record.client_info }
    client.ready = true
    client.room = this
    this.clients.add(client)

    const resp: TInfo<IRespRejoin> = {
      client: client.client_info,
      room: this.room_info,
      next_seq: next,
      resps,
    }
    client.resp(req.type, req.pid, resp).catch(() => void 0)
    this.broadcast(req.type, { client: client.client_info, room: this.room_info }, client)
    for (const pl of this.clients)
      pl.resp(MsgEnum.Chat, '', { target: 'room', sender: SystemPlayerInfo, text: `玩家[${client.client_info.name}]重新连接`, seq: next_msg_seq() }).catch(() => void 0)
    return true
  }
  protected schedule_cleanup() {
    if (this._cleanup_timer) return;
    this._cleanup_timer = setTimeout(() => {
      this._cleanup_timer = void 0;
      if (!this.clients.size) this.ctx.room_mgr.del(this)
    }, REJOIN_TTL)
  }
  continue_without_leavers() {
    for (const [client_id, record] of this.rejoin_records) {
      if (this.bot_clients.has(client_id)) continue;
      this.bot_clients.add(client_id)
      this.pending_bot_events.push({ client_id, to_bot: true })
      for (const pl of this.clients)
        pl.resp(MsgEnum.Chat, '', { target: 'room', sender: SystemPlayerInfo, text: `玩家[${record.client_info.name}]掉线，由电脑接管`, seq: next_msg_seq() }).catch(() => void 0)
    }
  }
  abandon(client: Client, req: IReqAbandon) {
    console.log(`[${Room.TAG}::abandon]`)
    const fail = (error: string) => {
      client.resp(req.type, req.pid, { code: ErrCode.AbandonFailed, error }).catch(() => void 0)
      return false
    }
    const { client_id, secret } = req
    if (!client_id || !secret) return fail('invalid abandon request')
    const record = this.rejoin_records.get(client_id)
    if (!record || record.secret !== secret) return fail('abandon record not found')
    this.rejoin_records.delete(client_id)
    if (!this.bot_clients.has(client_id)) {
      this.bot_clients.add(client_id)
      this.pending_bot_events.push({ client_id, to_bot: true })
    }
    const resp: TInfo<IRespAbandon> = { client: record.client_info }
    this.broadcast(req.type, resp)
    for (const pl of this.clients)
      pl.resp(MsgEnum.Chat, '', { target: 'room', sender: SystemPlayerInfo, text: `玩家[${record.client_info.name}]已离开对局`, seq: next_msg_seq() }).catch(() => void 0)
    client.resp(req.type, req.pid, resp).catch(() => void 0)
    if (!this.clients.size && !this.rejoin_records.size)
      this.ctx.room_mgr.del(this)
    return true
  }
  protected cancel_cleanup() {
    if (!this._cleanup_timer) return;
    clearTimeout(this._cleanup_timer)
    this._cleanup_timer = void 0
  }
  join(client: Client, req: IReqJoinRoom = { type: MsgEnum.JoinRoom, is_req: true, pid: '' }) {
    console.log(`[${Room.TAG}::join]`)
    const { clients } = this;
    const { client_info, room } = client;
    if (clients.has(client)) return false;
    if (!client_info) return false;
    if (room) return false;

    if (this.clients.size >= this.max_players) {
      client.resp(req.type, req.pid, { code: ErrCode.RoomIsFull, error: 'Room is full!' })
      return false
    }
    const { pwd: a_pwd = '' } = this;
    const { pwd: b_pwd = '' } = req;
    if (a_pwd != b_pwd) {
      client.resp(req.type, req.pid, { code: ErrCode.RoomPwdWrong, error: 'Wrong password!' })
      return false
    }
    const { lfw_version = '', data_infos = [] } = req
    if (this.lfw_version && this.lfw_version !== (lfw_version?.trim() ?? '')) {
      client.resp(req.type, req.pid, { code: ErrCode.RoomVersionMismatch, error: `LFW 版本不匹配! (房间: ${this.lfw_version}, 你: ${lfw_version})` })
      return false
    }
    const room_md5s = this.data_infos.map(v => v.md5?.trim() ?? '').filter(Boolean)
    if (room_md5s.length) {
      const join_md5s = (data_infos ?? []).map(v => v.md5?.trim() ?? '').filter(Boolean)
      const matched =
        room_md5s.length === join_md5s.length &&
        room_md5s.every(m => join_md5s.includes(m))
      if (!matched) {
        client.resp(req.type, req.pid, {
          code: ErrCode.RoomVersionMismatch,
          error: `数据包 MD5 不匹配! (房间: ${room_md5s.join(', ')}, 你: ${join_md5s.join(', ')})`
        })
        return false
      }
    }
    const all_client_ready = Array.from(this.clients).every(v => v.ready)
    if (this._tick_seq >= 0 || all_client_ready) {
      client.resp(req.type, req.pid, { code: ErrCode.RoomAlreadyStart, error: 'Game of this room has already started!' })
      return false
    }


    clients.add(client);
    client.ready = false
    client.room = this;
    const { room_info } = this;

    const resp: TInfo<IRespJoinRoom> = {
      client: client_info,
      room: room_info
    }
    this.broadcast(req.type, resp, client)
    client.resp(req.type, req.pid, resp).catch(() => void 0)
    for (const pl of clients)
      pl.resp(MsgEnum.Chat, '', { target: 'room', sender: SystemPlayerInfo, text: `玩家[${client_info.name}]加入了房间`, seq: next_msg_seq() }).catch(() => void 0)
    return true;
  }

  close(client: Client, info?: TInfo<IReqCloseRoom>) {
    console.log(`[${Room.TAG}::close]`)
    const req: IReqCloseRoom = { ...info, type: MsgEnum.CloseRoom, is_req: true, pid: '' }
    const { clients: players } = this;
    const { client_info: player_info, room } = client;
    if (!players.has(client)) return;
    if (!player_info) return;
    if (room !== this) return;

    const { room_info } = this
    const resp: TInfo<IRespCloseRoom> = {
      room: room_info
    }
    this.broadcast(req.type, resp, client)
    client.resp(req.type, req.pid, resp).catch(() => void 0)
    for (const pl of players) pl.resp(MsgEnum.Chat, '', { target: 'room', sender: SystemPlayerInfo, text: '房间已关闭', seq: next_msg_seq() }).catch(() => void 0)
    for (const pl of players) delete pl.room
    players.clear()
    this.ctx.room_mgr.del(this)
  }

  start(client: Client, info?: TInfo<IReqRoomStart>) {
    const req: IReqRoomStart = { ...info, is_req: true, type: MsgEnum.RoomStart, pid: '' }
    const { clients: players } = this;
    if (players.size < this.min_players) {
      client.resp(req.type, req.pid, { code: ErrCode.PlayersTooFew, error: 'players are too few' }).catch(() => void 0)
      return;
    }

    this._start_sync = resolve_sync(this.sync_mode, this.max_rtt());
    const start_info: TInfo<IRespRoomStart> = {
      seed: this.seed,
      sync_mode: this._start_sync.sync_mode,
      input_delay: this._start_sync.input_delay,
    };
    this.broadcast(req.type, start_info, client)
    client.resp(req.type, req.pid, start_info).catch(() => void 0)
    this._tick_seq = 0
  }

  set_sync(client: Client, req: IReqRoomSync) {
    if (req.sync_mode) this.sync_mode = req.sync_mode;
    const resp: TInfo<IRespRoomSync> = { room: this.room_info };
    this.broadcast(req.type, resp);
    client.resp(req.type, req.pid, resp).catch(() => void 0);
  }

  protected max_rtt(): number {
    let ret = 0;
    for (const c of this.clients)
      ret = Math.max(ret, c.rtt);
    return ret;
  }

  broadcast<T extends MsgEnum, Resp extends IResp = IMsgRespMap[T]>(type: T, resp: TInfo<Resp>, ...excludes: Client[]) {
    for (const c of this.clients)
      if (!excludes.some(v => v === c))
        c.resp(type, '', resp).catch(e => { })
  }

  tick(client: Client, req: IReqTick) {
    const seq = req.seq;
    if (typeof seq !== 'number' || seq < this._tick_seq) return;
    if (seq > this._tick_seq + MAX_PIPELINE) return;
    req.client_id = client.client_info?.id;
    if (seq === 0)
      req.client_name = client.client_info?.name;
    let map = this.tick_req_maps.get(seq);
    if (!map) this.tick_req_maps.set(seq, map = new Map());
    map.set(client, req);
    this.flush_ticks();
  }

  protected drop_pending_reqs(client: Client) {
    for (const [seq, map] of this.tick_req_maps) {
      map.delete(client);
      if (!map.size) this.tick_req_maps.delete(seq);
    }
  }

  protected flush_ticks() {
    for (; ;) {
      const curr = this.tick_req_maps.get(this._tick_seq);
      if (!curr || curr.size !== this.clients.size) break;
      const resp: TInfo<IRespTick> = { seq: this._tick_seq, reqs: [] }
      for (const [, r] of curr)
        resp.reqs?.push(r)
      if (this.pending_bot_events.length)
        resp.bot_events = this.pending_bot_events.splice(0)
      this.broadcast(MsgEnum.Tick, resp)
      this.tick_resp_cache.set(this._tick_seq, resp)
      while (this.tick_resp_cache.size > MAX_TICK_CACHE)
        this.tick_resp_cache.delete(this.tick_resp_cache.keys().next().value!)
      this.tick_req_maps.delete(this._tick_seq);
      this._tick_seq++;
    }
  }

  set_pwd(client: Client, req: IReqRoomPwd) {
    this.pwd = req.pwd ?? '';
  }
}
