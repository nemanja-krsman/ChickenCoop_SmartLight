# 🐔 Smart Chicken Coop Lighting System

An ESP32-based, standalone lighting controller featuring **two independent AC dimming channels**, programmable lighting schedules, and gradual sunrise/sunset transitions.

## 💡 Background & Motivation

A consistent daily light-dark cycle is important for poultry welfare. The appropriate lighting schedule depends on the type and age of the birds, as well as the management system. Gradual changes in light intensity can help chickens adjust more naturally to transitions between light and darkness.

To address this need, I developed an automated lighting controller designed for a small-scale chicken coop. The system provides programmable lighting schedules and smooth brightness transitions without relying on an internet connection.

Commercial smart lighting solutions may present challenges for small-scale installations, including:

- **Internet dependency:** Reliable Wi-Fi connectivity may not be available in or around a chicken coop.
- **Cost:** Commercial automation solutions may be unnecessarily expensive for small-scale setups.

This project provides a cost-effective, standalone solution built around an ESP32 microcontroller and a custom dual-channel AC dimmer circuit.

## 📚 Project Documentation

- [Hardware overview](doc/HardwareOverview.md) — system architecture, circuit details, prototype components, installation photos, and safety information.
- [Software flow](doc/SoftwareFlow.md) — diagrams of startup, scheduled dimming, and Bluetooth command handling.

## 💻 Firmware Overview & Configuration

The firmware is designed to operate independently of external internet connectivity. It combines time-based scheduling, RTC communication, zero-cross synchronization, and two-channel brightness control.

### 📡 Bluetooth Configuration

The system supports Bluetooth Classic and Bluetooth Low Energy (BLE) for initial setup and device configuration.

The preferred communication mode can be selected at compile time using a preprocessor definition:

```cpp
#define USE_BLE
```

This allows the system to be configured from a compatible smartphone application or a Bluetooth serial terminal, depending on the selected implementation.

### 🕒 Setting the Current Time

The RTC time can be updated by sending a four-digit command in `hhmm` format.

**Example command:**

```text
1400
```

**Result:** Sets the RTC time to 14:00 (2:00 PM).

### 💡 Configuring Lighting Schedules — L1 / L2

Each lighting channel has its own schedule.

**Command format:**

```text
[Lamp][Full-Brightness Time];[Sunrise Duration];[Dimming Start Time];[Sunset Duration]
```

**Example command:**

```text
L12045;5;2130;10
```

This command configures Lamp 1 as follows:

| Parameter | Value | Description |
|---|---|---|
| Lamp | `L1` | Selects lighting channel 1 |
| Full-brightness time | `2045` | The lamp reaches full brightness at 20:45 |
| Sunrise duration | `5` | Gradual turn-on begins five minutes earlier, at 20:40 |
| Dimming start time | `2130` | Gradual turn-off begins at 21:30 |
| Sunset duration | `10` | The lamp reaches zero brightness at 21:40 |

The same command structure can be used to configure the second lighting channel by selecting `L2`.

The exact command syntax and timing behavior are determined by the firmware implementation.

## ⚠️ Safety

**This project involves hazardous mains voltage (230 V AC).** The design is for educational and engineering reference only, not a substitute for a verified, compliant electrical design. Read the [hardware overview](HardwareOverview.md#safety-warning) and have mains-voltage work reviewed or performed by a qualified person.
