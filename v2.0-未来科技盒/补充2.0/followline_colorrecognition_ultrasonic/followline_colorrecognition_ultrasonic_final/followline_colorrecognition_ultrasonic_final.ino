/*
  DigitalReadSerial

  Reads a digital input on pin 2, prints the result to the Serial Monitor

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/DigitalReadSerial
*/
#include <Arduino.h>
#include "time.h"
#include "Wire.h"
#include "veml6040.h"
#include "Ultrasonic.h"

// 任务句柄
TaskHandle_t ColorTaskHandle = NULL;

//舵机引脚定义
#define STEER_PIN1 47
#define STEER_PIN2 48
hw_timer_t * time_servo1 = NULL;
volatile uint16_t pwmValue1 = 128;  
volatile uint16_t pwmValue2 = 128; 

// 循迹模块引脚定义
int LEFT = 2;
int RIGHT = 1;

// 用户引脚及蜂鸣器
#define USER_KEY1 21
#define USER_BUZZER 26
volatile uint16_t buzzerValue = 128;  
hw_timer_t * time_buzzer = NULL;

// 电机定义
#define PWM_PERIOD 255
#define MOTOR_PWM1 11
#define MOTOR_PWM2 12
#define MOTOR_PWM3 13
#define MOTOR_PWM4 14
#define MOTOR_PWM5 15
#define MOTOR_PWM6 16
#define MOTOR_PWM7 17
#define MOTOR_PWM8 18
#define LED_ROW1 33
#define LED_ROW2 34
#define LED_ROW3 35
#define LED_COL1 36
#define LED_COL2 37
#define LED_COL3 38

// 超声波雷达
Ultrasonic ultrasonic(5);
// 颜色识别
VEML6040 RGBWSensor;

#define I2C_SDA 39
#define I2C_SCL 40

uint16_t red = 0;
uint16_t blue = 0;
uint16_t green = 0;
uint16_t white = 0;
bool go_decide = false;

int towar_left = 0;
int towar_right = 0;

bool obstacle = false;
long RangeInInches;
long RangeInCentimeters;

hw_timer_t * time_motor = NULL;

// 矩阵LED
int led_3x3[3][3] = {0};
int lastLit[2] = {-1, -1}; // 记录上次点亮的LED位置

// 巡线任务RTOS函数
void ColorTask(void *parameter) {
  for (;;) {
    red = RGBWSensor.getRed();
    green = RGBWSensor.getGreen();
    blue = RGBWSensor.getBlue() * 1.5;
    white = RGBWSensor.getWhite();

    RangeInInches = ultrasonic.MeasureInInches();
    vTaskDelay(50 / portTICK_PERIOD_MS);
    RangeInCentimeters = ultrasonic.MeasureInCentimeters(); // two measurements should keep an interval

    Serial.print("line follower : left ");
    Serial.print(towar_left);
    Serial.print("  right ");
    Serial.println(towar_right);
    Serial.print(RangeInInches);//0~157 inches
    Serial.println(" inch");
    Serial.print(RangeInCentimeters);//0~400cm
    Serial.println(" cm");

    Serial.print("RED: ");
    Serial.print(RGBWSensor.getRed());  
    Serial.print(" GREEN: ");
    Serial.print(RGBWSensor.getGreen());  
    Serial.print(" BLUE: ");
    Serial.print(RGBWSensor.getBlue());  
    Serial.print(" WHITE: ");
    Serial.println(RGBWSensor.getWhite());
      
    // 确定最大颜色值及其索引
    int maxColor = 100;
    int ledIndex[2] = {-1, -1}; // 对应于led_3x3[0][0]

    if (white > 1000 && white < 2500){
      if (red > maxColor) {
        maxColor = red;
        ledIndex[0] = 0;
        ledIndex[1] = 0; // 更新为led_3x3[0][0]
      }
      if (green > maxColor) {
        maxColor = green;
        ledIndex[0] = 1;
        ledIndex[1] = 1; // 更新为led_3x3[1][1]
      }
      if (blue > maxColor) {
        maxColor = blue;
        ledIndex[0] = 2;
        ledIndex[1] = 2; // 更新为led_3x3[2][2]
      }
    }

    // 如果有一个最大颜色
    if (ledIndex[0] != -1) {
      lastLit[0] = ledIndex[0];
      lastLit[1] = ledIndex[1];
      // 更新LED硬件状态
      func_led();
    }
    else {
      digitalWrite(LED_ROW1, LOW);
      digitalWrite(LED_ROW2, LOW);
      digitalWrite(LED_ROW3, LOW);
      digitalWrite(LED_COL1, HIGH);
      digitalWrite(LED_COL2, HIGH);
      digitalWrite(LED_COL3, HIGH);
    }
  
    if(white > 2500){
      go_decide = false;
    }
    else{
      go_decide = true;
    }
    
    vTaskDelay(50 / portTICK_PERIOD_MS); // 延迟500ms
  }
}


void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
    
  Wire.begin(I2C_SDA, I2C_SCL);
  if(!RGBWSensor.begin()) {
    Serial.println("ERROR: couldn't detect the sensor");
    while(1){}
  }

	RGBWSensor.setConfiguration(VEML6040_IT_320MS + VEML6040_AF_AUTO + VEML6040_SD_ENABLE);
	
  Serial.println("Vishay VEML6040 RGBW color sensor auto mode example");
  Serial.println("CCT: Correlated color temperature in \260K");
  Serial.println("AL: Ambient light in lux");

  pinMode(LEFT, INPUT);
  pinMode(RIGHT, INPUT);
  pinMode(LED_ROW1, OUTPUT);
  pinMode(LED_ROW2, OUTPUT);
  pinMode(LED_ROW3, OUTPUT);
  pinMode(LED_COL1, OUTPUT);
  pinMode(LED_COL2, OUTPUT);
  pinMode(LED_COL3, OUTPUT);

  // 蜂鸣器
  pinMode(USER_BUZZER, OUTPUT);
  time_buzzer = timerBegin(0, 80, true);
  // 配置时钟中断函数
  timerAttachInterrupt(time_buzzer, &func_buzzer, true);
  // 设置时钟周期为0.01ms
  timerAlarmWrite(time_buzzer, 10, true);
  // 使能时钟中断
  timerAlarmEnable(time_buzzer);

  buzzerValue = 0;
  func_buzzer();

  //舵机
  pinMode(STEER_PIN1, OUTPUT);
  pinMode(STEER_PIN2, OUTPUT);
  // 新建时钟 用于软件实现pwm
  time_servo1 = timerBegin(1, 80, true);
  // 配置时钟中断函数
  timerAttachInterrupt(time_servo1, &func_servo, true);
  // 设置时钟周期为1ms
  timerAlarmWrite(time_servo1, 100, true);
  // 使能时钟中断
  timerAlarmEnable(time_servo1);

//  time_servo2 = timerBegin(2, 80, true);
//  // 配置时钟中断函数
//  timerAttachInterrupt(time_servo2, &func_servo2, true);
//  // 设置时钟周期为1ms
//  timerAlarmWrite(time_servo2, 100000, true);
//  // 使能时钟中断
//  timerAlarmEnable(time_servo2);

  // motor pwm
  pinMode(MOTOR_PWM1, OUTPUT);  //左上前进
  pinMode(MOTOR_PWM2, OUTPUT);  //左上后退
  pinMode(MOTOR_PWM3, OUTPUT);  //右上后退
  pinMode(MOTOR_PWM4, OUTPUT);  //右上前进
  pinMode(MOTOR_PWM5, OUTPUT);  //左下前进
  pinMode(MOTOR_PWM6, OUTPUT);  //左下后退
  pinMode(MOTOR_PWM7, OUTPUT);  //右下后退
  pinMode(MOTOR_PWM8, OUTPUT);  //右下前进
  analogWriteFrequency(100000);
  delay(300);

  // 巡线任务
  xTaskCreatePinnedToCore(
    ColorTask,             // 任务函数
    "Color Task",          // 任务名称
    2048,              // 栈大小
    NULL,              // 传递给任务的参数
    1,                 // 优先级
    &ColorTaskHandle,      // 任务句柄
    0                  // 指定运行在核心0
  );
}

// the loop routine runs over and over again forever:
void loop() {
  func_motor();
//  motor_stop();
//  motor_stop();
//  motor_go();

  long distance1 = ultrasonic.MeasureInCentimeters(); // two measurements should keep an interval
  if (distance1 < 8) {
    buzzerValue = 128;
    func_buzzer();
    delay(1000);
    buzzerValue = 0;
    func_buzzer();
    delay(1000);

    motor_stop();
    delay(1000);

    pwmValue1 = 13;
    for (pwmValue2 = 4; pwmValue2 <= 10; pwmValue2++){     // 机械臂舵机控制，前决定初始位置，后决定落下位置（越小越高）
      delay(50);
    }
    delay(1000);
    for (pwmValue1 = 13; pwmValue1 >=6; pwmValue1--){      // 夹子舵机控制，前决定初始位置，后决定合并位置（越小越紧）
      delay(50);
    }
    delay(1000);
    for (pwmValue2 = 10; pwmValue2 >= 4; pwmValue2--){
      delay(50);
    }
    delay(3000);
  }
  delay(200);
}


void motor_func(){                                     
  if(towar_left == 1 & towar_right ==1){ //前进
    motor_go();
  }
  else if(towar_left == 0 & towar_right ==1){                                  //偏向左，向右转
    motor_right();
  }
  else if(towar_left ==1 &towar_right ==0){                                 //向右偏，向左转
    motor_left();
  }
  else{                                                            //跑过头，向后退
    if(!go_decide){
      motor_back();
    }
    else{
      motor_go();
    }
  }
}

void motor_stop(){
  analogWrite(MOTOR_PWM1, 0);
  analogWrite(MOTOR_PWM2, 0);
  analogWrite(MOTOR_PWM3, 0);
  analogWrite(MOTOR_PWM4, 0);
  analogWrite(MOTOR_PWM5, 0);
  analogWrite(MOTOR_PWM6, 0);
  analogWrite(MOTOR_PWM7, 0);
  analogWrite(MOTOR_PWM8, 0);
}

void motor_go(){
  analogWrite(MOTOR_PWM1, 180);
  analogWrite(MOTOR_PWM2, 0);
  analogWrite(MOTOR_PWM3, 0);
  analogWrite(MOTOR_PWM4, 180);
  analogWrite(MOTOR_PWM5, 180);
  analogWrite(MOTOR_PWM6, 0);
  analogWrite(MOTOR_PWM7, 0);
  analogWrite(MOTOR_PWM8, 180);
}
void motor_back(){
  analogWrite(MOTOR_PWM1, 0);
  analogWrite(MOTOR_PWM2, 180);
  analogWrite(MOTOR_PWM3, 180);
  analogWrite(MOTOR_PWM4, 0);
  analogWrite(MOTOR_PWM5, 0);
  analogWrite(MOTOR_PWM6, 180);
  analogWrite(MOTOR_PWM7, 180);
  analogWrite(MOTOR_PWM8, 0); 
}
void motor_left(){
  analogWrite(MOTOR_PWM1, 0);
  analogWrite(MOTOR_PWM2, 90);
  analogWrite(MOTOR_PWM3, 0);
  analogWrite(MOTOR_PWM4, 200);
  analogWrite(MOTOR_PWM5, 0);
  analogWrite(MOTOR_PWM6, 90);
  analogWrite(MOTOR_PWM7, 0);
  analogWrite(MOTOR_PWM8, 200);
}
void motor_right(){
  analogWrite(MOTOR_PWM1, 200);
  analogWrite(MOTOR_PWM2, 0);
  analogWrite(MOTOR_PWM3, 90);
  analogWrite(MOTOR_PWM4, 0);
  analogWrite(MOTOR_PWM5, 200);
  analogWrite(MOTOR_PWM6, 0);
  analogWrite(MOTOR_PWM7, 90);
  analogWrite(MOTOR_PWM8, 0);
}

void func_led() {
  bool rowStates[3] = {LOW, LOW, LOW};
  bool colStates[3] = {HIGH, HIGH, HIGH};

  // 只需要更新一次状态
  rowStates[lastLit[0]] = HIGH;
  colStates[lastLit[1]] = LOW;

  digitalWrite(LED_ROW1, rowStates[0]);
  digitalWrite(LED_ROW2, rowStates[1]);
  digitalWrite(LED_ROW3, rowStates[2]);
  digitalWrite(LED_COL1, colStates[0]);
  digitalWrite(LED_COL2, colStates[1]);
  digitalWrite(LED_COL3, colStates[2]);
}

void func_buzzer(){
  static uint8_t buzzerCount = 0;
  if (++buzzerCount < buzzerValue) {
    digitalWrite(USER_BUZZER, LOW);  // 当计数达到设定的占空比时，输出低电平
  }
  if (buzzerCount >= PWM_PERIOD) {
    buzzerCount = 0;
    digitalWrite(USER_BUZZER, HIGH);  // 当计数达到PWM周期时，重置计数并输出高电平
  }
}

void func_motor(){
  towar_left=digitalRead(LEFT);
  towar_right=digitalRead(RIGHT);
  motor_func();
}

void func_servo(){
  static uint8_t pwmCount = 0;
   static uint8_t pwmCount2 = 0;
  if (++pwmCount >= pwmValue1) {
    digitalWrite(STEER_PIN1, LOW);  // 当计数达到设定的占空比时，输出低电平
//    digitalWrite(STEER_PIN2, LOW); 
  }
  if (pwmCount >= PWM_PERIOD) {
    pwmCount = 0;
    digitalWrite(STEER_PIN1, HIGH);  // 当计数达到PWM周期时，重置计数并输出高电平
//    digitalWrite(STEER_PIN2, HIGH); 
  }
  if (++pwmCount2 >= pwmValue2) {
//    digitalWrite(STEER_PIN1, LOW);  // 当计数达到设定的占空比时，输出低电平
    digitalWrite(STEER_PIN2, LOW); 
  }
  if (pwmCount2 >= PWM_PERIOD) {
    pwmCount2 = 0;
//    digitalWrite(STEER_PIN1, HIGH);  // 当计数达到PWM周期时，重置计数并输出高电平
    digitalWrite(STEER_PIN2, HIGH);
  } 
}
