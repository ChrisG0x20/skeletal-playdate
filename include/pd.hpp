//
// Copyright (c) 2022 Christopher Gassib
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CLGPD_HPP
#define CLGPD_HPP

extern "C"
{
#include <pd_api.h>
}

// To optionally execute C++ initialization functions at startup.
// NOTE: This stuff is being ignored for now; instead, handling everything inside kEventInit.
#if TARGET_PLAYDATE
typedef const void(*init_routine_t)();
init_routine_t __preinit_array_start;
init_routine_t __preinit_array_end;
init_routine_t __init_array_start;
init_routine_t __init_array_end;
init_routine_t __fini_array_start;
init_routine_t __fini_array_end;
#endif // TARGET_PLAYDATE

namespace pd {
    constexpr int LcdWidth = LCD_COLUMNS;
    constexpr int LcdHeight = LCD_ROWS;
    constexpr int LcdRowStride = LCD_ROWSIZE;

    decltype(playdate_sys::error) error = nullptr;
    //playdate_sys::realloc realloc = nullptr;

    //void (*logToConsole)(const char* fmt, ...) = nullptr;
    decltype(playdate_sys::logToConsole) logToConsole = nullptr;
    decltype(playdate_sys::resetElapsedTime) resetElapsedTime = nullptr;
    decltype(playdate_sys::getElapsedTime) getElapsedTime = nullptr;
    decltype(playdate_sys::setUpdateCallback) setUpdateCallback = nullptr;
    decltype(playdate_sys::drawFPS) drawFPS = nullptr;
    decltype(playdate_sys::formatString) formatString = nullptr;
    decltype(playdate_sys::getBatteryPercentage) getBatteryPercentage = nullptr;
    decltype(playdate_sys::getBatteryVoltage) getBatteryVoltage = nullptr;
    decltype(playdate_sys::getButtonState) getButtonState = nullptr;
    decltype(playdate_sys::getCrankAngle) getCrankAngle = nullptr;
    decltype(playdate_sys::getCrankChange) getCrankChange = nullptr;
    decltype(playdate_sys::isCrankDocked) isCrankDocked = nullptr;
    decltype(playdate_display::getWidth) getWidth = nullptr;
    decltype(playdate_display::getHeight) getHeight = nullptr;
    decltype(playdate_display::setRefreshRate) setRefreshRate = nullptr;

    decltype(playdate_graphics::clear) clear = nullptr;

	// Returns the current display frame buffer. Rows are 32-bit aligned, so the
	// row stride is 52 bytes, with the extra 2 bytes per row ignored. Bytes are
	// MSB-ordered; i.e., the pixel in column 0 is the 0x80 bit of the first
	// byte of the row.
    decltype(playdate_graphics::getFrame) getFrame = nullptr; // row stride = LCD_ROWSIZE

	// Returns a bitmap containing the contents of the display buffer. The
	// system owns this bitmap—​do not free it!
	//playdate_graphics::getDisplayFrame getDisplayFrame = nullptr; // row stride = LCD_ROWSIZE

	// Only valid in the simulator, returns the debug framebuffer as a bitmap.
	// Function is NULL on device.
    decltype(playdate_graphics::getDebugBitmap) getDebugBitmap = nullptr; // valid in simulator only, function is NULL on device

	//playdate_graphics::copyFrameBufferBitmap copyFrameBufferBitmap = nullptr;

	// After updating pixels in the buffer returned by getFrame(), you must tell
	// the graphics system which rows were updated. This function marks a
	// contiguous range of rows as updated (e.g., markUpdatedRows(0,LCD_ROWS-1)
	// tells the system to update the entire display). Both “start” and “end”
	// are included in the range.
    decltype(playdate_graphics::markUpdatedRows) markUpdatedRows = nullptr;

	// Manually flushes the current frame buffer out to the display. This
	// function is automatically called after each pass through the run loop, so
	// there shouldn’t be any need to call it yourself.
	//playdate_graphics::display display = nullptr;

    inline void InitializePlaydateAPI(PlaydateAPI* pPlaydate)
    {
            const auto pd = pPlaydate;

            // hook platform API functions
            pd::error = pd->system->error;
            pd::logToConsole = pd->system->logToConsole;
            pd::resetElapsedTime = pd->system->resetElapsedTime;
            pd::getElapsedTime = pd->system->getElapsedTime;
            pd::setUpdateCallback = pd->system->setUpdateCallback;
            pd::drawFPS = pd->system->drawFPS;
            pd::formatString = pd->system->formatString;
            pd::getBatteryPercentage = pd->system->getBatteryPercentage;
            pd::getBatteryVoltage = pd->system->getBatteryVoltage;
            pd::getButtonState = pd->system->getButtonState;
            pd::getCrankAngle = pd->system->getCrankAngle;
            pd::getCrankChange = pd->system->getCrankChange;
            pd::isCrankDocked = pd->system->isCrankDocked;
            pd::getWidth = pd->display->getWidth;
            pd::getHeight = pd->display->getHeight;
            pd::setRefreshRate = pd->display->setRefreshRate;
            pd::clear = pd->graphics->clear;
            pd::getFrame = pd->graphics->getFrame;
            pd::getDebugBitmap = pd->graphics->getDebugBitmap;
            pd::markUpdatedRows = pd->graphics->markUpdatedRows;
    }

#if TARGET_PLAYDATE
typedef const void(*init_routine_t)();
init_routine_t __preinit_array_start;
init_routine_t __preinit_array_end;
init_routine_t __init_array_start;
init_routine_t __init_array_end;
init_routine_t __fini_array_start;
init_routine_t __fini_array_end;

inline void ExecuteFunctions(const init_routine_t *const first, const init_routine_t *const last)
{
    for (auto it = first; it <= last; it++)
    {
        if (nullptr != it)
        {
            (*it)();
        }
    }
}

// execute function marked with __attribute__((constructor))
inline void InitializeGlobalVariables(PlaydateAPI* pPlaydate)
{
    pPlaydate->system->logToConsole("initializing globals");
    ExecuteFunctions(&__preinit_array_start, &__preinit_array_end);
    ExecuteFunctions(&__init_array_start, &__init_array_end);
    pPlaydate->system->logToConsole("done initializing globals");
}

inline void FinalizeGlobalVariables()
{
    pd::logToConsole("finalizing globals");
    ExecuteFunctions(&__fini_array_start, &__fini_array_end);
    pd::logToConsole("done finalizing globals");
}
#else
inline void InitializeGlobalVariables(PlaydateAPI* pPlaydate) { ((void)pPlaydate); } // NOP
inline void FinalizeGlobalVariables() {} // NOP
#endif // TARGET_PLAYDATE
} // namespace pd

// These are functions the C++ std library expects to find, but don't
// exist on the device. Some of them could be implemented to a degree.
extern "C"
{
    void _exit(int code)
    {
        while (1)
        {
            pd::error("exited with code %d.", code);
        }
    }

    int _read(int file, char* ptr, int len)
    {
        return 0;
    }

    int _lseek(int file, int pos, int whence)
    {
        // TODO: errno (no such file)
        return -1;
    }

    int _isatty(int file)
    {
        return 0;
    }

    int _fstat(int file, struct stat *st)
    {
        return 0;
    }

    int _close(int file)
    {
        // TODO: errno (no such file handle)
        return -1;
    }

    int _write(int handle, char* data, int size)
    {
        // TODO: errno (no such open file)
        return -1;
    }

    int _getpid(void)
    {
        return 1;
    }

    int _kill(int pid, int sig)
    {
        return 0;
    }
} // extern "C"

#endif // CLGPD_HPP
