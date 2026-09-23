import classNames from "classnames";
import React, { useCallback, useRef, useState } from "react";
import styles from "./VerticalSlider.module.scss";

export interface IVerticalSliderProps {
  value: number;
  className?: string;
  onChange?(v: number): void;
}

const clamp_unit = (v: number) => (v < 0 ? 0 : v > 1 ? 1 : v);

export const VerticalSlider: React.FC<IVerticalSliderProps> = (props) => {
  const { value, className, onChange } = props;
  const ref_track = useRef<HTMLDivElement>(null);
  const [dragging, set_dragging] = useState(false);

  const value_from_y = useCallback((client_y: number) => {
    const track = ref_track.current;
    if (!track) return value;
    const rect = track.getBoundingClientRect();
    if (rect.height <= 0) return value;
    return clamp_unit(1 - (client_y - rect.top) / rect.height);
  }, [value]);

  const on_pointer_down: React.PointerEventHandler<HTMLDivElement> = (e) => {
    if (e.pointerType === "mouse" && e.button !== 0) return;
    e.preventDefault();
    e.stopPropagation();
    e.currentTarget.setPointerCapture(e.pointerId);
    set_dragging(true);
    onChange?.(value_from_y(e.clientY));
  };

  const on_pointer_move: React.PointerEventHandler<HTMLDivElement> = (e) => {
    if (!dragging) return;
    e.preventDefault();
    e.stopPropagation();
    onChange?.(value_from_y(e.clientY));
  };

  const on_pointer_end: React.PointerEventHandler<HTMLDivElement> = () => set_dragging(false);

  const v = clamp_unit(value);

  return (
    <div
      ref={ref_track}
      className={classNames(styles.vertical_slider, { [styles.dragging]: dragging }, className)}
      onPointerDown={on_pointer_down}
      onPointerMove={on_pointer_move}
      onPointerUp={on_pointer_end}
      onPointerCancel={on_pointer_end}
      onLostPointerCapture={() => set_dragging(false)}
      onContextMenu={(e) => { e.preventDefault(); e.stopPropagation(); }}
    >
      <div className={styles.fill} style={{ height: `${v * 100}%` }} />
      <div className={styles.knob} style={{ bottom: `calc(${v * 100}% - ${v} * var(--knob_size))` }} />
    </div>
  );
};
