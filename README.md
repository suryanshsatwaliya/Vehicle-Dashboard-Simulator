# Vehicle Dashboard Information Simulator

A fully terminal-based vehicle dashboard simulator written in pure C (C99).
It simulates the instrument cluster and trip computer of a car: speedometer,
tachometer, automatic gearbox, fuel, engine temperature, battery, indicators,
lights, warning system and a trip log.

No external libraries are needed. Works on Linux, macOS and Windows
(Windows 10+ terminal, PowerShell, or MinGW).

## Features

- Live speed, RPM and throttle gauges with colour-coded bars
- Automatic 5-speed gearbox with neutral
- Ignition and engine start/stop logic (checks fuel, battery, movement)
- Three drive modes: ECO, NORMAL, SPORT (different acceleration, top speed, fuel use)
- Fuel level, estimated range, instant and average consumption
- Engine temperature model (overheats if pushed hard)
- Battery voltage model (drains with ignition/lights, charges when engine runs)
- Blinking left/right indicators and hazard lights
- Headlights, high beam, handbrake, seatbelt and door status
- Warning panel: low fuel, overheating, low battery, redline, overspeed,
  seatbelt, door open, handbrake engaged while driving
- Trip computer: odometer, trip distance, time, average/top speed
- Trip log saved to `trip_log.txt`

## Build and Run

### Linux / macOS
```
make
./dashboard
```

### Windows (MinGW / gcc)
```
gcc -std=c99 -O2 -o dashboard.exe src/*.c
dashboard.exe
```
Or with `mingw32-make`. Any Windows C compiler works; the code uses
`<windows.h>`/`<conio.h>` automatically on Windows.

Tip: use a terminal at least 70 columns x 35 rows.

## Controls

| Key | Action |
|-----|--------|
| I | Ignition on/off |
| E | Start / stop engine |
| W or Up | Increase throttle |
| S or Down | Decrease throttle |
| SPACE | Brake |
| M | Cycle drive mode (ECO / NORMAL / SPORT) |
| Z or Left | Left indicator |
| C or Right | Right indicator |
| X | Hazard lights |
| H | Headlights |
| U | High beam |
| P | Handbrake |
| B | Seatbelt |
| D | Door open/closed |
| R | Refuel (vehicle must be stopped, engine off) |
| T | Reset trip computer |
| L | Save trip summary to `trip_log.txt` |
| Q | Quit (trip is auto-saved) |

## Quick demo

1. Press **I** (ignition), **B** (fasten seatbelt), **E** (start engine)
2. Press **W** a few times to build speed; watch the gears shift automatically
3. Press **M** for SPORT mode and push the RPM towards the redline
4. Press **Z**, **X**, **H** to see indicators, hazards and lights
5. Press **SPACE** to brake, **Q** to quit and see the trip summary

## Project Structure

```
vehicle-dashboard-simulator/
├── Makefile
├── README.md
└── src/
    ├── main.c        main loop and key handling
    ├── vehicle.c/.h  vehicle state, physics, warnings (simulation logic)
    ├── dashboard.c/.h terminal rendering (ANSI colours)
    ├── logger.c/.h   trip log file writer
    └── platform.c/.h keyboard input / sleep / ANSI setup (Windows + POSIX)
```

## How it works

- The simulation runs at 10 ticks per second (`TICK_MS` in `main.c`).
- Each tick: read keys -> `vehicle_update()` -> `dashboard_render()`.
- Distance, fuel and trip time run **10x faster** than real time
  (`SIM_TIME_SCALE` in `vehicle.h`) so fuel use is visible in a short demo.
  Set it to `1.0` for real-time behaviour.
- The screen is redrawn in place using ANSI escape codes (no flicker).

## Customising

- Tank size, starting fuel, odometer: `vehicle_init()` in `vehicle.c`
- Speed limit warning: `SPEED_LIMIT` in `vehicle.h`
- Gear ratios and shift points: `KMH_PER_KRPM` and `gear_for_speed()` in `vehicle.c`
- Mode behaviour: the `mode_*` functions in `vehicle.c`

## Possible extensions

- Reverse gear and manual gearbox
- Random road events (traffic, hills)
- Save/load vehicle state
- ncurses UI for a fancier layout

## License

Free to use for learning and academic projects.
