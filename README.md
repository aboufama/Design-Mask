# DESIGNmask ESP32 Linear Actuator Controller

Simple Arduino sketch for an Adafruit ESP32 Feather V2 / HUZZAH32 ESP32 Feather V2 driving one AGFRC C1.5CLS 1.5g linear actuator over USB Serial.

The firmware starts with the actuator detached. It does not move on boot. It only sends pulses after a serial command or browser slider movement.

## Upload

1. Open `arduino/servo_controller/servo_controller.ino` in Arduino IDE.
2. Install board support: `esp32` by Espressif Systems.
3. Install library: `ESP32Servo`.
4. Select board: `Adafruit ESP32 Feather V2`.
5. Upload over USB-C.

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

Use Chrome or Edge, click `Connect serial`, choose the Feather, then use the single actuator slider.

## Serial Settings

- Baud: `115200`
- Line ending: `Newline`

## Commands

| Command | Meaning |
| --- | --- |
| `HELP` | Show commands |
| `STATUS` | Show actuator state |
| `S 50` | Move actuator to 50 percent |
| `S 1 50` | Same as `S 50`, accepted for browser compatibility |
| `US 1500` | Move actuator to 1500 microseconds |
| `C` | Center at 1500 microseconds |
| `TEST` | Slow jog: 1300us, 1700us, 1500us |
| `D` | Detach actuator / stop pulses |

The slider maps `0-100%` to `1000-2000us`.

## Wiring

| Actuator wire | Connect to |
| --- | --- |
| Signal | Feather `D33 / GPIO33` only |
| V+ | Feather `USB` pin for light no-load testing, or external `3.7V-6.0V` + |
| GND | Feather GND and servo supply GND |

Servo power is separate from serial. Serial sends commands only. Only plug in one actuator.

Violent shaking is not normal. If it happens, remove actuator power, use only one actuator, check common ground, avoid mechanical binding, and prefer a separate servo supply over the Feather `USB` pin.
