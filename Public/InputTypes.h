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

enum class KeyMod : uint8_t {
    None = 0,
    Shift = 1,
    Control = 2,
    Alt = 4,
    Super = 8,
    CapsLock = 16,
    NumLock = 32
};

struct alignas(4) KeyCombo
{
    union {
        struct {
            KeyCode key;
            KeyMod mods;
        };
        uint32_t raw;
    };
    KeyCombo() = default;
    KeyCombo(const KeyCombo& other) :
        raw{other.raw} {}
    KeyCombo(const KeyCode key, const KeyMod mods) :
        key{key}, mods{mods} {};
    KeyCombo(const int key, const int mods) :
        key{static_cast<KeyCode>(key)}, mods{static_cast<KeyMod>(mods)} {}
    KeyCombo& operator=(const KeyCombo& rhs) {
        key = rhs.key;
        mods = rhs.mods;
        return *this;
    }
    KeyCombo(KeyCombo&& other) noexcept {
        key = other.key;
        mods = other.mods;
        other.key = KeyCode{};
        other.mods = KeyMod{};
    }
    KeyCombo& operator=(KeyCombo&& other) noexcept {
        key = other.key;
        mods = other.mods;
        other.key = KeyCode{};
        other.mods = KeyMod{};
        return *this;
    }
    friend bool operator==(const KeyCombo& lhs, const KeyCombo& rhs) {
        return lhs.raw == rhs.raw;
    }
};


struct KeyComboHash {
    size_t operator()(KeyCombo const& value) const noexcept {
        uint64_t x = static_cast<uint64_t>(value.raw);
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        return x;
    }
};

typedef uint16_t ScanCode;

enum class GLFW_MouseButton : uint8_t
{
    Button1 = GLFW_MOUSE_BUTTON_1,
    Left = GLFW_MOUSE_BUTTON_LEFT,
    Button2 = GLFW_MOUSE_BUTTON_2,
    Right = GLFW_MOUSE_BUTTON_RIGHT,
    Button3 = GLFW_MOUSE_BUTTON_3,
    Middle = GLFW_MOUSE_BUTTON_MIDDLE,
    Button4 = GLFW_MOUSE_BUTTON_4,
    Button5 = GLFW_MOUSE_BUTTON_5,
    Button6 = GLFW_MOUSE_BUTTON_6,
    Button7 = GLFW_MOUSE_BUTTON_7,
    Button8 = GLFW_MOUSE_BUTTON_8,
    ENUM_END
};

enum class MButton : uint8_t
{
    None = 0,

    Button1 =   1 << 0,
    Left =      1 << 0,

    Button2 =   1 << 1,
    Right =     1 << 1,

    Button3 =   1 << 2,
    Middle =    1 << 2,

    Button4 =   1 << 3,
    Button5 =   1 << 4,
    Button6 =   1 << 5,
    Button7 =   1 << 6,
    Button8 =   1 << 7
};


INLINE static MButton operator|(MButton a, MButton b) {
    return static_cast<MButton>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
        );
}

INLINE static KeyMod operator|(KeyMod  a, KeyMod  b) {
    return static_cast<KeyMod >(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
        );
}

// template<typename A, typename B>
//     requires
//         std::is_convertible_v<A, uint8_t> &&
//         std::is_convertible_v<B, uint8_t>
// INLINE static MButton operator|(A a, B b) {
//     return static_cast<MButton>(
//         static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
//         );
// }
// template<typename A, typename B>
//     requires
//         std::is_convertible_v<A, uint8_t> &&
//         std::is_convertible_v<B, uint8_t>
// INLINE static MButton operator&(A a, B b) {
//     return static_cast<MButton>(
//         static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
//         );
// }
//
// template<typename A, typename B>
//     requires
//         std::is_convertible_v<A, uint8_t> &&
//         std::is_convertible_v<B, uint8_t>
// INLINE static KeyMod operator|(A a, B b) {
//     return static_cast<KeyMod>(
//         static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
//         );
// }
// template<typename A, typename B>
//     requires
//         std::is_convertible_v<A, uint8_t> &&
//         std::is_convertible_v<B, uint8_t>
// INLINE static KeyMod operator&(A a, B b) {
//     return static_cast<KeyMod>(
//         static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
//         );
// }

struct MouseButtonState {
    uint8_t raw;

    template <typename I>
    constexpr bool isButtonDown(const I button) const {
        if constexpr (std::is_same_v<I, MButton>)
            return raw & button;
        if constexpr (std::is_same_v<I, GLFW_MouseButton>)
            return raw = 1 << button;
        else
            static_assert(false);
        return false;
    }
    constexpr std::string getBitString() const {
        std::string str{};
        for (size_t i = 8; i >= 1; --i) {
            str.append(std::string{
                static_cast<char>(
                    static_cast<uint8_t>(raw & 1 << (i-1)) + 48
                    )
            });
        }
        return str;
    }
};



template<typename I>
    requires std::is_integral_v<I>
constexpr MButton glfwToMButton(const I button) {
    switch (button) {
        default: return MButton::None;

        case GLFW_MOUSE_BUTTON_LEFT:
            return MButton::Button1;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return MButton::Button2;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return MButton::Button3;
        case GLFW_MOUSE_BUTTON_4:
            return MButton::Button4;
        case GLFW_MOUSE_BUTTON_5:
            return MButton::Button5;
        case GLFW_MOUSE_BUTTON_6:
            return MButton::Button6;
        case GLFW_MOUSE_BUTTON_7:
            return MButton::Button7;
        case GLFW_MOUSE_BUTTON_8:
            return MButton::Button8;
    }
}

struct MouseState
{
    glm::ivec2 relativePosition{};
    glm::ivec2 lastPositionDelta{};
    glm::ivec2 absoluteScreenPosition{};
    glm::i16vec2 wheel{};
    MouseButtonState buttonState{};

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
                           buttonState.getBitString()
                           );
    }
};

enum class ActionState : uint8_t
{
    Stopped = 0,
    Started = 1,
    KeyRepeat = 2,
    Performed = 3
};

INLINE static constexpr KeyCode glfwToKeyCode(const short key) {
    switch (key) {
        default: return KeyCode::Unknown;

        case GLFW_KEY_A: return KeyCode::A;
        case GLFW_KEY_B: return KeyCode::B;
        case GLFW_KEY_C: return KeyCode::C;
        case GLFW_KEY_D: return KeyCode::D;
        case GLFW_KEY_E: return KeyCode::E;
        case GLFW_KEY_F: return KeyCode::F;
        case GLFW_KEY_G: return KeyCode::G;
        case GLFW_KEY_H: return KeyCode::H;
        case GLFW_KEY_I: return KeyCode::I;
        case GLFW_KEY_J: return KeyCode::J;
        case GLFW_KEY_K: return KeyCode::K;
        case GLFW_KEY_L: return KeyCode::L;
        case GLFW_KEY_M: return KeyCode::M;
        case GLFW_KEY_N: return KeyCode::N;
        case GLFW_KEY_O: return KeyCode::O;
        case GLFW_KEY_P: return KeyCode::P;
        case GLFW_KEY_Q: return KeyCode::Q;
        case GLFW_KEY_R: return KeyCode::R;
        case GLFW_KEY_S: return KeyCode::S;
        case GLFW_KEY_T: return KeyCode::T;
        case GLFW_KEY_U: return KeyCode::U;
        case GLFW_KEY_V: return KeyCode::V;
        case GLFW_KEY_W: return KeyCode::W;
        case GLFW_KEY_X: return KeyCode::X;
        case GLFW_KEY_Y: return KeyCode::Y;
        case GLFW_KEY_Z: return KeyCode::Z;

        case GLFW_KEY_0: return KeyCode::Digit0;
        case GLFW_KEY_1: return KeyCode::Digit1;
        case GLFW_KEY_2: return KeyCode::Digit2;
        case GLFW_KEY_3: return KeyCode::Digit3;
        case GLFW_KEY_4: return KeyCode::Digit4;
        case GLFW_KEY_5: return KeyCode::Digit5;
        case GLFW_KEY_6: return KeyCode::Digit6;
        case GLFW_KEY_7: return KeyCode::Digit7;
        case GLFW_KEY_8: return KeyCode::Digit8;
        case GLFW_KEY_9: return KeyCode::Digit9;

        case GLFW_KEY_ESCAPE: return KeyCode::Escape;
        case GLFW_KEY_TAB: return KeyCode::Tab;
        case GLFW_KEY_CAPS_LOCK: return KeyCode::CapsLock;
        case GLFW_KEY_ENTER: return KeyCode::Enter;
        case GLFW_KEY_SPACE: return KeyCode::Space;
        case GLFW_KEY_BACKSPACE: return KeyCode::Backspace;
        case GLFW_KEY_INSERT: return KeyCode::Insert;
        case GLFW_KEY_DELETE: return KeyCode::Delete;
        case GLFW_KEY_HOME: return KeyCode::Home;
        case GLFW_KEY_END: return KeyCode::End;
        case GLFW_KEY_PAGE_UP: return KeyCode::PageUp;
        case GLFW_KEY_PAGE_DOWN: return KeyCode::PageDown;
        case GLFW_KEY_LEFT: return KeyCode::ArrowLeft;
        case GLFW_KEY_RIGHT: return KeyCode::ArrowRight;
        case GLFW_KEY_UP: return KeyCode::ArrowUp;
        case GLFW_KEY_DOWN: return KeyCode::ArrowDown;
        case GLFW_KEY_PRINT_SCREEN: return KeyCode::PrintScreen;
        case GLFW_KEY_SCROLL_LOCK: return KeyCode::ScrollLock;
        case GLFW_KEY_PAUSE: return KeyCode::PauseBreak;

        case GLFW_KEY_MINUS: return KeyCode::Minus;
        case GLFW_KEY_EQUAL: return KeyCode::Equals;
        case GLFW_KEY_LEFT_BRACKET: return KeyCode::LeftBracket;
        case GLFW_KEY_RIGHT_BRACKET: return KeyCode::RightBracket;
        case GLFW_KEY_BACKSLASH: return KeyCode::Backslash;
        case GLFW_KEY_SEMICOLON: return KeyCode::Semicolon;
        case GLFW_KEY_APOSTROPHE: return KeyCode::Apostrophe;
        case GLFW_KEY_GRAVE_ACCENT: return KeyCode::Grave;
        case GLFW_KEY_COMMA: return KeyCode::Comma;
        case GLFW_KEY_PERIOD: return KeyCode::Period;
        case GLFW_KEY_SLASH: return KeyCode::Slash;

        case GLFW_KEY_LEFT_SHIFT: return KeyCode::LeftShift;
        case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RightShift;
        case GLFW_KEY_LEFT_CONTROL: return KeyCode::LeftControl;
        case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RightControl;
        case GLFW_KEY_LEFT_ALT: return KeyCode::LeftAlt;
        case GLFW_KEY_RIGHT_ALT: return KeyCode::RightAlt;
        case GLFW_KEY_LEFT_SUPER: return KeyCode::LeftSuper;
        case GLFW_KEY_RIGHT_SUPER: return KeyCode::RightSuper;

        case GLFW_KEY_KP_0: return KeyCode::Numpad0;
        case GLFW_KEY_KP_1: return KeyCode::Numpad1;
        case GLFW_KEY_KP_2: return KeyCode::Numpad2;
        case GLFW_KEY_KP_3: return KeyCode::Numpad3;
        case GLFW_KEY_KP_4: return KeyCode::Numpad4;
        case GLFW_KEY_KP_5: return KeyCode::Numpad5;
        case GLFW_KEY_KP_6: return KeyCode::Numpad6;
        case GLFW_KEY_KP_7: return KeyCode::Numpad7;
        case GLFW_KEY_KP_8: return KeyCode::Numpad8;
        case GLFW_KEY_KP_9: return KeyCode::Numpad9;
        case GLFW_KEY_KP_MULTIPLY: return KeyCode::NumpadMultiply;
        case GLFW_KEY_KP_ADD: return KeyCode::NumpadAdd;
        case GLFW_KEY_KP_SUBTRACT: return KeyCode::NumpadSubtract;
        case GLFW_KEY_KP_DECIMAL: return KeyCode::NumpadDecimal;
        case GLFW_KEY_KP_DIVIDE: return KeyCode::NumpadDivide;

        case GLFW_KEY_F1: return KeyCode::F1;
        case GLFW_KEY_F2: return KeyCode::F2;
        case GLFW_KEY_F3: return KeyCode::F3;
        case GLFW_KEY_F4: return KeyCode::F4;
        case GLFW_KEY_F5: return KeyCode::F5;
        case GLFW_KEY_F6: return KeyCode::F6;
        case GLFW_KEY_F7: return KeyCode::F7;
        case GLFW_KEY_F8: return KeyCode::F8;
        case GLFW_KEY_F9: return KeyCode::F9;
        case GLFW_KEY_F10: return KeyCode::F10;
        case GLFW_KEY_F11: return KeyCode::F11;
        case GLFW_KEY_F12: return KeyCode::F12;
    }

    return KeyCode::Unknown;
}

INLINE static constexpr int keyCodeToGLFW(const KeyCode keyCode) {
    switch (keyCode) {
        default: return GLFW_KEY_UNKNOWN;

        case KeyCode::A: return GLFW_KEY_A;
        case KeyCode::B: return GLFW_KEY_B;
        case KeyCode::C: return GLFW_KEY_C;
        case KeyCode::D: return GLFW_KEY_D;
        case KeyCode::E: return GLFW_KEY_E;
        case KeyCode::F: return GLFW_KEY_F;
        case KeyCode::G: return GLFW_KEY_G;
        case KeyCode::H: return GLFW_KEY_H;
        case KeyCode::I: return GLFW_KEY_I;
        case KeyCode::J: return GLFW_KEY_J;
        case KeyCode::K: return GLFW_KEY_K;
        case KeyCode::L: return GLFW_KEY_L;
        case KeyCode::M: return GLFW_KEY_M;
        case KeyCode::N: return GLFW_KEY_N;
        case KeyCode::O: return GLFW_KEY_O;
        case KeyCode::P: return GLFW_KEY_P;
        case KeyCode::Q: return GLFW_KEY_Q;
        case KeyCode::R: return GLFW_KEY_R;
        case KeyCode::S: return GLFW_KEY_S;
        case KeyCode::T: return GLFW_KEY_T;
        case KeyCode::U: return GLFW_KEY_U;
        case KeyCode::V: return GLFW_KEY_V;
        case KeyCode::W: return GLFW_KEY_W;
        case KeyCode::X: return GLFW_KEY_X;
        case KeyCode::Y: return GLFW_KEY_Y;
        case KeyCode::Z: return GLFW_KEY_Z;

        case KeyCode::Digit0: return GLFW_KEY_0;
        case KeyCode::Digit1: return GLFW_KEY_1;
        case KeyCode::Digit2: return GLFW_KEY_2;
        case KeyCode::Digit3: return GLFW_KEY_3;
        case KeyCode::Digit4: return GLFW_KEY_4;
        case KeyCode::Digit5: return GLFW_KEY_5;
        case KeyCode::Digit6: return GLFW_KEY_6;
        case KeyCode::Digit7: return GLFW_KEY_7;
        case KeyCode::Digit8: return GLFW_KEY_8;
        case KeyCode::Digit9: return GLFW_KEY_9;

        case KeyCode::Escape: return GLFW_KEY_ESCAPE;
        case KeyCode::Tab: return GLFW_KEY_TAB;
        case KeyCode::CapsLock: return GLFW_KEY_CAPS_LOCK;
        case KeyCode::Enter: return GLFW_KEY_ENTER;
        case KeyCode::Space: return GLFW_KEY_SPACE;
        case KeyCode::Backspace: return GLFW_KEY_BACKSPACE;
        case KeyCode::Insert: return GLFW_KEY_INSERT;
        case KeyCode::Delete: return GLFW_KEY_DELETE;
        case KeyCode::Home: return GLFW_KEY_HOME;
        case KeyCode::End: return GLFW_KEY_END;
        case KeyCode::PageUp: return GLFW_KEY_PAGE_UP;
        case KeyCode::PageDown: return GLFW_KEY_PAGE_DOWN;
        case KeyCode::ArrowLeft: return GLFW_KEY_LEFT;
        case KeyCode::ArrowRight: return GLFW_KEY_RIGHT;
        case KeyCode::ArrowUp: return GLFW_KEY_UP;
        case KeyCode::ArrowDown: return GLFW_KEY_DOWN;
        case KeyCode::PrintScreen: return GLFW_KEY_PRINT_SCREEN;
        case KeyCode::ScrollLock: return GLFW_KEY_SCROLL_LOCK;
        case KeyCode::PauseBreak: return GLFW_KEY_PAUSE;

        case KeyCode::Minus: return GLFW_KEY_MINUS;
        case KeyCode::Equals: return GLFW_KEY_EQUAL;
        case KeyCode::LeftBracket: return GLFW_KEY_LEFT_BRACKET;
        case KeyCode::RightBracket: return GLFW_KEY_RIGHT_BRACKET;
        case KeyCode::Backslash: return GLFW_KEY_BACKSLASH;
        case KeyCode::Semicolon: return GLFW_KEY_SEMICOLON;
        case KeyCode::Apostrophe: return GLFW_KEY_APOSTROPHE;
        case KeyCode::Grave: return GLFW_KEY_GRAVE_ACCENT;
        case KeyCode::Comma: return GLFW_KEY_COMMA;
        case KeyCode::Period: return GLFW_KEY_PERIOD;
        case KeyCode::Slash: return GLFW_KEY_SLASH;

        case KeyCode::LeftShift: return GLFW_KEY_LEFT_SHIFT;
        case KeyCode::RightShift: return GLFW_KEY_RIGHT_SHIFT;
        case KeyCode::LeftControl: return GLFW_KEY_LEFT_CONTROL;
        case KeyCode::RightControl: return GLFW_KEY_RIGHT_CONTROL;
        case KeyCode::LeftAlt: return GLFW_KEY_LEFT_ALT;
        case KeyCode::RightAlt: return GLFW_KEY_RIGHT_ALT;
        case KeyCode::LeftSuper: return GLFW_KEY_LEFT_SUPER;
        case KeyCode::RightSuper: return GLFW_KEY_RIGHT_SUPER;

        case KeyCode::Numpad0: return GLFW_KEY_KP_0;
        case KeyCode::Numpad1: return GLFW_KEY_KP_1;
        case KeyCode::Numpad2: return GLFW_KEY_KP_2;
        case KeyCode::Numpad3: return GLFW_KEY_KP_3;
        case KeyCode::Numpad4: return GLFW_KEY_KP_4;
        case KeyCode::Numpad5: return GLFW_KEY_KP_5;
        case KeyCode::Numpad6: return GLFW_KEY_KP_6;
        case KeyCode::Numpad7: return GLFW_KEY_KP_7;
        case KeyCode::Numpad8: return GLFW_KEY_KP_8;
        case KeyCode::Numpad9: return GLFW_KEY_KP_9;
        case KeyCode::NumpadMultiply: return GLFW_KEY_KP_MULTIPLY;
        case KeyCode::NumpadAdd: return GLFW_KEY_KP_ADD;
        case KeyCode::NumpadSubtract: return GLFW_KEY_KP_SUBTRACT;
        case KeyCode::NumpadDecimal: return GLFW_KEY_KP_DECIMAL;
        case KeyCode::NumpadDivide: return GLFW_KEY_KP_DIVIDE;

        case KeyCode::F1: return GLFW_KEY_F1;
        case KeyCode::F2: return GLFW_KEY_F2;
        case KeyCode::F3: return GLFW_KEY_F3;
        case KeyCode::F4: return GLFW_KEY_F4;
        case KeyCode::F5: return GLFW_KEY_F5;
        case KeyCode::F6: return GLFW_KEY_F6;
        case KeyCode::F7: return GLFW_KEY_F7;
        case KeyCode::F8: return GLFW_KEY_F8;
        case KeyCode::F9: return GLFW_KEY_F9;
        case KeyCode::F10: return GLFW_KEY_F10;
        case KeyCode::F11: return GLFW_KEY_F11;
        case KeyCode::F12: return GLFW_KEY_F12;

        case KeyCode::Unknown: return GLFW_KEY_UNKNOWN;
    }

    return GLFW_KEY_UNKNOWN;
}


// case GLFW_KEY_LEFT_SHIFT: return KeyCode::LeftShift;
// case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RightShift;
// case GLFW_KEY_LEFT_CONTROL: return KeyCode::LeftControl;
// case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RightControl;
// case GLFW_KEY_LEFT_ALT: return KeyCode::LeftAlt;
// case GLFW_KEY_RIGHT_ALT: return KeyCode::RightAlt;
// case GLFW_KEY_LEFT_SUPER: return KeyCode::LeftSuper;
// case GLFW_KEY_RIGHT_SUPER: return KeyCode::RightSuper;


#endif