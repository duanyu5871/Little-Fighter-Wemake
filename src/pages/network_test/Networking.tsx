
import type { IPlayerInfoCallback } from "@/LFW";
import { LFW } from "@/LFW";
import { MsgEnum, type IRespRoomStart, type NetSyncMode } from "@/Net";
import { useStateRef } from "@/hooks/useStateRef";
import { useEffect, useRef, useState, type CSSProperties } from "react";
import { ChatBox } from "./ChatBox";
import { Connection } from "./Connection";
import { ConnectionBox } from "./ConnectionBox";
import { current_connection } from "./current_connection";
import { DelayNetworkDriver } from "./DelayNetworkDriver";
import { LFWNetworkDriver } from "./LFWNetworkDriver";
import { LockstepNetworkDriver } from "./LockstepNetworkDriver";
import { RoomBox } from "./RoomBox";
import { RoomsBox } from "./RoomsBox";
import styles from "./styles.module.scss";
import { TriState } from "./TriState";
import { useCallbacks } from "./useCallbacks";
import { useRoom } from "./useRoom";
export interface INetworkingProps {
  lf2?: LFW | undefined | null;
  on_close?(): void;
  /** 调试用：强制同步模式，缺省时听服务器下发 */
  sync_mode?: NetSyncMode;
  /** 调试用：强制提前帧数 */
  input_delay?: number;
  /** 调试用：房间列表返回全部同步模式的房间 */
  show_all_rooms?: boolean;
}

export function Networking(props: INetworkingProps) {
  const { lf2, on_close, sync_mode, input_delay, show_all_rooms } = props;
  const ref_lf2 = useRef(lf2);
  ref_lf2.current = lf2;
  const [conn_state, set_conn_state] = useState<TriState>(TriState.False);
  const [conn, set_conn] = useStateRef<Connection | null>(null)
  const { room } = useRoom(conn)
  const ref_updater = useRef<LFWNetworkDriver | null>(null);
  const create_driver = (resp: IRespRoomStart) => {
    const mode = sync_mode ?? resp.sync_mode ?? 'lockstep';
    const driver = mode === 'delay'
      ? new DelayNetworkDriver(input_delay ?? resp.input_delay ?? 2)
      : new LockstepNetworkDriver();
    driver.conn = conn;
    driver.lf2 = lf2;
    ref_updater.current = driver;
    current_connection.driver = driver;
    return driver;
  };
  useEffect(() => {
    current_connection.conn = conn;
    return () => {
      current_connection.conn = null;
      current_connection.driver = null;
    };
  }, [conn]);
  const [started, set_started] = useState(false)
  const chat_style = use_fade_style(!!conn_state)
  useCallbacks(conn?.callbacks, {
    on_message: (resp, conn) => {
      const me = conn.client;
      if (!lf2 || !me) return;
      switch (resp.type) {
        case MsgEnum.ClientInfo:
          ref_updater.current?.update_client(resp);
          break;
        case MsgEnum.RoomStart:
          create_driver(resp).on_room_start(resp);
          set_started(true)
          break;
        case MsgEnum.Dataset:
          ref_updater.current?.update_dataset(resp)
          break;
        case MsgEnum.KeyTick:
        case MsgEnum.Tick: {
          ref_updater.current?.on_tick(resp);
          break;
        }
      }
    }
  }, [lf2])

  useCallbacks(lf2?.callbacks, {
    on_loading_end: () => {
      if (!lf2 || !conn) return;
      if (lf2.zips.zips.length < 1) return;
      conn?.send(MsgEnum.Tick, { seq: 0 });
    }
  }, [lf2, conn])

  useCallbacks(lf2?.world.callbacks, {
    on_dataset_change: (k, value, prev) => ref_updater.current?.on_dataset_change(k, value, prev),
  }, [lf2, conn])

  useEffect(() => {
    if (!lf2) return;
    const sync_player_names = () => {
      if (!conn) return;
      const player_names: string[] = []
      for (const [, { name }] of lf2.players)
        if (player_names.length < 8)
          player_names.push(name)
      conn.set_players(player_names)
    }
    const callback: IPlayerInfoCallback = { on_name_changed: sync_player_names }
    const watched = Array.from(lf2.players.values()).filter(v => v.local)
    for (const player of watched) player.callbacks.add(callback)
    sync_player_names()
    return () => {
      for (const player of watched) player.callbacks.del(callback)
    }
  }, [lf2, conn])


  return <>
    <ConnectionBox
      lf2={lf2}
      on_conn_change={set_conn}
      on_state_change={set_conn_state}
      on_close={on_close}
      className={styles.rooms_box}
      style={display_or_not(!conn_state)} />
    <RoomsBox
      conn={conn}
      conn_state={conn_state}
      show_all_rooms={show_all_rooms}
      style={display_or_not(conn_state && !room)} />
    <RoomBox
      conn={conn}
      className={styles.rooms_box}
      style={display_or_not(conn_state && room && !started)} />
    <ChatBox
      conn={conn}
      className={styles.chat_box}
      style={chat_style} />
  </>
}

const display_or_not = (v: any) => ({ display: v ? void 0 : 'none' })

/** 淡入淡出：隐藏时先过渡 opacity，之后再 display:none，避免瞬间消失 */
function use_fade_style(visible: boolean): CSSProperties | undefined {
  const [style, set_style] = useState<CSSProperties | undefined>(() => visible ? void 0 : { display: 'none' })
  useEffect(() => {
    if (visible) {
      set_style({ opacity: 0 })
      let raf2 = 0
      const raf1 = requestAnimationFrame(() => {
        raf2 = requestAnimationFrame(() => set_style({ opacity: 1 }))
      })
      return () => { cancelAnimationFrame(raf1); cancelAnimationFrame(raf2) }
    }
    set_style({ opacity: 0 })
    const tid = setTimeout(() => set_style({ display: 'none' }), 160)
    return () => clearTimeout(tid)
  }, [visible])
  return style
}