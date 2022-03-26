/*
 * Widescreen patch for Silent Heroes: Elite Troops of World War II by zocker_160
 *
 * This source code is licensed under GPL-v3
 *
 */
#pragma once
#include <Windows.h>

struct memoryPTR {
    DWORD base_address;
    int total_offsets;
    int offsets[];
};

struct threadData {
	bool bDebugMode;
    bool bCameraPatch;
    float fMinHeight;
    float fMaxHeight;
    float fZoomStep;
};

const int version_maj = 1;
const int version_min = 0;

const int RETRY_COUNT = 20;

DWORD WINAPI PatchThread(LPVOID param);
