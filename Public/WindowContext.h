#ifndef WINDOWSURFACE_HPP
#define WINDOWSURFACE_HPP

#include "ErrorCodes.hpp"
#ifdef BUILD_GLFW

#ifndef GLFW_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>
#include <unordered_map>

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
    uint16_t width{0};
    uint16_t height{0};
    std::string& appName() { return m_appName; }

    Event<const int, const int> windowResized;
    Event<const WindowState> windowStateChanged;

    WindowContext() = default;
    WindowContext(WindowContext&& other) = default;
    WindowContext(const WindowContext&) = delete;
    WindowContext& operator=(const WindowContext&) = delete;

    template <std::integral I>
    static ErrorCode create(
        const char* appName, const char* wndTitle, const I width, const I height,
        WindowContext* outCtx)
    {
        if (outCtx == nullptr)
            return ErrorCode::GLFW_CONTEXT_IS_NULL;

        if (!glfwInit())
            return ErrorCode::GLFW_UNKNOWN_INIT_ERROR;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        outCtx->m_window = glfwCreateWindow(width, height, wndTitle, nullptr, nullptr);
        if (!outCtx->m_window )
            return ErrorCode::GLFW_FAILED_WND_CREATION;

        outCtx->m_appName = std::string{appName};
        outCtx->m_windowTitle = std::string{wndTitle};
        outCtx->width = static_cast<uint16_t>(width);
        outCtx->height = static_cast<uint16_t>(height);

        s_windowMap[outCtx->m_window] = outCtx;

        glfwSetWindowTitle(outCtx->m_window, outCtx->m_appName.c_str());
        glfwSetWindowSizeCallback(outCtx->m_window, framebufferSizeCallback);
        glfwSetWindowIconifyCallback(outCtx->m_window, windowIconifyCallback);
        glfwSetWindowMaximizeCallback(outCtx->m_window, windowMaximizeCallback);
        glfwSetWindowFocusCallback(outCtx->m_window, windowFocusCallback);
        glfwSetWindowCloseCallback(outCtx->m_window, windowCloseCallback);

        return ErrorCode::OK;
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
};

#endif

#endif // WINDOWSURFACE_HPP