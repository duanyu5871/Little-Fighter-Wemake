# LFW 弹幕桥（B站直播 -> 弹幕互动游戏）

独立的 Node 服务：连接 B站直播弹幕流，把观众互动转成游戏指令发给弹幕互动游戏页面。
不依赖、也不修改仓库里的 `server/`，可单独部署（和 OBS/浏览器同机运行即可）。

## 安装与运行

```powershell
cd danmu-bridge
npm install
node index.mjs --room 12345
```

参数（也可用对应环境变量）：

| 参数 | 环境变量 | 说明 |
| --- | --- | --- |
| `--room <id>` | `BILI_ROOM_ID` | 直播间号，短号/真实房间号都行（必填） |
| `--sessdata <v>` | `BILI_SESSDATA` | 登录 cookie 的 SESSDATA，可选；游客也能收弹幕，登录态更稳 |
| `--uid <id>` | `BILI_UID` | 配 SESSDATA 用的 DedeUserID，可选 |
| `--port <port>` | `DANMU_BRIDGE_PORT` | 游戏页面连接端口，默认 8066 |
| `--host <host>` | `DANMU_BRIDGE_HOST` | 监听地址，默认 127.0.0.1（页面在别的设备上打开时才需要 `0.0.0.0`） |
| `--join <k1,k2>` | `DANMU_JOIN_KEYWORDS` | 触发入队的关键词；不填 = 任意弹幕都入队 |
| `--cheer <k1,k2>` | `DANMU_CHEER_KEYWORDS` | 触发应援的关键词，默认 `加油,666,应援` |
| `--join-cooldown <ms>` | `DANMU_JOIN_COOLDOWN` | 同一观众两次入队尝试的最小间隔，默认 5000 |
| `--debug` | - | 打印所有弹幕事件，排障用 |

## 官方开放平台模式（推荐，有应用时）

```powershell
node index.mjs --mode open --app-id 12345 --access-key <xxx> --access-key-secret <yyy> --code <主播身份码>
```

流程：`POST /v2/app/start`（HMAC-SHA256 签名）换 `game_id` + 长连地址 + 鉴权体 → wss `op7` 鉴权 → 20 秒双心跳（wss `op2` + `POST /v2/app/heartbeat`）→ 断线自动重连（鉴权失败会重新 start）；Ctrl+C 退出时自动 `POST /v2/app/end`。

前置条件：

- 完成直播开放平台入驻，拿到 `access_key` / `access_key_secret`（审核通过后邮件发放）
- 应用为互动玩法类型，并**向 B站运营申请开通长连消息类型**（弹幕/礼物/点赞等，未申请收不到消息）
- 主播身份码 `code`：主播在直播姬启动你开发中的玩法后产生（联调阶段用官方测试入口拿到的 code）

消息映射：`LIVE_OPEN_PLATFORM_DM` → 入队/应援；`LIVE_OPEN_PLATFORM_SEND_GIFT` / `_GUARD` / `_SUPER_CHAT` → 应援；`LIVE_OPEN_PLATFORM_LIKE` → 刷新活跃。

## 游戏侧对接

游戏页面加 URL 参数即可连接本服务：

- 同机默认地址：`#/?DANMU_WS=1`
- 自定义地址：`#/?DANMU_WS=ws://192.168.1.5:8066`（值里若含 `&` 需 URL 编码）

页面会周期上报状态，桥的控制台会打印：`[游戏] teams8 排队 12 / 场上 30`。

## 事件映射

| B站事件 | 游戏指令 |
| --- | --- |
| 弹幕（命中入队规则） | `join` 入队（uid 去重；同一人已在场上时不会重复入队） |
| 弹幕（命中应援关键词） | `cheer` 应援（引擎内 3 秒 CD） |
| 进入直播间 / 关注 / 分享 | `touch` 刷新活跃时间 |
| 点赞 | `touch` |
| 礼物 / 上舰 / 醒目留言 | `touch` + `cheer` |
| 开播 / 未开播 | 日志提示 |

## 关于"离开直播间"

B站常规弹幕流没有离场事件（只有进入/互动），所以采用活跃超时兜底：

- 桥侧：收到该用户任何互动就 `touch(uid)`（最多 30 秒发一次，避免刷屏）
- 引擎侧：排队中超过 5 分钟没有任何 `touch` 会自动移出队列
  （`src/LFW/ui/component/DanmuGameLogic.ts` 里的 `QUEUE_IDLE_TIMEOUT`，可按直播节奏调整）
- 已经在场上打的人不受超时影响，只在战死或换关时出局
- 若将来数据源能提供离场事件，收到后发 `{ type: "leave", uid }` 即可立即出队（引擎已支持）

## 常见问题

- **认证失败（code 非 0）**：通常是 token 过期或风控，服务会自动重连并重新取 token；频繁失败建议加 `--sessdata`。
- **官方模式返回 `4001 应用无效`**：检查 `--app-id` / `--access-key` / `--access-key-secret` 是否配对（用假密钥探测也会得到这个返回，说明网络与签名没问题）。
- **官方模式收不到弹幕**：确认已向 B站运营申请开通消息类型，以及 `--code` 是当前主播本次启动产生的、未过期。
- **断流**：服务内置心跳（30 秒）与 90 秒无消息看门狗，断开后按 3s → 30s 退避重连。
- **收不到弹幕但要先确认房间**：先看控制台打印的 `房间 xxx -> yyy` 是否正确；用 `--debug` 看原始事件。
- **页面没反应**：确认游戏页 URL 带了 `DANMU_WS` 参数；桥和页面必须网络互通（同机就查防火墙/端口）。
