#include <Wire.h>

// ========= PCA9685 基本参数 =========
#define PCA9685_ADDR    0x40    // 默认 I2C 地址
#define PCA9685_MODE1   0x00
#define PCA9685_PRESCALE 0xFE
#define PWM_FREQ        50      // 舵机 50Hz
#define SERVO_MIN       150     // 0° 对应脉宽（按你之前的代码）
#define SERVO_MAX       600     // 180° 对应脉宽

// ========= ESP32 I2C 引脚 =========
const int I2C_SDA_PIN = 14;     // SDA = GPIO14
const int I2C_SCL_PIN = 15;     // SCL = GPIO15

// ========= 10 个舵机通道（0~9） =========
const int NUM_SERVOS = 10;
const int SERVO_CHANNELS[NUM_SERVOS] = {0,1,2,3,4,5,6,7,8,9};

// 张开 / 抓紧 对应角度（你可以根据实际再微调）
const int OPEN_ANGLE  = 0;      // 全部张开
const int CLOSE_ANGLE = 90;     // 全部抓紧（一般 60~120 之间自己调）

bool pcaOk = false;             // PCA9685 是否初始化成功
bool allClosed = false;         // 当前是否“全部抓紧”状态

// ====== 函数声明 ======
bool pca9685_init();
bool wireWriteRegister(uint8_t reg, uint8_t data);
bool setServoAngle(int channel, int angle);
void moveAllToAngle(int angle);

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== 五指张合测试（PCA9685 10 通道） ===");
  Serial.printf("I2C SDA = %d, SCL = %d\n", I2C_SDA_PIN, I2C_SCL_PIN);

  // 初始化 I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // 初始化 PCA9685
  Serial.print("初始化 PCA9685 ... ");
  if (pca9685_init()) {
    pcaOk = true;
    Serial.println("成功！");
  } else {
    pcaOk = false;
    Serial.println("失败！请检查接线和电源。");
  }

  if (pcaOk) {
    // 上电先全部张开
    Serial.println("上电：全部张开到 OPEN_ANGLE");
    moveAllToAngle(OPEN_ANGLE);
    allClosed = false;
  }

  Serial.println("串口输入 '1'，在【全部张开】和【全部抓紧】之间切换。");
}

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '1') {                      // 只处理字符 '1'
      if (!pcaOk) {
        Serial.println("PCA9685 未初始化成功，无法控制舵机！");
        return;
      }

      if (allClosed) {
        Serial.println("命令：全部张开");
        moveAllToAngle(OPEN_ANGLE);
        allClosed = false;
      } else {
        Serial.println("命令：全部抓紧");
        moveAllToAngle(CLOSE_ANGLE);
        allClosed = true;
      }
    }
  }
}

// ========= 让所有舵机一起到指定角度 =========
void moveAllToAngle(int angle) {
  for (int i = 0; i < NUM_SERVOS; i++) {
    setServoAngle(SERVO_CHANNELS[i], angle);
    delay(20);   // 每个舵机之间稍微错峰一下，减小瞬间电流
  }
}

// ========= 初始化 PCA9685 =========
bool pca9685_init() {
  // 先写 0x00 尝试唤醒
  if (!wireWriteRegister(PCA9685_MODE1, 0x00)) {
    return false;
  }
  delay(10);

  // 计算预分频，使频率为 PWM_FREQ
  double prescale_val = 25000000.0;         // 25MHz 时钟
  prescale_val /= 4096.0;
  prescale_val /= (double)PWM_FREQ;
  prescale_val -= 1.0;
  uint8_t prescale = (uint8_t)(prescale_val + 0.5);  // 四舍五入

  // 进入 sleep 模式，才能改预分频
  if (!wireWriteRegister(PCA9685_MODE1, 0x10)) {
    return false;
  }
  if (!wireWriteRegister(PCA9685_PRESCALE, prescale)) {
    return false;
  }
  // 退出 sleep，正常工作
  if (!wireWriteRegister(PCA9685_MODE1, 0x00)) {
    return false;
  }
  delay(10);
  // 打开自动递增
  if (!wireWriteRegister(PCA9685_MODE1, 0xA0)) {
    return false;
  }

  return true;
}

// ========= 向寄存器写 1 字节 =========
bool wireWriteRegister(uint8_t reg, uint8_t data) {
  Wire.beginTransmission(PCA9685_ADDR);
  Wire.write(reg);
  Wire.write(data);
  int result = Wire.endTransmission();
  return (result == 0);
}

// ========= 设置某个通道的舵机角度 =========
bool setServoAngle(int channel, int angle) {
  if (angle < 0)   angle = 0;
  if (angle > 180) angle = 180;

  // 将角度映射到脉宽
  int pulse = map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
  uint16_t on  = 0;
  uint16_t off = pulse;

  Wire.beginTransmission(PCA9685_ADDR);
  Wire.write(0x06 + 4 * channel);   // 每个通道4字节，从0x06开始
  Wire.write(on & 0xFF);            // ON_L
  Wire.write(on >> 8);              // ON_H
  Wire.write(off & 0xFF);           // OFF_L
  Wire.write(off >> 8);             // OFF_H
  int result = Wire.endTransmission();

  if (result != 0) {
    Serial.print("设置通道失败：");
    Serial.println(channel);
    return false;
  }
  return true;
}
