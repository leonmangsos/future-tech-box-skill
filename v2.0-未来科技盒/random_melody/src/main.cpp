/**
 * 按键A触发随机旋律
 * 
 * 功能：按下按键A后，蜂鸣器播放一段随机旋律
 * 硬件：按键A (GPIO21), 蜂鸣器 (GPIO26)
 */

#include <Arduino.h>

// 引脚定义
#define KEY_A_PIN   21
#define BUZZER_PIN  26

// 音符频率定义 (Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_REST 0   // 休止符

// 预定义的旋律数组
// 旋律1: 小星星
const int melody1[] = {
  NOTE_C5, NOTE_C5, NOTE_G5, NOTE_G5, NOTE_A5, NOTE_A5, NOTE_G5, NOTE_REST,
  NOTE_F5, NOTE_F5, NOTE_E5, NOTE_E5, NOTE_D5, NOTE_D5, NOTE_C5, NOTE_REST
};
const int melody1Len = 16;

// 旋律2: 欢乐颂片段
const int melody2[] = {
  NOTE_E5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_D5,
  NOTE_C5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_E5, NOTE_D5, NOTE_D5, NOTE_REST
};
const int melody2Len = 16;

// 旋律3: 生日快乐片段
const int melody3[] = {
  NOTE_C5, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_F5, NOTE_E5, NOTE_REST,
  NOTE_C5, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_G5, NOTE_F5, NOTE_REST
};
const int melody3Len = 14;

// 旋律4: 上升音阶
const int melody4[] = {
  NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5,
  NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6, NOTE_REST
};
const int melody4Len = 16;

// 旋律5: 警报音
const int melody5[] = {
  NOTE_C5, NOTE_G5, NOTE_C5, NOTE_G5, NOTE_C5, NOTE_G5, NOTE_REST,
  NOTE_E5, NOTE_A5, NOTE_E5, NOTE_A5, NOTE_E5, NOTE_A5, NOTE_REST
};
const int melody5Len = 14;

// 旋律6: 胜利号角
const int melody6[] = {
  NOTE_G4, NOTE_G4, NOTE_G4, NOTE_C5, NOTE_REST, NOTE_C5, NOTE_C5, NOTE_C5, NOTE_E5,
  NOTE_REST, NOTE_E5, NOTE_G5, NOTE_C6, NOTE_REST
};
const int melody6Len = 14;

// 旋律7: 下降音阶
const int melody7[] = {
  NOTE_C6, NOTE_B5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5,
  NOTE_B4, NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_C4, NOTE_REST
};
const int melody7Len = 16;

// 旋律8: 跳跃音符
const int melody8[] = {
  NOTE_C5, NOTE_E5, NOTE_C5, NOTE_G5, NOTE_C5, NOTE_E5, NOTE_G5, NOTE_REST,
  NOTE_D5, NOTE_F5, NOTE_D5, NOTE_A5, NOTE_D5, NOTE_F5, NOTE_A5, NOTE_REST
};
const int melody8Len = 16;

// 按键状态
bool lastKeyState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// 播放状态
bool isPlaying = false;
int currentNote = 0;
const int* currentMelody = nullptr;
int currentMelodyLen = 0;
unsigned long noteStartTime = 0;
const int NOTE_DURATION = 150;  // 每个音符150ms
const int NOTE_GAP = 30;        // 音符间隔30ms

void setup() {
  Serial.begin(115200);
  
  // 初始化引脚
  pinMode(KEY_A_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // 初始化随机数种子
  randomSeed(analogRead(0));
  
  Serial.println("=================================");
  Serial.println("   按键A触发随机旋律");
  Serial.println("=================================");
  Serial.println("按下按键A播放随机旋律");
  Serial.println();
  
  // 开机提示音
  tone(BUZZER_PIN, NOTE_C5, 100);
  delay(150);
  tone(BUZZER_PIN, NOTE_E5, 100);
  delay(150);
  tone(BUZZER_PIN, NOTE_G5, 100);
  delay(150);
  noTone(BUZZER_PIN);
  
  Serial.println("准备就绪！");
}

// 选择随机旋律
void selectRandomMelody() {
  int melodyIndex = random(1, 9);  // 1-8
  
  switch (melodyIndex) {
    case 1:
      currentMelody = melody1;
      currentMelodyLen = melody1Len;
      Serial.println("🎵 播放: 小星星");
      break;
    case 2:
      currentMelody = melody2;
      currentMelodyLen = melody2Len;
      Serial.println("🎵 播放: 欢乐颂");
      break;
    case 3:
      currentMelody = melody3;
      currentMelodyLen = melody3Len;
      Serial.println("🎵 播放: 生日快乐");
      break;
    case 4:
      currentMelody = melody4;
      currentMelodyLen = melody4Len;
      Serial.println("🎵 播放: 上升音阶");
      break;
    case 5:
      currentMelody = melody5;
      currentMelodyLen = melody5Len;
      Serial.println("🎵 播放: 警报音");
      break;
    case 6:
      currentMelody = melody6;
      currentMelodyLen = melody6Len;
      Serial.println("🎵 播放: 胜利号角");
      break;
    case 7:
      currentMelody = melody7;
      currentMelodyLen = melody7Len;
      Serial.println("🎵 播放: 下降音阶");
      break;
    case 8:
      currentMelody = melody8;
      currentMelodyLen = melody8Len;
      Serial.println("🎵 播放: 跳跃音符");
      break;
  }
  
  currentNote = 0;
  isPlaying = true;
  noteStartTime = millis();
}

// 非阻塞播放旋律
void updateMelody() {
  if (!isPlaying) return;
  
  unsigned long now = millis();
  unsigned long elapsed = now - noteStartTime;
  
  // 检查是否该播放下一个音符
  if (elapsed >= NOTE_DURATION + NOTE_GAP) {
    currentNote++;
    
    if (currentNote >= currentMelodyLen) {
      // 旋律播放完毕
      noTone(BUZZER_PIN);
      isPlaying = false;
      Serial.println("✅ 旋律播放完成\n");
      return;
    }
    
    noteStartTime = now;
    
    // 播放当前音符
    int freq = currentMelody[currentNote];
    if (freq > 0) {
      tone(BUZZER_PIN, freq);
    } else {
      noTone(BUZZER_PIN);  // 休止符
    }
  }
  else if (elapsed >= NOTE_DURATION) {
    // 在音符间隔期间静音
    noTone(BUZZER_PIN);
  }
}

// 开始播放旋律
void startMelody() {
  selectRandomMelody();
  
  // 播放第一个音符
  if (currentMelodyLen > 0 && currentMelody[0] > 0) {
    tone(BUZZER_PIN, currentMelody[0]);
  }
}

void loop() {
  unsigned long now = millis();
  
  // 读取按键状态
  bool currentKeyState = digitalRead(KEY_A_PIN);
  
  // 检测按键A下降沿（按下）+ 非阻塞消抖
  if (lastKeyState == HIGH && currentKeyState == LOW) {
    if (now - lastDebounceTime > DEBOUNCE_DELAY) {
      // 如果当前没有在播放，则开始新旋律
      if (!isPlaying) {
        Serial.println("\n>>> 按键A按下!");
        startMelody();
      }
      lastDebounceTime = now;
    }
  }
  
  // 更新按键状态
  lastKeyState = currentKeyState;
  
  // 更新旋律播放（非阻塞）
  updateMelody();
}
