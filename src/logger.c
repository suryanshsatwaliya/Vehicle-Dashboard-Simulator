#include <stdio.h>
#include <time.h>
#include "logger.h"

int logger_save_trip(const Vehicle *v, const char *path)
{
    FILE *f = fopen(path, "a");
    time_t now = time(NULL);
    char ts[32];
    int secs = (int)v->trip_time;
    double avg_speed = v->trip_time > 0.0 ? v->trip / (v->trip_time / 3600.0) : 0.0;
    double avg_cons  = v->trip > 0.05 ? v->trip_fuel / v->trip * 100.0 : 0.0;

    if (!f) return -1;
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(f, "==== Trip log: %s ====\n", ts);
    fprintf(f, "Distance       : %.2f km\n", v->trip);
    fprintf(f, "Drive time     : %02d:%02d:%02d\n", secs / 3600, (secs / 60) % 60, secs % 60);
    fprintf(f, "Average speed  : %.1f km/h\n", avg_speed);
    fprintf(f, "Top speed      : %.0f km/h\n", v->top_speed);
    fprintf(f, "Fuel used      : %.2f L\n", v->trip_fuel);
    fprintf(f, "Avg consumption: %.1f L/100km\n", avg_cons);
    fprintf(f, "Fuel remaining : %.1f L\n", v->fuel);
    fprintf(f, "Odometer       : %.1f km\n\n", v->odometer);

    fclose(f);
    return 0;
}
