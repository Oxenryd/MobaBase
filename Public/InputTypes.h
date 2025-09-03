#ifndef INPUTTYPES_H
#define INPUTTYPES_H

#include <cstdint>
#include <format>
#include <string>

enum class KeyCode : uint16_t
{
    Unknown = 0,

    // Letters
    A, B, C, D, E, F, G,
    H, I, J, K, L, M, N,
    O, P, Q, R, S, T, U,
    V, W, X, Y, Z,

    // Numbers (top row)
    Digit0, Digit1, Digit2, Digit3, Digit4,
    Digit5, Digit6, Digit7, Digit8, Digit9,

    // Numpad
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadMultiply, NumpadAdd, NumpadSubtract, NumpadDecimal, NumpadDivide,

    // Function keys
    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,

    // Modifier keys
    LeftShift, RightShift,
    LeftControl, RightControl,
    LeftAlt, RightAlt,
    LeftSuper, RightSuper,  // Windows key or Command key

    // Other keys
    Escape,
    Tab, CapsLock, Enter, Space,
    Backspace, Insert, Delete,
    Home, End, PageUp, PageDown,
    ArrowUp, ArrowDown, ArrowLeft, ArrowRight,

    PrintScreen, ScrollLock, PauseBreak,

    // Symbols
    Minus, Equals,
    LeftBracket, RightBracket,
    Backslash, Semicolon,
    Apostrophe, Grave,
    Comma, Period, Slash,
};


enum class MouseButtonDownState
{
#ifdef BUILD_WIN
    None = 0,
    Left = 0x0001,
    Right = 0x0002,
    ShiftKey = 0x0004,
    ControlKey = 0x0008,
    Middle = 0x0010,
    XButton1 = 0x0020,
    XButton2 = 0x0040
#endif
};

enum class MButton : uint8_t
{
    None = 0x00,
    Left = 0x01,
    Right = 0x02,
    ShiftKey = 0x04,
    ControlKey = 0x08,
    Middle = 0x10,
    M4 = 0x20,
    M5 = 0x40,
    _ENUM_END = 0x80
};

//template<typename A, typename B>
//inline MButton operator|(A a, B b) {
//    return static_cast<MouseButton>(
//        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
//        );
//}
//template<typename A, typename B>
//inline MButton operator&(A a, B b) {
//    return static_cast<MouseButton>(
//        static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
//        );
//}

struct MouseState
{

    glm::ivec2 relativePosition{};
    glm::ivec2 lastPositionDelta{};
    glm::ivec2 absoluteScreenPosition{};
    glm::i16vec2 wheel{};
    SizedBitField<uint8_t> buttonState{};

    std::string to_String() {
        return std::format("\n absPos: {},{}\trelPos: {},{}\twhDelta: {},{}\tpDelta: {},{}\tbState: {}",
                           absoluteScreenPosition.x,
                           absoluteScreenPosition.y,
                           relativePosition.x,
                           relativePosition.y,
                           wheel.x,
                           wheel.y,
                           lastPositionDelta.x,
                           lastPositionDelta.y,
                           buttonState.getField());
    }

};

enum class KeyAction : uint8_t
{
    None = 0,
    Press = 1,
    Release = 2,
    Hold = 3
};

struct KeyEvent
{
    KeyCode code;
    KeyAction action;
};

INLINE static KeyCode VkeyToKeyCode(USHORT vkey, USHORT makeCode, USHORT flags) {
    //const bool isE0 = (flags & RI_KEY_E0);
    //const bool isE1 = (flags & RI_KEY_E1);  // rarely used

    switch (vkey) {
        case 'A': return KeyCode::A;
        case 'B': return KeyCode::B;
        case 'C': return KeyCode::C;
        case 'D': return KeyCode::D;
        case 'E': return KeyCode::E;
        case 'F': return KeyCode::F;
        case 'G': return KeyCode::G;
        case 'H': return KeyCode::H;
        case 'I': return KeyCode::I;
        case 'J': return KeyCode::J;
        case 'K': return KeyCode::K;
        case 'L': return KeyCode::L;
        case 'M': return KeyCode::M;
        case 'N': return KeyCode::N;
        case 'O': return KeyCode::O;
        case 'P': return KeyCode::P;
        case 'Q': return KeyCode::Q;
        case 'R': return KeyCode::R;
        case 'S': return KeyCode::S;
        case 'T': return KeyCode::T;
        case 'U': return KeyCode::U;
        case 'V': return KeyCode::V;
        case 'W': return KeyCode::W;
        case 'X': return KeyCode::X;
        case 'Y': return KeyCode::Y;
        case 'Z': return KeyCode::Z;

        case '0': return KeyCode::Digit0;
        case '1': return KeyCode::Digit1;
        case '2': return KeyCode::Digit2;
        case '3': return KeyCode::Digit3;
        case '4': return KeyCode::Digit4;
        case '5': return KeyCode::Digit5;
        case '6': return KeyCode::Digit6;
        case '7': return KeyCode::Digit7;
        case '8': return KeyCode::Digit8;
        case '9': return KeyCode::Digit9;

        case VK_ESCAPE: return KeyCode::Escape;
        case VK_TAB: return KeyCode::Tab;
        case VK_CAPITAL: return KeyCode::CapsLock;
        case VK_RETURN: return KeyCode::Enter;
        case VK_SPACE: return KeyCode::Space;
        case VK_BACK: return KeyCode::Backspace;
        case VK_INSERT: return KeyCode::Insert;
        case VK_DELETE: return KeyCode::Delete;
        case VK_HOME: return KeyCode::Home;
        case VK_END: return KeyCode::End;
        case VK_PRIOR: return KeyCode::PageUp;
        case VK_NEXT: return KeyCode::PageDown;
        case VK_LEFT: return KeyCode::ArrowLeft;
        case VK_RIGHT: return KeyCode::ArrowRight;
        case VK_UP: return KeyCode::ArrowUp;
        case VK_DOWN: return KeyCode::ArrowDown;
        case VK_SNAPSHOT: return KeyCode::PrintScreen;
        case VK_SCROLL: return KeyCode::ScrollLock;
        case VK_PAUSE: return KeyCode::PauseBreak;

        case VK_OEM_MINUS: return KeyCode::Minus;
        case VK_OEM_PLUS: return KeyCode::Equals;
        case VK_OEM_4: return KeyCode::LeftBracket;
        case VK_OEM_6: return KeyCode::RightBracket;
        case VK_OEM_5: return KeyCode::Backslash;
        case VK_OEM_1: return KeyCode::Semicolon;
        case VK_OEM_7: return KeyCode::Apostrophe;
        case VK_OEM_3: return KeyCode::Grave;
        case VK_OEM_COMMA: return KeyCode::Comma;
        case VK_OEM_PERIOD: return KeyCode::Period;
        case VK_OEM_2: return KeyCode::Slash;

        case VK_LSHIFT: return KeyCode::LeftShift;
        case VK_RSHIFT: return KeyCode::RightShift;
        case VK_LCONTROL: return KeyCode::LeftControl;
        case VK_RCONTROL: return KeyCode::RightControl;
        case VK_LMENU: return KeyCode::LeftAlt;
        case VK_RMENU: return KeyCode::RightAlt;
        case VK_LWIN: return KeyCode::LeftSuper;
        case VK_RWIN: return KeyCode::RightSuper;

        case VK_NUMPAD0: return KeyCode::Numpad0;
        case VK_NUMPAD1: return KeyCode::Numpad1;
        case VK_NUMPAD2: return KeyCode::Numpad2;
        case VK_NUMPAD3: return KeyCode::Numpad3;
        case VK_NUMPAD4: return KeyCode::Numpad4;
        case VK_NUMPAD5: return KeyCode::Numpad5;
        case VK_NUMPAD6: return KeyCode::Numpad6;
        case VK_NUMPAD7: return KeyCode::Numpad7;
        case VK_NUMPAD8: return KeyCode::Numpad8;
        case VK_NUMPAD9: return KeyCode::Numpad9;
        case VK_MULTIPLY: return KeyCode::NumpadMultiply;
        case VK_ADD: return KeyCode::NumpadAdd;
        case VK_SUBTRACT: return KeyCode::NumpadSubtract;
        case VK_DECIMAL: return KeyCode::NumpadDecimal;
        case VK_DIVIDE: return KeyCode::NumpadDivide;

        case VK_F1: return KeyCode::F1;
        case VK_F2: return KeyCode::F2;
        case VK_F3: return KeyCode::F3;
        case VK_F4: return KeyCode::F4;
        case VK_F5: return KeyCode::F5;
        case VK_F6: return KeyCode::F6;
        case VK_F7: return KeyCode::F7;
        case VK_F8: return KeyCode::F8;
        case VK_F9: return KeyCode::F9;
        case VK_F10: return KeyCode::F10;
        case VK_F11: return KeyCode::F11;
        case VK_F12: return KeyCode::F12;
    }

    return KeyCode::Unknown;
}

INLINE static KeyEvent MapRawKeyboardEvent(const RAWKEYBOARD& keyboard) {
    const USHORT vkey = keyboard.VKey;
    const USHORT makeCode = keyboard.MakeCode;
    const USHORT flags = keyboard.Flags;

    // Handle generic remapping
    USHORT mappedVKey = vkey;
    bool isE0 = (flags & RI_KEY_E0);
    bool isBreak = (flags & RI_KEY_BREAK);

    if (vkey == VK_SHIFT) {
        mappedVKey = static_cast<USHORT>(MapVirtualKey(makeCode, MAPVK_VSC_TO_VK_EX));
    } else if (vkey == VK_CONTROL) {
        mappedVKey = isE0 ? VK_RCONTROL : VK_LCONTROL;
    } else if (vkey == VK_MENU) {
        mappedVKey = isE0 ? VK_RMENU : VK_LMENU;
    }

    KeyCode code = VkeyToKeyCode(mappedVKey, makeCode, flags);

    KeyAction action = isBreak ? KeyAction::Release : KeyAction::Press;

    // Use GetAsyncKeyState to check if modifier keys are currently down
    //bool shiftDown = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) ||
    //    (GetAsyncKeyState(VK_RSHIFT) & 0x8000);
    //bool ctrlDown = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) ||
    //    (GetAsyncKeyState(VK_RCONTROL) & 0x8000);
    //bool altDown = (GetAsyncKeyState(VK_LMENU) & 0x8000) ||
    //    (GetAsyncKeyState(VK_RMENU) & 0x8000);

    return KeyEvent{
        .code = code,
        .action = action
    };
}

#endif