#ifndef VEHICLE_H
#define VEHICLE_H

/* Distance, fuel and trip time run this many times faster than real time,
 * so a demo shows meaningful fuel use in a few minutes. Set to 1.0 for
 * "real" behaviour. */
#define SIM_TIME_SCALE 10.0
#define SPEED_LIMIT    100.0
#define MAX_WARNINGS   8
#define MSG_LEN        80

typedef enum { MODE_ECO = 0, MODE_NORMAL, MODE_SPORT } DriveMode;

typedef struct {
    /* power state */
    int    ignition;
    int    engine_on;

    /* driving data */
    double speed;          /* km/h */
    double rpm;
    int    gear;           /* 0 = neutral, 1..5 */
    double throttle;       /* 0.0 .. 1.0 */
    int    brake_ticks;    /* >0 while brake is held */
    DriveMode mode;

    /* resources */
    double fuel;           /* litres */
    double fuel_capacity;
    double temp;           /* engine temperature, deg C */
    double battery;        /* volts */

    /* trip computer */
    double odometer;       /* km */
    double trip;           /* km */
    double trip_time;      /* simulated seconds */
    double trip_fuel;      /* litres used on trip */
    double top_speed;
    double inst_lph;       /* litres per hour */
    double inst_l100;      /* litres per 100 km */

    /* switches */
    int left_ind, right_ind, hazard;
    int headlights, high_beam;
    int seatbelt, door_open, handbrake;

    /* UI helpers */
    unsigned long tick;
    int  blink;
    char message[MSG_LEN];
    int  message_ticks;
} Vehicle;

typedef struct {
    const char *text;
    int level;             /* 1 = caution (yellow), 2 = critical (red) */
} Warning;

void        vehicle_init(Vehicle *v);
void        vehicle_update(Vehicle *v, double dt);
void        vehicle_message(Vehicle *v, const char *fmt, ...);
const char *vehicle_mode_name(DriveMode m);
double      vehicle_range_km(const Vehicle *v);
int         vehicle_get_warnings(const Vehicle *v, Warning *out, int max);

void vehicle_toggle_ignition(Vehicle *v);
void vehicle_toggle_engine(Vehicle *v);
void vehicle_adjust_throttle(Vehicle *v, double delta);
void vehicle_brake(Vehicle *v);
void vehicle_toggle_left(Vehicle *v);
void vehicle_toggle_right(Vehicle *v);
void vehicle_toggle_hazard(Vehicle *v);
void vehicle_toggle_headlights(Vehicle *v);
void vehicle_toggle_highbeam(Vehicle *v);
void vehicle_toggle_handbrake(Vehicle *v);
void vehicle_toggle_seatbelt(Vehicle *v);
void vehicle_toggle_door(Vehicle *v);
void vehicle_cycle_mode(Vehicle *v);
void vehicle_refuel(Vehicle *v);
void vehicle_reset_trip(Vehicle *v);

#endif
