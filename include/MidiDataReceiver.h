#ifndef MIDI_MANAGER_H_
#define MIDI_MANAGER_H_

#include <Arduino.h>
#include <usbh_midi.h>

typedef void (*NoteOnEvent_t)(bool isOn);
typedef void (*ControlChangeEvent_t)(uint8_t value);

class MidiDataReceiver {
public:
    MidiDataReceiver();
    int8_t init();
    void loop();

    void setOnEmergencyStop(NoteOnEvent_t event);
    void setOnSwitchDirection(NoteOnEvent_t event);
    void setOnSwitchPoint(NoteOnEvent_t event);
    void setOnAccel(NoteOnEvent_t event);
    void setOnBrake(NoteOnEvent_t event);

    void setOnChangeAccelSize(ControlChangeEvent_t event);
    void setOnChangeBrakeSize(ControlChangeEvent_t event);
    void setOnChangeDecelSize(ControlChangeEvent_t event);
    void setOnChangeMaxSpeed(ControlChangeEvent_t event);

private:
    static const uint8_t kPadChannel;
    static const uint8_t kControlChannel;

    static const uint8_t kPadActionCin;
    static const uint8_t kPadNoteEmergencyStop;
    static const uint8_t kPadNoteSwitchDirection;
    static const uint8_t kPadNoteSwitchPoint;

    static const uint8_t kSpeedControlCin;
    static const uint8_t kSpeedAccelBrakeOnCin;
    static const uint8_t kSpeedAccelBrakeOffCin;
    static const uint8_t kControlNumAccel;
    static const uint8_t kControlNumBrake;
    static const uint8_t kControlNumDecel;
    static const uint8_t kControlNumMaxSpeed;
    

    USB usb_;
    USBH_MIDI midi_;

    NoteOnEvent_t onEmergencyStop;
    NoteOnEvent_t onSwitchDirection;
    NoteOnEvent_t onSwitchPoint;

    NoteOnEvent_t onAccel;
    NoteOnEvent_t onBrake;
    ControlChangeEvent_t onChangeAccelSize;
    ControlChangeEvent_t onChangeBrakeSize;
    ControlChangeEvent_t onChangeDecelSize;
    ControlChangeEvent_t onChangeMaxSpeed;
};

#endif //MIDI_MANAGER_H_