/**
 * 未来科技盒 2.0 引脚定义
 * 功能：小车前进，按键A停止
 */

#ifndef PINS_H
#define PINS_H

// ============ 按键引脚 ============
#define KEY_A   21    // 按键A (按下时 LOW)
#define KEY_B   0     // 按键B (按下时 LOW)

// ============ 电机引脚 ============
// 小车形态电机布局：
//         前方
//     ┌─────────┐
//     │  M1  M2 │   M1=左上  M2=右上
//     │         │
//     │  M3  M4 │   M3=左下  M4=右下
//     └─────────┘
//         后方

// M1 - 左上电机
#define M1_FWD  11    // M1 正转
#define M1_REV  12    // M1 反转

// M2 - 右上电机 (注意：正反转引脚顺序与左侧相反)
#define M2_FWD  14    // M2 正转
#define M2_REV  13    // M2 反转

// M3 - 左下电机
#define M3_FWD  15    // M3 正转
#define M3_REV  16    // M3 反转

// M4 - 右下电机 (注意：正反转引脚顺序与左侧相反)
#define M4_FWD  18    // M4 正转
#define M4_REV  17    // M4 反转

// ============ PWM 配置 ============
#define PWM_FREQ       5000   // PWM 频率 5kHz
#define PWM_RESOLUTION 8      // 8位分辨率 (0-255)

#endif // PINS_H
