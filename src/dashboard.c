#include <stdio.h>
#include "dashboard.h"

#define RESET  "\033[0m"
#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define RED    "\033[31m"
#define GREEN  "\033[32m"
#define YELLOW "\033[33m"
#define CYAN   "\033[36m"
#define EOL    "\033[K\n"
#define RULE   "------------------------------------------------------------------"
#define DRULE  "=================================================================="

static void bar(double frac, int width, const char *color)
{
    int i, filled;
    if (frac < 0.0) frac = 0.0;
    if (frac > 1.0) frac = 1.0;
    filled = (int)(frac * width + 0.5);
    printf("[%s", color);
    for (i = 0; i < width; i++) {
        if (i == filled) printf(RESET DIM);
        putchar(i < filled ? '#' : '-');
    }
    printf(RESET "]");
}

static void flag(const char *name, const char *state, const char *color)
{
    printf("%s: %s%s" RESET "   ", name, color, state);
}

void dashboard_splash(void)
{
    printf("\033[2J\033[H");
    printf(CYAN DRULE "\n" RESET);
    printf(BOLD CYAN "        VEHICLE DASHBOARD INFORMATION SIMULATOR\n" RESET);
    printf(CYAN DRULE "\n\n" RESET);
    printf("              ______\n");
    printf("         ____//__||__\\\\___\n");
    printf("        /  _          _    \\\n");
    printf("       |__/ \\________/ \\____|\n");
    printf("          \\_/        \\_/\n\n");
    printf("   A terminal based simulator written in C.\n\n");
    printf("   I Ignition   E Engine   W/S Throttle   SPACE Brake\n");
    printf("   Z/C Indicators   X Hazard   H Lights   Q Quit\n\n");
    printf(YELLOW "   Press any key to start..." RESET "\n");
    fflush(stdout);
}

void dashboard_render(const Vehicle *v)
{
    Warning w[MAX_WARNINGS];
    int nw, i, secs;
    const char *col;
    double avg_speed, avg_cons, fuel_frac;
    int left_on, right_on;
    char gear;

    printf("\033[H");
    printf(BOLD CYAN DRULE EOL);
    printf("        VEHICLE DASHBOARD INFORMATION SIMULATOR" EOL);
    printf(DRULE RESET EOL);

    /* ---- ignition off: dark dashboard ---- */
    if (!v->ignition) {
        printf(EOL);
        printf(DIM "                  [ DASHBOARD OFF ]" RESET EOL);
        printf(DIM "              Press I to turn the ignition on" RESET EOL);
        printf(EOL);
        printf(" ODOMETER  %.1f km" EOL, v->odometer);
        printf(" Battery   %.1f V" EOL, v->battery);
        printf(RULE EOL);
        if (v->message_ticks > 0) printf(" > %s" EOL, v->message); else printf(EOL);
        for (i = 0; i < 24; i++) printf(EOL);
        printf("\033[J");
        fflush(stdout);
        return;
    }

    nw = vehicle_get_warnings(v, w, MAX_WARNINGS);
    gear = v->engine_on ? (v->gear == 0 ? 'N' : (char)('0' + v->gear)) : '-';

    printf(" IGNITION: " GREEN "ON" RESET "   ENGINE: %s%-8s" RESET "   MODE: " CYAN "%-6s" RESET "   GEAR: " BOLD "%c" RESET EOL,
           v->engine_on ? GREEN : DIM, v->engine_on ? "RUNNING" : "OFF",
           vehicle_mode_name(v->mode), gear);
    printf(RULE EOL);

    /* ---- speed / rpm / throttle ---- */
    col = v->speed > SPEED_LIMIT + 30.0 ? RED : (v->speed > SPEED_LIMIT ? YELLOW : GREEN);
    printf(" SPEED     " BOLD "%s%5.0f" RESET " km/h  ", col, v->speed);
    bar(v->speed / 220.0, 32, col);
    printf(EOL);

    col = v->rpm > 6500.0 ? RED : (v->rpm > 5000.0 ? YELLOW : GREEN);
    printf(" RPM       " BOLD "%s%5.0f" RESET "        ", col, v->rpm);
    bar(v->rpm / 8000.0, 32, col);
    printf(EOL);

    printf(" THROTTLE  %5.0f%%        ", v->throttle * 100.0);
    bar(v->throttle, 32, CYAN);
    printf("  BRAKE: %s" RESET EOL, v->brake_ticks > 0 ? RED "ON" : DIM "--");
    printf(RULE EOL);

    /* ---- fuel / temp / battery ---- */
    fuel_frac = v->fuel / v->fuel_capacity;
    col = fuel_frac < 0.05 ? RED : (fuel_frac < 0.15 ? YELLOW : GREEN);
    printf(" FUEL      %5.1f L (%3.0f%%)  ", v->fuel, fuel_frac * 100.0);
    bar(fuel_frac, 24, col);
    printf("  Range ~%.0f km" EOL, vehicle_range_km(v));

    col = v->temp > 105.0 ? RED : (v->temp > 98.0 ? YELLOW : (v->temp < 60.0 ? CYAN : GREEN));
    printf(" ENGINE    %5.1f C         ", v->temp);
    bar((v->temp - 20.0) / 100.0, 24, col);
    printf(EOL);

    col = v->battery < 11.0 ? RED : (v->battery < 11.8 ? YELLOW : GREEN);
    printf(" BATTERY   %5.1f V         ", v->battery);
    bar((v->battery - 9.0) / 6.0, 24, col);
    printf(EOL);
    printf(RULE EOL);

    /* ---- indicators & switches ---- */
    left_on  = v->left_ind  || v->hazard;
    right_on = v->right_ind || v->hazard;
    printf(" SIGNALS   %s<===" RESET "      %s[ HAZARD ]" RESET "      %s===>" RESET EOL,
           (left_on && v->blink)  ? BOLD GREEN : DIM,
           (v->hazard && v->blink) ? BOLD RED : DIM,
           (right_on && v->blink) ? BOLD GREEN : DIM);

    printf(" LIGHTS    ");
    flag("HEADLIGHTS", v->headlights ? "ON" : "OFF", v->headlights ? GREEN : DIM);
    flag("HIGH BEAM",  v->high_beam  ? "ON" : "OFF", v->high_beam  ? CYAN  : DIM);
    printf(EOL);

    printf(" STATUS    ");
    flag("HANDBRAKE", v->handbrake ? "ON" : "OFF", v->handbrake ? RED : DIM);
    flag("SEATBELT",  v->seatbelt ? "OK" : "OFF", v->seatbelt ? GREEN : YELLOW);
    flag("DOOR",      v->door_open ? "OPEN" : "CLOSED", v->door_open ? RED : GREEN);
    printf(EOL);
    printf(RULE EOL);

    /* ---- trip computer ---- */
    secs = (int)v->trip_time;
    avg_speed = v->trip_time > 0.0 ? v->trip / (v->trip_time / 3600.0) : 0.0;
    avg_cons  = v->trip > 0.05 ? v->trip_fuel / v->trip * 100.0 : 0.0;

    printf(" ODOMETER  %.1f km     TRIP  %.1f km     TIME  %02d:%02d:%02d" EOL,
           v->odometer, v->trip, secs / 3600, (secs / 60) % 60, secs % 60);
    printf(" AVG SPEED %.1f km/h   TOP SPEED  %.0f km/h" EOL, avg_speed, v->top_speed);
    if (v->speed > 3.0)
        printf(" CONSUMPTION  now %.1f L/100km   trip avg %.1f L/100km" EOL, v->inst_l100, avg_cons);
    else
        printf(" CONSUMPTION  now %.1f L/h        trip avg %.1f L/100km" EOL, v->inst_lph, avg_cons);
    printf(RULE EOL);

    /* ---- warnings (fixed 4 lines) ---- */
    printf(" WARNINGS" EOL);
    for (i = 0; i < 4; i++) {
        if (i < nw) printf("  %s! %s" RESET EOL, w[i].level == 2 ? BOLD RED : YELLOW, w[i].text);
        else        printf(EOL);
    }
    printf(RULE EOL);

    if (v->message_ticks > 0) printf(CYAN " > %s" RESET EOL, v->message);
    else printf(EOL);

    printf(DIM " I ignition  E engine  W/S throttle  SPACE brake  M mode  P handbrake" EOL);
    printf(" Z/C signals  X hazard  H lights  U high beam  B belt  D door" EOL);
    printf(" R refuel  T reset trip  L save log  Q quit" RESET EOL);

    printf("\033[J");
    fflush(stdout);
}

void dashboard_summary(const Vehicle *v)
{
    int secs = (int)v->trip_time;
    double avg_speed = v->trip_time > 0.0 ? v->trip / (v->trip_time / 3600.0) : 0.0;
    double avg_cons  = v->trip > 0.05 ? v->trip_fuel / v->trip * 100.0 : 0.0;

    printf("\n" DRULE "\n");
    printf("  TRIP SUMMARY\n");
    printf(DRULE "\n");
    printf("  Distance      : %.2f km\n", v->trip);
    printf("  Drive time    : %02d:%02d:%02d\n", secs / 3600, (secs / 60) % 60, secs % 60);
    printf("  Average speed : %.1f km/h\n", avg_speed);
    printf("  Top speed     : %.0f km/h\n", v->top_speed);
    printf("  Fuel used     : %.2f L (%.1f L/100km)\n", v->trip_fuel, avg_cons);
    printf("  Fuel left     : %.1f L\n", v->fuel);
    printf("  Odometer      : %.1f km\n", v->odometer);
    printf(DRULE "\n");
}
