#include <sys/stat.h>
#include <unistd.h>

int _close(int file) {
    return -1; // Return -1 to indicate failure for unsupported operations
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR; // Character device (like UART)
    return 0;
}

int _getpid(void) {
    return 1; // Arbitrary positive number (1) since there's no actual process ID
}

int _isatty(int file) {
    return 1; // Always return 1, indicating output is to a terminal (e.g., UART)
}

int _kill(int pid, int sig) {
    return -1; // Return -1 to indicate unsupported operation
}

off_t _lseek(int file, off_t offset, int whence) {
    return 0; // Seek is unsupported, so return 0
}

int _read(int file, char *ptr, int len) {
    return 0; // No actual reading, return 0 (EOF)
}

int _write(int file, const char *ptr, int len) {
    // Stub write function. If you have a UART, you can output here.
    // For now, just return the length to pretend success
    return len;
}