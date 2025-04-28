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

typedef struct {
  uint8_t accel;
  uint8_t max;
  uint8_t tick;
  uint8_t padding[1];
} SpeedInfo_t;

static const SpeedInfo_t SPEED_INFO[6] = {
  {0, SPEED_LIMIT, 255},
  {1, 30, 3},
  {2, 45, 2} ,
  {3, 60, 1},
  {4, 75, 1},
  {6, SPEED_LIMIT, 1},
};

static const TickType_t TICK_PERIOD_DECEL = (100 / portTICK_RATE_MS);
static const TickType_t TICK_PERIOD_TO_SEND_QUEUE = (10 / portTICK_RATE_MS);
static const TickType_t TICK_PERIOD_TO_RECV_QUEUE = (10 / portTICK_RATE_MS);

TrainController train_controller;

TimerHandle_t timerUpdateSpeed;
TaskHandle_t taskSpeedControl;
QueueHandle_t queueSpeedControl;
SemaphoreHandle_t semaphoreDecel;

static uint8_t accel = 0;
static uint8_t brakeSize = 0;
static uint8_t decelSize = 0;
static uint8_t maxSpeed = SPEED_LIMIT;

static bool is_left = false;
static bool is_evacute = false;

Display display(SPEED_LIMIT);
// M5GFX display;

static void onChangedHandle(HandleState_t handle)
{
  switch (handle)
  {
  case EmergencyBrake:
    display.setSpeed(0);
    train_controller.setSpeed(0);
    break;
  case Center:
    accel = 0;
    brakeSize = 0;
    break;
  case Power1:
    accel = 1;
    break;
  case Power2:
    accel = 2;
    break;
  case Power3:
    accel = 3;
    break;
  case Power4:
    accel = 4;
    break;
  case Power5:
    accel = 5;
    break;
  case Brake1:
    brakeSize = 1;
    break;
  case Brake2:
    brakeSize = 2;
    break;
  case Brake3:
    brakeSize = 3;
    break;
  case Brake4:
    brakeSize = 4;
    break;
  case Brake5:
    brakeSize = 6;
    break;
  case Brake6:
    brakeSize = 8;
    break;
  case Brake7:
    brakeSize = 10;
    break;
  case Brake8:
    brakeSize = 15;
    break;
  }

  if (handle < Center)
  {
    accel = 0;
  }
  else if (handle > Center)
  {
    brakeSize = 0;
  }
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
  if (IS_BUTTON_DOWN(additional_button, Plus) && decelSize < 20)
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
  static int decelCount = 0;
  static int accelCount = 0;
  Serial.println("start task....");
  while (true)
  {
    while (xQueueReceive(queueSpeedControl, &value, TICK_PERIOD_TO_RECV_QUEUE) != pdTRUE);

    decelCount++;
    accelCount++;

    int8_t base_speed = train_controller.current_speed() > SPEED_LIMIT ? SPEED_LIMIT : train_controller.current_speed();

    if (accel > 0) {
      SpeedInfo_t current = SPEED_INFO[accel];
      if (accelCount >= current.tick) {
        if (base_speed < current.max) {
          if (base_speed + current.accel >= current.max) {
            base_speed = current.max;
          } else {
            base_speed += current.accel;
          }
          
        } else if (base_speed > current.max) {
          if (base_speed - current.accel <= current.max) {
            base_speed = current.max;
          } else {
            base_speed -= current.accel;
          }
        }
        accelCount = 0;
      }
    } else {
      accelCount = 0;
    }

    base_speed -= brakeSize;

    if (decelSize > 0)
    {

      if (decelCount >= (21 - decelSize))
      {
        decelCount = 0;
        base_speed -= 1;
      }
    }

    display.setSpeed(base_speed, true);
    train_controller.setSpeed(base_speed);
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
  timerUpdateSpeed = xTimerCreate("timer decel", TICK_PERIOD_DECEL, pdTRUE, NULL, onTickUpdateSpeed);
  xTaskCreate(taskSpeedControlProc, "speed control task", 4096, NULL, 1, NULL);

  xTimerStart(timerUpdateSpeed, 10 / portTICK_RATE_MS);
}

void loop()
{
  usb.Task();
}
