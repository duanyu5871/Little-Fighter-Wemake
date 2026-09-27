import { Base, type IIconProps } from "./Base";
export function Refresh(props: IIconProps) {
  return (
    <Base {...props}>
      <Base.Path d={o.paths[0]} />
      <Base.Path d={o.paths[1]} />
    </Base>
  );
}
const o = Object.assign(Refresh, Base)
Refresh.paths = [
  'M 8.69 3.31 A 3.8 3.8 0 1 1 4.7 2.43',
  'M 3.25 2.04 L 4.7 2.43 L 3.84 3.66',
]
