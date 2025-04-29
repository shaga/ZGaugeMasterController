#include <Arduino.h>
#include <M5Unified.h>
#include "TrainController.h"
#include "MidiDataReceiver.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "MasterController.h"
#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>
#include "display.h"
// #include <M5GFX.h>

USB usb;
USBHub hub(&usb);
HIDUniversal hid(&usb);
MasterControllerEvents masconEvents;
MasterController masscon(&masconEvents);

static const uint8_t SPEED_LIMIT = 85;

// 力行段階の定義
typedef struct {
    uint8_t max_speed;   // 最大速度 (PWM値)
    uint8_t base_accel;  // 基本加速度 (PWM値)
    uint8_t period;      // 加速周期 (ティック数)
} PowerNotchInfo_t;

static const PowerNotchInfo_t POWER_NOTCH[5] = {  // ノッチ1-5
    {20, 4, 5},          // ノッチ1: 最大速度25, 基本加速度3, 3ティックに1回加速
    {45, 4, 3},          // ノッチ2: 最大速度45, 基本加速度6, 2ティックに1回加速
    {65, 4, 2},          // ノッチ3: 最大速度60, 基本加速度5, 毎ティック加速
    {75, 5, 1},          // ノッチ4: 最大速度75, 基本加速度7, 毎ティック加速
    {85, 8, 1}          // ノッチ5: 最大速度85, 基本加速度10, 毎ティック加速
};

// ブレーキ段階の定義
typedef struct {
    uint8_t period;      // 減速周期 (ティック数)
    uint8_t decel;       // 減速度 (PWM値/ティック)
} BrakeNotchInfo_t;

static const BrakeNotchInfo_t BRAKE_NOTCH[9] = {  // ブレーキ1-8 + 非常
    {4, 1},               // ブレーキ1: 3ティックに1回減速, 減速度1
    {3, 1},               // ブレーキ2: 2ティックに1回減速, 減速度1
    {2, 1},               // ブレーキ3: 毎ティック減速, 減速度1
    {1, 1},               // ブレーキ4: 毎ティック減速, 減速度3
    {1, 2},               // ブレーキ5: 毎ティック減速, 減速度6
    {1, 5},               // ブレーキ6: 毎ティック減速, 減速度10
    {1, 9},               // ブレーキ7: 毎ティック減速, 減速度15
    {1, 15},              // ブレーキ8: 毎ティック減速, 減速度20
    {1, 30}               // 非常ブレーキ: 毎ティック減速, 減速度50
};

// 環境抵抗の定義
typedef struct {
    uint8_t decel;        // 減速量 (PWM値/ティック)
    uint8_t period;       // 減速周期 (ティック数)
} EnvironmentResistance_t;

static const EnvironmentResistance_t ENV_RESISTANCE[11] = {
    {0, 0},               // レベル0: 抵抗なし
    {1, 30},              // レベル1: 10ティックに1回減速1
    {1, 25},              // レベル2: 9ティックに1回減速1
    {1, 20},              // レベル3: 8ティックに1回減速1
    {1, 16},              // レベル4: 7ティックに1回減速1
    {1, 13},              // レベル5: 6ティックに1回減速1
    {1, 10},              // レベル6: 5ティックに1回減速1
    {1, 8},               // レベル7: 4ティックに1回減速1
    {1, 6},               // レベル8: 3ティックに1回減速1
    {1, 4},               // レベル9: 2ティックに1回減速1
    {1, 2}                // レベル10: 毎ティック減速1
};

static const TickType_t TICK_PERIOD_UPDATE_SPEED = (50 / portTICK_RATE_MS);
static const TickType_t TICK_PERIOD_TO_SEND_QUEUE = (10 / portTICK_RATE_MS);
static const TickType_t TICK_PERIOD_TO_RECV_QUEUE = (10 / portTICK_RATE_MS);

TrainController train_controller;

TimerHandle_t timerUpdateSpeed;
TaskHandle_t taskSpeedControl;
QueueHandle_t queueSpeedControl;
SemaphoreHandle_t semaphoreDecel;

static uint8_t decelSize = 0;
static uint8_t maxSpeed = SPEED_LIMIT;
static HandleState_t handle_state = Center;  // ハンドル状態の追加

static bool is_left = false;
static bool is_evacute = false;

Display display(SPEED_LIMIT);
// M5GFX display;

static void onChangedHandle(HandleState_t handle)
{
  handle_state = handle;  // ハンドル状態の更新
}

static void onChangedHat(HatState_t hat)
{
  if (train_controller.is_running()) {
    return;
  }

  if (hat == UpLeft || hat == Left || hat == DownLeft)
  {
    is_left = true;
    train_controller.setRunBack(true);
  }
  else if (hat == DownRight || hat == Right || hat == UpRight)
  {
    is_left = false;
    train_controller.setRunBack(false);
  }

  if (hat == UpRight || hat == Up || hat == UpLeft)
  {
    is_evacute = true;
    train_controller.setPointState(true);
  }
  else if (hat == DownLeft || hat == Down || hat == DownRight)
  {
    is_evacute = false;
    train_controller.setPointState(false);
  }

  display.drawRail(is_left, is_evacute, true);
}

static void onChangedAdditionalButton(AdditionalButton_t additional_button)
{
  if (IS_BUTTON_DOWN(additional_button, Plus) && decelSize < 10)
  {
    decelSize++;
  }

  if (IS_BUTTON_DOWN(additional_button, Minus) && decelSize > 0)
  {
    decelSize--;
  }

  if (IS_BUTTON_DOWN(additional_button, Home))
  {
    decelSize = 0;
  }

  display.drawDamp(decelSize);
}

static void taskSpeedControlProc(void *param)
{
  uint32_t value;
  static int8_t current_speed = 0;  // 現在の速度
  static uint8_t tick_count = 0;    // ティックカウンタ（環境抵抗用）

  while (true) {
    while (xQueueReceive(queueSpeedControl, &value, TICK_PERIOD_TO_RECV_QUEUE) != pdTRUE);

    // 非常ブレーキの処理
    if (handle_state == EmergencyBrake) {
      BrakeNotchInfo_t current = BRAKE_NOTCH[8];  // 非常ブレーキは配列の最後
      current_speed -= current.decel;  // 毎ティック最大減速度で減速
      if (current_speed < 0) current_speed = 0;
      display.setSpeed(current_speed, true);
      train_controller.setSpeed(current_speed);
      continue;
    }

    // 力行制御
    if (handle_state > Center) {
      int index = 0;
      if (handle_state == Power1) index = 0;
      else if (handle_state == Power2) index = 1;
      else if (handle_state == Power3) index = 2;
      else if (handle_state == Power4) index = 3;
      else if (handle_state == Power5) index = 4;
      else continue;

      PowerNotchInfo_t current = POWER_NOTCH[index];
      if (tick_count % current.period == 0) {  // 周期に応じて加速
        int8_t speed_diff = current.max_speed - current_speed;
        if (speed_diff > 0) {
          int8_t accel = (current.base_accel * speed_diff) / current.max_speed;
          if (accel < 1) accel = 1;
          current_speed += accel;
        } else if (speed_diff < 0) {
          int8_t decel = (current.base_accel * (-speed_diff)) / current_speed;
          if (decel < 1) decel = 1;
          current_speed -= decel;
        }
      }
    }
    // ブレーキ制御
    else if (handle_state < Center) {
      int index = 0;
      if (handle_state == Brake1) index = 0;
      else if (handle_state == Brake2) index = 1;
      else if (handle_state == Brake3) index = 2;
      else if (handle_state == Brake4) index = 3;
      else if (handle_state == Brake5) index = 4;
      else if (handle_state == Brake6) index = 5;
      else if (handle_state == Brake7) index = 6;
      else if (handle_state == Brake8) index = 7;
      else continue;

      BrakeNotchInfo_t current = BRAKE_NOTCH[index];
      if (tick_count % current.period == 0) {
        current_speed -= current.decel;
      }
    }

    // 環境抵抗の処理
    if (decelSize > 0 && handle_state == Center) {
      EnvironmentResistance_t current = ENV_RESISTANCE[decelSize];
      if (tick_count % current.period == 0) {
        current_speed -= current.decel;
      }
    }

    // 速度の下限チェック
    if (current_speed < 0) current_speed = 0;

    // ティックカウンタの更新
    tick_count++;

    display.setSpeed(current_speed, true);
    train_controller.setSpeed(current_speed);
  }
}

static void onTickUpdateSpeed(TimerHandle_t tiemr)
{
  uint32_t v = 0;
  xQueueSend(queueSpeedControl, &v, TICK_PERIOD_TO_SEND_QUEUE);
}

static void initMasconn()
{
  if (usb.Init() == -1)
  {
    Serial.println("OSC did not start.");
  }

  delay(200);

  if (!hid.SetReportParser(0, &masscon))
  {
    ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1);
  }

  masconEvents.setOnChangedHandle(onChangedHandle);
  masconEvents.setOnChangedHat(onChangedHat);
  masconEvents.setOnChangedAdditionalButton(onChangedAdditionalButton);
}

void setup()
{
  // put your setup code here, to run once:
  M5.begin();


  Serial.begin(115200);

  display.begin();
  display.drawRail(is_left, is_evacute, true);
  display.drawDamp(0);

  initMasconn();

  train_controller.begin();

  queueSpeedControl = xQueueCreate(10, 10 / portTICK_RATE_MS);
  timerUpdateSpeed = xTimerCreate("timer decel", TICK_PERIOD_UPDATE_SPEED, pdTRUE, NULL, onTickUpdateSpeed);
  xTaskCreate(taskSpeedControlProc, "speed control task", 4096, NULL, 1, NULL);

  xTimerStart(timerUpdateSpeed, 10 / portTICK_RATE_MS);
}

void loop()
{
  usb.Task();
}
