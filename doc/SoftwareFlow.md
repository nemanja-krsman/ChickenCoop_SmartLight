# Chicken Coop Smart Light — Software Flow

This page illustrates how the ESP32 firmware starts, updates the lamps, and
processes Bluetooth commands. The diagrams use Mermaid and render in Markdown
viewers that support Mermaid, such as GitHub and the VS Code Markdown preview.

## Overall program flow

```mermaid
flowchart TD
    A([Power on]) --> B[setup]
    B --> C[Start serial and BLE]
    C --> D[Start RTC]
    D --> E[Initialize both dimmers at 0%]
    E --> F{{Arduino calls loop repeatedly}}
    F --> G[Check for Bluetooth commands]
    G --> H[Read current time from RTC]
    H --> I{Second changed?}
    I -- No --> F
    I -- Yes --> J[Calculate lamp 1 power]
    J --> K[Calculate lamp 2 power]
    K --> L[Apply power to both dimmers]
    L --> M[Print time and power to Serial]
    M --> F
```

`ChickenCoop.ino` contains the Arduino `loop()` entry point. It calls
`runChickenCoopLoop()` in `ChickenCoop.cpp`. Bluetooth is checked on each loop
iteration; lamp power is recalculated when the RTC second changes.

## How a lamp's power is calculated

The same schedule calculation is performed independently for each lamp. The
start and end points are calculated from that lamp's configuration.

```mermaid
flowchart TD
    A[Current RTC time and lamp schedule] --> B{Inside sunrise interval?}
    B -- Yes --> C[Increase power linearly from 0% to 100%]
    B -- No --> D{Inside full-power interval?}
    D -- Yes --> E[Set power to 100%]
    D -- No --> F{Inside sunset interval?}
    F -- Yes --> G[Decrease power linearly from 100% to 0%]
    F -- No --> H[Set power to 0%]
    C --> I[Apply result to dimmer]
    E --> I
    G --> I
    H --> I
```

Intervals that cross midnight are supported. A zero-power result turns the
dimmer off; a positive result turns it on and sets its power level.

## Bluetooth command flow

BLE is selected by default with `USE_BLE`. The alternate build path uses
Bluetooth Classic serial.

```mermaid
flowchart TD
    A[Phone or Bluetooth client sends command] --> B{Bluetooth mode}
    B -- BLE --> C[BLE write callback collects characters]
    C --> D[Newline or carriage return completes command]
    B -- Classic --> E[Read input through newline]
    D --> F[handleBluetoothCommands]
    E --> F
    F --> G[processIncomingCommand]
    G --> H{Command format}
    H -- HHMM --> I[Set RTC hour and minute]
    H -- L1 or L2 schedule --> J[Validate times and durations]
    J --> K[Update selected lamp configuration]
    I --> L[Send response to Serial and Bluetooth]
    K --> L
```

### Supported command formats

| Command | Example | Effect |
| --- | --- | --- |
| `HHMM` | `1430` | Set the RTC time to 14:30, preserving its date and setting seconds to zero. |
| `L1HHMM;SUNRISE;HHMM;SUNSET` | `L12030;15;2045;15` | Set lamp 1's full-power start, sunrise duration, ramp-down start, and sunset duration. |
| `L2HHMM;SUNRISE;HHMM;SUNSET` | `L22100;15;2115;15` | Set the same schedule fields for lamp 2. |

Times use 24-hour `HHMM` format, and durations are in minutes. Updated lamp
settings are held in memory and return to their defaults after a restart.
