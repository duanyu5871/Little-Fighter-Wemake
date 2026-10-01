export function is_arr<T = unknown>(v: unknown): v is T[] {
  return Array.isArray(v);
}
