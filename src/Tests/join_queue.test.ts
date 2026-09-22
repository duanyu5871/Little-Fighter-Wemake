import { JoinQueue, pick_join_team } from "../LFW/helper/JoinQueue";

const entrant = (uid: string, name: string = uid) => ({ uid, name });

test("JoinQueue: FIFO、去重、上限、移除", () => {
  const q = new JoinQueue(3);
  expect(q.enqueue(entrant("a"))).toBe(true);
  expect(q.enqueue(entrant("a"))).toBe(false);
  expect(q.enqueue(entrant(""))).toBe(false);
  expect(q.enqueue(entrant("b"))).toBe(true);
  expect(q.enqueue(entrant("c"))).toBe(true);
  expect(q.enqueue(entrant("d"))).toBe(false);
  expect(q.size).toBe(3);
  expect(q.has("b")).toBe(true);
  expect(q.all.map((v) => v.uid)).toEqual(["a", "b", "c"]);

  expect(q.remove("b")).toBe(true);
  expect(q.remove("b")).toBe(false);
  expect(q.dequeue()!.uid).toBe("a");
  expect(q.enqueue(entrant("d"))).toBe(true);
  expect(q.dequeue()!.uid).toBe("c");
  expect(q.dequeue()!.uid).toBe("d");
  expect(q.dequeue()).toBeUndefined();

  q.enqueue(entrant("e"));
  q.clear();
  expect(q.size).toBe(0);
  expect(q.enqueue(entrant("e"))).toBe(true);
});

test("pick_join_team: 最少存活优先、满员/全灭不补、平手优先阵亡队", () => {
  const caps = new Map([["t1", 3], ["t2", 3], ["t3", 3]]);
  const order = [...caps.keys()];

  expect(
    pick_join_team(new Map([["t1", 1], ["t2", 2], ["t3", 2]]), caps, null, order),
  ).toBe("t1");

  expect(
    pick_join_team(new Map([["t1", 3], ["t2", 3], ["t3", 3]]), caps, null, order),
  ).toBeUndefined();

  expect(
    pick_join_team(new Map([["t1", 3], ["t2", 0], ["t3", 3]]), caps, "t2", order),
  ).toBeUndefined();

  expect(
    pick_join_team(new Map([["t1", 3], ["t2", 2], ["t3", 2]]), caps, "t3", order),
  ).toBe("t3");

  expect(
    pick_join_team(new Map([["t1", 3], ["t2", 2], ["t3", 2]]), caps, null, ["t2", "t3"]),
  ).toBe("t2");

  expect(
    pick_join_team(new Map([["t2", 2]]), caps, null, ["t2"]),
  ).toBe("t2");
});
