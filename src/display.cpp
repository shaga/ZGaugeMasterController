#include "display.h"

const int8_t Display::SPEED_MIN = 0;
const int8_t Display::SPEED_MAX = 127;
const float Display::SPEED_START_DEG = 150;
const float Display::SPEED_RANGE_DEG = 240;
const float Display::SPEED_END_DEG = SPEED_START_DEG + SPEED_RANGE_DEG;

Display::Display(int8_t max_speed): 
    canvas_speed_(&display_),
    canvas_rail_(&display_),
    max_speed_(max_speed) {

}

void Display::begin() {
    display_.begin();
    display_.setRotation(2);
    display_.setBaseColor(BLACK);
    display_.clear();

    canvas_speed_.createSprite(display_.width(), 180);
    // canvas_speed_.setBaseColor(TFT_TRANSPARENT);

    canvas_rail_.createSprite(display_.width(), 140);
    canvas_rail_.setBaseColor(BLACK);
}

void Display::setSpeed(int8_t speed, bool is_push) {
    if (speed < SPEED_MIN) speed = 0;
    else if (speed > max_speed_) speed = max_speed_;
    float deg = SPEED_START_DEG + SPEED_RANGE_DEG * speed / SPEED_MAX;
    canvas_speed_.clear();
    if (speed > 0)
        canvas_speed_.fillArc(display_.width() / 2, display_.width() / 2, display_.width() / 2, display_.width() / 2 - 10, SPEED_START_DEG, deg, GREEN);
    if (speed < SPEED_MAX)
        canvas_speed_.fillArc(display_.width() / 2, display_.width() / 2, display_.width() / 2, display_.width() / 2 - 10, deg, SPEED_END_DEG, BLACK);

    if (is_push) {
        canvas_speed_.pushSprite(0, 0);
    }
}

void Display::drawRail(bool is_left, bool is_evacute, bool is_push) {
    canvas_rail_.clear();

    canvas_rail_.drawRoundRect(10, 10, 220, 90, 45, WHITE);
    if (is_evacute) {
        canvas_rail_.drawLine(60, 99, 180, 99, BLACK);
        canvas_rail_.drawLine(60, 99, 90, 119, WHITE);
        canvas_rail_.drawLine(180, 99, 150, 119, WHITE);
        canvas_rail_.drawLine(90, 119, 150, 119, WHITE);
    }

    if (is_push) {
        canvas_rail_.pushSprite(0, display_.height() - canvas_rail_.height());
    }
}
