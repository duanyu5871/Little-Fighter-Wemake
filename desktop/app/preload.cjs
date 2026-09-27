const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("runtime", {
  WindowMinimise: () => ipcRenderer.send("lfj:minimize"),
  WindowToggleMaximise: () => ipcRenderer.send("lfj:toggle-maximize"),
  WindowIsMaximised: () => ipcRenderer.invoke("lfj:is-maximized"),
  WindowIsFullscreen: () => ipcRenderer.invoke("lfj:is-fullscreen"),
  WindowFullscreen: () => ipcRenderer.send("lfj:fullscreen", true),
  WindowUnfullscreen: () => ipcRenderer.send("lfj:fullscreen", false),
  Quit: () => ipcRenderer.send("lfj:quit"),
  SetLang: (lang) => ipcRenderer.send("lfj:lang", String(lang ?? "")),
});

// 模组目录：游戏页面启动时把 mods/ 里的 zip 自动读进来（语义见 src/desktop_mods.ts）
contextBridge.exposeInMainWorld("lfwm_mods", {
  list: () => ipcRenderer.invoke("lfwm:mods"),
  read: (id) => ipcRenderer.invoke("lfwm:mod", String(id ?? "")),
});

// 更新状态：主进程广播给游戏页面，由页面画成 DOM 角标（语义见 src/desktop_update.ts）
contextBridge.exposeInMainWorld("lfwm_update", {
  state: () => ipcRenderer.invoke("lfwm:update-state"),
  check: () => ipcRenderer.send("lfwm:update-check"),
  install: () => ipcRenderer.send("lfwm:update-install"),
  on_state: (cb) => {
    if (typeof cb !== "function") return;
    ipcRenderer.on("lfwm:update", (_event, state) => cb(state));
  },
});
