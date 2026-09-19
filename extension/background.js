// 点击扩展图标 → 在新标签页打开游戏
chrome.action.onClicked.addListener(() => {
  chrome.tabs.create({ url: 'index.html' });
});

const UPDATE_KEY = 'lfw_update_available';

function is_newer_version(a, b) {
  const x = `${a}`.split('.');
  const y = `${b}`.split('.');
  for (let i = 0; i < Math.max(x.length, y.length); i++) {
    const d = (+x[i] || 0) - (+y[i] || 0);
    if (d) return d > 0;
  }
  return false;
}

async function clear_update_flag() {
  await chrome.storage.local.remove(UPDATE_KEY);
  await chrome.action.setBadgeText({ text: '' });
}

async function sync_update_flag() {
  const data = await chrome.storage.local.get(UPDATE_KEY);
  const version = `${data?.[UPDATE_KEY] ?? ''}`;
  if (version && !is_newer_version(version, chrome.runtime.getManifest().version)) {
    await clear_update_flag();
  }
}

chrome.runtime.onUpdateAvailable.addListener((details) => {
  chrome.storage.local.set({ [UPDATE_KEY]: details?.version ?? '' });
  chrome.action.setBadgeText({ text: '!' });
});

chrome.runtime.onInstalled.addListener(() => {
  sync_update_flag();
});

chrome.runtime.onStartup.addListener(() => {
  sync_update_flag();
});

sync_update_flag();

