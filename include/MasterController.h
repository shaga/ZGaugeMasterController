#ifndef __MASTERCONTROLLER_H__
#define __MASTERCONTROLLER_H__

#include <usbhid.h>

#define MASK_HAT                            (0x0F)
#define IS_BUTTON_DOWN(state, btn)          ((state & btn) ==  btn)

typedef enum {
    EmergencyBrake = 0x00,
    Brake8 = 0x05,
    Brake7 = 0x13,
    Brake6 = 0x20,
    Brake5 = 0x2E,
    Brake4 = 0x3C,
    Brake3 = 0x49,
    Brake2 = 0x57,
    Brake1 = 0x65,
    Center = 0x80,
    Power1 = 0x9F,
    Power2 = 0xB7,
    Power3 = 0xCE,
    Power4 = 0xE6,
    Power5 = 0xFF,
} HandleState_t;

typedef enum {
    YButton = 0x01,
    BButton = 0x02,
    AButton = 0x04,
    XButton = 0x08,
    LButton = 0x10,
    RButton = 0x20,
    ZLButton = 0x40,
    ZRButton = 0x80,
} Button_t;

typedef enum {
    Minus = 0x01,
    Plus = 0x02,
    Home = 0x10,
    Camera = 0x20,
} AdditionalButton_t;

typedef enum {
    None = 0x0f,
    Up = 0x00,
    UpRight = 0x01,
    Right = 0x02,
    DownRight = 0x03,
    Down = 0x04,
    DownLeft = 0x05,
    Left = 0x06,
    UpLeft = 0x07,
} HatState_t;

typedef void (*HandleEvent_t)(HandleState_t state);
typedef void (*HatEvent_t)(HatState_t state);
typedef void (*ButtonEvent_t)(Button_t button);
typedef void (*AdditionalButtonEvent_t)(AdditionalButton_t button);

struct GamePadEventData {
    uint8_t X, Y, Z1, Z2, Rz;
};

class MasterControllerEvents {
public:
    MasterControllerEvents();
    void setOnChangedHandle(HandleEvent_t onChangedHandle);
    void setOnChangedHat(HatEvent_t onChangedHat);
    void setOnChangedButton(ButtonEvent_t onChangeButton);
    void setOnChangedAdditionalButton(AdditionalButtonEvent_t setOnChangedAdditionalButton);

    virtual void OnGamePadChanged(const GamePadEventData *evt);
    virtual void OnHatSwitch(uint8_t hat);
    virtual void OnButtonUp(uint8_t but_id);
    virtual void OnButtonDn(uint8_t but_id);

private:
    HandleEvent_t onChangedHandle_;
    HatEvent_t onChangedHat_;
    ButtonEvent_t onChangedButton_;
    AdditionalButtonEvent_t onChangedAdditionalButton_;

    uint8_t before_handle_;
    uint8_t before_hat_;
    uint8_t before_button_;
    uint8_t before_additional_button_;
};

#define RPT_GEMEPAD_LEN        5

class MasterController : public HIDReportParser {
    MasterControllerEvents *joyEvents_;

    uint8_t oldPad_[RPT_GEMEPAD_LEN];
    uint8_t oldHat_;
    uint16_t oldButtons_;

public:
    MasterController(MasterControllerEvents *evt);

    virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

#endif // __MASTERCONTROLLER_H__ 