export enum CollisionVal {
  /**
   * 攻击者类型
   */
  AttackerType = "attacker_type",

  /**
   * 被攻击者类型
   */
  VictimType = "victim_type",

  /** 
   * 被攻击者是否为当前跟踪对象 
   */
  VictimIsChasing = "victim_is_chasing",

  /** IItrInfo.effect */
  ItrEffect = "itr_effect",

  /** IItrInfo.kind */
  ItrKind = "itr_kind",

  /** 碰撞双方朝向是否相同，朝向相同:1，朝向不同:0 */
  SameFacing = "same_facing",

  /** 攻击者的 IFrameInfo.state */
  AttackerState = "attacker_state",

  /** 受击者的 IFrameInfo.state */
  VictimState = "victim_state",

  AttackerHasHolder = "attacker_has_holder",
  VictimHasHolder = "victim_has_holder",
  AttackerHasHolding = "attacker_has_holding",
  VictimHasHolding = "victim_has_holding",

  /** 碰撞双方是否同队，同队:1，敌对:0 */
  SameTeam = "same_team",
  /** 攻击者的数据ID */
  AttackerOID = "attacker_oid",
  /** 受击者的数据ID */
  VictimOID = "victim_oid",
  /** IBdyInfo.kind */
  BdyKind = "bdy_kind",
  VictimFrameId = "victim_frame_id",
  VictimFrameIndex_ICE = "victim_frame_index_ice",
  /** IItrInfo.fall */
  ItrFall = "itr_fall",
  AttackerThrew = "attacker_threw",
  VictimThrew = "victim_threw",
  VictimIsFreezableBall = "victim_freezable_ball",
  AttackerIsFreezableBall = "attacker_freezable_ball",
  ArmorWork = "armor_work",
  V_FrameBehavior = "v_frame_behavior",
  NoItrEffect = "no_itr_effect",
  /** 攻击者的剩余血量百分比，整数，值范围:[0,100] */
  A_HP_P = "a_hp_p",
  /** 被击者的剩余血量百分比，整数，值范围:[0,100] */
  V_HP_P = "v_hp_p",
  /** 是否开启了LF2.NET作弊码，开启:1，未开启:0 */
  LF2_NET_ON = "lf2_net_on",
  BdyHitFlag = "bdy_hit_flag",
  ItrHitFlag = "itr_hit_flag",
  /** IBdyInfo.code */
  BdyCode = "bdy_code",
  /** IItrInfo.code */
  ItrCode = "itr_code",
  VToughness = "v_toughness",
  AToughness = "a_toughness",
  AttackerBaseType = "attacker_base_type",
  VictimBaseType = "victim_base_type",
  /* 接近的速度X，大于0时，“攻击者”速度朝向“受击者” */
  AClosingSpeedX = "a_closing_speed_x",
  /* 接近的速度Y，大于0时，“攻击者”速度朝向“受击者” */
  AClosingSpeedY = "a_closing_speed_y",
  /* 接近的速度Z，大于0时，“攻击者”速度朝向“受击者” */
  AClosingSpeedZ = "a_closing_speed_z",
  /** 谁射的attacker */
  AEmitter = "a_emitter",
  /** 谁射的victim */
  VEmitter = "v_emitter",

  AHitAttack /**/ = 'a_hit_attack',
  AHitJump   /**/ = 'a_hit_jump',
  AHitDefend /**/ = 'a_hit_defend',
  AHitUp     /**/ = 'a_hit_up',
  AHitDown   /**/ = 'a_hit_down',
  AHitLeft   /**/ = 'a_hit_left',
  AHitRight  /**/ = 'a_hit_right',
  VHitAttack /**/ = 'v_hit_attack',
  VHitJump   /**/ = 'v_hit_jump',
  VHitDefend /**/ = 'v_hit_defend',
  VHitUp     /**/ = 'v_hit_up',
  VHitDown   /**/ = 'v_hit_down',
  VHitLeft   /**/ = 'v_hit_left',
  VHitRight  /**/ = 'v_hit_right',

  AClickAttack /**/ = 'a_click_attack',
  AClickJump   /**/ = 'a_click_jump',
  AClickDefend /**/ = 'a_click_defend',
  AClickUp     /**/ = 'a_click_up',
  AClickDown   /**/ = 'a_click_down',
  AClickLeft   /**/ = 'a_click_left',
  AClickRight  /**/ = 'a_click_right',
  VClickAttack /**/ = 'v_click_attack',
  VClickJump   /**/ = 'v_click_jump',
  VClickDefend /**/ = 'v_click_defend',
  VClickUp     /**/ = 'v_click_up',
  VClickDown   /**/ = 'v_click_down',
  VClickLeft   /**/ = 'v_click_left',
  VClickRight  /**/ = 'v_click_right',
  
  ADbcAttack /**/ = 'a_dbclick_attack',
  ADbcJump   /**/ = 'a_dbclick_jump',
  ADbcDefend /**/ = 'a_dbclick_defend',
  ADbcUp     /**/ = 'a_dbclick_up',
  ADbcDown   /**/ = 'a_dbclick_down',
  ADbcLeft   /**/ = 'a_dbclick_left',
  ADbcRight  /**/ = 'a_dbclick_right',
  VDbcAttack /**/ = 'v_dbclick_attack',
  VDbcJump   /**/ = 'v_dbclick_jump',
  VDbcDefend /**/ = 'v_dbclick_defend',
  VDbcUp     /**/ = 'v_dbclick_up',
  VDbcDown   /**/ = 'v_dbclick_down',
  VDbcLeft   /**/ = 'v_dbclick_left',
  VDbcRight  /**/ = 'v_dbclick_right',
}
export const CollisionValDescriptions: Record<CollisionVal, string> = {
  [CollisionVal.AttackerType]: "",
  [CollisionVal.VictimType]: "",
  [CollisionVal.VictimIsChasing]: "",
  [CollisionVal.ItrEffect]: "",
  [CollisionVal.ItrKind]: "",
  [CollisionVal.SameFacing]: "",
  [CollisionVal.AttackerState]: "",
  [CollisionVal.VictimState]: "",
  [CollisionVal.AttackerHasHolder]: "",
  [CollisionVal.VictimHasHolder]: "",
  [CollisionVal.AttackerHasHolding]: "",
  [CollisionVal.VictimHasHolding]: "",
  [CollisionVal.SameTeam]: "",
  [CollisionVal.AttackerOID]: "",
  [CollisionVal.VictimOID]: "",
  [CollisionVal.BdyKind]: "",
  [CollisionVal.VictimFrameId]: "",
  [CollisionVal.VictimFrameIndex_ICE]: "",
  [CollisionVal.ItrFall]: "",
  [CollisionVal.AttackerThrew]: "",
  [CollisionVal.VictimThrew]: "",
  [CollisionVal.VictimIsFreezableBall]: "",
  [CollisionVal.AttackerIsFreezableBall]: "",
  [CollisionVal.ArmorWork]: "",
  [CollisionVal.V_FrameBehavior]: "",
  [CollisionVal.NoItrEffect]: "",
  [CollisionVal.A_HP_P]: "",
  [CollisionVal.V_HP_P]: "",
  [CollisionVal.LF2_NET_ON]: "",
  [CollisionVal.BdyHitFlag]: "",
  [CollisionVal.ItrHitFlag]: "",
  [CollisionVal.BdyCode]: "",
  [CollisionVal.ItrCode]: "",
  [CollisionVal.VToughness]: "",
  [CollisionVal.AToughness]: "",
  [CollisionVal.AttackerBaseType]: "",
  [CollisionVal.VictimBaseType]: "",
  [CollisionVal.AClosingSpeedX]: "",
  [CollisionVal.AClosingSpeedY]: "",
  [CollisionVal.AClosingSpeedZ]: "",
  [CollisionVal.AEmitter]: "",
  [CollisionVal.VEmitter]: "",
  [CollisionVal.AHitAttack]:/* */ "",
  [CollisionVal.AHitJump]: /*   */"",
  [CollisionVal.AHitDefend]:/* */ "",
  [CollisionVal.AHitUp]:/*     */ "",
  [CollisionVal.AHitDown]:/*   */ "",
  [CollisionVal.AHitLeft]:/*   */ "",
  [CollisionVal.AHitRight]:/*  */ "",
  [CollisionVal.VHitAttack]:/* */ "",
  [CollisionVal.VHitJump]:/*   */ "",
  [CollisionVal.VHitDefend]:/* */ "",
  [CollisionVal.VHitUp]:/*     */ "",
  [CollisionVal.VHitDown]:/*   */ "",
  [CollisionVal.VHitLeft]:/*   */ "",
  [CollisionVal.VHitRight]:/*  */ "",
  [CollisionVal.AClickAttack]:/* */ "",
  [CollisionVal.AClickJump]: /*   */"",
  [CollisionVal.AClickDefend]:/* */ "",
  [CollisionVal.AClickUp]:/*     */ "",
  [CollisionVal.AClickDown]:/*   */ "",
  [CollisionVal.AClickLeft]:/*   */ "",
  [CollisionVal.AClickRight]:/*  */ "",
  [CollisionVal.VClickAttack]:/* */ "",
  [CollisionVal.VClickJump]:/*   */ "",
  [CollisionVal.VClickDefend]:/* */ "",
  [CollisionVal.VClickUp]:/*     */ "",
  [CollisionVal.VClickDown]:/*   */ "",
  [CollisionVal.VClickLeft]:/*   */ "",
  [CollisionVal.VClickRight]:/*  */ "",  
  [CollisionVal.ADbcAttack]:/* */ "",
  [CollisionVal.ADbcJump]: /*   */"",
  [CollisionVal.ADbcDefend]:/* */ "",
  [CollisionVal.ADbcUp]:/*     */ "",
  [CollisionVal.ADbcDown]:/*   */ "",
  [CollisionVal.ADbcLeft]:/*   */ "",
  [CollisionVal.ADbcRight]:/*  */ "",
  [CollisionVal.VDbcAttack]:/* */ "",
  [CollisionVal.VDbcJump]:/*   */ "",
  [CollisionVal.VDbcDefend]:/* */ "",
  [CollisionVal.VDbcUp]:/*     */ "",
  [CollisionVal.VDbcDown]:/*   */ "",
  [CollisionVal.VDbcLeft]:/*   */ "",
  [CollisionVal.VDbcRight]:/*  */ "",
}
export const C_Val = CollisionVal;
export type C_Val = CollisionVal;