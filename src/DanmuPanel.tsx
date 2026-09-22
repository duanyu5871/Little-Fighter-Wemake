import { useEffect, useState } from "react";
import type { LFW } from "./LFW";
import type { ILFWCallback } from "./LFW/ILFWCallback";
import { DanmuGameLogic, type DanmuGameMode, type IDanmuViewerStat } from "./LFW/ui/component/DanmuGameLogic";
import { danmu_hints } from "./danmu_bridge";
import type { UIComponent } from "./LFW/ui/component/UIComponent";
import csses from "./DanmuPanel.module.scss";

interface IDanmuPanelVM {
  mode: DanmuGameMode;
  mode_label: string;
  stage: string;
  waiting_next: boolean;
  on_stage: number;
  queue_size: number;
  queue_names: string[];
  cheers: string[];
  stats: IDanmuViewerStat[];
  hint: string;
}

const MODE_TEXT: Record<DanmuGameMode, string> = {
  ffa: "各自为战",
  teams8: "八队混战",
  coop: "合作闯关",
};

const CHEER_VISIBLE_FRAMES = 60 * 8;

function make_vm(logic: DanmuGameLogic): IDanmuPanelVM {
  const stats = logic.viewer_stats();
  const now = logic.time;
  const hint_tick = Math.floor(now / (60 * 6));
  return {
    mode: logic.mode,
    mode_label: MODE_TEXT[logic.mode] ?? logic.mode,
    stage: logic.stage_name,
    waiting_next: logic.waiting_next_stage,
    on_stage: stats.filter((v) => v.alive).length,
    queue_size: logic.join_queue.size,
    queue_names: logic.join_queue.all.slice(0, 6).map((v) => v.name),
    cheers: logic.cheer_feed
      .filter((v) => now - v.time <= CHEER_VISIBLE_FRAMES)
      .slice(-4)
      .map((v) => v.name),
    stats: stats.slice(0, 10),
    hint: danmu_hints.texts.length ? danmu_hints.at(hint_tick) : "",
  };
}

export function DanmuPanel(props: { lfw: LFW | undefined }) {
  const { lfw } = props;
  const [vm, set_vm] = useState<IDanmuPanelVM | null>(null);
  useEffect(() => {
    if (!lfw) return;
    let logic: DanmuGameLogic | null = null;
    let timer: ReturnType<typeof setInterval> | null = null;
    const close = () => {
      if (timer !== null) clearInterval(timer);
      timer = null;
      logic = null;
      set_vm(null);
    };
    const callback: ILFWCallback = {
      on_component_broadcast(component: UIComponent, msg: string) {
        if (msg === DanmuGameLogic.BROADCAST_ON_START) {
          logic = component as DanmuGameLogic;
          if (timer === null) timer = setInterval(() => { if (logic) set_vm(make_vm(logic)); }, 1000);
          set_vm(make_vm(logic));
        } else if (msg === DanmuGameLogic.BROADCAST_ON_STOP) {
          close();
        }
      },
    };
    lfw.callbacks.add(callback);
    return () => {
      lfw.callbacks.del(callback);
      close();
    };
  }, [lfw]);
  if (!vm) return null;
  return (
    <div className={csses.panel}>
      <div className={csses.header}>
        <span>{vm.mode_label}</span>
        <span>{vm.mode === "coop" ? "观众" : "场上"} {vm.on_stage}/{DanmuGameLogic.MAX_FIGHTERS}</span>
      </div>
      {vm.mode === "coop" && (
        <div className={csses.stage}>
          {vm.stage ? `第 ${vm.stage} 关` : ""}{vm.waiting_next ? " · 即将进入下一关" : ""}
        </div>
      )}
      {vm.queue_size > 0 && (
        <div className={csses.section}>
          <div className={csses.title}>排队入场 {vm.queue_size}</div>
          <div className={csses.names}>
            {vm.queue_names.join("、")}{vm.queue_size > vm.queue_names.length ? "…" : ""}
          </div>
        </div>
      )}
      {vm.cheers.length > 0 && (
        <div className={csses.section}>
          <div className={csses.title}>应援</div>
          {vm.cheers.map((name, i) => (
            <div key={`${name}_${i}`} className={csses.cheer}>
              {name} 应援 · 回血 {DanmuGameLogic.CHEER_HP_RATIO * 100}%
            </div>
          ))}
        </div>
      )}
      {vm.stats.length > 0 && (
        <div className={csses.section}>
          <div className={csses.title}>战绩</div>
          {vm.stats.map((s, i) => (
            <div key={s.uid} className={csses.row}>
              <span className={csses.rank}>{i + 1}</span>
              <span className={s.alive ? csses.name_alive : csses.name}>{s.name}</span>
              <span className={csses.nums}>击败 {s.kills} · 阵亡 {s.deads} · 出场 {s.spawns}</span>
            </div>
          ))}
        </div>
      )}
      {vm.hint ? <div className={csses.hint}>{vm.hint}</div> : null}
    </div>
  );
}
