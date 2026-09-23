Little Fighter Wemake - 界面文案（托盘菜单语言文件）

用法
- 本目录放置 <语言码>.json5（或 .json），启动时自动加载；文件名就是语言码
- 语言码：zh-hans / zh-hant / en（en 即英文），或任意语言码（如 ja、de，配合启动参数 --lang ja 使用）
- 文件内容是一个对象：键 = 文案项，值 = 文案（字符串，可用 %1/%2 等占位符）
- 只写想改的项即可：其余项、以及没有对应文件的语言，都用内置文案
  （内置自带 en / zh-hans / zh-hant 三套，本目录留空也能正常显示）
- 语言选择：启动参数 --lang <语言码>；不指定时跟随游戏内语言（游戏为英文时托盘也是英文）

示例（ja.json5）

{
  server_on: "サーバー：%1",
  quit: "終了",
}

可用键

server_on        服务器开启状态的文案，%1 = 服务器地址
server_off       服务器未开启时的文案
allow_lan        允许局域网连接
copy_server_addr 复制联机地址
allow_game_lan   允许局域网访问游戏页面
copy_game_addr   复制游戏页面地址
open_tool        打开数据工具（命令行）
copy_tool_cmd    复制数据工具命令
open_data_dir    打开数据目录
show_window      显示游戏窗口
quit             退出
