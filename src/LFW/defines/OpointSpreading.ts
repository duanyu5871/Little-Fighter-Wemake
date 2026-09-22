/**
 * 物体产生时的扩散模式
 */
export enum OpointSpreading {
  /**
   * 跟LF2一样(大概)
   * 
   * Z轴速度计算: velocity.z = (index - (count - 1) / 2) * 2.5
   */
  Normal = 0,
  /**
   * 随机扩散
   * 
   * 扩散速度向量的xyz将从随机取。
   * gen_spread_x, gen_spread_y, gen_spread_z
   */
  Spreading = 1,
  /**
   * 范围内随机扩散
   */
  FloatRange = 2,
}
export const OpointSpreadingDescriptions: Record<OpointSpreading, string> = {
  [OpointSpreading.Normal]: "",
  [OpointSpreading.Spreading]: "",
  [OpointSpreading.FloatRange]: "",
}
