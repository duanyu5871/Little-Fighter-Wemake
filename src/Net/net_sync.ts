import type { NetSyncMode } from "./IMsg_RoomStart";

export type RoomSyncMode = NetSyncMode | 'auto';

export const NET_UPS = 60;
export const LOCKSTEP_RTT_RATIO = 1.25;
export const RTT_JITTER_MARGIN = 1.3;
export const MIN_LEAD = 2;
export const MAX_LEAD = 4;
export const UNKNOWN_RTT = 100;

export function recommend_sync(max_rtt: number, ups: number = NET_UPS): { sync_mode: NetSyncMode, input_delay: number } {
  const tick_ms = 1000 / ups;
  const rtt = max_rtt || UNKNOWN_RTT;
  if (rtt <= tick_ms * LOCKSTEP_RTT_RATIO)
    return { sync_mode: 'lockstep', input_delay: 1 };
  const lead = Math.ceil((rtt * RTT_JITTER_MARGIN) / tick_ms);
  return { sync_mode: 'delay', input_delay: Math.min(MAX_LEAD, Math.max(MIN_LEAD, lead)) };
}

export function resolve_sync(choice: RoomSyncMode, max_rtt: number, ups: number = NET_UPS): { sync_mode: NetSyncMode, input_delay: number } {
  const rec = recommend_sync(max_rtt, ups);
  if (choice === 'lockstep') return { sync_mode: 'lockstep', input_delay: 1 };
  if (choice === 'delay') return { sync_mode: 'delay', input_delay: rec.input_delay };
  return rec;
}
