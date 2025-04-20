#ifndef TRAIN_CONTROLLER_H_
#define TRAIN_CONTROLLER_H_

#include <Arduino.h>
#include <M5Module4EncoderMotor.h>

class TrainController {
public:
    TrainController();
    void begin();
    bool is_running();
    bool run_back();
    bool is_waiting_lien();
    int8_t current_speed();
    void setSpeed(int8_t speed);
    void accelSpeed(int8_t speed);
    void brakeSpeed(int8_t speed);
    void switchPoint();
    void setPointState(bool is_wating_line);
    void switchDirection();
    void setRunBack(bool run_back);
    
private:
    static const uint8_t IDX_SPEED;
    static const uint8_t IDX_POINT_LEFT;
    static const uint8_t IDX_POINT_RIGHT;

    void outputSwitch();

    M5Module4EncoderMotor driver_;
    bool is_waiting_line_;
    bool run_back_;
    int8_t speed_;
};

#endif //TRAIN_CONTROLLER_H_