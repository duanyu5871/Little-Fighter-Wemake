import JSON5 from "json5";
import type { IJSON5 } from "../LFW/ditto/Instance";

export const __JSON5: IJSON5 = {
  parse: (text) => JSON5.parse(text),
  stringify: (value) => JSON5.stringify(value),
};
