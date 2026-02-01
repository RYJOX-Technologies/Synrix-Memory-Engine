#ifdef _WIN32
#include <windows.h>
#include <io.h>

// Windows compatibility functions
int fsync(int fd) {
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE) return -1;
    return FlushFileBuffers(hFile) ? 0 : -1;
}
#endif
