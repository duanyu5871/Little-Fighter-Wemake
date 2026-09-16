import type { Dispatch, RefObject, SetStateAction } from "react";
import { useCallback, useRef, useState } from "react";

export function useStateRef<T>(initial_state: T | (() => T)): [T, Dispatch<SetStateAction<T>>, RefObject<T>] {
  const [value, _set_value] = useState(initial_state);
  const ref_value = useRef(value);
  const set_value = useCallback((v: SetStateAction<T>) => {
    if (typeof v === "function") {
      const fn = v as (p: T) => T;
      const new_value = fn(ref_value.current);
      _set_value(ref_value.current = new_value);
    } else {
      _set_value(ref_value.current = v);
    }
  }, []);
  return [value, set_value, ref_value];
}
