#include "TrainController.h"

const uint8_t TrainController::IDX_SPEED = 0;
const uint8_t TrainController::IDX_POINT_LEFT = 2;
const uint8_t TrainController::IDX_POINT_RIGHT = 3;

TrainController::TrainController() {
    is_waiting_line_ = false;
    run_back_ = 0;
    speed_ = 0;
}

void TrainController::begin() {
    while (!driver_.begin(&Wire, MODULE_4ENCODERMOTOR_ADDR, 21, 22)) {
        Serial.println("failed to begin motor driver");
        delay(100);
    }

    uint8_t version;
    bool ret = driver_.getFirmwareVersion(&version);
    Serial.printf("ret: %d / version: %d / addr: %02X\n", ret, version, driver_.getI2CAddress());



    driver_.setMode(IDX_SPEED, NORMAL_MODE);
    driver_.setMotorSpeed(IDX_SPEED, 0);

    driver_.setMode(IDX_POINT_LEFT, NORMAL_MODE);
    driver_.setMode(IDX_POINT_RIGHT, NORMAL_MODE);
    outputSwitch();
}

bool TrainController::is_running() {
    return speed_ > 0;
}

bool TrainController::run_back() {
    return run_back_;
}

void TrainController::setRunBack(bool run_back) {
    if (is_running()) return;

    run_back_ = run_back;
}

void TrainController::switchDirection() {
    if (is_running()) return;
    run_back_ = !run_back_;
}

int8_t TrainController::current_speed() {
    return speed_;
}

void TrainController::setSpeed(int8_t speed) {
    if (speed < 0) speed = 0;
    if (speed == speed_) return;
    speed_ = speed;

    int8_t value = speed_ * (run_back_ ? -1 : 1);
    driver_.setMotorSpeed(IDX_SPEED, value);
}

void TrainController::accelSpeed(int8_t speed) {
    int next = speed_ + speed;
    if (next > 127) next = 127;
    else if (next < 0) next = 0;

    setSpeed(next);
}

void TrainController::brakeSpeed(int8_t speed) {
    int next = speed_ - speed;
    if (next > 127) next = 127;
    else if (next < 0) next = 0;

    setSpeed(next);
}

bool TrainController::is_waiting_lien() {
    return !is_waiting_line_;
}

void TrainController::setPointState(bool is_wating_line) {
    if (is_running()) return;
    is_waiting_line_ = is_wating_line;
    outputSwitch();
}

void TrainController::switchPoint() {
    is_waiting_line_ = !is_waiting_line_;
    outputSwitch();
}

void TrainController::outputSwitch() {
    int8_t pwm = is_waiting_line_ ? -127 : 127;
    driver_.setMotorSpeed(IDX_POINT_LEFT, pwm);
    driver_.setMotorSpeed(IDX_POINT_RIGHT, pwm);
    delay(50);
    driver_.setMotorSpeed(IDX_POINT_LEFT, 0);
    driver_.setMotorSpeed(IDX_POINT_RIGHT, 0);
}