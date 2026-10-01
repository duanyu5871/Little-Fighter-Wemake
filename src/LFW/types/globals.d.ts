/**
 * 纯 ES 环境下的 Web API 类型补充声明
 * 不使用 DOM lib，仅声明代码中实际用到的接口
 */

/** 最小化 ImageBitmap 声明 — 仅包含项目实际使用的成员 */
interface ImageBitmap {
  readonly width: number;
  readonly height: number;
  /** 释放 ImageBitmap 持有的资源 */
  close(): void;
}
