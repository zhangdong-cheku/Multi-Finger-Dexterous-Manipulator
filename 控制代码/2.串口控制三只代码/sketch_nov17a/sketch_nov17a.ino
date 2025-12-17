#include <ESP32Servo.h>

const int SERVO_PIN = 2;  // 连接第一个舵机的 GPIO 引脚
Servo servo1;

void setup() {
  Serial.begin(115200);  // 初始化串口
  servo1.setPeriodHertz(50);  // 设置频率为 50Hz
  servo1.attach(SERVO_PIN, 500, 2500);  // 附加舵机到 GPIO 引脚
  servo1.write(0);  // 初始化舵机为 0 度（完全收回）
}

void loop() {
  servo1.write(180);  // 设置舵机到 180 度（完全张开）
  delay(2000);  // 等待 2 秒
  servo1.write(0);  // 设置舵机回到 0 度（完全收回）
  delay(2000);  // 等待 2 秒
}
