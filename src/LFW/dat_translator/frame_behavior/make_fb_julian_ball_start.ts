import { OID as OID } from "../../defines/OID";
import type { IFrameInfo } from "../../defines/IFrameInfo";
import { OpointKind } from "../../defines/OpointKind";
import { OpointSpreading } from "../../defines/OpointSpreading";
import { ensure } from "../../utils/container_help/ensure";

export function make_fb_julian_ball_start(frame: IFrameInfo) {
  frame.opoint = ensure(frame.opoint, {
    kind: OpointKind.Normal,
    oid: OID.JulianBall,
    x: frame.centerx,
    y: frame.centery,
    dvx: 8,
    action: { id: "50" },
    spreading: OpointSpreading.FloatRange,
    gen_spread_z: "rand(-15, 15) / 10",
    gen_spread_y: "rand(-5, 5) / 10",
  });
}
