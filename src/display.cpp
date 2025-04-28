#include "display.h"

const int8_t Display::SPEED_MIN = 0;
const int8_t Display::SPEED_MAX = 127;
const float Display::SPEED_START_DEG = 150;
const float Display::SPEED_RANGE_DEG = 240;
const float Display::SPEED_END_DEG = SPEED_START_DEG + SPEED_RANGE_DEG;

Display::Display(int8_t max_speed): 
    max_speed_(max_speed),
    before_speed_(0) {

}

void Display::begin() {
    display_.begin();
    display_.setRotation(0);
    display_.setBaseColor(BLACK);
    display_.clear();

    canvas_rail_.setColorDepth(8);
    canvas_rail_.setBaseColor(BLACK);
    canvas_rail_.createSprite(display_.width(), 140);
    canvas_rail_.setTextColor(WHITE);
    canvas_rail_.setTextDatum(middle_center);
    canvas_rail_.setFont(&fonts::lgfxJapanGothic_40);

    canvas_damp_.setColorDepth(8);
    canvas_damp_.setBaseColor(BLACK);
    canvas_damp_.createSprite(100, 90);
    canvas_damp_.setFont(&fonts::lgfxJapanGothic_40);
    canvas_damp_.setTextColor(WHITE);
    canvas_damp_.setTextDatum(middle_center);

    display_.fillArc(display_.width() / 2, display_.width() / 2, display_.width() / 2, display_.width() / 2 - 10, SPEED_START_DEG, SPEED_END_DEG, DARKGREY);
}

void Display::setSpeed(int8_t speed, bool is_push) {
    if (speed < SPEED_MIN) speed = 0;
    else if (speed > max_speed_) speed = max_speed_;
    float deg = SPEED_START_DEG + SPEED_RANGE_DEG * speed / max_speed_;
    // canvas_speed_.clear();
    if (speed > before_speed_)
        display_.fillArc(display_.width() / 2, display_.width() / 2, display_.width() / 2, display_.width() / 2 - 10, SPEED_START_DEG, deg, GREEN);
    if (speed < before_speed_)
        display_.fillArc(display_.width() / 2, display_.width() / 2, display_.width() / 2, display_.width() / 2 - 10, deg, SPEED_END_DEG, DARKGREY);
    before_speed_ = speed;
}

void Display::drawRail(bool is_left, bool is_evacute, bool is_push) {
    canvas_rail_.clear();

    const int TRIANGLE_HEIGHT = 13;
    const int BASE_X1 = display_.width() / 2 - 4;
    const int BASE_X2 = display_.width() / 2 + 4;

    canvas_rail_.drawRoundRect(10, 30, 220, 90, 45, WHITE);
    if (is_evacute) {
        canvas_rail_.drawLine(60, 30, 180, 30, BLACK);
        canvas_rail_.drawLine(60, 30, 90, 10, WHITE);
        canvas_rail_.drawLine(180, 30, 150, 10, WHITE);
        canvas_rail_.drawLine(90, 10, 150, 10, WHITE);
    }

    if (is_left) {
        canvas_rail_.drawString("左周り", canvas_rail_.width() / 2, 75);
    } else {
        canvas_rail_.drawString("右周り", canvas_rail_.width() / 2, 75);
    }

    if (is_push) {
        canvas_rail_.pushSprite(&display_, 0, display_.height() - canvas_rail_.height());
    }
}

void Display::drawDamp(uint8_t damp) {
    canvas_damp_.clear();
    char buff[3];
    
    sprintf(buff, "%d", damp);
    canvas_damp_.drawString("抵抗", canvas_damp_.width() / 2, canvas_damp_.height() / 2 - 20);
    canvas_damp_.drawString(buff, canvas_damp_.width() / 2, canvas_damp_.height() / 2 + 20);
    canvas_damp_.pushSprite(&display_, 70, 70);
}
