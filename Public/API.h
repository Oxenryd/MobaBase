//
// Created by oxenryd on 2026-01-15.
//

#ifndef API_H
#define API_H


inline void assume(bool cond) {
#if defined(__clang__) || defined(__GNUC__)
    if (!cond) std::__terminate();
#elif defined(_MSC_VER)
    __assume(cond);
#else
    (void)cond;
#endif
}

#if defined(_WIN32)
    #if defined(BUILD_DLL)
        #define ENGINE_API __declspec(dllexport)
    #else
        #define ENGINE_API __declspec(dllimport)
    #endif
#else
    #include <bits/c++config.h>
    #define ENGINE_API __attribute__((visibility("default")))
#endif

#endif //MOBABASE_API_H