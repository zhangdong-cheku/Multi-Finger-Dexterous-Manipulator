#include <ESP32Servo.h>

// ====== 舵机对象 ======
// 拇指
Servo thumbL;  // 舵机1，左
Servo thumbR;  // 舵机2，右

// 食指
Servo indexL;  // 舵机3，左
Servo indexR;  // 舵机4，右

// 中指
Servo middleL; // 舵机5，左
Servo middleR; // 舵机6，右

// ====== 引脚映射（按你画的图）======
const int PIN_THUMB_L  = 14; // 舵机1
const int PIN_THUMB_R  = 13; // 舵机2
const int PIN_INDEX_L  = 12; // 舵机3
const int PIN_INDEX_R  = 11; // 舵机4
const int PIN_MIDDLE_L = 10; // 舵机5
const int PIN_MIDDLE_R = 9;  // 舵机6

// 每根手指的中位 & 弯曲量（后面可以分别微调）
const int MID_THUMB   = 90;
const int DELTA_THUMB = 35;

const int MID_INDEX   = 90;
const int DELTA_INDEX = 35;

const int MID_MIDDLE   = 90;
const int DELTA_MIDDLE = 35;

bool allClosed = false;  // 當前是否三指都處於"抓緊"狀態

// 通用：設置一根手指兩個舵機角度
void setFinger(Servo &servoL, Servo &servoR, int mid, int delta)
{
  int left  = mid + delta;
  int right = mid - delta;

  left  = constrain(left,  0, 180);
  right = constrain(right, 0, 180);

  servoL.write(left);
  servoR.write(right);
}

// ====== 拇指控制 ======
void openThumb()  { setFinger(thumbL,  thumbR,  MID_THUMB,   0); }
void closeThumb() { setFinger(thumbL,  thumbR,  MID_THUMB,   DELTA_THUMB); }

// ====== 食指控制 ======
void openIndex()  { setFinger(indexL,  indexR,  MID_INDEX,   0); }
void closeIndex() { setFinger(indexL,  indexR,  MID_INDEX,   DELTA_INDEX); }

// ====== 中指控制 ======
void openMiddle()  { setFinger(middleL, middleR, MID_MIDDLE,  0); }
void closeMiddle() { setFinger(middleL, middleR, MID_MIDDLE,  DELTA_MIDDLE); }

// 三指一起
void openAll()
{
  openThumb();
  openIndex();
  openMiddle();
}

void closeAll()
{
  closeThumb();
  closeIndex();
  closeMiddle();
}

void setup()
{
  Serial.begin(115200);

  // 分配 6 個定時器
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  ESP32PWM::allocateTimer(4);
  ESP32PWM::allocateTimer(5);

  thumbL.setPeriodHertz(50);
  thumbR.setPeriodHertz(50);
  indexL.setPeriodHertz(50);
  indexR.setPeriodHertz(50);
  middleL.setPeriodHertz(50);
  middleR.setPeriodHertz(50);

  thumbL.attach(PIN_THUMB_L,   500, 2500);
  thumbR.attach(PIN_THUMB_R,   500, 2500);
  indexL.attach(PIN_INDEX_L,   500, 2500);
  indexR.attach(PIN_INDEX_R,   500, 2500);
  middleL.attach(PIN_MIDDLE_L, 500, 2500);
  middleR.attach(PIN_MIDDLE_R, 500, 2500);

  openAll();  // 上電先全部張開

  Serial.println("=== 三指聯動 測試 ===");
  Serial.println("輸入 1：三指在【全部張開 <-> 全部抓緊】之間切換");
  Serial.println("輸入 o：全部張開");
  Serial.println("輸入 c：全部抓緊");
}

void loop()
{
  if (Serial.available() > 0) {
    char c = Serial.read();
    Serial.print("收到：");
    Serial.println(c);

    if (c == '1') {
      if (allClosed) {
        openAll();
        allClosed = false;
        Serial.println("動作：全部張開");
      } else {
        closeAll();
        allClosed = true;
        Serial.println("動作：全部抓緊");
      }
    } else if (c == 'o' || c == 'O') {
      openAll();
      allClosed = false;
      Serial.println("命令 o：全部張開");
    } else if (c == 'c' || c == 'C') {
      closeAll();
      allClosed = true;
      Serial.println("命令 c：全部抓緊");
    }
  }
}
