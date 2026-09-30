import { md5 } from "@/DittoImpl";
import { EntityEnum, GK, LFW, LFWKeyEvent, PlayerInfo, is_bot_ctrl, mt_cases, round_float, sus_cases, world_dataset_fields, type IWorldDataset } from "@/LFW";
import { MsgEnum, type IKeyEvent, type IReqTick, type IRespClientInfo, type IRespDataset, type IRespRoomStart, type IRespTick, type TInfo, type TRejoinTick } from "@/Net";
import type { IRespKeyTick } from "@/Net/IMsg_KeyTick";
import type { Connection } from "./Connection";
import { EntitySnapshotBuffer } from "./EntitySnapshotBuffer";
import { SyncChecker } from "./SyncChecker";

export function safe_json(value: unknown): string {
  const seen = new WeakSet<object>();
  try {
    return JSON.stringify(value, (_key, val) => {
      if (typeof val !== "object" || val === null) return val;
      const name = val.constructor?.name;
      if (name && /XML|Element|Node/.test(name)) return void 0;
      if (seen.has(val)) return void 0;
      seen.add(val);
      return val;
    }) ?? "null";
  } catch {
    return "?";
  }
}

let _safe_check_warn_at = 0;
export function safe_check(fn: () => string): string {
  try {
    return fn();
  } catch (e) {
    const now = Date.now();
    if (now - _safe_check_warn_at > 1000) {
      _safe_check_warn_at = now;
      console.warn(`[LFWNetworkDriver] sync check field failed:`, e);
    }
    return "?";
  }
}

function with_from(cmd: string, client_id?: string): string {
  return client_id ? `${cmd} --from=${client_id}` : cmd;
}

export abstract class LFWNetworkDriver {
  static readonly TAG = 'Lf2NetworkDriver';
  debugging: boolean = true;
  conn?: Connection | null;
  lfw?: LFW | null;
  resp?: IRespTick | IRespKeyTick | null;
  _failed: boolean = false;
  _suspended: boolean = false;
  _snapshot1?: EntitySnapshotBuffer;
  _snapshot2?: EntitySnapshotBuffer;
  _datas: SyncChecker = new SyncChecker('datas');
  _randoms?: SyncChecker;
  _objects?: SyncChecker;
  _events?: SyncChecker;
  _suspicious?: SyncChecker;
  _failed_checker?: string;
  protected _applying_dataset = false;
  protected _reverting = false;
  protected _last_run_seq = -1;
  protected _last_req?: TInfo<IReqTick>;
  get rejoin_seq(): number { return this._last_run_seq + 1 }
  is_owner() {
    const { conn } = this;
    if (!conn) return false;
    const { room, client: me } = conn;
    if (!room || !me) return false;
    return room.owner?.id === me.id;
  }
  suspend() {
    if (this._suspended) return;
    this._suspended = true;
    this.lfw?.world.sleep();
  }
  resume() {
    if (!this._suspended) return;
    this._suspended = false;
    const { lfw: lf2 } = this;
    if (!lf2) return;
    lf2.events.length = 0;
    lf2.cmds.length = 0;
    lf2.world.awake();
  }
  abstract get lead(): number;
  abstract before_update: () => void;
  abstract after_update: () => void;
  protected abstract on_tick_data(resp: IRespTick | IRespKeyTick): void;
  abstract begin_rejoin(resps: TRejoinTick[], next_seq: number): void;
  protected on_start(): void { }
  on_dataset_change(k?: keyof IWorldDataset, _value?: unknown, prev?: unknown) {
    const { conn, lfw: lf2 } = this;
    if (!conn || !lf2) return;
    if (this.is_owner()) {
      conn.send(MsgEnum.Dataset, { dataset: lf2.world.dataset.dump_dataset() }).catch(() => void 0);
      return;
    }
    if (this._applying_dataset || this._reverting) return;
    if (typeof k === 'undefined') return;
    this._reverting = true;
    (lf2.world.dataset as any)[k] = prev;
    this._reverting = false;
    console.warn(`仅房主可修改世界数据集: ${String(k)} 已回滚`);
  }
  on_room_start(resp: IRespRoomStart) {
    const { conn, lfw: lf2 } = this;
    const me = conn?.client;
    if (!conn || !lf2 || !me) return;
    lf2.world.sleep();
    const clients = conn.room?.clients;
    if (clients?.length) {
      for (const client of clients) {
        for (let i = 1; i <= 4; i++) {
          const id = `${client.id}#${i}`;
          const name = client.players?.[i - 1] ?? i.toString();
          const player = new PlayerInfo(id, name, false, client.id === me.id);
          lf2.players.set(id, player);
        }
      }
    }
    lf2.mt.debugging = this.debugging;
    if (this.debugging) {
      this._snapshot1 = new EntitySnapshotBuffer();
      this._snapshot2 = new EntitySnapshotBuffer();
      this._objects = new SyncChecker('objects');
      this._events = new SyncChecker('events');
      this._randoms = new SyncChecker('randoms');
      this._suspicious = new SyncChecker('suspicious');
    }

    const ups_arr = [30, 60]
    const atom_time_arr = ups_arr.map(v => round_float(60 / v))
    const double_click_interval_arr = ups_arr.map(v => 30 * v / 60)
    const key_hit_duration_arr = ups_arr.map(v => 10 * v / 60)
    const v = 1;
    lf2.world.dataset.UPS = ups_arr[v];
    lf2.world.dataset.atom_time = atom_time_arr[v];
    lf2.world.dataset.wait_offset = 0;
    lf2.world.dataset.fvy_f = -0.5;
    lf2.world.dataset.double_click_interval = double_click_interval_arr[v];
    lf2.world.dataset.key_hit_duration = key_hit_duration_arr[v];

    lf2.load(...LFW.ZIPS);
    lf2.layers.set_page({ id: "network_loading" }, 0);
    lf2.pointings.enabled = false;
    lf2.keyboard.enabled = false;
    lf2.mt.reset(resp.seed ?? 0, this.debugging);

    lf2.reset_new_id();
    lf2.reset_new_team();

    if (this.is_owner())
      this.on_dataset_change();
  }
  update_dataset(resp: IRespDataset) {
    const { lfw: lf2 } = this;
    if (!lf2) return;
    const incoming = resp.dataset;
    if (!incoming) return;
    const { dataset } = lf2.world;
    const local_only = new Set<keyof IWorldDataset>(['sync_render']);
    this._applying_dataset = true;
    for (const key of world_dataset_fields.keys()) {
      if (local_only.has(key)) continue;
      const value = (incoming as any)[key];
      if (typeof value !== 'undefined')
        (dataset as any)[key] = value;
    }
    this._applying_dataset = false;
  }
  update_client(resp: IRespClientInfo) {
    const { lfw: lf2 } = this;
    const { client } = resp;
    if (!client) return;
    if (!lf2) return;

    for (let i = 1; i <= 4; i++) {
      const id = `${client.id}#${i}`;
      const name = client.players?.[i - 1] ?? i.toString();
      const player = lf2.players.get(id);
      if (!player) continue;
      player.set_name(name, true);
    }
  }
  on_tick(resp: IRespTick | IRespKeyTick) {
    const { conn, lfw: lf2 } = this;
    if (!conn || !lf2) return;
    if (this._failed) return;
    if (typeof resp.seq !== 'number') return;
    if (resp.seq === 0) this.start(lf2);
    this.on_tick_data(resp);
  }
  protected start(lf2: LFW) {
    lf2.keyboard.enabled = true;
    lf2.pointings.enabled = true;
    lf2.world.after_update = this.after_update;
    lf2.world.before_update = this.before_update;
    lf2.world.reset_game_time();
    lf2.layers.set_page({ id: "main_page" }, 0);
    this.on_start();
  }
  protected run_tick(seq: number, resp: IRespTick | IRespKeyTick): void {
    const { lfw: lf2, conn } = this;
    if (!lf2 || !conn) return;
    const { world } = lf2;
    const { reqs } = resp;
    const me = conn.client;
    if (!me) {
      console.error(`[${LFWNetworkDriver.TAG}::run_tick] failed! 'conn.client' got ${me}`);
      return world.sleep();
    }
    if (!reqs?.length) {
      console.error(`[${LFWNetworkDriver.TAG}::run_tick] failed! 'resp.reqs.length' got ${reqs?.length}`);
      return world.sleep();
    }
    this.resp = resp;
    this._last_run_seq = seq;
    this.apply_bot_events(resp);
    const req_events: IKeyEvent[] = lf2.events.map<IKeyEvent>(r => ({
      client_id: me.id,
      player_id: me.id + '#' + r.player,
      game_key: r.game_key,
      pressed: r.pressed,
    }));
    const req: TInfo<IReqTick> = {
      seq: seq + this.lead,
      cmds: lf2.cmds.map(cmd => with_from(cmd, me.id)),
      events: req_events
    };
    if (seq == 0) {
      const groups: [string, Array<{ id?: string }>][] = [
        ['objects', lf2.datas.objects],
        ['backgrounds', lf2.datas.backgrounds],
        ['bots', lf2.datas.bots],
        ['stages', lf2.datas.stages],
      ];
      req._d = safe_check(() => groups
        .map(([k, list]) => `${k}=` + (list ?? []).map(v => `${v?.id ?? '?'}:${md5(safe_json(v))}`).join(','))
        .join('|'));
    }
    if (this._events) req._a = safe_check(() => `game_time=${lf2.world.game_time}`);
    if (this._randoms) req._r = safe_check(() => mt_cases.submit());
    if (this._objects) req._p = safe_check(() => Array.from(lf2.world.entities).map((e) => {
      try {
        const { x, y, z } = e.position;
        const { x: vx, y: vy, z: vz } = e.velocity;
        const t = EntityEnum[e.data.type]
        const b = is_bot_ctrl(e.ctrl) ? e.ctrl.fsm.state?.key : 'x';
        return [t, e.id, e.name, e.frame.id, b, x, y, z, vx, vy, vz].join('_');
      } catch {
        return '?';
      }
    }).join('￥'));
    if (this._suspicious) req._s = safe_check(() => sus_cases.submit());
    if (!this._failed) {
      this._last_req = req;
      conn.send_nowait(MsgEnum.Tick, req);
    }
    lf2.cmds.length = 0;
    lf2.events.length = 0;
    this._objects?.reset();
    this._randoms?.reset();
    this._events?.reset();
    this._suspicious?.reset();
    for (const req of reqs) {
      const { _d, _r, _p, _a, _s } = req;
      if (seq == 0) this.sync_check(this._datas, _d, resp)
      if (this._events) this.sync_check(this._events, _a, resp);
      if (this._randoms) this.sync_check(this._randoms, _r, resp);
      if (this._objects) this.sync_check(this._objects, _p, resp);
      if (this._suspicious) this.sync_check(this._suspicious, _s, resp);
      if (this._failed) break;
    }
    if (this._failed) this.dump_snapshots();
    if (this._failed) world.sleep();
    if (this._failed) return;

    if (this.debugging) this._snapshot1?.capture(lf2.world.entities)
    for (const req of reqs) {
      const { cmds, events } = req;
      if (cmds?.length) cmds.forEach(cmd => lf2.push_cmd(cmd));
      if (!events?.length) continue;
      for (const { player_id, pressed = false, game_key = '' } of events) {
        if (!player_id) continue;
        const gk = game_key as GK;
        const le = new LFWKeyEvent(player_id, pressed, gk, gk);
        lf2.events.push(le);
      }
    }
  };
  protected apply_bot_events(resp: IRespTick | IRespKeyTick) {
    const { lfw: lf2 } = this;
    const events = (resp as IRespTick).bot_events;
    if (!lf2 || !events?.length) return;
    for (const { client_id, to_bot } of events) {
      if (!client_id) continue;
      const prefix = client_id + '#';
      for (const [player_id] of lf2.players)
        if (player_id.startsWith(prefix))
          lf2.set_player_bot(player_id, !!to_bot);
    }
  }
  continue_solo() {
    const { lfw: lf2 } = this;
    if (!lf2) return;
    for (const [player_id, player] of lf2.players)
      if (!player.mine) lf2.set_player_bot(player_id, true);
    lf2.world.before_update = void 0;
    lf2.world.after_update = void 0;
    lf2.events.length = 0;
    lf2.cmds.length = 0;
    this._suspended = false;
    lf2.world.awake();
  }
  private dump_snapshots() {
    const { _snapshot1, _snapshot2 } = this;
    if (!_snapshot1 || !_snapshot2) return;
    const data = {
      game_time: this.lfw?.world.game_time,
      snapshot1: this._snapshot1?.to_readable(),
      snapshot2: this._snapshot2?.to_readable(),
      events: [
        this._events?.result?.value1?.split('￥'),
        this._events?.result?.value2?.split('￥')
      ],
      randoms: [
        this._randoms?.result?.value1?.split('￥'),
        this._randoms?.result?.value2?.split('￥')
      ],
      objects: [
        this._objects?.result?.value1?.split('￥'),
        this._objects?.result?.value2?.split('￥')
      ],
      suspicious: [
        this._suspicious?.result?.value1?.split('￥'),
        this._suspicious?.result?.value2?.split('￥')
      ]
    };
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    const ua = navigator.userAgent;
    const browser = /Edg\//.test(ua) ? 'edge'
      : /OPR\/|Opera/.test(ua) ? 'opera'
        : /Firefox\//.test(ua) ? 'firefox'
          : /Chrome\//.test(ua) ? 'chrome'
            : /Safari\//.test(ua) ? 'safari'
              : 'unknown';
    a.download = `lf2-snapshot-${data.game_time ?? Date.now()}_${browser}_${md5(ua)}.json`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }
  private sync_check(checker: SyncChecker, value: string | undefined, resp: IRespTick | IRespKeyTick) {
    if (!checker.test(value)) return;
    checker.print_error();
    console.error(resp);
    this._failed = true;
  }
}
