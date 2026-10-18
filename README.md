# 🐔 Smart Chicken Coop Lighting System

An ESP32-based, standalone lighting controller featuring **two independent AC dimming channels**, programmable lighting schedules, and gradual sunrise/sunset transitions.

## 💡 Background & Motivation

A consistent daily light-dark cycle is important for poultry welfare. The appropriate lighting schedule depends on the type and age of the birds, as well as the management system. Gradual changes in light intensity can help chickens adjust more naturally to transitions between light and darkness.

To address this need, I developed an automated lighting controller designed for a small-scale chicken coop. The system provides programmable lighting schedules and smooth brightness transitions without relying on an internet connection.

Commercial smart lighting solutions may present challenges for small-scale installations, including:

- **Internet dependency:** Reliable Wi-Fi connectivity may not be available in or around a chicken coop.
- **Cost:** Commercial automation solutions may be unnecessarily expensive for small-scale setups.

This project provides a cost-effective, standalone solution built around an ESP32 microcontroller and a custom dual-channel AC dimmer circuit.

## 📐 System Architecture

The following block diagram illustrates the overall system architecture.

<img width="667" height="448" alt="System architecture block diagram" src="https://github.com/user-attachments/assets/100bcef6-561d-4301-851b-0eca61856f04" />

The system consists of four primary functional blocks:

- **Real-Time Clock (RTC):** Maintains the time used for scheduled lighting operations, including during power outages when supported by its backup battery.
- **ESP32 Microcontroller:** Executes the scheduling logic, processes zero-cross detection signals, and controls the timing of triac firing pulses.
- **Zero-Cross Detection Circuit:** Provides synchronization signals corresponding to the AC voltage zero crossings, allowing the ESP32 to synchronize its control logic with the mains waveform.
- **Dual-Channel AC Dimmer:** Independently controls the brightness of two connected lighting loads through phase-angle control.

## 🛠️ Hardware Design & Circuit Breakdown

### 1. Zero-Cross Detection Circuit

<img width="1206" height="494" alt="Zero-cross detection circuit schematic" src="https://github.com/user-attachments/assets/930e057e-5b9e-47b7-80ae-229bc3ae8983" />

The zero-cross detection circuit synchronizes the ESP32 with the AC mains waveform.

- **Rectification:** A DF1504S-T bridge rectifier processes the AC input for the detection circuit.
- **Optoisolation:** A 4N25 optocoupler provides electrical isolation between the mains-side detection circuit and the low-voltage control circuitry. Two 120 kΩ resistors connected in parallel limit the current through the optocoupler's input LED.
- **Signal generation:** The optocoupler's output transistor switches according to the detected mains waveform. The ESP32 reads the resulting signal through a GPIO input configured for interrupt-based synchronization.

The firmware uses these synchronization events to determine when to trigger the triacs during each AC half-cycle. The resulting firing delay determines the amount of power delivered to each compatible lighting load.

### 2. Real-Time Clock (RTC) — DS1307

<img width="732" height="534" alt="DS1307 real-time clock schematic" src="https://github.com/user-attachments/assets/0b4e038c-1075-4c09-b41f-438c8c04269c" />

The DS1307 RTC maintains the system time independently of the ESP32's reset state.

- **Timekeeping and backup:** A crystal oscillator provides the RTC's time base, while a backup battery (BT1) helps maintain timekeeping when the main supply is unavailable.
- **I²C communication:** The RTC communicates with the ESP32 over the SDA and SCL lines.
- **Voltage considerations:** The DS1307 normally operates from a 5 V supply. The I²C pull-up arrangement must be compatible with both the RTC and the ESP32's 3.3 V GPIOs. The ESP32 must never be exposed to an I²C pull-up voltage that exceeds its GPIO ratings.

### 3. Dual-Channel AC Dimmer Circuit

<img width="840" height="764" alt="Dual-channel AC dimmer schematic" src="https://github.com/user-attachments/assets/2a8d3538-d082-4fd0-ac87-476af83ffb05" />

The dimmer circuit provides independent brightness control for two AC lighting loads.

**Triac triggering and optoisolation:**Each channel uses a MOC3021-series optotriac driver and a BTA16-600B power triac. The optotriac driver provides an isolation barrier between the low-voltage control circuitry and the mains-side triac control circuit.

**Phase-angle control:** After detecting an AC zero crossing, the ESP32 waits for a calculated delay before generating a triac firing pulse. Adjusting this delay changes the portion of each AC half-cycle delivered to the load, allowing the controller to regulate brightness.


**Transient suppression:** An RC snubber network, consisting of the corresponding resistor and capacitor across each triac, helps limit voltage transients and unwanted rapid voltage changes. Its component values and suitability depend on the load and must be validated for the intended application.

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

## 🛠️ Prototyping & Installation

A custom PCB is not strictly necessary to experiment with the system's control logic. A prototype can also be assembled using suitable off-the-shelf modules.

### AC Dimmer Module

A compatible RobotDyn AC Light Dimmer Module can be used for prototyping, provided its interface, channel count, load compatibility, and electrical ratings match the application.

<img width="548" height="733" alt="AC dimmer module" src="https://github.com/user-attachments/assets/6b103478-393c-45f2-a305-c7f1c044256b" />

### RTC Module

A DS1307-based I²C real-time clock module with a backup battery can provide the timekeeping functionality.

<img width="563" height="263" alt="RTC module" src="https://github.com/user-attachments/assets/aeb95772-337f-4ba3-8b10-b58593a15f86" />

### Microcontroller

A standard ESP32 development board can be used to run the firmware and handle communication, scheduling, and dimmer synchronization.

## 📸 Prototype & Field Installation

The following photos document the prototype testing process and the final installation.

### Figure 1 — Bench Testing

Initial testing of the ESP32, RTC, and AC dimmer module.

<img width="1000" height="740" alt="Bench testing setup" src="https://github.com/user-attachments/assets/c4eef0f8-6f38-423e-a624-009a0b80892f" />

<img width="840" height="505" alt="Prototype electronics setup" src="https://github.com/user-attachments/assets/6836b739-108e-4efa-ba5e-d42502dab76b" />

### Figure 2 — Field Installation

The final enclosure and wiring installed inside the chicken coop.

<img width="821" height="605" alt="Installed controller enclosure" src="https://github.com/user-attachments/assets/60cf1e94-c5a7-4317-8dd4-4caa77ea1aee" />

<img width="913" height="684" alt="Completed field installation" src="https://github.com/user-attachments/assets/79ec0c5f-ec79-4686-bc28-c5e958638bf0" />

## ⚠️ Safety Warning

**This project involves hazardous mains voltage (230 V AC).**

Working with mains electricity can cause fatal electric shock, serious injury, fire, or property damage. The circuit diagrams and firmware are provided for educational and engineering reference purposes; they are not a substitute for a verified, compliant electrical design.

- Disconnect the mains supply before assembly, modification, or troubleshooting, and verify that the circuit is de-energized before touching it.
- Maintain appropriate isolation, creepage distances, and clearance distances between mains-voltage and low-voltage circuits.
- Use a suitable enclosure that prevents accidental contact with live components and provides appropriate protection against environmental conditions.
- Ensure that all components, PCB traces, connectors, wiring, and protective devices are correctly rated for the intended voltage, current, load, and operating environment.
- Have the mains-voltage design, installation, and testing reviewed or performed by a qualified person.

**Disclaimer:** This project is provided for educational and informational purposes only. Anyone building, modifying, or using it is responsible for ensuring that the resulting system is safe and complies with applicable electrical safety standards and regulations. The author accepts no responsibility for injury, property damage, or losses resulting from the construction or use of this project.

---

## 📂 Repository Contents

- **Hardware:** Circuit schematics, PCB design files, and related hardware documentation.
- **Software:** Embedded firmware for the ESP32-based lighting controller.
- **Documentation:** System architecture, circuit explanations, configuration examples, and installation photos.
