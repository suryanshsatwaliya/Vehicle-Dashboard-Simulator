#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "platform.h"

static volatile sig_atomic_t g_interrupted = 0;
static void on_sigint(int sig) { (void)sig; g_interrupted = 1; }
int platform_interrupted(void) { return g_interrupted; }

#ifdef _WIN32
/* ------------------------------ Windows ------------------------------ */
#include <windows.h>
#include <conio.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static DWORD g_old_mode = 0;
static int   g_mode_saved = 0;

void platform_init(void)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    signal(SIGINT, on_sigint);
    if (GetConsoleMode(out, &mode)) {
        g_old_mode = mode;
        g_mode_saved = 1;
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    printf("\033[2J\033[H\033[?25l");
    fflush(stdout);
}

void platform_shutdown(void)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (g_mode_saved) SetConsoleMode(out, g_old_mode);
    printf("\033[?25h\033[0m\n");
    fflush(stdout);
}

int platform_kbhit(void) { return _kbhit(); }

int platform_getch(void)
{
    int c = _getch();
    if (c == 0 || c == 224) {
        int k = _getch();
        switch (k) {
            case 72: return 'w';
            case 80: return 's';
            case 75: return 'z';
            case 77: return 'c';
        }
        return 0;
    }
    return c;
}

void platform_sleep_ms(int ms) { Sleep((DWORD)ms); }

#else
/* ------------------------- Linux / macOS (POSIX) ------------------------- */
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

static struct termios g_saved;
static int g_raw = 0;

void platform_init(void)
{
    struct termios t;
    signal(SIGINT, on_sigint);
    if (tcgetattr(STDIN_FILENO, &g_saved) == 0) {
        t = g_saved;
        t.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        g_raw = 1;
    }
    printf("\033[2J\033[H\033[?25l");
    fflush(stdout);
}

void platform_shutdown(void)
{
    if (g_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_saved);
        g_raw = 0;
    }
    printf("\033[?25h\033[0m\n");
    fflush(stdout);
}

int platform_kbhit(void)
{
    fd_set fds;
    struct timeval tv = {0, 0};
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

int platform_getch(void)
{
    unsigned char c, s;
    if (read(STDIN_FILENO, &c, 1) != 1) return 0;
    if (c == 27) {                         /* escape sequence (arrow keys) */
        if (platform_kbhit() && read(STDIN_FILENO, &s, 1) == 1 && s == '[') {
            if (platform_kbhit() && read(STDIN_FILENO, &s, 1) == 1) {
                switch (s) {
                    case 'A': return 'w';
                    case 'B': return 's';
                    case 'C': return 'c';
                    case 'D': return 'z';
                }
            }
        }
        return 0;
    }
    return c;
}

void platform_sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}
#endif
