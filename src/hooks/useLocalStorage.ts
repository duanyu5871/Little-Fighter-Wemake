import { useEffect, useState } from "react";
import type { Dispatch, SetStateAction } from "react";

type Ret<S> = readonly [S, Dispatch<SetStateAction<S>>];
type Init<S> = S | (() => S);

function take_initial_state<T>(initial_state: Init<T> | undefined, type: string): T | undefined {
  if (typeof initial_state === type) return initial_state as T;
  if (typeof initial_state === "function") return (initial_state as () => T)();
  return void 0;
}

function useSaving(type: string, name: string, val: unknown) {
  useEffect(() => {
    if (typeof val === type) localStorage.setItem(name, "" + val);
    else localStorage.removeItem(name);
  }, [name, val, type]);
}

export function useLocalString<S extends string = string>(name: string): Ret<S | undefined>;
export function useLocalString<S extends string = string>(name: string, initial_state: Init<S>): Ret<S>;
export function useLocalString<S extends string = string>(name: string, initial_state?: Init<S>) {
  const [val, set_val] = useState<S | undefined>(() => {
    const v = localStorage.getItem(name);
    if (typeof v === "string") return v as S;
    const ret = take_initial_state(initial_state, "string");
    if (typeof ret === "string") localStorage.setItem(name, ret);
    return ret;
  });
  useSaving("string", name, val);
  return [val, set_val] as Ret<S | undefined>;
}

export function useLocalNumber<S extends number = number>(name: string): Ret<S | undefined>;
export function useLocalNumber<S extends number = number>(name: string, initial_state: Init<S>): Ret<S>;
export function useLocalNumber<S extends number = number>(name: string, initial_state?: Init<S>) {
  const [val, set_val] = useState<S | undefined>(() => {
    const v = localStorage.getItem(name);
    if (typeof v === "string" && v) {
      const n = Number(v);
      if (!Number.isNaN(n)) return n as S;
    }
    const ret = take_initial_state(initial_state, "number");
    if (typeof ret === "number") localStorage.setItem(name, "" + ret);
    return ret;
  });
  useSaving("number", name, val);
  return [val, set_val] as Ret<S | undefined>;
}

export function useLocalBoolean<S extends boolean = boolean>(name: string): Ret<S | undefined>;
export function useLocalBoolean<S extends boolean = boolean>(name: string, initial_state: Init<S>): Ret<S>;
export function useLocalBoolean<S extends boolean = boolean>(name: string, initial_state?: Init<S>) {
  const [val, set_val] = useState<S | undefined>(() => {
    const v = localStorage.getItem(name);
    if (typeof v === "string") return (v === "true") as S;
    const ret = take_initial_state(initial_state, "boolean");
    if (typeof ret === "boolean") localStorage.setItem(name, "" + ret);
    return ret;
  });
  useSaving("boolean", name, val);
  return [val, set_val] as Ret<S | undefined>;
}
