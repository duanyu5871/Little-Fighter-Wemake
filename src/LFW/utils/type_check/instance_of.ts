type Cls<T> = new (...args: unknown[]) => T
export function instance_of<T>(value: unknown, type: Cls<T>): value is T {
  return typeof type.prototype === "function" && value instanceof type;
}
