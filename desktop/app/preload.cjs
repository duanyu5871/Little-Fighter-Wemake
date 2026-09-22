const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("runtime", {
  WindowMinimise: () => ipcRenderer.send("lfj:minimize"),
  WindowToggleMaximise: () => ipcRenderer.send("lfj:toggle-maximize"),
  WindowIsMaximised: () => ipcRenderer.invoke("lfj:is-maximized"),
  WindowIsFullscreen: () => ipcRenderer.invoke("lfj:is-fullscreen"),
  WindowFullscreen: () => ipcRenderer.send("lfj:fullscreen", true),
  WindowUnfullscreen: () => ipcRenderer.send("lfj:fullscreen", false),
  Quit: () => ipcRenderer.send("lfj:quit"),
});
