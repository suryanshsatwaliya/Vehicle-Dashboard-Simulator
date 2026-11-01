#ifndef PLATFORM_H
#define PLATFORM_H

/* Thin cross-platform layer: raw keyboard input, sleeping, ANSI setup. */
void platform_init(void);
void platform_shutdown(void);
int  platform_kbhit(void);        /* non-zero if a key is waiting          */
int  platform_getch(void);        /* read one key (arrows mapped to wasd)  */
void platform_sleep_ms(int ms);
int  platform_interrupted(void);  /* non-zero after Ctrl+C                 */

#endif
