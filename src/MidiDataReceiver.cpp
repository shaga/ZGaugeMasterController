#include <usbh_midi.h>
#include "MidiDataReceiver.h"

#define MIDI_PACKET_SIZE            4
#define IDX_MIDI_CN_CIN             0
#define IDX_MIDI_CMD_CHANNEL        1
#define IDX_NOTE                    2
#define IDX_VELOCITY                3
#define IDX_CC_NUM                  2
#define IDX_CC_VALUE                3
#define NOTE_OCTAVE_LEN             12
#define NOTE_BASE_C                 0
#define NOTE_BASE_D                 2
#define NOTE_BASE_E                 4
#define NOTE_BASE_F                 5
#define NOTE_BASE_G                 7
#define NOTE_BASE_A                 9
#define NOTE_BASE_B                 11

static inline uint8_t getCin(uint8_t cn_cin) {
    return cn_cin & 0x0F;
}

static inline uint8_t getChannel(uint8_t cmd_channel) {
    return cmd_channel & 0x0F;
}

static inline bool isBlackKeyNote(uint8_t note) {
    switch (note % 12) {
        case NOTE_BASE_C:
        case NOTE_BASE_D:
        case NOTE_BASE_E:
        case NOTE_BASE_F:
        case NOTE_BASE_G:
        case NOTE_BASE_A:
        case NOTE_BASE_B:
            return true;
        default:
            return false;
    }
}

static bool is_connected = false;

static void onInit() {
    is_connected = true;
}

static void onRelease() {
    is_connected = false;
}

const uint8_t MidiDataReceiver::kPadChannel = 8;
const uint8_t MidiDataReceiver::kControlChannel = 1;

const uint8_t MidiDataReceiver::kPadActionCin = 0x09;
const uint8_t MidiDataReceiver::kPadNoteEmergencyStop = 0x30;
const uint8_t MidiDataReceiver::kPadNoteSwitchDirection = 0x32;
const uint8_t MidiDataReceiver::kPadNoteSwitchPoint = 0x34;

const uint8_t MidiDataReceiver::kSpeedControlCin = 0x0B;
const uint8_t MidiDataReceiver::kSpeedAccelBrakeOnCin = 0x09;
const uint8_t MidiDataReceiver::kSpeedAccelBrakeOffCin = 0x08;
const uint8_t MidiDataReceiver::kControlNumAccel = 0x14;
const uint8_t MidiDataReceiver::kControlNumBrake = 0x15;
const uint8_t MidiDataReceiver::kControlNumDecel = 0x16;
const uint8_t MidiDataReceiver::kControlNumMaxSpeed = 0x17;


MidiDataReceiver::MidiDataReceiver(): usb_(), midi_(&usb_) {
    onEmergencyStop = NULL;
    onSwitchDirection = NULL;
    onSwitchPoint = NULL;
    onAccel = NULL;
    onBrake = NULL;
    onChangeAccelSize = NULL;
    onChangeBrakeSize = NULL;
    onChangeDecelSize = NULL;
}

int8_t MidiDataReceiver::init() {
    if (usb_.Init() == -1) return -1;

    is_connected = false;
    midi_.attachOnInit(onInit);

    return 0;
}

void MidiDataReceiver::setOnEmergencyStop(NoteOnEvent_t event) {
    onEmergencyStop = event;
}

void MidiDataReceiver::setOnSwitchDirection(NoteOnEvent_t event) {
    onSwitchDirection = event;
}

void MidiDataReceiver::setOnSwitchPoint(NoteOnEvent_t event) {
    onSwitchPoint = event;
}

void MidiDataReceiver::setOnAccel(NoteOnEvent_t event) {
    onAccel = event;
}

void MidiDataReceiver::setOnBrake(NoteOnEvent_t event) {
    onBrake = event;
}

void MidiDataReceiver::setOnChangeAccelSize(ControlChangeEvent_t event) {
    onChangeAccelSize = event;
}

void MidiDataReceiver::setOnChangeBrakeSize(ControlChangeEvent_t event) {
    onChangeBrakeSize = event;
}

void MidiDataReceiver::setOnChangeDecelSize(ControlChangeEvent_t event) {
    onChangeDecelSize = event;
}

void MidiDataReceiver::setOnChangeMaxSpeed(ControlChangeEvent_t event) {
    onChangeMaxSpeed = event;
}

void MidiDataReceiver::loop() {
    usb_.Task();

    if (!is_connected) {
        return;
    }

    uint8_t buffer[MIDI_EVENT_PACKET_SIZE];
    uint16_t recved_size = 0;

    if (midi_.RecvData(&recved_size, buffer) != 0) return;

    for (uint16_t i = 0; i < recved_size; i += MIDI_PACKET_SIZE) {
        uint8_t cin = getCin(buffer[i + IDX_MIDI_CN_CIN]);
        uint8_t channel = getChannel(buffer[i + IDX_MIDI_CMD_CHANNEL]);

        if (channel == kPadChannel && cin == kPadActionCin) {
            // PAD動作
            uint8_t note = buffer[i + IDX_NOTE];

            switch (note) {
                case kPadNoteEmergencyStop:
                    if (onEmergencyStop != NULL) {
                        onEmergencyStop(true);
                    }
                    break;
                case kPadNoteSwitchDirection:
                    if (onSwitchDirection != NULL) {
                        onSwitchDirection(true);
                    }
                    break;
                case kPadNoteSwitchPoint:
                    if (onSwitchPoint != NULL) {
                        onSwitchPoint(true);
                    }
                    break;
            }
        } else if (channel == kControlChannel) {
            // スピード調整

            if (cin == kSpeedControlCin) {
                uint8_t num = buffer[i + IDX_CC_NUM];
                uint8_t value = buffer[i + IDX_CC_VALUE];

                switch (num) {
                    case kControlNumAccel:
                        if (onChangeAccelSize != NULL) {
                            onChangeAccelSize(value);
                        }
                        break;
                    case kControlNumBrake:
                        if (onChangeBrakeSize != NULL) {
                            onChangeBrakeSize(value);
                        }
                        break;
                    case kControlNumDecel:
                        if (onChangeDecelSize != NULL) {
                            onChangeDecelSize(value);
                        }
                        break;
                    case kControlNumMaxSpeed:
                        if (onChangeMaxSpeed != NULL) {
                            onChangeMaxSpeed(value);
                        }
                    default:
                        break;
                }
            } else if (cin == kSpeedAccelBrakeOnCin || cin == kSpeedAccelBrakeOffCin) {
                uint8_t note = buffer[i + IDX_NOTE];
                if (isBlackKeyNote(note)) {
                    if (onAccel != NULL) {
                        onAccel(cin == kSpeedAccelBrakeOnCin);
                    }
                } else {
                    if (onBrake != NULL) {
                        onBrake(cin == kSpeedAccelBrakeOnCin);
                    }
                }
            }
        }
    }
}
