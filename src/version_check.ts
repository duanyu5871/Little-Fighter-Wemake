const CHECK_DELAY = 3000;
const RELOAD_KEY = 'lfj_version_reloaded';

function reloaded_stamp(): string {
  try {
    return sessionStorage.getItem(RELOAD_KEY) ?? '';
  } catch {
    return '';
  }
}

function mark_reloaded(stamp: number) {
  try {
    sessionStorage.setItem(RELOAD_KEY, `${stamp}`);
  } catch {
  }
}

function version_url(): string {
  const url = new URL(location.href);
  url.search = '';
  url.hash = '';
  if (!url.pathname.endsWith('/')) url.pathname = url.pathname.replace(/[^/]*$/, '');
  url.pathname += 'version.json';
  url.searchParams.set('time', `${Date.now()}`);
  return url.href;
}

function reload_to_latest() {
  const url = new URL(location.href);
  url.searchParams.set('time', `${Date.now()}`);
  location.replace(url.href);
}

async function fetch_stamp(): Promise<number | undefined> {
  const resp = await fetch(version_url(), { cache: 'no-store' });
  if (!resp.ok) return void 0;
  const data = await resp.json() as { stamp?: unknown };
  return typeof data?.stamp === 'number' ? data.stamp : void 0;
}

async function check_version() {
  try {
    const stamp = await fetch_stamp();
    if (!stamp || stamp === BUILD_STAMP) return;
    if (reloaded_stamp() === `${stamp}`) return;
    mark_reloaded(stamp);
    reload_to_latest();
  } catch {
  }
}

export function start_version_check() {
  if (!VERSION_CHECK) return;
  setTimeout(check_version, CHECK_DELAY);
}
