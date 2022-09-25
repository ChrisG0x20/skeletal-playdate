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

#include <new>
#include <cstdarg>

namespace pd {
    constexpr int LcdWidth = LCD_COLUMNS;
    constexpr int LcdHeight = LCD_ROWS;
    constexpr int LcdRowStride = LCD_ROWSIZE;

    extern decltype(playdate_sys::error) error;
    extern decltype(playdate_sys::realloc) realloc;

    //void (*logToConsole)(const char* fmt, ...);
    extern decltype(playdate_sys::logToConsole) logToConsole;
    extern decltype(playdate_sys::resetElapsedTime) resetElapsedTime;
    extern decltype(playdate_sys::getElapsedTime) getElapsedTime;
    extern decltype(playdate_sys::setUpdateCallback) setUpdateCallback;
    extern decltype(playdate_sys::drawFPS) drawFPS;
    extern decltype(playdate_sys::formatString) formatString;
    extern decltype(playdate_sys::getBatteryPercentage) getBatteryPercentage;
    extern decltype(playdate_sys::getBatteryVoltage) getBatteryVoltage;
    extern decltype(playdate_sys::getButtonState) getButtonState;
    extern decltype(playdate_sys::getCrankAngle) getCrankAngle;
    extern decltype(playdate_sys::getCrankChange) getCrankChange;
    extern decltype(playdate_sys::isCrankDocked) isCrankDocked;
    extern decltype(playdate_display::getWidth) getWidth;
    extern decltype(playdate_display::getHeight) getHeight;
    extern decltype(playdate_display::setRefreshRate) setRefreshRate;

    extern decltype(playdate_graphics::clear) clear;

	// Returns the current display frame buffer. Rows are 32-bit aligned, so the
	// row stride is 52 bytes, with the extra 2 bytes per row ignored. Bytes are
	// MSB-ordered; i.e., the pixel in column 0 is the 0x80 bit of the first
	// byte of the row.
    extern decltype(playdate_graphics::getFrame) getFrame; // row stride = LCD_ROWSIZE

	// Returns a bitmap containing the contents of the display buffer. The
	// system owns this bitmap—​do not free it!
	//playdate_graphics::getDisplayFrame getDisplayFrame; // row stride = LCD_ROWSIZE

	// Only valid in the simulator, returns the debug framebuffer as a bitmap.
	// Function is NULL on device.
    extern decltype(playdate_graphics::getDebugBitmap) getDebugBitmap; // valid in simulator only, function is NULL on device

	//playdate_graphics::copyFrameBufferBitmap copyFrameBufferBitmap;

	// After updating pixels in the buffer returned by getFrame(), you must tell
	// the graphics system which rows were updated. This function marks a
	// contiguous range of rows as updated (e.g., markUpdatedRows(0,LCD_ROWS-1)
	// tells the system to update the entire display). Both “start” and “end”
	// are included in the range.
    extern decltype(playdate_graphics::markUpdatedRows) markUpdatedRows;

    extern decltype(playdate_file::geterr) geterr;
    extern decltype(playdate_file::listfiles) listfiles;
    extern decltype(playdate_file::stat) stat;
    extern decltype(playdate_file::mkdir) mkdir;
    extern decltype(playdate_file::unlink) unlink;
    extern decltype(playdate_file::rename) rename;
    extern decltype(playdate_file::open) open;
    extern decltype(playdate_file::close) close;
    extern decltype(playdate_file::read) read;
    extern decltype(playdate_file::write) write;
    extern decltype(playdate_file::flush) flush;
    extern decltype(playdate_file::tell) tell;
    extern decltype(playdate_file::seek) seek;

	// Manually flushes the current frame buffer out to the display. This
	// function is automatically called after each pass through the run loop, so
	// there shouldn’t be any need to call it yourself.
	//playdate_graphics::display display;

    void InitializePlaydateAPI(PlaydateAPI* pPlaydate);
    void logToConsoleVaList(const char* format, va_list args);

    // To optionally execute C++ initialization functions at startup.
    // NOTE: This stuff is being ignored for now; instead, handling everything inside kEventInit.
    // execute function marked with __attribute__((constructor))
    void InitializeGlobalVariables(PlaydateAPI* pPlaydate);
    void FinalizeGlobalVariables();

//    template<typename T, typename ...Params>
//    inline T* New(Params&&... args)
//    {
//        pd::logToConsole("calling pd::New()");
//        const auto pTemp = pd::realloc(nullptr, sizeof(T));
//        if (nullptr != pTemp)
//        {
//            pd::logToConsole("SUCCESS: allocated memory in pd::New()");
//            return new(pTemp) T(std::forward<Params>(args)...);
//        }
//
//        pd::logToConsole("ERROR: failed to allocate memory for new object instance");
//        return nullptr;
//    }
} // namespace pd

void* operator new(std::size_t count) noexcept(false);
void* operator new(std::size_t count, const std::nothrow_t& tag) noexcept;
void operator delete(void* p) noexcept;

#if TARGET_PLAYDATE
// These are functions the C++ std library expects to find, but don't
// exist on the device. Some of them could be implemented to a degree.
extern "C"
{
    // void _exit(int code);
    // int _read(int file, char* ptr, int len);
    // long _lseek(int fd, int offset, int origin);
    // int _isatty(int file);
    // int _fstat(int file, struct stat *st);
    // int _open(const char* filename, int oflag, int pmode);
    // int _close(int file);
    // int _write(int handle, char* data, int size);
    // int _getpid(void);
    // int _kill(int pid, int sig);
} // extern "C"
#endif // TARGET_PLAYDATE

#endif // CLGPD_HPP
