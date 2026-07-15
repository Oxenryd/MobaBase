#ifndef WINDOWSURFACE_HPP
#define WINDOWSURFACE_HPP

#ifdef BUILD_GLFW

#ifndef GLFW_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>
#include <unordered_map>

#include "ErrorCodes.hpp"

enum class WindowState : uint8_t {
    RestoredFromMin = 0,
    RestoredFromMax = 1,
    Minimized = 2,
    Maximized = 3,
    LostFocus = 4,
    GotFocus = 5,
    Closed = 6
};

class WindowContext {
    inline static std::unordered_map<GLFWwindow*, WindowContext*> s_windowMap;
    GLFWwindow* m_window{nullptr};
    std::string m_appName{};
    std::string m_windowTitle{};
    uint16_t m_width{0};
    uint16_t m_height{0};
    ErrorCode m_currentEC{ErrorCode::OK};

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
        const auto ctxIT = s_windowMap.find(window);
        if (ctxIT == s_windowMap.end()) return;
        ctxIT->second->windowResized.notify(width, height);
    }

    static void windowIconifyCallback(GLFWwindow* window, int iconified) {
        const auto ctxIT = s_windowMap.find(window);
        if (ctxIT == s_windowMap.end())
            return;
        if (iconified)
            ctxIT->second->windowStateChanged.notify(WindowState::Minimized);
        else
            ctxIT->second->windowStateChanged.notify(WindowState::RestoredFromMin);
    }

    static void windowMaximizeCallback(GLFWwindow* window, int maximized) {
        const auto ctxIT = s_windowMap.find(window);
        if (ctxIT == s_windowMap.end())
            return;
        if (maximized)
            ctxIT->second->windowStateChanged.notify(WindowState::Maximized);
        else
            ctxIT->second->windowStateChanged.notify(WindowState::RestoredFromMax);
    }

    static void windowFocusCallback(GLFWwindow* window, int focused) {
        const auto ctxIT = s_windowMap.find(window);
        if (ctxIT == s_windowMap.end())
            return;
        if (focused)
            ctxIT->second->windowStateChanged.notify(WindowState::GotFocus);
        else
            ctxIT->second->windowStateChanged.notify(WindowState::LostFocus);
    }

    static void windowCloseCallback(GLFWwindow* window) {
        const auto ctxIT = s_windowMap.find(window);
        if (ctxIT == s_windowMap.end())
            return;
        ctxIT->second->windowStateChanged.notify(WindowState::Closed);
    }

public:
    std::string& appName() { return m_appName; }

    Event<const int, const int> windowResized;
    Event<const WindowState> windowStateChanged;

    template <std::integral I>
    WindowContext(const char* appName, const char* wndTitle, const I width, const I height) {

        if (!glfwInit()) {
            m_currentEC = ErrorCode::GLFW_UNKNOWN_INIT_ERROR;
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_window = glfwCreateWindow(width, height, wndTitle, nullptr, nullptr);
        if (!m_window ) {
            m_currentEC = ErrorCode::GLFW_FAILED_WND_CREATION;
            return;
        }


        m_appName = std::string{appName};
        m_windowTitle = std::string{wndTitle};
        m_width = static_cast<uint16_t>(width);
        m_height = static_cast<uint16_t>(height);

        s_windowMap[m_window] = this;

        glfwSetWindowTitle(m_window, m_appName.c_str());
        glfwSetWindowSizeCallback(m_window, framebufferSizeCallback);
        glfwSetWindowIconifyCallback(m_window, windowIconifyCallback);
        glfwSetWindowMaximizeCallback(m_window, windowMaximizeCallback);
        glfwSetWindowFocusCallback(m_window, windowFocusCallback);
        glfwSetWindowCloseCallback(m_window, windowCloseCallback);
    }

    WindowContext() = delete;
    WindowContext(WindowContext&& other) = delete;
    WindowContext(const WindowContext&) = delete;
    WindowContext& operator=(const WindowContext&) = delete;
    WindowContext& operator=(WindowContext&& other) = delete;
    ~WindowContext() {
        glfwDestroyWindow(m_window);
    }

    [[nodiscard]] const char* appName_c_str() const { return m_appName.c_str(); }

    GLFWwindow* window() const { return m_window; }
    GLFWwindow* window() { return m_window; }

    static void showWindow(int) {}

    void setWindowTitle(const std::string_view title)
    {
        m_windowTitle = title;
        glfwSetWindowTitle(m_window, m_windowTitle .c_str());
    }

    static std::vector<const char*> getVulkanInstanceExtensions() {
        uint32_t count = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&count);
        return std::vector(exts, exts + count);
    }

    VkResult createSurface(const VkInstance instance, VkSurfaceKHR* out) const {
        return glfwCreateWindowSurface(instance, m_window, nullptr, out);
    }

    ErrorCode getCurrentError() const { return m_currentEC; }

    uint16_t width() const { return m_width; }
    uint16_t height() const { return m_height; }
};

#endif

#endif // WINDOWSURFACE_HPP