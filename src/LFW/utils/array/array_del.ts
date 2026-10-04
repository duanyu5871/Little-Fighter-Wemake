export function array_del<T>(array: T[], target: T): boolean {
  let i = 0;
  let j = 0;
  for (; j < array.length; i++, j++) {
    if (array[j] === target) {
      i -= 1;
    } else {
      array[i] = array[j];
    }
  }
  array.length = i + 1;
  return i !== j;
}
