import { Button } from "@/Component/Buttons/Button";
import { Flex } from "@/Component/Flex";
import Frame from "@/Component/Frame";
import { Text } from "@/Component/Text";
import styles from "./styles.module.scss";

export interface IMatchPauseNoticeAction {
  text: string;
  onClick(): void;
}

export interface IMatchPauseNoticeProps {
  title: string;
  lines: string[];
  actions?: IMatchPauseNoticeAction[];
}

export function MatchPauseNotice(props: IMatchPauseNoticeProps) {
  const { title, lines, actions } = props;
  return (
    <Frame className={styles.match_notice} hoverable={false}>
      <Flex direction="column" align="center" gap={10}>
        <Text style={{ fontWeight: "bold" }}>{title}</Text>
        <Text size="ss" style={{ whiteSpace: "pre-line", textAlign: "center", opacity: 0.8 }}>
          {lines.join("\n")}
        </Text>
        {actions?.length ? (
          <Flex direction="row" align="center" justify="center" gap={8}>
            {actions.map((a) => (
              <Button key={a.text} onClick={() => a.onClick()}>{a.text}</Button>
            ))}
          </Flex>
        ) : null}
      </Flex>
    </Frame>
  );
}
