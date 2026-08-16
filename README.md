# Custom Dance Pad Controller (Pump it Up)

A custom five-panel dance pad controller built with homemade sensors, cardboard, and an Arduino Leonardo with gameplay and diagnostics software. Made for Pump it Up style dance rhythm games.

The project converts physical panel presses into a keyboard input while handling unreliable electrical behavior from the homemade contact sensors. It also includes a diagnostic mode for visualizing inputs, measuring contact bounce, and evaluating debounce performance.

# Features
- Five input panels.
- USB keyboard input through an Arduino Leonardo.
- Software debouncing for noisy physical contacts.
- Separate gameplay and diagnostics modes.
- Real-time panel visualization through the Serial Monitor.
- Calibration mode for collecting repeated press/release samples to measure:
    - Average contact bounce duration
    - Maximum contact bounce duration
    - Raw state transitions per press/release
    - Longest quiet period during bouncing
    - Rejected false presses
    - Rejected false releases

# Why I Built It

Homemade dance pad contacts don't produce a clean signal. A single press rapidly alternates between HIGH and LOW before settling. Without a debounce delay, one press could register as multiple and affect gameplay quality.

Instead of choosing a debounce delay randomly or through trial and error, I added a calibration system that measures the electrical behavior of each panel during press and release of a pad. These measurements make it possible to compare the debounce threshold against real sensor behavior, and help decide the best delay to minimize latency between presses and keyboard input.
