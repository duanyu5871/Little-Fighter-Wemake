import { is_num } from "../type_check";

export function to_num(v: unknown, or: number): number;
export function to_num(v: unknown, or?: number): number | undefined;
export function to_num(v: unknown, or?: number): number | undefined {
  const n = Number(v);
  return is_num(n) ? n : or;
}
