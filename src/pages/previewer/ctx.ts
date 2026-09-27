import type { LFW } from "@/LFW";
import { createContext, useContext } from "react";

/** 画布显示模式：直接对应 css 的 `object-fit` */
export type TCanvasFit = "contain" | "cover" | "fill" | "none";

export interface IPreviewerContext {
  lfw?: LFW;
  ready: boolean;
  /** 画布显示模式（背景 / 物体两个 tab 共用） */
  fit: TCanvasFit;
  set_fit(v: TCanvasFit): void;
}

export const PreviewerContext = createContext<IPreviewerContext>({ ready: false, fit: "contain", set_fit: () => void 0 });

export function usePreviewer(): IPreviewerContext {
  return useContext(PreviewerContext);
}
