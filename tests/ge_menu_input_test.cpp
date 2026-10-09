#include "ge_menu_input.h"

#include <cassert>
#include <iostream>

namespace {
constexpr double kCooldown = 0.15;
constexpr uint16_t kA = 0x1000, kB = 0x2000;

uint16_t Filter(ge::MenuButtonDebounce& debounce, uint16_t buttons, double now,
                bool menu = true) {
  return debounce.Filter(buttons, menu, now, kCooldown);
}
}  // namespace

int main() {
  // The first press is immediate and remains continuously held. A rapid second
  // press is rejected for its entire hold, never delivered after a delay.
  {
    ge::MenuButtonDebounce debounce;
    assert(Filter(debounce, kA, 0) == kA);
    assert(Filter(debounce, kA, 0.02) == kA);
    assert(Filter(debounce, 0, 0.03) == 0);
    assert(Filter(debounce, kA, 0.04) == 0);
    assert(Filter(debounce, kA, 0.50) == 0);
    assert(Filter(debounce, 0, 0.51) == 0);
    assert(Filter(debounce, kA, 0.52) == kA);
  }

  // Different buttons have independent cooldowns, so confirm followed by cancel
  // is responsive even when another confirm would be blocked.
  {
    ge::MenuButtonDebounce debounce;
    Filter(debounce, kA, 0);
    Filter(debounce, 0, 0.01);
    assert(Filter(debounce, kA | kB, 0.02) == kB);
  }

  // Accepted D-pad holds pass through every frame rather than becoming pulses.
  {
    ge::MenuButtonDebounce debounce;
    for (int frame = 0; frame < 240; ++frame)
      assert(Filter(debounce, 0x0002, static_cast<double>(frame) / 120) == 0x0002);
  }

  // Debounce is based on elapsed time, independent of frame rate.
  for (int fps : {30, 60, 120, 240}) {
    ge::MenuButtonDebounce debounce;
    uint16_t previous = 0;
    int presses = 0;
    for (int frame = 0; frame < fps; ++frame) {
      const double now = static_cast<double>(frame) / fps;
      const bool down = now < 0.06 || (now >= 0.09 && now < 0.23) || now >= 0.30;
      const uint16_t output = Filter(debounce, down ? kA : 0, now);
      presses += output != 0 && previous == 0;
      previous = output;
    }
    assert(presses == 2);
  }

  // Gameplay bypasses the guard and clears blocked presses. Tracking the raw
  // hold across menu entry also avoids inventing a new press on a state change.
  {
    ge::MenuButtonDebounce debounce;
    Filter(debounce, kA, 0);
    Filter(debounce, 0, 0.01);
    assert(Filter(debounce, kA, 0.02) == 0);
    assert(Filter(debounce, kA, 0.03, false) == kA);
    assert(Filter(debounce, kA, 0.04) == kA);
  }

  // Setting the cooldown to zero disables an already-active guard immediately.
  {
    ge::MenuButtonDebounce debounce;
    Filter(debounce, kA, 0);
    Filter(debounce, 0, 0.01);
    assert(Filter(debounce, kA, 0.02) == 0);
    assert(debounce.Filter(kA, true, 0.03, 0) == kA);
  }

  // Controllers do not interfere with each other's button presses.
  {
    ge::MenuButtonDebounce first, second;
    Filter(first, kA, 0);
    Filter(first, 0, 0.01);
    assert(Filter(first, kA, 0.02) == 0);
    assert(Filter(second, kA, 0.02) == kA);
  }
  std::cout << "Controller menu button debounce checks passed\n";
}
