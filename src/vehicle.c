#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "vehicle.h"

#define IDLE_RPM  800.0
#define MAX_RPM   7500.0
#define REDLINE   6500.0

/* km/h per 1000 rpm for gears 1..5 */
static const double KMH_PER_KRPM[6] = { 0.0, 8.0, 15.0, 24.0, 34.0, 45.0 };

static double clampd(double x, double lo, double hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

static double mode_accel(DriveMode m)
{
    switch (m) { case MODE_ECO: return 7.0; case MODE_SPORT: return 16.0; default: return 11.0; }
}
static double mode_top_speed(DriveMode m)
{
    switch (m) { case MODE_ECO: return 140.0; case MODE_SPORT: return 220.0; default: return 180.0; }
}
static double mode_fuel_factor(DriveMode m)
{
    switch (m) { case MODE_ECO: return 0.8; case MODE_SPORT: return 1.35; default: return 1.0; }
}
static double mode_shift_scale(DriveMode m)
{
    switch (m) { case MODE_ECO: return 0.85; case MODE_SPORT: return 1.25; default: return 1.0; }
}

static int gear_for_speed(double speed, DriveMode m)
{
    double s = speed / mode_shift_scale(m);
    if (s < 25.0)  return 1;
    if (s < 45.0)  return 2;
    if (s < 70.0)  return 3;
    if (s < 100.0) return 4;
    return 5;
}

const char *vehicle_mode_name(DriveMode m)
{
    switch (m) { case MODE_ECO: return "ECO"; case MODE_SPORT: return "SPORT"; default: return "NORMAL"; }
}

void vehicle_message(Vehicle *v, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(v->message, MSG_LEN, fmt, ap);
    va_end(ap);
    v->message_ticks = 30;
}

void vehicle_init(Vehicle *v)
{
    memset(v, 0, sizeof *v);
    v->fuel_capacity = 45.0;
    v->fuel = 35.0;
    v->temp = 30.0;
    v->battery = 12.6;
    v->odometer = 12345.6;
    v->mode = MODE_NORMAL;
    v->seatbelt = 0;
    vehicle_message(v, "Welcome! Press I for ignition, then E to start the engine.");
}

/* ------------------------------ simulation ------------------------------ */

void vehicle_update(Vehicle *v, double dt)
{
    double sdt = dt * SIM_TIME_SCALE;
    double accel = 0.0, decel = 0.0, top, target_rpm, target_temp, rate = 0.0, used, dist;
    int braking;

    v->tick++;
    v->blink = (int)((v->tick / 5) % 2);
    if (v->message_ticks > 0) v->message_ticks--;
    braking = v->brake_ticks > 0;
    if (v->brake_ticks > 0) v->brake_ticks--;

    /* out of fuel */
    if (v->engine_on && v->fuel <= 0.0) {
        v->fuel = 0.0;
        v->engine_on = 0;
        v->throttle = 0.0;
        vehicle_message(v, "Engine stalled: out of fuel!");
    }

    /* speed */
    top = mode_top_speed(v->mode);
    if (v->engine_on && !v->handbrake) {
        accel = v->throttle * mode_accel(v->mode) * (1.0 - v->speed / top);
        if (v->gear >= 1) accel *= (1.6 - 0.2 * v->gear);
        if (accel < 0.0) accel = 0.0;
    }
    if (v->speed > 0.0) {
        decel = 0.4 + v->speed * v->speed * 0.00008;
        if (v->engine_on && v->throttle < 0.05) decel += 0.8;   /* engine braking */
        if (braking)      decel += 28.0;
        if (v->handbrake) decel += 12.0;
    }
    v->speed = clampd(v->speed + (accel - decel) * dt, 0.0, 250.0);
    if (v->speed < 0.3 && accel <= 0.0) v->speed = 0.0;

    /* distance / trip */
    dist = v->speed * sdt / 3600.0;
    v->odometer += dist;
    v->trip += dist;
    if (v->engine_on) v->trip_time += sdt;
    if (v->speed > v->top_speed) v->top_speed = v->speed;

    /* gearbox + rpm */
    if (v->engine_on) {
        if (v->speed < 1.0 && v->throttle < 0.05) v->gear = 0;
        else v->gear = gear_for_speed(v->speed, v->mode);

        if (v->gear == 0) {
            target_rpm = IDLE_RPM + v->throttle * 3000.0;
        } else {
            target_rpm = v->speed / KMH_PER_KRPM[v->gear] * 1000.0 + v->throttle * 2500.0;
            if (target_rpm < IDLE_RPM) target_rpm = IDLE_RPM;
        }
        target_rpm = clampd(target_rpm, 0.0, MAX_RPM);
    } else {
        v->gear = 0;
        target_rpm = 0.0;
    }
    v->rpm += (target_rpm - v->rpm) * 0.3;
    if (!v->engine_on && v->rpm < 20.0) v->rpm = 0.0;

    /* fuel */
    if (v->engine_on) {
        rate = (0.8 + v->throttle * 7.0 + v->speed * 0.025) * mode_fuel_factor(v->mode);
        if (v->throttle < 0.05 && v->speed > 20.0) rate = 0.3;  /* fuel cut-off */
    }
    used = rate * sdt / 3600.0;
    v->fuel -= used;
    if (v->fuel < 0.0) v->fuel = 0.0;
    v->trip_fuel += used;
    v->inst_lph = rate;
    v->inst_l100 = v->speed > 3.0 ? rate / v->speed * 100.0 : 0.0;

    /* temperature */
    if (v->engine_on) {
        target_temp = 88.0 + (v->rpm - IDLE_RPM) / 7000.0 * 25.0 - v->speed * 0.05;
        if (v->speed < 20.0 && v->throttle > 0.5) target_temp += 8.0;
        v->temp += (target_temp - v->temp) * 0.02;
    } else {
        v->temp += (25.0 - v->temp) * 0.004;
    }

    /* battery */
    if (v->engine_on) {
        v->battery += (14.1 - v->battery) * 0.5 * dt;
    } else {
        double drain = 0.0;
        if (v->ignition)   drain += 0.010;
        if (v->headlights) drain += 0.010;
        if (v->hazard)     drain += 0.005;
        v->battery -= drain * dt;
        if (v->battery < 9.0) v->battery = 9.0;
    }
}

/* ------------------------------ controls ------------------------------ */

void vehicle_toggle_ignition(Vehicle *v)
{
    if (v->engine_on) { vehicle_message(v, "Stop the engine first."); return; }
    v->ignition = !v->ignition;
    vehicle_message(v, v->ignition ? "Ignition ON" : "Ignition OFF");
}

void vehicle_toggle_engine(Vehicle *v)
{
    if (v->engine_on) {
        if (v->speed > 5.0) { vehicle_message(v, "Cannot stop engine while moving!"); return; }
        v->engine_on = 0;
        v->throttle = 0.0;
        vehicle_message(v, "Engine stopped");
        return;
    }
    if (!v->ignition)       { vehicle_message(v, "Turn the ignition on first (I)."); return; }
    if (v->fuel <= 0.0)     { vehicle_message(v, "No fuel! Refuel with R."); return; }
    if (v->battery < 10.5)  { vehicle_message(v, "Battery too low to start the engine."); return; }
    v->engine_on = 1;
    v->battery -= 0.6;      /* cranking dip */
    vehicle_message(v, "Engine started");
}

void vehicle_adjust_throttle(Vehicle *v, double delta)
{
    if (!v->engine_on) { vehicle_message(v, "Engine is off."); return; }
    v->throttle = clampd(v->throttle + delta, 0.0, 1.0);
}

void vehicle_brake(Vehicle *v)
{
    v->throttle = 0.0;
    v->brake_ticks = 3;
}

void vehicle_toggle_left(Vehicle *v)
{
    v->left_ind = !v->left_ind;
    if (v->left_ind) v->right_ind = 0;
    vehicle_message(v, v->left_ind ? "Left indicator ON" : "Left indicator OFF");
}

void vehicle_toggle_right(Vehicle *v)
{
    v->right_ind = !v->right_ind;
    if (v->right_ind) v->left_ind = 0;
    vehicle_message(v, v->right_ind ? "Right indicator ON" : "Right indicator OFF");
}

void vehicle_toggle_hazard(Vehicle *v)
{
    v->hazard = !v->hazard;
    vehicle_message(v, v->hazard ? "Hazard lights ON" : "Hazard lights OFF");
}

void vehicle_toggle_headlights(Vehicle *v)
{
    v->headlights = !v->headlights;
    if (!v->headlights) v->high_beam = 0;
    vehicle_message(v, v->headlights ? "Headlights ON" : "Headlights OFF");
}

void vehicle_toggle_highbeam(Vehicle *v)
{
    if (!v->headlights) { vehicle_message(v, "Turn headlights on first (H)."); return; }
    v->high_beam = !v->high_beam;
    vehicle_message(v, v->high_beam ? "High beam ON" : "High beam OFF");
}

void vehicle_toggle_handbrake(Vehicle *v)
{
    v->handbrake = !v->handbrake;
    vehicle_message(v, v->handbrake ? "Handbrake engaged" : "Handbrake released");
}

void vehicle_toggle_seatbelt(Vehicle *v)
{
    v->seatbelt = !v->seatbelt;
    vehicle_message(v, v->seatbelt ? "Seatbelt fastened" : "Seatbelt unfastened");
}

void vehicle_toggle_door(Vehicle *v)
{
    v->door_open = !v->door_open;
    vehicle_message(v, v->door_open ? "Door opened" : "Door closed");
}

void vehicle_cycle_mode(Vehicle *v)
{
    v->mode = (DriveMode)((v->mode + 1) % 3);
    vehicle_message(v, "Drive mode: %s", vehicle_mode_name(v->mode));
}

void vehicle_refuel(Vehicle *v)
{
    if (v->engine_on || v->speed > 0.0) { vehicle_message(v, "Stop the vehicle and engine to refuel."); return; }
    v->fuel = v->fuel_capacity;
    vehicle_message(v, "Tank filled to %.0f L", v->fuel_capacity);
}

void vehicle_reset_trip(Vehicle *v)
{
    v->trip = 0.0;
    v->trip_time = 0.0;
    v->trip_fuel = 0.0;
    v->top_speed = v->speed;
    vehicle_message(v, "Trip computer reset");
}

/* ------------------------------ derived data ------------------------------ */

double vehicle_range_km(const Vehicle *v)
{
    double cons = (v->trip > 1.0 && v->trip_fuel > 0.0) ? v->trip_fuel / v->trip * 100.0 : 7.0;
    return v->fuel / cons * 100.0;
}

int vehicle_get_warnings(const Vehicle *v, Warning *out, int max)
{
    int n = 0;
    double fuel_frac = v->fuel / v->fuel_capacity;

#define ADD(txt, lvl) do { if (n < max) { out[n].text = (txt); out[n].level = (lvl); n++; } } while (0)

    if (fuel_frac < 0.05)       ADD("FUEL CRITICAL - refuel immediately", 2);
    else if (fuel_frac < 0.15)  ADD("LOW FUEL", 1);

    if (v->temp > 105.0)        ADD("ENGINE OVERHEATING - ease off the throttle", 2);

    if (v->battery < 11.0)      ADD("BATTERY CRITICAL", 2);
    else if (v->battery < 11.8) ADD("LOW BATTERY", 1);

    if (v->rpm > REDLINE)       ADD("REDLINE - reduce throttle", 2);
    if (v->speed > SPEED_LIMIT) ADD("SPEED LIMIT EXCEEDED", 1);

    if (!v->seatbelt && v->engine_on) {
        if (v->speed > 10.0) ADD("FASTEN SEATBELT", 2);
        else                 ADD("Seatbelt not fastened", 1);
    }
    if (v->door_open && v->engine_on) {
        if (v->speed > 5.0)  ADD("DOOR OPEN WHILE DRIVING", 2);
        else                 ADD("Door open", 1);
    }
    if (v->handbrake && v->engine_on) {
        if (v->speed > 5.0)  ADD("HANDBRAKE ENGAGED WHILE DRIVING", 2);
        else                 ADD("Handbrake engaged", 1);
    }
#undef ADD
    return n;
}
