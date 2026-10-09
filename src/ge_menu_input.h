#pragma once

#include <array>
#include <cstdint>
#include <limits>

namespace ge {

// Reject a second physical press of the same button within a short cooldown.
// Accepted holds pass through continuously; this does not generate repeats or
// have access to stick axes. Each controller owns its own debounce state.
class MenuButtonDebounce {
 public:
  uint16_t Filter(uint16_t buttons, bool menu_active, double now, double cooldown) {
    for (int i = 0; i < 16; ++i) {
      const uint16_t bit = static_cast<uint16_t>(1u << i);
      Button& state = buttons_[i];
      const bool down = (buttons & bit) != 0;
      if (!menu_active || cooldown <= 0) {
        state = {};
        state.down = down;
        continue;
      }
      if (!down) {
        state.blocked = false;
      } else if (!state.down) {
        state.blocked = now - state.last_press < cooldown;
        if (!state.blocked) state.last_press = now;
      }
      state.down = down;
      // A rejected press remains rejected until released, even after its
      // cooldown expires. Never deliver a delayed, unexpected activation.
      if (state.blocked) buttons &= static_cast<uint16_t>(~bit);
    }
    return buttons;
  }

 private:
  struct Button {
    bool down = false;
    bool blocked = false;
    double last_press = -std::numeric_limits<double>::infinity();
  };

  std::array<Button, 16> buttons_{};
};

}  // namespace ge
