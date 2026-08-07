#ifndef PINS_H
#define PINS_H

// ============================================
// 未来科技盒 3.0 引脚定义
// ============================================

// --- RGB LED 矩阵 (WS2812) ---
#define RGB_PIN     33    // RGB LED 数据总线引脚
#define NUM_LEDS    9     // 3×3 = 9个LED

// --- 按键 ---
#define KEY_A       21    // 按键A (按下LOW)
#define KEY_B       0     // 按键B/BOOT (按下LOW)

// --- 超声波传感器 (接口2) ---
// 接口2 有两个引脚: GPIO7, GPIO8
// Grove 单总线超声波: 只使用一个引脚 (Trig和Echo共用)
#define ULTRASONIC_PIN  7  // 接口2-GPIO7 (Grove单总线超声波)

// --- 接口2 备用引脚 (如使用4线超声波模块) ---
// #define ULTRASONIC_TRIG  7   // 接口2-GPIO7
// #define ULTRASONIC_ECHO  8   // 接口2-GPIO8

#endif // PINS_H
