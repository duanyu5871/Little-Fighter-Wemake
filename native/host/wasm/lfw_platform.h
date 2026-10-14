/* WASM 宿主的 C ABI（DESIGN「宿主层与 WASM 对接设计」W4 的一张函数指针表 + 导出）。
 *
 * 纯 C 头：emscripten glue（`native/web/host.mjs`）与将来的桌面壳都能吃同一份。
 * 约定（逐条对应 DESIGN W4 / W7）：
 *   * 只用标量、`lfw_char16*` + `len`、或 WASM 线性内存里的 scratch 指针；结构体不按值过界；
 *   * 返回 `int32`：`0` = 成功，负 = 失败（`-1` 未初始化 / `-2` 状态不对 / `-3` 版本不符 …）；
 *   * 「没有句柄」沿用端口的既有约定：`clock_add` / `timeout_add` / `render_add` 回 `0`
 *     （`lfw/base/clock.h:47-55`、`lfw/base/render_scheduler.h:31-38`）⇒ JS 回 0 就等于
 *     headless 的 `ManualClock` / `NoopTimeout`（手动模式，可复现）。
 *
 * 这一版（步行骨架，W13 第 1 步）只过界 **时钟 + 日志 + 帧控制**；渲染 / 音频 / 图片 /
 * 输入按 W13 的顺序留给后面几刀，表里**还没有**那些字段（不是假实现）。
 */
#ifndef LFW_HOST_WASM_PLATFORM_H
#define LFW_HOST_WASM_PLATFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__cplusplus)
typedef char16_t lfw_char16;
#else
/* C 侧没有 `char16_t`；wasm32 小端下与端口全程使用的 `char16_t` 同布局（W7：UTF-16 直传） */
typedef uint16_t lfw_char16;
#endif

/* JS 填完表就调 `lfw_platform_init`；之后表按值拷进 WASM，不再被读。 */
#define LFW_ABI_VERSION 1u

/* 日志级别（`ILfwHost::log/debug_msg/warn/error` 四口，`native/lfw/lfw.h:199-205`）。 */
#define LFW_LOG_LOG 0u
#define LFW_LOG_DEBUG 1u
#define LFW_LOG_WARN 2u
#define LFW_LOG_ERROR 3u

typedef struct LfwPlatform {
  uint32_t abi_version; /* 必须 == LFW_ABI_VERSION */
  uint32_t struct_size; /* 结构体字节数：允许将来在尾部追加字段 */
  void* user;           /* JS 自己的上下文（WASM 不解释，原样回传） */

  /* ---- W4 簇 9：时钟 / 定时 / 渲染调度（反向回调，JS 持句柄、WASM 持 cb_id）---- */
  double (*clock_now)(void* user); /* `ILfwHost::now` / `IClock::now`（`Clock.ts`） */
  int32_t (*clock_add)(void* user,
                       uint32_t cb_id); /* 一次性批量唤醒；回 0 = 没有句柄（手动模式） */
  void (*clock_del)(void* user, int32_t handle);
  int32_t (*clock_hidden)(void* user);                            /* `IClock::hidden` */
  int32_t (*timeout_add)(void* user, uint32_t cb_id, double ms);  /* `setTimeout`；回 0 = 没有句柄 */
  void (*timeout_del)(void* user, int32_t timer_id);
  int32_t (*render_add)(void* user, uint32_t cb_id); /* raf：**重复**回调；回 0 = 没有句柄 */
  void (*render_del)(void* user, int32_t handle);

  /* ---- W4 簇 1：日志（`ILfwHost::warn/error/log/debug_msg`）---- */
  void (*log)(void* user, uint32_t level, const lfw_char16* s, uint32_t len);
} LfwPlatform;

/* ---- 导出（JS -> WASM）---- */

uint32_t lfw_abi_version(void);

/* 出入参共用的 UTF-16 scratch（W7）：JS 把入参写进来、拿指针 + 长度调下面的函数；
 * 出参（digest 十六进制 / 错误消息）也写在这块里，长度由调用方返回。 */
uint32_t lfw_scratch_ptr(void);      /* 返回的裸指针只在「下一次进入 WASM 之前」有效 */
uint32_t lfw_scratch_capacity(void); /* char16 个数（含结尾 NUL 的余量） */

/* 平台表：JS 先 `addFunction` 造好各回调，再把表写进 WASM 堆，最后调这里（0 = 成功）。 */
int32_t lfw_platform_init(const LfwPlatform* table);

/* 把常量字符串（stage id / fighter id / zip 目录）注册进 WASM，拿一个句柄（0 = 失败）。 */
uint32_t lfw_str_put(const lfw_char16* s, uint32_t len);

int32_t lfw_create(uint32_t dev);
int32_t lfw_add_zip(const lfw_char16* memfs_dir, uint32_t len);
int32_t lfw_load(void);
/* 装配一场对局：`host/lfw_run.cpp` 那边的口径（stage/bg + 出场的角色）。 */
int32_t lfw_boot(uint32_t stage_h, uint32_t fighter_a_h, uint32_t fighter_b_h, uint32_t players,
                 uint32_t vs);
/* 推一帧：`World::update_once(dt)`。dt 与 JS 的时钟推进量必须一致（W8）。 */
int32_t lfw_frame(double dt_ms);

/* 观测量（JS 侧要的「真的在跑」的证据） */
double lfw_clock_now(void); /* WASM 侧读到的 JS 时钟（`clock_now` 回调） */
uint32_t lfw_frames(void);
uint32_t lfw_entities(void);
uint32_t lfw_ghosts(void);
double lfw_lifetime(void);
double lfw_total_hp(void);
uint32_t lfw_warn_count(void);
uint32_t lfw_error_count(void);
uint32_t lfw_log_count(void);
/* 各数据表条数：0=fighters 1=weapons 2=balls 3=entities 4=stages 5=bgs 6=bots 7=moves */
uint32_t lfw_data_count(uint32_t which);
/* 诊断：把 `lfw_run` 的 `[state]` 那一行文本写进 scratch（逐实体字段 + bg 运动），
 * 返回 UTF-16 长度。用来跟 headless 的同一行逐字对比，定位分叉点。 */
uint32_t lfw_dump_state(void);
/* 一帧摘要（与 `lfw_run` 的 `frame_digest` 同一份口径，可用来越宿主对拍）：
 * 写进 scratch，返回 UTF-16 长度（0 = 还没建世界）。 */
uint32_t lfw_digest(void);
/* 最近一次失败的消息（scratch + 长度）。 */
uint32_t lfw_last_error_len(void);

/* 反向回调（W4 的 (c)）：JS 触发时钟/定时/raf 回调时调它。 */
void lfw_invoke(uint32_t cb_id);

/* 边界探针：把三个槽各注册一次（走 `lfw::clock_add/timeout_add/render_add`），
 * `lfw_probe_hits(which)` 数命中次数（0=clock 1=timeout 2=render）。 */
int32_t lfw_probe_schedule(double timeout_ms);
uint32_t lfw_probe_hits(uint32_t which);

void lfw_shutdown(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LFW_HOST_WASM_PLATFORM_H */
