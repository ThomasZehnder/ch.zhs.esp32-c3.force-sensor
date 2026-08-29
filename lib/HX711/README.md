# HX711 Library (Project-Local Fork)

This folder contains a local fork of the popular [bogde/HX711](https://github.com/bogde/HX711) library, customized specifically for this ESP32-C3 force sensor project.

## Key Differences from Upstream

### 1. Non-Blocking Interrupt Handling in `read()`

- **Upstream behavior:** The original `read()` method wraps the 24-bit bit-shifting loop inside `noInterrupts()` and `interrupts()` blocks to prevent timing jitter on the `PD_SCK` clock line.
- **Local modification:** Interrupts **remain enabled** during the 24-bit shift-in process.

### Why was this change made?

Since this ESP32-C3 device simultaneously runs a Wi-Fi stack, web server, and handles background tasks:
1. **Wi-Fi Stability:** Disabling interrupts during bit-banging blocks the Wi-Fi task watchdog/stack, leading to interrupted or failed HTTP requests and dropped client connections.
2. **System Responsiveness:** Keeping interrupts enabled ensures the real-time firmware loop, web server, and display updates remain responsive.
3. **Resilience over Rigidity:** If a system interrupt occasionally interferes with a bit-read, the resulting corrupted sample is simply filtered out or corrected in subsequent readings; the HX711 automatically resets its conversion cycle on the next clock pulse.

## License

This library retains the original MIT License (Copyright (c) 2018 Bogdan Necula).
