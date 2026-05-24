# ExplorerBotESP32

An ESP32-S3 + Arduino robot scaffold that uses the OpenAI API as a reasoning co-pilot while exploring and self-planning upgrades.

## What this version adds
- Targets **ESP32-S3** (`esp32-s3-devkitc-1`).
- Uses ChatGPT for two tasks:
  1. navigation action suggestions (`forward`, `left`, `right`, `stop`)
  2. upgrade requirement suggestions within a strict budget
- Uploads its latest upgrade requirements to an **FTP server**.
- Enforces a fixed upgrade budget of **$30 AUD**.

## Project layout
- `src/main.ino` – robot loop + FTP upload routine.
- `lib/ChatGPTClient` – minimal OpenAI client wrapper.
- `include/config.example.h` – copy to `include/config.h` and fill credentials.
- `platformio.ini` – PlatformIO config for ESP32-S3.

## Quick start
1. Install [PlatformIO](https://platformio.org/).
2. Copy config:
   ```bash
   cp include/config.example.h include/config.h
   ```
3. Fill `include/config.h`:
   - Wi-Fi credentials
   - OpenAI API key
   - FTP host/user/password/base path
4. Build/upload:
   ```bash
   pio run
   pio run -t upload
   pio device monitor
   ```

## How “it decides how it lives”
Every upload interval, the robot sends current state to the model and asks for an upgrade shortlist with this line format:

`item | max_price_aud | purchase_url | benefit`

The prompt hard-caps total recommendations to a **30 AUD** budget. You can review the uploaded `*_requirements.txt` from FTP and buy parts via the links.

## Safety
- Treat cloud suggestions as advisory only.
- Keep local safety checks in firmware (already blocks unsafe forward motion).
- Add emergency stop hardware before autonomous movement.
- Replace `setInsecure()` with certificate pinning before production deployment.
