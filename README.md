# EduBot: The Academic Buddy

An IoT study companion that helps students stay off their phones. To start a Pomodoro session, the student places their phone on EduBot's dock. If the phone is picked up mid-session, the timer pauses and EduBot records how long the phone was away ("time distracted"). Progress shows on the device's LCD and on a live web dashboard.

Final project for Computer Organization and Architecture (December 2024).

## How it works

```
Arduino (timer, dock sensor, LCD, buttons, buzzer)
   │  JSON over serial
   ▼
ESP32 (Wi-Fi bridge)
   │  MQTT  (topic: pomodoro/data)
   ▼
Node.js + Express server
   │  Server-Sent Events
   ▼
Browser dashboard (time left + time distracted)
```

- **Dock detection:** a photoresistor under the dock reads dark when a phone is on it. Red LED = no phone, yellow LED = phone docked.
- **Setting the timer:** three buttons select minutes/seconds, increment the value, and confirm.
- **Pausing:** removing the phone pauses the countdown and starts the "time distracted" counter.
- **Finishing:** the LCD blinks and the buzzer sounds when the session ends.

## Repository layout

| Path | What it is |
|---|---|
| `firmware/arduino_pomodoro_timer/` | Arduino sketch: timer logic, buttons, LCD, LDR dock sensor, LEDs, buzzer |
| `firmware/esp32_mqtt_bridge/` | ESP32 sketch: reads timer data from the Arduino over serial and publishes it to MQTT |
| `server.js` | Node.js server: subscribes to MQTT and streams updates to the browser via SSE |
| `public/` | Dashboard page |

## Hardware

Arduino, ESP32, 16×2 I2C LCD, photoresistor (LDR), 3 push buttons, red and yellow LEDs, buzzer. The Arduino's TX/RX connect to the ESP32's RX (GPIO 16) / TX (GPIO 17).

## Running it

1. **Arduino:** install the `LiquidCrystal_I2C` library and upload `firmware/arduino_pomodoro_timer`.
2. **ESP32:** install `PubSubClient` and `ArduinoJson`, fill in your Wi-Fi name, password, and MQTT broker IP in `firmware/esp32_mqtt_bridge/esp32_mqtt_bridge.ino`, then upload.
3. **MQTT broker:** run one on your network (for example, Mosquitto on port 1883).
4. **Dashboard:**
   ```
   npm install
   MQTT_BROKER=mqtt://<broker-ip> node server.js
   ```
   Then open http://localhost:3000.

## Team

Built by Alessa L. Arong, Jure Rhoanne Q. Batohanon, and Maria Katrina O. Esclamado.
