#ifndef INPUTTYPES_H
#define INPUTTYPES_H

#include <cstdint>

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



#endif