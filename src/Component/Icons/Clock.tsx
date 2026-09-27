import { Base, type IIconProps } from "./Base";
export function Clock(props: IIconProps) {
  return (
    <Base {...props}>
      <Base.Path d={o.paths[0]} />
      <Base.Path d={o.paths[1]} />
    </Base>
  );
}
const o = Object.assign(Clock, Base)
Clock.paths = [
  'M 2 6 A 4 4 0 1 1 10 6 A 4 4 0 1 1 2 6',
  'M 6 3.4 L 6 6 L 7.9 7.1',
]
