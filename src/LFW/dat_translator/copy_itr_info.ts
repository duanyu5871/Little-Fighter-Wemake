import type { IItrInfo } from "../defines";
import { Ditto } from "../ditto/Instance";
export function copy_itr_info(
  src: IItrInfo,
  edit: Partial<IItrInfo>,
): IItrInfo {
  return { ...(Ditto.JSON5.parse(Ditto.JSON5.stringify(src)) as any), ...edit };
}
