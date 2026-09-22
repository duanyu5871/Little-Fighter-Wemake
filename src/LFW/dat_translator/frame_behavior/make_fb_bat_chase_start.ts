import { OID as OID } from "../../defines/OID";
import type { IFrameInfo } from "../../defines/IFrameInfo";
import { OpointKind } from "../../defines/OpointKind";
import { OpointMultiEnum } from "../../defines/OpointMultiEnum";
import { OpointSpreading } from "../../defines/OpointSpreading";
import { ensure } from "../../utils/container_help/ensure";

export function make_fb_bat_chase_start(frame: IFrameInfo) {
  frame.opoint = ensure(frame.opoint, {
    kind: OpointKind.Normal,
    oid: OID.BatChase,
    x: frame.centerx,
    y: frame.centery,
    action: { id: "0" },
    multi: {
      type: OpointMultiEnum.AccordingEnemies,
      min: 3,
      skip_zero: false,
    },
    spreading: OpointSpreading.Spreading,
    gen_spread_x: "bag(-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6)",
    gen_spread_z: "bag(-2,-1,0,1,2)",
  });
}
