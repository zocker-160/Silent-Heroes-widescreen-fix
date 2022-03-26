/*
 * Widescreen patch for Silent Heroes: Elite Troops of World War II by zocker_160
 *
 * This source code is licensed under GPL-v3
 *
 */
#include "ResPatch.h"
#include <Windows.h>
#include <iostream>
#include <sstream>

DWORD TextureResolutionLimit = 0xE8376;

//DWORD CameraMaxH = 0x4FE1F4; // 100000 if not init
//DWORD CameraMinH = 0x4FE1F0; // 1 if not init

//DWORD CameraZoomStep = 0x46576C;
DWORD CameraZoomStepPTR = 0x39AD70;

//DWORD CameraStatus = 0x4FEDE8;
//DWORD GameLoadStatus = 0x4E0834;

DWORD CameraObject = 0x4FE1E0;

memoryPTR CameraMinH = {
    CameraObject,
    1,
    { 0x20 }
};

memoryPTR CameraMaxH = {
    CameraObject,
    1,
    { 0x24 }
};

/*###################################*/

// reading and writing stuff / helper functions and other crap

/* update memory protection and read with memcpy */
void protectedRead(void* dest, void* src, int n) {
    DWORD oldProtect = 0;
    VirtualProtect(dest, n, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(dest, src, n);
    VirtualProtect(dest, n, oldProtect, &oldProtect);
}
/* read from address into read buffer of length len */
bool readBytes(void* read_addr, void* read_buffer, int len) {
    // compile with "/EHa" to make this work
    // see https://stackoverflow.com/questions/16612444/catch-a-memory-access-violation-in-c
    try {
        protectedRead(read_buffer, read_addr, len);
        return true;
    }
    catch (...) {
        return false;
    }
}
/* write patch of length len to destination address */
void writeBytes(void* dest_addr, void* patch, int len) {
    protectedRead(dest_addr, patch, len);
}

/* fiddle around with the pointers */
HMODULE getBaseAddress() {
    return GetModuleHandle(NULL);
}
DWORD* calcAddress(DWORD appl_addr) {
    return (DWORD*)((DWORD)getBaseAddress() + appl_addr);
}
DWORD* tracePointer(memoryPTR* patch) {
    DWORD* location = calcAddress(patch->base_address);

    for (int i = 0; i < patch->total_offsets; i++) {
        location = (DWORD*)(*location + patch->offsets[i]);
    }
    return location;
}

void GetDesktopResolution(int& hor, int& vert) {
    hor = GetSystemMetrics(SM_CXSCREEN);
    vert = GetSystemMetrics(SM_CYSCREEN);
}
float calcAspectRatio(int horizontal, int vertical) {
    if (horizontal != 0 && vertical != 0) {
        return (float)horizontal / (float)vertical;
    }
    else {
        return -1.0f;
    }
}
float getAspectRatio() {
    int horizontal, vertical;
    GetDesktopResolution(horizontal, vertical);
    return calcAspectRatio(horizontal, vertical);
}

/* other helper functions and stuff */
void showMessage(float val) {
    std::cout << "DEBUG: " << val << "\n";
}
void showMessage(int val) {
    std::cout << "DEBUG: " << val << "\n";
}
void showMessage(LPCSTR val) {
    std::cout << "DEBUG: " << val << "\n";
}
void startupMessage() {
    std::cout << "Resolution Patch by zocker_160 - Version: v" << version_maj << "." << version_min << "\n";
    std::cout << "Debug mode enabled!\n";
    std::cout << "Waiting for application startup...\n";
}

bool fcmp(float a, float b) {
    return fabs(a - b) < FLT_EPSILON;
}

int MainEntry(threadData* tData) {
    FILE* f;

    if (tData->bDebugMode) {
        AllocConsole();
        freopen_s(&f, "CONOUT$", "w", stdout);
        startupMessage();
    }

    /* fix for "Texture or surface size is too big (esurface.cpp, 129)"  */
    int newResLimit = 4096;
    int* textureLimit_p = (int*)(calcAddress(TextureResolutionLimit));

    showMessage(*textureLimit_p);

    // wait until value can be written (fix for Steam version)
    for (int i = 0; i < RETRY_COUNT; i++) {
        if (*textureLimit_p != 2048) {
            showMessage("Unexpected value - retrying...");
            Sleep(200);
            if (i - 1 == RETRY_COUNT) {
                showMessage("Resolution limit could not be set - exiting");
                return 0;
            }

        } else {
            showMessage("Patching resolution limit...");
            writeBytes(textureLimit_p, &newResLimit, 4);
            break;
        }
    }

    showMessage(*textureLimit_p);

    if (!tData->bCameraPatch || getAspectRatio() < calcAspectRatio(16, 9)) {
        showMessage("ignoring camera patch and exit");
        return 0;
    }

    Sleep(1000);

    /* camera and zoom patch */
    int* camStat = (int*)calcAddress(CameraObject);
    float* minH = (float*)tracePointer(&CameraMinH);
    float* maxH = (float*)tracePointer(&CameraMaxH);

    float* zoomStepPTR = (float*)calcAddress(CameraZoomStepPTR);
    float* newZoomStepPTR = &tData->fZoomStep;

    showMessage("writing zoom step pointer");
    writeBytes(zoomStepPTR, &newZoomStepPTR, 4);

    for (;; Sleep(1000)) {
        if ( (!fcmp(*minH, 1.0f) || !fcmp(*maxH, 100000.f)) && *camStat != 0 ) {
            if (*minH != tData->fMinHeight || *maxH != tData->fMaxHeight) {
                showMessage("writing values...");
                writeBytes(minH, &tData->fMinHeight, 4);
                writeBytes(maxH, &tData->fMaxHeight, 4);
            }
        }
    }

    return 0;
}

DWORD WINAPI PatchThread(LPVOID param) {
    return MainEntry(reinterpret_cast<threadData*>(param));
}
