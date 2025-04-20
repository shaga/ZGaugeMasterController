#include "MasterController.h"

MasterController::MasterController(MasterControllerEvents *evt) : joyEvents_(evt),
                                                                  oldHat_(0xDE),
                                                                  oldButtons_(0)
{
    for (uint8_t i = 0; i < RPT_GEMEPAD_LEN; i++)
        oldPad_[i] = 0xD;
}

void MasterController::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf)
{
    bool match = true;

    // Checking if there are changes in report since the method was last called
    for (uint8_t i = 0; i < RPT_GEMEPAD_LEN; i++)
        if (buf[i] != oldPad_[i])
        {
            match = false;
            break;
        }

    // Calling Game Pad event handler
    if (!match && joyEvents_)
    {
        joyEvents_->OnGamePadChanged((const GamePadEventData *)buf);
        for (uint8_t i = 0; i < RPT_GEMEPAD_LEN; i++)
            oldPad_[i] = buf[i];
    }

    uint8_t hat = (buf[5] & 0xF);

    // Calling Hat Switch event handler
    if (hat != oldHat_ && joyEvents_)
    {
        joyEvents_->OnHatSwitch(hat);
        oldHat_ = hat;
    }

    uint16_t buttons = (0x0000 | buf[6]);
    buttons <<= 4;
    buttons |= (buf[5] >> 4);
    uint16_t changes = (buttons ^ oldButtons_);

    // Calling Button Event Handler for every button changed
    if (changes)
    {
        for (uint8_t i = 0; i < 0x0C; i++)
        {
            uint16_t mask = (0x0001 << i);
            if (((mask & changes) > 0) && joyEvents_)
            {
                if ((buttons & mask) > 0)
                    joyEvents_->OnButtonDn(i + 1);
                else
                    joyEvents_->OnButtonUp(i + 1);
            }
        }
        oldButtons_ = buttons;
    }
}

MasterControllerEvents::MasterControllerEvents() : before_handle_(0),
                                                   before_hat_(0),
                                                   before_button_(0),
                                                   before_additional_button_(0),
                                                   onChangedHandle_(NULL),
                                                   onChangedHat_(NULL),
                                                   onChangedButton_(NULL),
                                                   onChangedAdditionalButton_(NULL)
{
}

void MasterControllerEvents::setOnChangedHandle(HandleEvent_t onChangedHandle)
{
    onChangedHandle_ = onChangedHandle;
}

void MasterControllerEvents::setOnChangedHat(HatEvent_t onChangedHat)
{
    onChangedHat_ = onChangedHat;
}

void MasterControllerEvents::setOnChangedButton(ButtonEvent_t onChangedButton)
{
    onChangedButton_ = onChangedButton;
}

void MasterControllerEvents::setOnChangedAdditionalButton(AdditionalButtonEvent_t onChangedAdditionalButton)
{
    onChangedAdditionalButton_ = onChangedAdditionalButton;
}

void MasterControllerEvents::OnGamePadChanged(const GamePadEventData *evt)
{
    uint8_t handle = evt->Rz;
    uint8_t hat = (evt->Z1 & MASK_HAT);
    uint8_t button = evt->X;
    uint8_t additional_button = evt->Y;

    // Serial.printf("x:%02x / y:%02x / z1:%02x / z2:%02x / Rz:%02x\n", evt->X, evt->Y, evt->Z1, evt->Z2, evt->Rz);

    if (hat != before_hat_) {
        if (onChangedHat_) {
            onChangedHat_((HatState_t)hat);
        }
        before_hat_ = hat;
    }

    if (additional_button != before_additional_button_) {
        if (onChangedAdditionalButton_) {
            onChangedAdditionalButton_((AdditionalButton_t)additional_button);
        }
        before_additional_button_ = additional_button;
    }

    if (button != before_button_) {
        if (onChangedButton_) {
            onChangedButton_((Button_t)button);
        }
        before_button_ = button;
    }

    if (handle != before_handle_) {
        if (onChangedHandle_) {
            onChangedHandle_((HandleState_t)handle);
        }
        before_handle_ = handle;
    }
}

void MasterControllerEvents::OnHatSwitch(uint8_t hat)
{
}

void MasterControllerEvents::OnButtonUp(uint8_t but_id)
{
}

void MasterControllerEvents::OnButtonDn(uint8_t but_id)
{
}