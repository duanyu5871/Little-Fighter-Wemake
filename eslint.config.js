import js from '@eslint/js'
import globals from 'globals'
import reactHooks from 'eslint-plugin-react-hooks'
import reactRefresh from 'eslint-plugin-react-refresh'
import tseslint from 'typescript-eslint'

export default tseslint.config(
  { ignores: ['dist', 'src/LFW/dist'] },
  {
    extends: [js.configs.recommended, ...tseslint.configs.recommended],
    files: ['**/*.{ts,tsx}'],
    languageOptions: {
      ecmaVersion: 2020,
      globals: globals.browser,
    },
    plugins: {
      'react-hooks': reactHooks,
      'react-refresh': reactRefresh,
    },
    rules: {
      ...reactHooks.configs.recommended.rules,
      "@typescript-eslint/no-explicit-any": ["warn"],
      "prefer-const": ["warn"],
      "no-debugger": ["warn"],
      'react-refresh/only-export-components': [
        'warn',
        { allowConstantExport: true },
      ],
    },
  },
  // LFW 核心：只允许标准 ECMAScript，宿主能力一律经 Ditto 注入。
  // 类型层由 src/LFW/tsconfig.json 的 lib/types 守护；这里补它管不到的动态访问。
  {
    files: ['src/LFW/**/*.ts'],
    rules: {
      'no-restricted-globals': [
        'error',
        ...[
          'window', 'document', 'navigator', 'location', 'localStorage',
          'sessionStorage', 'indexedDB',
          'fetch', 'XMLHttpRequest', 'WebSocket', 'EventSource',
          'performance', 'requestAnimationFrame', 'cancelAnimationFrame',
          'setTimeout', 'setInterval', 'clearTimeout', 'clearInterval',
          'queueMicrotask', 'setImmediate', 'structuredClone',
          'atob', 'btoa', 'crypto', 'URL', 'URLSearchParams',
          'Blob', 'File', 'FileReader', 'FormData', 'Image', 'ImageBitmap',
          'createImageBitmap', 'OffscreenCanvas', 'Worker', 'MessageChannel',
          'BroadcastChannel', 'TextEncoder', 'TextDecoder',
          'AbortController', 'AbortSignal',
          'alert', 'confirm', 'prompt', 'console',
          'process', 'Buffer', 'require', 'module', 'exports',
          '__dirname', '__filename', 'global', 'globalThis', 'Deno', 'Bun',
        ].map(name => ({
          name,
          message: `LFW 只允许标准 ECMAScript，禁止使用宿主全局 '${name}'；需要该能力请经 Ditto 注入。`,
        })),
      ],
      'no-restricted-syntax': [
        'error',
        {
          selector: "CallExpression[callee.name='eval']",
          message: 'LFW 禁止 eval。',
        },
        {
          selector: "NewExpression[callee.name='Function']",
          message: 'LFW 禁止 new Function。',
        },
        {
          selector: "CallExpression[callee.name='Function']",
          message: 'LFW 禁止 Function()。',
        },
      ],
    },
  },
)
