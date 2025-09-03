#ifndef MEMDBEUGFILTER_HPP
#define MEMDEBUGFILTER_HPP

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <iostream>
#include <cstring>
#include <string>

// Your filter function
int MyAllocHook(
    int allocType,
    void* userData,
    size_t size,
    int blockType,
    long requestNumber,
    const unsigned char* filename,
    int lineNumber
) {
    // Allow all frees and reallocations to pass
    if (allocType != _HOOK_ALLOC) {
        return 1;
    }

    // If filename is null, it's not from a file compiled with /Zi, so skip filtering
    if (filename) {
        // Convert filename to string for easy search
        std::string file = reinterpret_cast<const char*>(filename);

        // Only break/dump if allocation comes from *your* code path
        // Change "MyProject" to a unique substring of your project's file paths
        if (file.find(SOURCE_DIR) == std::string::npos) {
            return 1; // Ignore this allocation
        }
    }

    // If we get here, it’s an allocation from *your* code
    // You can set a conditional breakpoint here
    // Example: if (size == 128) __debugbreak();

    return 1; // Allow allocation
}

void InitLeakFilter() {
    _CrtSetAllocHook(MyAllocHook);
}

static const char* kProjectPathFilter = SOURCE_DIR;

void __cdecl MyDumpClient(void* pUserData, size_t size) {
    
    long requestNumber;
    char* fileName = nullptr;
    int lineNumber;
    //int blockType;

    if (_CrtIsValidHeapPointer(pUserData) &&
        _CrtIsMemoryBlock(pUserData, (unsigned int)size, &requestNumber, &fileName, &lineNumber)) {
        if (fileName && strstr(fileName, kProjectPathFilter)) {
            std::cout << "{" << requestNumber << "} normal block @ "
                << pUserData << ", " << size << " bytes, file: "
                << fileName << ":" << lineNumber << "\n";
        }
    }
}

void DumpFilteredLeaks() {
    _CrtSetDumpClient(MyDumpClient);
}


// CRT Report Hook - this gets called for ALL CRT debug output
int __cdecl MyReportHook(int reportType, char* message, int* returnValue) {
    // Only process memory leak reports
    if (reportType == _CRT_WARN) {
        // Parse the message to extract allocation info
        if (strstr(message, "normal block")) {
            // Extract request number from message like "{15655} normal block at..."
            long requestNumber = 0;
            if (sscanf_s(message, "{%ld}", &requestNumber) == 1) {

                // Filter based on request number ranges - adjust these values!
                // Looking at your dump, you have ranges like 170-17024
                // The lower numbers (170-379) might be system/CRT allocations
                // Higher numbers (17000+) are likely your project allocations
                bool isProjectAllocation = (requestNumber >= 300 && requestNumber <= 20000);

                if (isProjectAllocation) {
                    // Output to console instead of debug output window
                    std::cout << message;
                    // Suppress the original message (return TRUE means "suppress")
                    return TRUE;
                } else {
                    // Suppress non-project allocations
                    return TRUE;
                }
            }
        }
        // Handle other leak-related messages
        else if (strstr(message, "Detected memory leaks!") ||
                 strstr(message, "Dumping objects") ||
                 strstr(message, "Object dump complete")) {
            // Let these header/footer messages through to console
            std::cout << message;
            return TRUE; // Suppress from debug output
        }
    }

    // For non-leak messages, let them go to debug output normally
    return FALSE;
}

// Better approach: Use memory checkpoints and manual enumeration
void DumpFilteredLeaksManual() {
    // First, redirect CRT output to a string buffer or suppress it
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

    // Create a memory state checkpoint
    _CrtMemState memState;
    _CrtMemCheckpoint(&memState);

    // Manually walk through allocations
    // Note: This is complex and requires walking the debug heap
    // For now, let's use a simpler approach with report filtering

    //std::cout << "Filtered memory leaks (project-specific):\n";
    //std::cout << "Dumping objects ->\n";

    // We'll need to implement a custom walker or use the report hook approach above
    // This is a placeholder for the manual enumeration approach

    _CrtDumpMemoryLeaks();
}





#endif