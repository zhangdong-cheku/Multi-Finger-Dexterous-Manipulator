// =======================================================
// hand_arduino_v1_servo1-2.ino
// 舵机1-2 控制（PCA9685 通道 0 和 1）
//
// 硬件:
//   ESP32-S3 → PCA9685: SDA = 14, SCL = 15
//   PCA9685 通道: 0 = 舵机1, 1 = 舵机2
//
// 角度定义: 0°, 90°, 180°
// 最大角度: 180°
// 最小角度: 0°
// 初始角度: 90°
// 平滑移动 Delay: 5 ms
//
// 串口: 115200 （PC 这边打开 COM3）
//
// —— 串口协议（配合 Python GUI 使用）——
//   GUI 按“启动1”      -> 发送 "MODE1\n"
//   GUI 按“启动2”      -> 发送 "MODE2\n"
//   竖直滑块(拇指)拖动 -> 发送 "THUMB:角度\n"  角度范围 90~180
//   左右滑块拖动       -> 发送 "LR:角度\n"      角度范围 0~180
//
// 模式1 (MODE1):
//   使用 THUMB:角度，范围 90~180
//   通道0 (舵机1): 90°→180°  (跟随角度 A)
//   通道1 (舵机2): 90°→0°    (角度 = 180 - A)
//
// 模式2 (MODE2):
//   按下后先把两个舵机都平滑复位到 90°
//   然后使用 LR:角度，范围 0~180
//   通道0 (舵机1): 0°→180°  (跟随角度 A)
//   通道1 (舵机2): 跟随通道0，一起 0°→180°
//
// 说明:
//   - THUMB 指令只在 MODE1 中有效，MODE2 中会被忽略。
//   - LR 指令只在 MODE2 中有效，MODE1 中会被忽略。
//   - 方便串口监视器手动测试：纯数字行会根据当前模式当作角度处理。
// =======================================================

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// -------- PCA9685 基本参数 --------
#define PCA9685_ADDR    0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

// 50Hz (舵机)
#define PWM_FREQ        50

// 这里用常见的 150~600 作为舵机脉宽范围（0~180 度）
#define SERVO_MIN_PULSE 150
#define SERVO_MAX_PULSE 600

// 舵机通道
const uint8_t SERVO_CH1 = 0;  // 舵机1 -> PCA9685 通道0
const uint8_t SERVO_CH2 = 1;  // 舵机2 -> PCA9685 通道1

// 角度参数
const int ANGLE_MIN = 0;
const int ANGLE_MAX = 180;
const int ANGLE_INIT = 90;

// 平滑移动延时
const uint16_t STEP_DELAY_MS = 5;

// 当前角度（软件记忆）
int servo1Angle = ANGLE_INIT;
int servo2Angle = ANGLE_INIT;

// 当前模式: 0=空闲, 1=模式1, 2=模式2
int currentMode = 0;

// =======================================================
// 工具函数
// =======================================================

// 将角度映射为 PCA9685 的脉宽 (0~4095 步)
uint16_t angleToPulse(int angle) {
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  return map(angle, ANGLE_MIN, ANGLE_MAX, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
}

// 设置指定通道的舵机角度
void setServoAngle(uint8_t ch, int angle) {
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  uint16_t pulselen = angleToPulse(angle);
  pwm.setPWM(ch, 0, pulselen);
}

// 让两个舵机一起平滑移动到目标角度
void moveServosSmooth(int target1, int target2) {
  target1 = constrain(target1, ANGLE_MIN, ANGLE_MAX);
  target2 = constrain(target2, ANGLE_MIN, ANGLE_MAX);

  if (servo1Angle == target1 && servo2Angle == target2) {
    return; // 不用动
  }

  int step1 = (target1 > servo1Angle) ? 1 : -1;
  int step2 = (target2 > servo2Angle) ? 1 : -1;

  if (servo1Angle == target1) step1 = 0;
  if (servo2Angle == target2) step2 = 0;

  while (servo1Angle != target1 || servo2Angle != target2) {
    if (servo1Angle != target1) {
      servo1Angle += step1;
    }
    if (servo2Angle != target2) {
      servo2Angle += step2;
    }

    setServoAngle(SERVO_CH1, servo1Angle);
    setServoAngle(SERVO_CH2, servo2Angle);

    delay(STEP_DELAY_MS);
  }

  setServoAngle(SERVO_CH1, servo1Angle);
  setServoAngle(SERVO_CH2, servo2Angle);
}

// =======================================================
// 不同来源的角度处理函数
// =======================================================

// 来自 THUMB:angle —— 只在 MODE1 中有效
void handleThumbAngle(int angleIn) {
  if (currentMode != 1) {
    Serial.println("[THUMB] 当前不是 MODE1，忽略 THUMB 指令");
    return;
  }

  // 模式1：输入 90~180
  // 通道1: 90~180
  // 通道2: 90~0 (180 - A)
  int A = constrain(angleIn, 90, 180);

  int target1 = A;
  int target2 = 180 - A;  // A=90 -> 90, A=180 -> 0

  Serial.print("[MODE1] THUMB 角度=");
  Serial.print(A);
  Serial.print(" -> ch1=");
  Serial.print(target1);
  Serial.print(" ch2=");
  Serial.println(target2);

  moveServosSmooth(target1, target2);
}

// 来自 LR:angle —— 只在 MODE2 中有效
void handleLRAngle(int angleIn) {
  if (currentMode != 2) {
    Serial.println("[LR] 当前不是 MODE2，忽略 LR 指令");
    return;
  }

  // 模式2：输入 0~180
  // 通道1: 0~180
  // 通道2: 跟随通道1
  int A = constrain(angleIn, 0, 180);

  int target1 = A;
  int target2 = A;

  Serial.print("[MODE2] LR 角度=");
  Serial.print(A);
  Serial.print(" -> ch1=ch2=");
  Serial.println(target1);

  moveServosSmooth(target1, target2);
}

// 备用：纯数字行，根据当前模式决定走哪条逻辑
void handlePlainNumber(int angleIn) {
  if (currentMode == 1) {
    handleThumbAngle(angleIn);  // 当作 THUMB
  } else if (currentMode == 2) {
    handleLRAngle(angleIn);     // 当作 LR
  } else {
    Serial.println("[数字指令] 当前模式=0，忽略");
  }
}

// 解析串口收到的一整行命令
void parseCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  Serial.print("[串口接收] ");
  Serial.println(line);

  // -------- 模式切换 --------
  if (line.equalsIgnoreCase("MODE1")) {
    currentMode = 1;
    Serial.println("[模式] 切换到 MODE1");
    return;
  }

  if (line.equalsIgnoreCase("MODE2")) {
    currentMode = 2;
    Serial.println("[模式] 切换到 MODE2");
    Serial.println("[MODE2] 先把两个舵机复位到 90°，然后再开始同步控制");

    // 启动2后，先把两个舵机平滑移动到 90°
    moveServosSmooth(ANGLE_INIT, ANGLE_INIT);

    return;
  }

  // -------- THUMB:angle --------
  if (line.startsWith("THUMB:") || line.startsWith("Thumb:") || line.startsWith("thumb:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] THUMB 指令角度解析失败");
      return;
    }
    handleThumbAngle(angle);
    return;
  }

  // -------- LR:angle --------
  if (line.startsWith("LR:") || line.startsWith("lr:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] LR 指令角度解析失败");
      return;
    }
    handleLRAngle(angle);
    return;
  }

  // -------- 纯数字：根据当前模式转给对应处理 --------
  bool allDigit = true;
  for (uint16_t i = 0; i < line.length(); i++) {
    if (!isDigit(line[i])) {
      allDigit = false;
      break;
    }
  }
  if (allDigit) {
    int angle = line.toInt();
    handlePlainNumber(angle);
    return;
  }

  Serial.println("[提示] 未识别的指令（可用: MODE1, MODE2, THUMB:角度, LR:角度）");
}

// =======================================================
// setup / loop
// =======================================================

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== hand_arduino_v1_servo1-2.ino ===");

  // I2C 初始化：SDA = 14, SCL = 15
  Wire.begin(14, 15);

  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);

  delay(10);

  // 初始化舵机到 90°
  setServoAngle(SERVO_CH1, ANGLE_INIT);
  setServoAngle(SERVO_CH2, ANGLE_INIT);
  servo1Angle = ANGLE_INIT;
  servo2Angle = ANGLE_INIT;

  Serial.println("[初始化] 舵机1/2 已复位到 90°");
  Serial.println("[提示] 发送 MODE1 / MODE2 切换模式；");
  Serial.println("       在 MODE1 下用 THUMB:角度（90~180）控制；");
  Serial.println("       在 MODE2 下用 LR:角度（0~180）控制。");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    parseCommand(line);
  }
}
