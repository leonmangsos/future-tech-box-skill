/**
 * 未来科技盒 2.0 引脚定义
 * 麦克纳姆轮小车横移演示
 */

#ifndef PINS_H
#define PINS_H

// ==================== 电机引脚 ====================
// 电机布局（俯视图）：
//     前方
//  ┌─────────┐
//  │  M1  M2 │  M1=左上  M2=右上
//  │         │
//  │  M3  M4 │  M3=左下  M4=右下
//  └─────────┘
//     后方

// M1 - 左上电机
#define M1_FWD 11  // 正转
#define M1_REV 12  // 反转

// M2 - 右上电机（GPIO顺序与左侧相反）
#define M2_FWD 14  // 正转
#define M2_REV 13  // 反转

// M3 - 左下电机
#define M3_FWD 15  // 正转
#define M3_REV 16  // 反转

// M4 - 右下电机（GPIO顺序与左侧相反）
#define M4_FWD 18  // 正转
#define M4_REV 17  // 反转

// ==================== 按键 ====================
#define KEY_A 21   // 按键 A（按下为 LOW）
#define KEY_B 0    // 按键 B / BOOT（按下为 LOW）

#endif // PINS_H
