import type { IEntityData } from "../../defines";


export function make_fighter_data_template(data: IEntityData) {
  data.base.bg_face ??= "sprite/MENU_BACK0.png";
  return data;
}
