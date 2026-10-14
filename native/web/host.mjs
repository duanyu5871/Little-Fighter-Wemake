// WASM 宿主的 JS 侧（DESIGN W4 / W7 / W8）：填 `LfwPlatform` 函数表、把数据包灌进 MEMFS、
// 然后**逐帧驱动** WASM 里的 LFW。这一刀只有三样东西过界：
//
//   JS -> WASM : `lfw_frame(dt)`（帧控制） + `clock_now()`（时钟）
//   WASM -> JS : `log(level, text)`（日志）
//
// 渲染 / 音频 / 图片 / 输入**没有**实现（W13 的顺序：clock → input → render → audio → images）。
//
// 关键约定（W7，`-sALLOW_MEMORY_GROWTH` 的直接后果）：
//   * 从 WASM 拿到的**裸指针**只在「下一次进入 WASM 之前」有效 ⇒ 用完立刻读；
//   * 不跨调用缓存 `HEAPU8/Uint16Array` 视图，每次都从 `m.*` 重取。

export const ABI_VERSION = 1;

export const LOG_LEVEL = { log: 0, debug: 1, warn: 2, error: 3 };

// `LfwPlatform` 在 wasm32 里的布局：必须与 `host/wasm/lfw_platform.h` 逐字对上。
// 全是 4 字节槽（u32 / void* / 函数指针在 wasm32 里都是 i32）。
const TABLE_SLOTS = [
  "abi_version",
  "struct_size",
  "user",
  "clock_now",
  "clock_add",
  "clock_del",
  "clock_hidden",
  "timeout_add",
  "timeout_del",
  "render_add",
  "render_del",
  "log",
];
const TABLE_BYTES = TABLE_SLOTS.length * 4;

const textDecoder = new TextDecoder("utf-16le");

function readUtf16(m, ptr, len) {
  if (len === 0) return "";
  // `TextDecoder` 吃字节视图：`buffer` 在这里现取（内存增长后旧视图会失效）。
  return textDecoder.decode(new Uint8Array(m.HEAPU8.buffer, ptr, len * 2));
}

function joinDir(path) {
  const i = path.lastIndexOf("/");
  return i <= 0 ? "/" : path.slice(0, i);
}

// 把 `{ path, bytes }` 灌进 emscripten 的 MEMFS（`FsZip` 就按目录树当 zip 读）。
export function installEntries(m, entries) {
  for (const e of entries) {
    m.FS.mkdirTree(joinDir(e.path));
    m.FS.writeFile(e.path, e.bytes);
  }
  return entries.length;
}

// ---- 数据来源（W9：JS 先把条目索引 + 字节备好，解析留在 WASM）----------------------------

// (a) Node：直接读磁盘目录（等价 `lfw_run --zip <dir>`），给 `native/web/smoke.mjs` 用。
export async function nodeDirEntries(srcDir, memRoot) {
  const fs = await import("node:fs");
  const path = await import("node:path");
  const out = [];
  const walk = (dir, rel) => {
    for (const ent of fs.readdirSync(dir, { withFileTypes: true })) {
      const abs = path.join(dir, ent.name);
      const next = rel === "" ? ent.name : `${rel}/${ent.name}`;
      if (ent.isDirectory()) walk(abs, next);
      else if (ent.isFile()) out.push({ path: `${memRoot}/${next}`, bytes: fs.readFileSync(abs) });
    }
  };
  walk(srcDir, "");
  return out;
}

// (b) 浏览器 / 任意宿主：一个 `bundle.json`（索引）+ `bundle.bin`（字节）——见 `make-bundle.mjs`。
// 这条路径与 (a) 灌进 MEMFS 的结果逐字节相同（`smoke.mjs --bundle` 就是它的验收）。
export function bundleEntries(index, binBytes, memRoot = "/") {
  const root = memRoot.endsWith("/") ? memRoot : `${memRoot}/`;
  // Node 的 `Buffer` 是共享池里的视图（`byteOffset != 0`）⇒ 先取它覆盖的那一段，
  // 否则 `offset` 会算错（踩过一次：读出来的是别的文件的开头）。
  const buffer = ArrayBuffer.isView(binBytes)
    ? binBytes.buffer.slice(binBytes.byteOffset, binBytes.byteOffset + binBytes.byteLength)
    : binBytes;
  return index.entries.map((e) => ({
    path: `${root}${e.path}`,
    bytes: new Uint8Array(buffer, e.offset, e.size),
  }));
}

export async function fetchBundle(indexUrl, binUrl, fetchImpl = fetch) {
  const index = await (await fetchImpl(indexUrl)).json();
  const bin = await (await fetchImpl(binUrl)).arrayBuffer();
  return { index, binBytes: bin };
}

// ---------------------------------------------------------------------------------------

/**
 * 造一个 WASM 宿主实例。
 *
 * @param {object}   o
 * @param {Function} o.moduleFactory  `lfw_wasm.mjs` 的默认导出
 * @param {string[]} [o.zips]         MEMFS 里的包目录（按顺序 `lfw_add_zip`）
 * @param {boolean}  [o.dev]
 * @param {boolean}  [o.live]         true = 真的实现 clock/timeout/render 三个槽（JS 定时器）；
 *                                    false = 三个 `add` 回 0（= headless 的 ManualClock，可复现）
 * @param {Function} [o.onLog]        `(level, text) => void`
 */
export async function createLfwHost(o) {
  const {
    moduleFactory,
    moduleOptions = {},
    zips = [],
    dev = false,
    live = false,
    onLog = null,
    clock = null, // 外部时钟（浏览器传 `() => performance.now()`）；不给就用内部的 `timeMs`
  } = o;

  const logs = [];
  const platform = {
    timeMs: 0, // 内部时钟（手动模式用；W4 的 `clock_now` 的默认来源）
    onLog: (level, text) => {
      logs.push({ level, text });
      if (onLog) onLog(level, text);
    },
    entries: [], // JS 灌进 MEMFS 的条目（`{path, bytes}` 的副本，测试用）
  };
  const nowMs = clock ?? (() => platform.timeMs);

  const m = await moduleFactory({
    print: (s) => platform.onLog(LOG_LEVEL.log, `[wasm:stdout] ${s}`),
    printErr: (s) => platform.onLog(LOG_LEVEL.error, `[wasm:stderr] ${s}`),
    ...moduleOptions,
  });

  // ---- 三个槽的 JS 侧实现（live=false 时全部回 0 ⇒ 手动模式）----
  // `clock.add` 是**一次性批量 flush**（TS `Clock.ts` 用 MessageChannel 在下一个宏任务里跑完
  // 整批并清空）；`timeout.add` 是一次性；`render.add` 是**重复** raf。三者语义不同，
  // 不能合并（`lfw/base/render_scheduler.h:7-14` 记着混用出过 UAF）。
  const clockBatch = [];
  const timeouts = [];
  const renders = [];
  let jsHandle = 0;

  const my = {
    clockNow: () => nowMs(),
    clockHidden: () => (typeof document !== "undefined" && document.hidden ? 1 : 0),
    clockAdd: (cbId) => {
      if (!live) return 0;
      const handle = ++jsHandle;
      clockBatch.push({ handle, cbId });
      return handle;
    },
    clockDel: (handle) => {
      const i = clockBatch.findIndex((e) => e.handle === handle);
      if (i >= 0) clockBatch.splice(i, 1);
    },
    timeoutAdd: (cbId, ms) => {
      if (!live) return 0;
      const handle = ++jsHandle;
      // 虚拟时间上的 `setTimeout`：到点由 `pump()` 触发（可复现，不依赖真实墙钟）。
      timeouts.push({ handle, cbId, at: nowMs() + ms });
      return handle;
    },
    timeoutDel: (handle) => {
      const i = timeouts.findIndex((e) => e.handle === handle);
      if (i >= 0) timeouts.splice(i, 1);
    },
    renderAdd: (cbId) => {
      if (!live) return 0;
      const handle = ++jsHandle;
      renders.push({ handle, cbId });
      return handle;
    },
    renderDel: (handle) => {
      const i = renders.findIndex((e) => e.handle === handle);
      if (i >= 0) renders.splice(i, 1);
    },
  };

  // ---- 填表（W4）：`addFunction` 造函数指针，写进 WASM 堆，再 `lfw_platform_init` ----
  // 每个 `sig` 必须与 `lfw_platform.h` 的声明逐字对上（`p`/`i` = i32、`d` = f64；
  // emscripten 把 JS 函数包成 wasm 函数时就是按这个签名建类型）。签名错了 → 调用点
  // `call_indirect` 类型不符，直接 trap。
  const fnPtrs = {
    clock_now: m.addFunction(() => my.clockNow(), "dp"),
    clock_add: m.addFunction((_user, cbId) => my.clockAdd(cbId), "ipi"),
    clock_del: m.addFunction((_user, handle) => my.clockDel(handle), "vpi"),
    clock_hidden: m.addFunction(() => my.clockHidden(), "ip"),
    timeout_add: m.addFunction((_user, cbId, ms) => my.timeoutAdd(cbId, ms), "ipid"),
    timeout_del: m.addFunction((_user, handle) => my.timeoutDel(handle), "vpi"),
    render_add: m.addFunction((_user, cbId) => my.renderAdd(cbId), "ipi"),
    render_del: m.addFunction((_user, handle) => my.renderDel(handle), "vpi"),
    log: m.addFunction(
      (_user, level, ptr, len) => platform.onLog(fromLogLevel(level), readUtf16(m, ptr, len)),
      "vpipi",
    ),
  };

  const table = new Uint8Array(TABLE_BYTES);
  const dv = new DataView(table.buffer);
  dv.setUint32(0, ABI_VERSION, true);
  dv.setUint32(4, TABLE_BYTES, true);
  dv.setUint32(8, 0, true);
  TABLE_SLOTS.slice(3).forEach((name, i) => dv.setUint32(12 + i * 4, fnPtrs[name], true));
  const tablePtr = m._malloc(TABLE_BYTES);
  m.HEAPU8.set(table, tablePtr); // 每次现取 HEAPU8：`_malloc` 可能已经让内存增长
  const initRc = m._lfw_platform_init(tablePtr);
  m._free(tablePtr);
  if (initRc !== 0) throw new Error(`lfw_platform_init failed: ${initRc}`);

  const putStr = (text) => {
    const cap = m._lfw_scratch_capacity();
    if (text.length > cap - 1) throw new Error(`string too long for the scratch: ${text.length}`);
    const ptr = m._lfw_scratch_ptr();
    m.stringToUTF16(text, ptr, text.length * 2 + 2);
    return { ptr, len: text.length };
  };

  const readScratch = (len) => readUtf16(m, m._lfw_scratch_ptr(), len);

  const host = {
    module: m,
    platform,
    logs,
    get clockMs() {
      return platform.timeMs;
    },
    /** 把 `{path, bytes}` 条目灌进 MEMFS（`bundleEntries` / `nodeDirEntries` 的产物）。 */
    install(entries) {
      platform.entries = entries;
      return installEntries(m, entries);
    },
    create(dv0 = dev) {
      const rc = m._lfw_create(dv0 ? 1 : 0);
      if (rc !== 0) throw new Error(`lfw_create failed: ${rc} (${host.lastError()})`);
      return rc;
    },
    /** MEMFS 里的包目录（`/prel` 之类）；按调用顺序就是 W4 的 zip 顺序。 */
    addZip(memDir) {
      const { ptr, len } = putStr(memDir);
      const rc = m._lfw_add_zip(ptr, len);
      if (rc !== 0) throw new Error(`lfw_add_zip(${memDir}) failed: ${rc} (${host.lastError()})`);
      return rc;
    },
    load() {
      const rc = m._lfw_load();
      if (rc !== 0) throw new Error(`lfw_load failed: ${rc} (${host.lastError()})`);
      return rc;
    },
    boot({ stage = "", fighterA = "", fighterB = "", players = 2, vs = false } = {}) {
      const rc = m._lfw_boot(
        host.str(stage),
        host.str(fighterA),
        host.str(fighterB),
        players,
        vs ? 1 : 0,
      );
      if (rc !== 0) throw new Error(`lfw_boot failed: ${rc} (${host.lastError()})`);
      return rc;
    },
    str(text) {
      if (!text) return 0;
      const { ptr, len } = putStr(text);
      const h = m._lfw_str_put(ptr, len);
      if (h === 0) throw new Error(`lfw_str_put failed for ${JSON.stringify(text)}`);
      return h;
    },
    /**
     * 推进一帧。**JS 先把自己的时钟推一格，再让 WASM 走一步** —— 与 `lfw_run` 的
     * `clock.advance(dt); world.update_once(dt);` 同一顺序（W8）。
     *
     * ⚠ 只在**手动模式**（三个槽的 `add` 回 0）用。若 JS 实现了时钟槽，端口自己的
     * `Ticker` 也会推世界（`LFW` ctor 里 `world.start_update()`）⇒ 两边一起推会一帧走两步。
     * 浏览器稳态应该只用 `advance() + pump()` 让 Ticker 推。
     */
    frame(dtMs) {
      platform.timeMs += dtMs;
      const rc = m._lfw_frame(dtMs);
      if (rc !== 0) throw new Error(`lfw_frame failed: ${rc}`);
      return rc;
    },
    /** 只推 JS 时钟、不碰世界（配合 `pump()` 让端口的 Ticker 自己推）。 */
    advance(dtMs) {
      platform.timeMs += dtMs;
    },
    /** 反向回调的汇点（W4 的 (c)）：跑掉积压的 clock 批 + 到点的 timeout + 每帧的 raf。 */
    pump({ renders: doRenders = true } = {}) {
      const batch = clockBatch.splice(0, clockBatch.length);
      for (const e of batch) m._lfw_invoke(e.cbId);
      for (let i = timeouts.length - 1; i >= 0; i--) {
        if (timeouts[i].at > nowMs()) continue;
        const [e] = timeouts.splice(i, 1);
        m._lfw_invoke(e.cbId);
      }
      if (doRenders) for (const e of renders.slice()) m._lfw_invoke(e.cbId);
      return batch.length;
    },
    probeSchedule(timeoutMs = 1e8) {
      return m._lfw_probe_schedule(timeoutMs); // 1e8ms：手动模式下永远到不了点
    },
    probeHits(which = 0) {
      return m._lfw_probe_hits(which);
    },
    clockNowSeenByWasm() {
      return m._lfw_clock_now();
    },
    stats() {
      return {
        frames: m._lfw_frames(),
        entities: m._lfw_entities(),
        ghosts: m._lfw_ghosts(),
        lifetime: m._lfw_lifetime(),
        totalHp: m._lfw_total_hp(),
        warns: m._lfw_warn_count(),
        errors: m._lfw_error_count(),
        logs: m._lfw_log_count(),
      };
    },
    dataCounts() {
      const names = ["fighters", "weapons", "balls", "entities", "stages", "bgs", "bots", "moves"];
      const out = {};
      names.forEach((n, i) => {
        out[n] = m._lfw_data_count(i);
      });
      return out;
    },
    digest() {
      const len = m._lfw_digest();
      return len === 0 ? "" : readScratch(len);
    },
    /** `lfw_run` 的 `[state]` 那一行（诊断用；跟 headless 逐字对比定位分叉点）。 */
    dumpState() {
      const len = m._lfw_dump_state();
      return len === 0 ? "" : readScratch(len);
    },
    lastError() {
      const len = m._lfw_last_error_len();
      return len === 0 ? "" : readScratch(len);
    },
    dispose() {
      m._lfw_shutdown();
      if (typeof m.removeFunction === "function") {
        for (const p of Object.values(fnPtrs)) {
          if (p) m.removeFunction(p);
        }
      }
    },
  };

  // zip 列表给了就顺手挂上（`lfw_create` 之后才允许 `lfw_add_zip`）。
  if (zips.length > 0) {
    host.create();
    for (const z of zips) host.addZip(z);
  }
  return host;
}

function fromLogLevel(level) {
  return { 0: LOG_LEVEL.log, 1: LOG_LEVEL.debug, 2: LOG_LEVEL.warn, 3: LOG_LEVEL.error }[level] ??
    LOG_LEVEL.log;
}
