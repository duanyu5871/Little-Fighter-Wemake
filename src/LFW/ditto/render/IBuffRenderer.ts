export interface IBuffRenderer {
  render(dt: number, df: number): void;
  dispose(): void;
}
