import { useCallback, useEffect } from "react";
import type { RefObject } from "react";

export type TShortcut = `ctrl+shift+alt+${string}` | `ctrl+shift+${string}` | `ctrl+alt+${string}` | `ctrl+${string}` | `shift+alt+${string}` | `shift+${string}` | `alt+${string}` | `${string}`;

type TShortcutTarget = Window | Document | Element;

export function useShortcut(shortcut: string | undefined, disabled: unknown, fn?: () => void, target?: TShortcutTarget): void;
export function useShortcut(shortcut: string | undefined, disabled: unknown, ref_btn?: RefObject<HTMLElement | null>, target?: TShortcutTarget): void;
export function useShortcut(shortcut: string | undefined, disabled: unknown, arg?: (() => void) | RefObject<HTMLElement | null>, target: TShortcutTarget = window) {
  const fn = useCallback(() => {
    if (typeof arg === "function") return arg();
    arg?.current?.focus();
    arg?.current?.click();
  }, [arg]);

  const _disabled = !!disabled;

  useEffect(() => {
    if (!shortcut || _disabled) return;
    const keys = shortcut.split("+").filter(v => v);
    if (!keys.length) return;
    const on_keydown = (e: KeyboardEvent) => {
      if (typeof e.key !== "string") return;
      const interrupt = () => {
        e.stopPropagation?.();
        e.preventDefault?.();
        e.stopImmediatePropagation?.();
      };
      if ((keys.indexOf("ctrl") >= 0) === !e.ctrlKey) return;
      if ((keys.indexOf("shift") >= 0) === !e.shiftKey) return;
      if ((keys.indexOf("alt") >= 0) === !e.altKey) return;
      if (e.key.toLowerCase() !== keys[keys.length - 1].toLowerCase()) return;
      fn();
      interrupt();
    };
    target.addEventListener("keydown", on_keydown as EventListener);
    return () => {
      target.removeEventListener("keydown", on_keydown as EventListener);
    };
  }, [shortcut, _disabled, target, fn]);
}
