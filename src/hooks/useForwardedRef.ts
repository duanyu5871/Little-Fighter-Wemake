import { useCallback, useRef } from "react";
import type { Ref, RefObject } from "react";

export function useForwardedRef<T>(...refs: Ref<T>[]): [RefObject<T | null>, (v: T | null) => void] {
  const ref = useRef<T | null>(null);
  const on_ref = useCallback((v: T | null) => {
    ref.current = v;
    for (const r of refs) {
      if (typeof r === "function") r(v);
      else if (r) r.current = v;
    }
  }, [refs]);
  return [ref, on_ref];
}
