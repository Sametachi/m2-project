#pragma once
#include <bitset>

// Values: https://msdn.microsoft.com/en-us/library/dd375731%28v=vs.85%29.aspx
enum VirtualKeyCode
{
    kVirtualKeyBack = 0x08,
    kVirtualKeyTab = 0x09,

    kVirtualKeyClear = 0x0C,
    kVirtualKeyReturn = 0x0D,

    kVirtualKeyShift = 0x10,
    kVirtualKeyControl = 0x11,
    kVirtualKeyMenu = 0x12,
    kVirtualKeyPause = 0x13,
    kVirtualKeyCapital = 0x14,

    kVirtualKeyEscape = 0x1B,
    kVirtualKeySpace = 0x20,

    kVirtualKeyPrior = 0x21,
    kVirtualKeyNext = 0x22,

    kVirtualKeyEnd = 0x23,
    kVirtualKeyHome = 0x24,

    kVirtualKeyLeft = 0x25,
    kVirtualKeyUp = 0x26,
    kVirtualKeyRight = 0x27,
    kVirtualKeyDown = 0x28,

    kVirtualKeySelect = 0x29,
    kVirtualKeyPrint = 0x2A,
    kVirtualKeyExecute = 0x2B,
    kVirtualKeySnapshot = 0x2C,

    kVirtualKeyInsert = 0x2D,
    kVirtualKeyDelete = 0x2E,

    kVirtualKeyHelp = 0x2F,

    kVirtualKey0 = 0x30,
    kVirtualKey1,
    kVirtualKey2,
    kVirtualKey3,
    kVirtualKey4,
    kVirtualKey5,
    kVirtualKey6,
    kVirtualKey7,
    kVirtualKey8,
    kVirtualKey9,

    kVirtualKeyA = 0x41,
    kVirtualKeyB,
    kVirtualKeyC,
    kVirtualKeyD,
    kVirtualKeyE,
    kVirtualKeyF,
    kVirtualKeyG,
    kVirtualKeyH,
    kVirtualKeyI,
    kVirtualKeyJ,
    kVirtualKeyK,
    kVirtualKeyL,
    kVirtualKeyM,
    kVirtualKeyN,
    kVirtualKeyO,
    kVirtualKeyP,
    kVirtualKeyQ,
    kVirtualKeyR,
    kVirtualKeyS,
    kVirtualKeyT,
    kVirtualKeyU,
    kVirtualKeyV,
    kVirtualKeyW,
    kVirtualKeyX,
    kVirtualKeyY,
    kVirtualKeyZ,

    kVirtualKeyNumpad0 = 0x60,
    kVirtualKeyNumpad1 = 0x61,
    kVirtualKeyNumpad2 = 0x62,
    kVirtualKeyNumpad3 = 0x63,
    kVirtualKeyNumpad4 = 0x64,
    kVirtualKeyNumpad5 = 0x65,
    kVirtualKeyNumpad6 = 0x66,
    kVirtualKeyNumpad7 = 0x67,
    kVirtualKeyNumpad8 = 0x68,
    kVirtualKeyNumpad9 = 0x69,

    kVirtualKeyMultiply = 0x6A,
    kVirtualKeyAdd = 0x6B,
    kVirtualKeySeparator = 0x6C,
    kVirtualKeySubtract = 0x6D,

    kVirtualKeyF1 = 0x70,
    kVirtualKeyF2 = 0x71,
    kVirtualKeyF3 = 0x72,
    kVirtualKeyF4 = 0x73,
    kVirtualKeyF5 = 0x74,
    kVirtualKeyF6 = 0x75,
    kVirtualKeyF7 = 0x76,
    kVirtualKeyF8 = 0x77,
    kVirtualKeyF9 = 0x78,
    kVirtualKeyF10 = 0x79,
    kVirtualKeyF11 = 0x7A,
    kVirtualKeyF12 = 0x7B,

    kVirtualKeyComma = 0xBC,
    kVirtualKeyOEM3 = 0xC0,
    // key below ESC on us keyboards (~`)
    kVirtualKeyOEM5 = 0xDC,
    // key below ESC on german keyboards (^°)
};

typedef uint8_t KeyCode;

class KeyboardInput
{
    public:
        KeyboardInput() = default;
    
        void OnKeyDown(KeyCode code);
        void OnKeyUp(KeyCode code);
        bool IsKeyPressed(KeyCode code) const;
    
    private:
        static const uint32_t kBitCount = 1 << (sizeof(KeyCode) * 8);
        std::bitset<kBitCount> m_keys;
};