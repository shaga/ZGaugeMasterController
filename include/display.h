#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <M5Unified.h>
#include <M5GFX.h>

class Display {
public:
    Display(int8_t max_speed = 127);
    void begin();

    void setSpeed(int8_t speed, bool is_push = false);
    void drawRail(bool is_left, bool is_evacute, bool is_push = false);

private:
    static const float SPEED_START_DEG;
    static const float SPEED_RANGE_DEG;
    static const float SPEED_END_DEG;
    static const int8_t SPEED_MIN;
    static const int8_t SPEED_MAX;
    int8_t max_speed_;
    M5GFX display_;
    M5Canvas canvas_speed_;
    M5Canvas canvas_rail_;
};

#endif //DISPLAY_H_
