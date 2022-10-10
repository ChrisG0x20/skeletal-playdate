#include "pd.hpp"
#include <cstdio>
#include <cassert>
#include <cmath>
#include "clg-math/clg_math.hpp"

#if TARGET_PLAYDATE
extern "C"
{
#include <sys/stat.h>
#include <ctime>
#include <fcntl.h>
}
#endif

namespace pd
{
decltype(playdate_sys::error) error = nullptr;
decltype(playdate_sys::realloc) realloc = nullptr;

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

// Drawing Functions
decltype(playdate_graphics::clear) clear = nullptr;
decltype(playdate_graphics::setBackgroundColor) setBackgroundColor = nullptr;
decltype(playdate_graphics::setStencil) setStencil = nullptr; // deprecated in favor of setStencilImage, which adds a "tile" flag
decltype(playdate_graphics::setDrawMode) setDrawMode = nullptr;
decltype(playdate_graphics::setDrawOffset) setDrawOffset = nullptr;
decltype(playdate_graphics::setClipRect) setClipRect = nullptr;
decltype(playdate_graphics::clearClipRect) clearClipRect = nullptr;
decltype(playdate_graphics::setLineCapStyle) setLineCapStyle = nullptr;
decltype(playdate_graphics::setFont) setFont = nullptr;
decltype(playdate_graphics::setTextTracking) setTextTracking = nullptr;
decltype(playdate_graphics::pushContext) pushContext = nullptr;
decltype(playdate_graphics::popContext) popContext = nullptr;

decltype(playdate_graphics::drawBitmap) drawBitmap = nullptr;
decltype(playdate_graphics::tileBitmap) tileBitmap = nullptr;
decltype(playdate_graphics::drawLine) drawLine = nullptr;
decltype(playdate_graphics::fillTriangle) fillTriangle = nullptr;
decltype(playdate_graphics::drawRect) drawRect = nullptr;
decltype(playdate_graphics::fillRect) fillRect = nullptr;
decltype(playdate_graphics::drawEllipse) drawEllipse = nullptr; // stroked inside the rect
decltype(playdate_graphics::fillEllipse) fillEllipse = nullptr;
decltype(playdate_graphics::drawScaledBitmap) drawScaledBitmap = nullptr;
decltype(playdate_graphics::drawText) drawText = nullptr;

// LCDBitmap
decltype(playdate_graphics::newBitmap) newBitmap = nullptr;
decltype(playdate_graphics::freeBitmap) freeBitmap = nullptr;
decltype(playdate_graphics::loadBitmap) loadBitmap = nullptr;
decltype(playdate_graphics::copyBitmap) copyBitmap = nullptr;
decltype(playdate_graphics::loadIntoBitmap) loadIntoBitmap = nullptr;
decltype(playdate_graphics::getBitmapData) getBitmapData = nullptr;
decltype(playdate_graphics::clearBitmap) clearBitmap = nullptr;
decltype(playdate_graphics::rotatedBitmap) rotatedBitmap = nullptr;

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

decltype(playdate_file::geterr) geterr = nullptr;
decltype(playdate_file::listfiles) listfiles = nullptr;
decltype(playdate_file::stat) stat = nullptr;
decltype(playdate_file::mkdir) mkdir = nullptr;
decltype(playdate_file::unlink) unlink = nullptr;
decltype(playdate_file::rename) rename = nullptr;
decltype(playdate_file::open) open = nullptr;
decltype(playdate_file::close) close = nullptr;
decltype(playdate_file::read) read = nullptr;
decltype(playdate_file::write) write = nullptr;
decltype(playdate_file::flush) flush = nullptr;
decltype(playdate_file::tell) tell = nullptr;
decltype(playdate_file::seek) seek = nullptr;

void InitializePlaydateAPI(PlaydateAPI* pPlaydate)
{
    const auto pd = pPlaydate;

    // hook platform API functions
    pd::error = pd->system->error;
    pd::realloc = pd->system->realloc;
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
    // Drawing Functions
    pd::clear = pd->graphics->clear;
    pd::setBackgroundColor = pd->graphics->setBackgroundColor;
    pd::setStencil = pd->graphics->setStencil;
    pd::setDrawMode = pd->graphics->setDrawMode;
    pd::setDrawOffset = pd->graphics->setDrawOffset;
    pd::setClipRect = pd->graphics->setClipRect;
    pd::clearClipRect = pd->graphics->clearClipRect;
    pd::setLineCapStyle = pd->graphics->setLineCapStyle;
    pd::setFont = pd->graphics->setFont;
    pd::setTextTracking = pd->graphics->setTextTracking;
    pd::pushContext = pd->graphics->pushContext;
    pd::popContext = pd->graphics->popContext;
    pd::drawBitmap = pd->graphics->drawBitmap;
    pd::tileBitmap = pd->graphics->tileBitmap;
    pd::drawLine = pd->graphics->drawLine;
    pd::fillTriangle = pd->graphics->fillTriangle;
    pd::drawRect = pd->graphics->drawRect;
    pd::fillRect = pd->graphics->fillRect;
    pd::drawEllipse = pd->graphics->drawEllipse;
    pd::fillEllipse = pd->graphics->fillEllipse;
    pd::drawScaledBitmap = pd->graphics->drawScaledBitmap;
    pd::drawText = pd->graphics->drawText;
    // LCDBitmap
    pd::newBitmap = pd->graphics->newBitmap;
    pd::freeBitmap = pd->graphics->freeBitmap;
    pd::loadBitmap = pd->graphics->loadBitmap;
    pd::copyBitmap = pd->graphics->copyBitmap;
    pd::loadIntoBitmap = pd->graphics->loadIntoBitmap;
    pd::getBitmapData = pd->graphics->getBitmapData;
    pd::clearBitmap = pd->graphics->clearBitmap;
    pd::rotatedBitmap = pd->graphics->rotatedBitmap;
    pd::getFrame = pd->graphics->getFrame;
    pd::getDebugBitmap = pd->graphics->getDebugBitmap;
    pd::markUpdatedRows = pd->graphics->markUpdatedRows;
    pd::geterr = pd->file->geterr;
    pd::listfiles = pd->file->listfiles;
    pd::stat = pd->file->stat;
    pd::mkdir = pd->file->mkdir;
    pd::unlink = pd->file->unlink;
    pd::rename = pd->file->rename;
    pd::open = pd->file->open;
    pd::close = pd->file->close;
    pd::read = pd->file->read;
    pd::write = pd->file->write;
    pd::flush = pd->file->flush;
    pd::tell = pd->file->tell;
    pd::seek = pd->file->seek;
}

void logToConsoleVaList(const char* format, va_list args)
{
   constexpr size_t len = 256;
   char buffer[len];
   size_t count = vsnprintf(buffer, len, format, args);
   if (count < len)
   {
       pd::logToConsole(buffer);
       return;
   }

   char* p = (char*)pd::realloc(nullptr, count + 1);
   if (nullptr == p)
   {
       pd::logToConsole("logToConsoleVaList() -- FAILED");
       return;
   }
   vsnprintf(p, count + 1, format, args);
   pd::logToConsole(p);
   pd::realloc(p, 0);
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
void InitializeGlobalVariables(PlaydateAPI* pPlaydate)
{
    pPlaydate->system->logToConsole("initializing globals");
    ExecuteFunctions(&__preinit_array_start, &__preinit_array_end);
    ExecuteFunctions(&__init_array_start, &__init_array_end);
    pPlaydate->system->logToConsole("done initializing globals");
}

void FinalizeGlobalVariables()
{
    pd::logToConsole("finalizing globals");
    ExecuteFunctions(&__fini_array_start, &__fini_array_end);
    pd::logToConsole("done finalizing globals");
}
#else
void InitializeGlobalVariables(PlaydateAPI* pPlaydate) { ((void)pPlaydate); } // NOP
void FinalizeGlobalVariables() {} // NOP
#endif // TARGET_PLAYDATE
} // namespace pd

#ifdef TARGET_PLAYDATE
namespace std
{
const nothrow_t nothrow;
}
//void* __dso_handle = (void*) &__dso_handle;
#endif // TARGET_PLAYDATE

void* operator new(std::size_t count) noexcept(false)
{
    assert(false); // NOTE: exceptions aren't setup to work on the device.
    return operator new(count, std::nothrow);
}

void* operator new(std::size_t count, const std::nothrow_t& tag) noexcept
{
    const auto pTemp = pd::realloc(nullptr, count);
    if (nullptr != pTemp)
    {
        pd::logToConsole("SUCCESS: allocated memory in global new operator: %u", count);
        return pTemp;
    }

    pd::logToConsole("ERROR: failed to allocate memory in global new operator: %u", count);
    return nullptr;
}

// NOTE: not sure if this is desirable
// void* operator new(std::size_t count, std::align_val_t al, const std::nothrow_t&) noexcept
// {
//     const auto pTemp = pd::realloc(nullptr, count + (static_cast<std::size_t>(al) - 1u));
//     if (nullptr != pTemp)
//     {
//         pd::logToConsole("ERROR: failed to allocate aligned memory in global new operator");
//         return nullptr;
//     }

//     return clg::align_pointer<static_cast<std::size_t>(al)>(pTemp);
// }

void operator delete(void* ptr) noexcept
{
    pd::logToConsole("calling global delete");
    if (nullptr == ptr)
    {
        pd::logToConsole("ERROR: global delete operator attempted to free nullptr");
        return;
    }

    pd::realloc(ptr, 0);
}

// NOTE: this is implicitly referenced by virtual destructors
void operator delete(void* ptr, std::size_t) noexcept
{
    pd::logToConsole("calling global delete");
    if (nullptr == ptr)
    {
        pd::logToConsole("ERROR: global delete operator attempted to free nullptr");
        return;
    }

    pd::realloc(ptr, 0);
}

#if TARGET_PLAYDATE
extern "C"
{

typedef struct file_handle_t
{
    char* path;
    SDFile* pdFile;
} file_handle;

const int MaxFileHandles = 64;
file_handle handle_table[MaxFileHandles];
void* __dso_handle = (void*) &__dso_handle;

int get_next_empty_file_handle_index()
{
    int i = 0;
    while (i < MaxFileHandles && nullptr != handle_table[i].pdFile)
    {
        i++;
    }

    if (i == MaxFileHandles)
    {
        return -1;
    }

    return i;
}

// returns information about a file
int _fstat(int fd, struct stat *buffer)
{
    pd::logToConsole("_fstat() called");
    memset(buffer, 0, sizeof(struct stat));
    assert(fd >= 0 && fd < MaxFileHandles);
    const auto path = handle_table[fd].path;
    FileStat fileStat = {0};
    const auto rval = pd::stat(path, &fileStat);
    buffer->st_size = fileStat.size;

    std::tm tm{};  // zero initialise
    tm.tm_year = fileStat.m_year - 1970; // TODO: verify playdate time_t is 1970-based
    tm.tm_mon = fileStat.m_month - 1; // playdate is one-based, time_t is zero-based
    tm.tm_mday = fileStat.m_day;
    tm.tm_hour = fileStat.m_hour;
    tm.tm_min = fileStat.m_minute;
    tm.tm_isdst = 0; // Not daylight saving
    std::time_t t = std::mktime(&tm);

    timespec ts;
    ts.tv_sec = t;
    ts.tv_nsec = 0;
    buffer->st_atim = ts;
    buffer->st_mtim = ts;
    buffer->st_ctim = ts;

    return rval;
}

int _open(const char* pathname, int flags, mode_t mode)
{
    pd::logToConsole("_open() called");
    FileOptions fileOptions = {};
    if (0 != (O_RDWR & flags))
    {
        fileOptions = kFileReadData;
    }
    else if (0 != (O_RDONLY & flags))
    {
        fileOptions = static_cast<FileOptions>(kFileRead | kFileReadData);
    }
    else if (0 != (O_WRONLY & flags))
    {
        fileOptions = kFileAppend;
    }

    if (0 != (O_CREAT & flags) || 0 != (O_TRUNC & flags))
    {
        fileOptions = static_cast<FileOptions>(fileOptions | kFileWrite);
    }

    if (0 != (O_APPEND & flags))
    {
        fileOptions = static_cast<FileOptions>(fileOptions | kFileAppend);
    }

    const auto rval = pd::open(pathname, fileOptions);
    if (nullptr == rval)
    {
        const auto err_msg = pd::geterr();
        pd::error(err_msg);
        return -1;
    }

    const auto fhIndex = get_next_empty_file_handle_index();
    auto& hFile = handle_table[fhIndex];
    const auto len = strlen(pathname);
    hFile.path = static_cast<char*>(pd::realloc(nullptr, len + 1)); // copy pathname
    if (nullptr == hFile.path)
    {
        pd::close(rval);
        return -1;
    }

    strcpy(hFile.path, pathname);
    hFile.pdFile = rval;
    return fhIndex;
}

int _close(int fd)
{
    pd::logToConsole("_close() called");
    assert(fd >= 0 && fd < MaxFileHandles);
    auto& hFile = handle_table[fd];
    const auto result = pd::close(hFile.pdFile);
    pd::realloc(hFile.path, 0);
    hFile.pdFile = nullptr;
    return result;
}

int _read(int fd, char* buffer, int buffer_size)
{
    pd::logToConsole("_read() called");
    pd::read(handle_table[fd].pdFile, buffer, static_cast<unsigned int>(buffer_size));
    return 0;
}

int _write(int fd, char* buffer, int count)
{
    pd::logToConsole("_write() called");
    pd::write(handle_table[fd].pdFile, buffer, static_cast<unsigned int>(count));
    return 0;
}

long _lseek(int fd, int offset, int origin)
{
    pd::logToConsole("_lseek() called");
    pd::seek(handle_table[fd].pdFile, offset, origin);
    return 0;
}

// test whether a file descriptor refers to a terminal
int _isatty(int fd)
{
    pd::logToConsole("_isatty() called");
    return 0;
}

void _fini()
{
    pd::logToConsole("_fini() called");
}

void _exit(const int status)
{
   while (1)
   {
       pd::error("exited with code %d.", status);
   }
}

int _getpid(void)
{
   pd::logToConsole("_getpid() called");
   return 1;
}

int _kill(int pid, int sig)
{
   pd::error("_kill() called %d, %d", pid, sig);
   return 0;
}

} // extern "C"
#endif // TARGET_PLAYDATE
