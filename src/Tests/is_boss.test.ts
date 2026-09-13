import { EntityEnum } from "../LFW/defines/EntityEnum";
import { EntityGroup } from "../LFW/defines/EntityGroup";
import { is_boss } from "../LFW/entity/type_check";

function obj(type: EntityEnum, group?: string[]) {
  return { id: 't', data: { id: 't', type, base: { name: 't', group } } };
}

test("is_boss: 斗士(type=Fighter)按 base.group 判定", () => {
  expect(is_boss(obj(EntityEnum.Fighter, [EntityGroup.Boss]))).toBe(true);
  expect(is_boss(obj(EntityEnum.Fighter, ['Hero', 'Boss']))).toBe(true);
  expect(is_boss(obj(EntityEnum.Entity, [EntityGroup.Boss]))).toBe(true);
  expect(is_boss(obj(EntityEnum.Fighter, ['Hero']))).toBe(false);
  expect(is_boss(obj(EntityEnum.Fighter, []))).toBe(false);
  expect(is_boss(obj(EntityEnum.Fighter))).toBe(false);
  expect(is_boss(obj(EntityEnum.Weapon, ['Hero']))).toBe(false);
  expect(is_boss(null)).toBe(false);
  expect(is_boss({})).toBe(false);
  expect(is_boss({ data: null })).toBe(false);
});
