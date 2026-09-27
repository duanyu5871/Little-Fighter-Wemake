/** 构建期注入的常量（见 server/rollup.config.mjs） */
declare const __GIT_COMMIT__: string;
declare const __GIT_DIRTY__: boolean;
declare const __BUILD_TIME__: string;

/** 打包时把 html 源文件变成字符串模块（rollup 见 rollup.config.mjs，esbuild 见 scripts/desktop-stage.mjs） */
declare module '*.html' {
  const html: string;
  export default html;
}
