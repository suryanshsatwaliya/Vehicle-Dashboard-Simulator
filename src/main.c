#include <ctype.h>
#include <stdio.h>
#include "vehicle.h"
#include "dashboard.h"
#include "logger.h"
#include "platform.h"

#define TICK_MS  100
#define LOG_FILE "trip_log.txt"

/* returns 0 when the user asks to quit */
static int handle_key(Vehicle *v, int c)
{
    switch (tolower(c)) {
        case 'i': vehicle_toggle_ignition(v);        break;
        case 'e': vehicle_toggle_engine(v);          break;
        case 'w': vehicle_adjust_throttle(v, 0.1);   break;
        case 's': vehicle_adjust_throttle(v, -0.1);  break;
        case ' ': vehicle_brake(v);                  break;
        case 'z': vehicle_toggle_left(v);            break;
        case 'c': vehicle_toggle_right(v);           break;
        case 'x': vehicle_toggle_hazard(v);          break;
        case 'h': vehicle_toggle_headlights(v);      break;
        case 'u': vehicle_toggle_highbeam(v);        break;
        case 'p': vehicle_toggle_handbrake(v);       break;
        case 'b': vehicle_toggle_seatbelt(v);        break;
        case 'd': vehicle_toggle_door(v);            break;
        case 'm': vehicle_cycle_mode(v);             break;
        case 'r': vehicle_refuel(v);                 break;
        case 't': vehicle_reset_trip(v);             break;
        case 'l':
            if (logger_save_trip(v, LOG_FILE) == 0)
                vehicle_message(v, "Trip saved to %s", LOG_FILE);
            else
                vehicle_message(v, "Could not write %s", LOG_FILE);
            break;
        case 'q': return 0;
        default: break;
    }
    return 1;
}

int main(void)
{
    Vehicle v;
    int running = 1;

    vehicle_init(&v);
    platform_init();

    dashboard_splash();
    while (!platform_kbhit() && !platform_interrupted())
        platform_sleep_ms(50);
    if (platform_kbhit()) platform_getch();

    while (running && !platform_interrupted()) {
        while (platform_kbhit() && running)
            running = handle_key(&v, platform_getch());

        vehicle_update(&v, TICK_MS / 1000.0);
        dashboard_render(&v);
        platform_sleep_ms(TICK_MS);
    }

    platform_shutdown();

    dashboard_summary(&v);
    if (v.trip > 0.1 && logger_save_trip(&v, LOG_FILE) == 0)
        printf("  Trip saved to %s\n\n", LOG_FILE);
    printf("  Thanks for driving. Goodbye!\n\n");
    return 0;
}
