import type { Connection } from "./Connection";
import type { LFWNetworkDriver } from "./LFWNetworkDriver";

export const current_connection: {
  conn: Connection | null;
  driver: LFWNetworkDriver | null;
} = { conn: null, driver: null };
