export const is_bool = (v: unknown): v is boolean => typeof v === "boolean";
export const is_false = (v: unknown): v is false => v === false;
export const is_true = (v: unknown): v is true => v === true;
