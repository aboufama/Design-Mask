# DESIGNmask ESP32 Linear Actuator Controller

Arduino firmware and a minimal browser controller for an Adafruit ESP32 Feather V2 / HUZZAH32 ESP32 Feather V2 driving five AGFRC C1.5CLS 1.5g linear actuators over USB Serial.

The firmware starts with every actuator detached. It does not move on boot. It only sends pulses after a serial command or browser slider movement.

## Upload

1. Open `arduino/servo_controller/servo_controller.ino` in Arduino IDE.
2. Install board support: `esp32` by Espressif Systems.
3. Select board: `Adafruit ESP32 Feather V2`.
4. Upload over USB-C.

If upload fails after connecting to the ESP32, set upload speed to `115200`. That was the reliable flash speed for the board tested here.

## Browser Control

Run a local static server from this repo:

```bash
python3 -m http.server 5173 --directory browser
```

Then open:

```text
http://localhost:5173
```

Use Chrome or Edge. The page shows five vertical sliders. Click `Connect ESP` to pick the board's serial port; dragging a slider or clicking an animation preset will also ask for the port if it is not connected yet.

The five motor sliders represent five vertical mask columns for blind/scale effects. The preset controls run column animations: `Wave` loops a traveling wave, `Raise` slowly raises every column, `Slide` raises columns left-to-right, and `Stop` halts the active preset.

The page also opens a small face tracking window for up to four people. It uses the browser camera plus MediaPipe FaceMesh, following the landmark, iris, blink, and gaze heuristics from [`arnaudlvq/Eye-Contact-RealTime-Detection`](https://github.com/arnaudlvq/Eye-Contact-RealTime-Detection). Allow camera access when the browser asks. The MediaPipe FaceMesh script is loaded from jsDelivr.

The camera window is divided into five vertical sections matching the five mask columns. If a detected face is making eye contact in a section, that section's motor is raised to `0%`; sections without eye contact rest at `100%`. Browser serial access still has to be opened by a user gesture first, such as clicking `Connect ESP`, dragging a slider, or clicking a preset.

Touching or keyboard-adjusting the `ALL` bar enters manual override: all five columns are hard-assigned to that height and eye-contact motor updates are ignored until a preset or `Stop` is clicked.

## Deploy To Vercel

This repo is configured for Vercel as a static site. Vercel runs `npm run build`, which copies only `browser/` into `dist/`, then serves `dist/`.

Project settings:

| Setting | Value |
| --- | --- |
| Framework Preset | `Other` |
| Build Command | `npm run build` |
| Output Directory | `dist` |
| Install Command | leave default |

Vercel serves over HTTPS, which is required for camera access and Web Serial in Chrome/Edge. The deployed page still loads MediaPipe FaceMesh from jsDelivr.

## Serial Settings

- Baud: `115200`
- Line ending: `Newline`

## Commands

| Command | Meaning |
| --- | --- |
| `HELP` | Show commands |
| `STATUS` | Show all actuator states |
| `S 50` | Move motor 1 to 50 percent |
| `S 3 50` | Move motor 3 to 50 percent |
| `US 1500` | Move motor 1 to 1500 microseconds |
| `US 3 1500` | Move motor 3 to 1500 microseconds |
| `C` | Center all motors at 1500 microseconds |
| `C 3` | Center motor 3 at 1500 microseconds |
| `D` | Detach all motors / stop pulses |
| `D 3` | Detach motor 3 / stop pulses |

The sliders map `0-100%` to `1000-2000us`.

## Wiring

| Motor | Signal pin |
| --- | --- |
| 1 | Feather `D33 / GPIO33` |
| 2 | Feather `GPIO32` |
| 3 | Feather `GPIO27` |
| 4 | Feather `GPIO12` |
| 5 | Feather `GPIO13` |

All actuators need a shared ground with the Feather. Servo power is separate from serial. Serial sends commands only. For five actuators, prefer an external `3.7V-6.0V` servo supply instead of powering all actuators from the Feather `USB` pin.

Violent shaking is not normal. If it happens, remove actuator power, check common ground, avoid mechanical binding, and use a separate servo supply.
