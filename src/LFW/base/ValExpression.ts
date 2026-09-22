import type { Entity } from "../entity/Entity";
import { round } from "../utils";

export type IValGetterFn = (e: Entity) => number;

export interface IValExpressionOptions {
  tag?: string;
  vars?: Record<string, IValGetterFn>;
}

const DEFAULT_VARS: Record<string, IValGetterFn> = {
  w: (e) => e.frame.width,
  h: (e) => e.frame.height,
  cx: (e) => e.frame.centerx,
  cy: (e) => e.frame.centery,
};

const is_digit = (c: string | undefined): boolean => !!c && c >= "0" && c <= "9";

const FLIP_VALUES: number[] = [-1, 1];

const is_ident_char = (c: string | undefined): boolean =>
  !!c && (
    (c >= "a" && c <= "z") ||
    (c >= "A" && c <= "Z") ||
    (c >= "0" && c <= "9") ||
    c === "_"
  );

class ValExpressionError extends Error {
  constructor(message: string, readonly index: number) {
    super(message);
  }
}

class ValExpressionParser {
  private index = 0;
  private readonly vars: Record<string, IValGetterFn>;
  constructor(
    private readonly text: string,
    private readonly tag: string,
    vars?: Record<string, IValGetterFn>,
  ) {
    this.vars = vars ? { ...DEFAULT_VARS, ...vars } : DEFAULT_VARS;
  }
  private fail(message: string): never {
    throw new ValExpressionError(message, this.index);
  }
  parse(): IValGetterFn {
    if (!this.text) this.fail("empty expression");
    const v = this.parse_expr();
    if (this.index < this.text.length)
      this.fail(`unexpected '${this.text.slice(this.index)}'`);
    return v;
  }
  private parse_expr(): IValGetterFn {
    let ret = this.parse_term();
    for (; ;) {
      const op = this.text[this.index];
      if (op !== "+" && op !== "-") return ret;
      ++this.index;
      const right = this.parse_term();
      const l = ret, r = right;
      ret = op === "+" ? (e) => l(e) + r(e) : (e) => l(e) - r(e);
    }
  }
  private parse_term(): IValGetterFn {
    let ret = this.parse_unary();
    for (; ;) {
      const op = this.text[this.index];
      if (op !== "*" && op !== "/") return ret;
      ++this.index;
      const right = this.parse_unary();
      const l = ret, r = right;
      ret = op === "*" ? (e) => l(e) * r(e) : (e) => l(e) / r(e);
    }
  }
  private parse_unary(): IValGetterFn {
    if (this.text[this.index] !== "-") return this.parse_primary();
    ++this.index;
    const v = this.parse_unary();
    return (e) => -v(e);
  }
  private parse_primary(): IValGetterFn {
    const c = this.text[this.index];
    if (c === void 0) this.fail("unexpected end of expression");
    if (c === "(") {
      ++this.index;
      const v = this.parse_expr();
      if (this.text[this.index] !== ")") this.fail("missing ')'");
      ++this.index;
      return v;
    }
    if (is_digit(c) || c === ".") return this.parse_number();
    if (is_ident_char(c)) return this.parse_ident();
    this.fail(`unexpected character '${c}'`);
  }
  private parse_number(): IValGetterFn {
    const start = this.index;
    while (is_digit(this.text[this.index])) ++this.index;
    if (this.text[this.index] === ".") {
      ++this.index;
      while (is_digit(this.text[this.index])) ++this.index;
    }
    const raw = this.text.slice(start, this.index);
    const value = Number(raw);
    if (!Number.isFinite(value)) this.fail(`bad number '${raw}'`);
    return () => value;
  }
  private parse_ident(): IValGetterFn {
    const start = this.index;
    while (is_ident_char(this.text[this.index])) ++this.index;
    const name = this.text.slice(start, this.index);
    if (this.text[this.index] !== "(") {
      const v = this.vars[name];
      if (!v) this.fail(`unknown identifier '${name}'`);
      return v;
    }
    ++this.index;
    const args: IValGetterFn[] = [];
    if (this.text[this.index] !== ")") {
      args.push(this.parse_expr());
      while (this.text[this.index] === ",") {
        ++this.index;
        args.push(this.parse_expr());
      }
    }
    if (this.text[this.index] !== ")") this.fail(`missing ')' for '${name}(...'`);
    ++this.index;
    return this.parse_call(name, args);
  }
  private parse_call(name: string, args: IValGetterFn[]): IValGetterFn {
    const tag = this.tag;
    switch (name) {
      case "rand": {
        if (args.length !== 2) this.fail(`'rand' expects 2 arguments, got ${args.length}`);
        const a = args[0]!, b = args[1]!;
        return (e) => {
          e.lfw.mt.mark = tag;
          return e.lfw.mt.range(a(e), b(e));
        };
      }
      case "pick": {
        if (!args.length) this.fail(`'pick' expects at least 1 argument`);
        const scratch: number[] = new Array(args.length);
        return (e) => {
          for (let i = 0; i < args.length; ++i) scratch[i] = args[i]!(e);
          e.lfw.mt.mark = tag;
          return e.lfw.mt.pick(scratch)!;
        };
      }
      case "flip": {
        if (args.length) this.fail(`'flip' expects no arguments`);
        return (e) => {
          e.lfw.mt.mark = tag;
          return e.lfw.mt.pick(FLIP_VALUES)!;
        };
      }
      case "round": {
        if (args.length !== 1) this.fail(`'round' expects 1 argument, got ${args.length}`);
        const a = args[0]!;
        return (e) => round(a(e));
      }
    }
    return this.fail(`unknown function '${name}'`);
  }
}

export class ValExpression {
  readonly text: string;
  readonly tag: string;
  readonly err?: string;
  private readonly _get: IValGetterFn;
  constructor(source: string, options?: IValExpressionOptions) {
    this.tag = options?.tag ?? "val_expr";
    const text = this.text = (source ?? "").replace(/\s/g, "");
    let get: IValGetterFn = () => 0;
    try {
      get = new ValExpressionParser(text, this.tag, options?.vars).parse();
    } catch (e) {
      const err = e as ValExpressionError;
      this.err = `[ValExpression] ${this.tag}: ${err.message} @${err.index} in "${text}"`;
    }
    this._get = get;
  }
  get(e: Entity): number {
    return this._get(e);
  }
}
