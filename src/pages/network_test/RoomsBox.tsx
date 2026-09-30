import { Input, type InputRef } from "@/Component/Input";
import { useFloating } from "@/hooks/useFloating";
import { useForwardedRef } from "@/hooks/useForwardedRef";
import { useStateRef } from "@/hooks/useStateRef";
import { LFW } from "@/LFW";
import classNames from "classnames";
import { type ForwardedRef, forwardRef, useCallback, useEffect, useMemo, useRef, useState } from "react";
import { useTranslation } from "react-i18next";
import { Button } from "../../Component/Buttons/Button";
import { Divider } from "../../Component/Divider";
import { ArrowLeft } from "../../Component/Icons/ArrowLeft";
import { ArrowRight } from "../../Component/Icons/ArrowRight";
import { Clock } from "../../Component/Icons/Clock";
import { Cross } from "../../Component/Icons/Cross";
import { Plus } from "../../Component/Icons/Plus";
import { Refresh } from "../../Component/Icons/Refresh";
import { Flex } from "../../Component/Flex";
import Frame, { type IFrameProps } from "../../Component/Frame";
import Show from "../../Component/Show";
import { Strong, Text } from "../../Component/Text";
import { type IRoomInfo, MsgEnum } from "../../Net";
import { Connection } from "./Connection";
import { SYNC_MODE_LABEL } from "./RoomBox";
import styles from "./styles.module.scss";
import { TriState } from "./TriState";
import { useCallbacks } from "./useCallbacks";
import { useRoom } from "./useRoom";
import { useRooms } from "./useRooms";

export interface IRoomsBoxProps extends IFrameProps {
  conn?: Connection | null;
  conn_state?: TriState;
  lfw?: LFW | null;
  /** 是否显示全部同步模式的房间（缺省由服务器过滤） */
  show_all_rooms?: boolean;
  page_size?: number;
}
function _RoomsBox(props: IRoomsBoxProps, f_ref: ForwardedRef<HTMLDivElement>) {
  const { t } = useTranslation()
  const {
    conn = null,
    conn_state = TriState.False,
    className,
    show_all_rooms,
    page_size = 5,
    ..._p
  } = props;

  const [room_creating, set_room_creating, ref_room_creating] = useStateRef<boolean>(false);
  const [room_joining, set_room_joining, ref_room_joining] = useStateRef<boolean>(false);
  const [auto_refresh, set_auto_refresh] = useState<boolean>(true);
  const { room } = useRoom(conn)
  const { rooms } = useRooms(conn)
  const cls_name = classNames(styles.rooms_box, className)

  const [page, set_page] = useState<number>(0);
  const page_count = Math.max(1, Math.ceil((rooms?.length ?? 0) / Math.max(1, page_size)));
  const page_index = Math.min(Math.max(0, page), page_count - 1);
  const page_rooms = useMemo(
    () => (rooms ?? []).slice(page_index * page_size, (page_index + 1) * page_size),
    [rooms, page_index, page_size])
  const goto_page = useCallback((p: number) => {
    set_page(Math.min(Math.max(0, p), page_count - 1))
  }, [page_count])
  useEffect(() => {
    if (page > page_count - 1) set_page(page_count - 1)
  }, [page, page_count])

  const update_rooms = useCallback(() => {
    if (!conn) return;
    conn.send(MsgEnum.ListRooms, { show_all: show_all_rooms }, { loose: true }).catch(e => { })
  }, [conn, show_all_rooms])

  const ref_rtt = useRef<HTMLSpanElement>(null)
  useCallbacks(conn?.callbacks, {
    on_ping: (resp, conn) => {
      if (resp.client !== conn.client?.id) return;
      const el = ref_rtt.current;
      if (el) el.innerText = `${conn.rtt}ms`
    }
  }, [])

  useEffect(() => {
    if (!conn || conn_state !== TriState.True || room)
      return;
    update_rooms();
    const c = conn.callbacks.add({
      on_message: (resp) => {
        switch (resp.type) {
          case MsgEnum.ExitRoom:
          case MsgEnum.Kick:
            update_rooms();
            break;
          case MsgEnum.CloseRoom:
            update_rooms();
            break;
        }
      }
    });
    return () => c()
  }, [conn, conn_state === TriState.True, room])

  useEffect(() => {
    if (!conn || !auto_refresh || conn_state !== TriState.True || room)
      return;
    const tid = setInterval(update_rooms, 3000);
    return () => clearInterval(tid)
  }, [conn, auto_refresh, conn_state, room, update_rooms])

  async function get_version_info() {
    return {
      lfw_version: LFW.VERSION_NAME,
      data_infos: await LFW.collect_data_infos(),
    }
  }

  function create_room() {
    if (
      ref_room_joining.current ||
      ref_room_creating.current
    ) return;
    if (!conn) return;
    set_room_creating(true)
    get_version_info().then((version_info) =>
      conn.send(MsgEnum.CreateRoom, {
        min_players: 1,
        max_players: 4,
        ...version_info,
      })
    ).then(() => {
      update_rooms()
    }).catch(e => {
      console.log(e)
    }).finally(() => {
      set_room_creating(false)
    })
  }

  function join_room(roomid: string, pwd?: string) {
    if (
      !conn ||
      ref_room_joining.current ||
      ref_room_creating.current
    ) return;
    set_room_joining(true)
    get_version_info().then((version_info) =>
      conn.send(MsgEnum.JoinRoom, { roomid, pwd, ...version_info })
    ).catch(e => {
      alert('' + e)
    }).finally(() => {
      set_room_joining(false)
    })
  }
  const [ref_floating_view, on_ref] = useForwardedRef(f_ref)
  useFloating({
    responser: ref_floating_view.current?.firstElementChild as HTMLElement,
    target: ref_floating_view.current,
    followPercent: true,
    resizable: true,
    min_width: 260,
    min_height: 29,
  })
  return (
    <Frame {..._p} className={cls_name} ref={on_ref}>
      <Flex gap={10} align='stretch' justify='space-between'>
        <Flex align='center' style={{ flex: 1, paddingLeft: 5, overflow: 'hidden' }} gap={5}>
          <Strong style={{ textOverflow: 'ellipsis', whiteSpace: 'nowrap', wordBreak: 'keep-all' }}>
            {t('room_list')}
          </Strong>
          <Text style={{ textOverflow: 'ellipsis', whiteSpace: 'nowrap', wordBreak: 'keep-all' }}>
            {conn?.url}
          </Text>
          <Text ref={ref_rtt} />
        </Flex>
        <Flex align='center'>
          <Show show={!room && conn_state && !room_joining && !room_creating}>
            <Button
              variants={['no_border', 'no_round', 'no_shadow']}
              title={t('create_room')}
              onClick={() => create_room()}>
              <Plus />
            </Button>
          </Show>
          <Button
            variants={['no_border', 'no_round', 'no_shadow']}
            title={t('refresh')}
            onClick={() => update_rooms()} >
            <Refresh />
          </Button>
          <Button
            variants={['no_border', 'no_round', 'no_shadow']}
            actived={auto_refresh}
            title={t('auto_refresh')}
            onClick={() => set_auto_refresh(!auto_refresh)} >
            <Clock />
          </Button>
          <Button
            variants={['no_border', 'no_round', 'no_shadow']}
            title={t('disconnect')}
            onClick={() => conn?.close()} >
            <Cross style={{ fontSize: '1.18em' }} />
          </Button>
        </Flex>
      </Flex>
      {rooms?.length === 0 ?
        <Flex direction='column' align='center' justify='center' style={{ height: 65, opacity: 0.5 }}>
          <Text style={{ textOverflow: 'ellipsis', whiteSpace: 'nowrap', wordBreak: 'keep-all' }}>
            {t('no_rooms')}
          </Text>
        </Flex> : <Divider />
      }
      <div className={styles.room_list}>
        {page_rooms.map(r => <RoomItem
          room={r}
          conn={conn}
          key={r.id}
          join_room={join_room} />)}
      </div>
      <Show show={(rooms?.length ?? 0) > page_size}>
        <Divider />
        <div className={styles.rooms_pager}>
          <Text size='ss' className={styles.rooms_pager_total}>
            {t('rooms_total').replace('%1', `${rooms?.length ?? 0}`)}
          </Text>
          <Flex align='center' justify='center' gap={8}>
            <Button
              variants={['no_border', 'no_round', 'no_shadow']}
              disabled={page_index <= 0}
              title={t('prev_page')}
              onClick={() => goto_page(page_index - 1)}>
              <ArrowLeft />
            </Button>
            <Text size='s' style={{ minWidth: 52, textAlign: 'center' }}>
              {page_index + 1} / {page_count}
            </Text>
            <Button
              variants={['no_border', 'no_round', 'no_shadow']}
              disabled={page_index >= page_count - 1}
              title={t('next_page')}
              onClick={() => goto_page(page_index + 1)}>
              <ArrowRight />
            </Button>
          </Flex>
        </div>
      </Show>
    </Frame>
  )
}

interface IRoomItemProps {
  room: IRoomInfo;
  conn: Connection | null;
  join_room(id: string, pwd?: string): void
}
function RoomItem(props: IRoomItemProps) {
  const { t } = useTranslation()
  const { room: r, join_room, conn } = props;
  const ref_input = useRef<InputRef>(null);
  const { room } = useRoom(conn)
  if (!conn) return null;
  const count = r.clients?.length ?? 0;
  const data_title = r.lfw_version || r.data_infos?.length ?
    `${t('lfw_version')}: ${r.lfw_version ?? '-'}\n` +
    (r.data_infos ?? []).map((v, i) =>
      `${t('data_package')}[${i + 1}] ${v.type ? `[${v.type}] ` : ''}${v.title ?? ''}${typeof v.version === 'number' ? ` v${v.version}` : ''}${v.md5 ? ` · ${v.md5}` : ''}`,
    ).join('\n')
    : ''
  const players_title = `${t('player_count')}: ${count}/${r.max_players}${r.min_players ? ` (≥${r.min_players})` : ''}\n` +
    (r.clients ?? []).map(c =>
      `${c.ready ? '✓ ' : ''}${c.name}${c.id === r.owner?.id ? ' 👑' : ''}`,
    ).join('\n')
  return (
    <Flex direction='column' gap={3} className={styles.room_card}
      style={{ padding: '6px 10px', boxSizing: 'border-box' }}>
      <Flex gap={8} align='center' justify='space-between'>
        <Flex gap={8} align='center' style={{ flex: 1, minWidth: 0, overflow: 'hidden' }}>
          <Strong className={styles.room_title} title={`${t('room_name')}: ${r.title}`}>
            {r.title}
          </Strong>
          <Text size='ss' className={styles.room_code} title={t('room_code')}>#{r.code}</Text>
          <Text
            size='ss'
            className={r.started ? styles.room_badge_running : styles.room_badge}
            title={r.started ? t('game_running') : t('waiting')}>
            {r.started ? t('game_running') : t('waiting')}
          </Text>
        </Flex>
        {
          r.need_pwd ? <Input
            prefix="🔒"
            style={{ width: 130, flex: 'none' }}
            variants={['no_border']}
            size='s'
            placeholder={t('pls_enter_pwd')}
            ref={ref_input} /> :
            null
        }
        <Button
          variants={['no_border', 'no_round', 'no_shadow']}
          disabled={!!room || r.started}
          onClick={() => join_room(r.id!, ref_input.current?.value?.toString() || void 0)}>
          {t("join")}
        </Button>
      </Flex>
      <Flex gap={10} align='center' className={styles.room_meta}>
        <Text size='ss' style={{ flex: 'none' }} title={t('room_owner')}>
          👑{r.owner?.name ?? '-'}
        </Text>
        <Text size='ss' style={{ flex: 'none' }} title={players_title}>
          {t('player_count')}: {count}/{r.max_players}
        </Text>
        <Text size='ss' style={{ flex: 'none' }} title={t('sync_mode')}>
          {t('sync_mode')}: {t(SYNC_MODE_LABEL[r.sync_mode ?? 'auto'])}
        </Text>
        <Show show={!!(r.lfw_version || r.data_infos?.length)}>
          <Text size='ss' style={{ minWidth: 0, overflow: 'hidden', textOverflow: 'ellipsis' }}
            title={data_title}>
            {r.lfw_version}{r.data_infos?.length ? ` · ${t('data_package')}×${r.data_infos.length}` : ''}
          </Text>
        </Show>
      </Flex>
    </Flex>
  )
}

export const RoomsBox = forwardRef<HTMLDivElement, IRoomsBoxProps>(_RoomsBox)

