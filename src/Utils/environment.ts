interface IChromeApi {
  runtime?: { id?: string };
  storage?: { local?: { get?: (keys: string) => Promise<Record<string, unknown>> } };
}

export const chrome_api = (globalThis as { chrome?: IChromeApi }).chrome;

export function is_extension_page(): boolean {
  return !!chrome_api?.runtime?.id;
}
