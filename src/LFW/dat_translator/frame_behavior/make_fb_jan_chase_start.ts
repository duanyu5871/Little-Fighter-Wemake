import { OID as OID } from "../../defines/OID";
import type { IFrameInfo } from "../../defines/IFrameInfo";
import { OpointKind } from "../../defines/OpointKind";
import { OpointMultiEnum } from "../../defines/OpointMultiEnum";
import { OpointSpreading } from "../../defines/OpointSpreading";
import { ensure } from "../../utils/container_help/ensure";

export function make_fb_jan_chase_start(frame: IFrameInfo, x: number = frame.centerx, y: number = frame.centery) {
  frame.opoint = ensure(frame.opoint, {
    kind: OpointKind.Normal,
    oid: OID.JanChase,
    x, y, dvy: 6,
    action: { id: "0" },
    multi: {
      type: OpointMultiEnum.AccordingEnemies,
      min: 1,
      skip_zero: true,
    },
    spreading: OpointSpreading.Spreading,
    gen_spread_x: "bag(-5,-4,-3,-2,-1,0,1,2,3,4,5)",
    gen_spread_y: "bag(2,2.5,3,3.5,4,4.5,5,5.5,6,6.5,7,7.5,8)",
  });
}
