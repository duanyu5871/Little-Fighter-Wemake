
import type { IStyle } from '../../defines/IStyle';
import { TeamEnum } from '../../defines/TeamEnum';
import { Defines } from '../../defines/defines';
import { TextInfo } from "../../ditto/image/TextInfo";
import { PlayerScore } from "./PlayerScore";
import { UIComponent } from "./UIComponent";

/**
 * Note: 
 *    你的这些组件是怎么想的... 
 *    以后最好避组件间免层层依赖的情况
 *      -Gim
 */
export class PlayerScorePlayerName extends UIComponent {
  static override readonly TAGS: string[] = ["PlayerScorePlayerName"];

  override on_show(): void {
    const fighter = this.node.lookup_component(PlayerScore)?.fighter;
    const text: string = fighter?.name ?? '-';
    const team = fighter?.team;
    const team_info = Defines.TeamInfoMap[team as TeamEnum] ?? Defines.TeamInfoMap[TeamEnum.Independent]
    Object.assign(this.node, {
      outlineColor: team_info.txt_outline_color,
      outlineWidth: 2,
      outlineAlpha: 1,
    })
    const style: IStyle = {
      font: "12px Arial",
      fill_style: team_info.txt_color
    };
    this.node.text = new TextInfo({ text, style });
  }
}
