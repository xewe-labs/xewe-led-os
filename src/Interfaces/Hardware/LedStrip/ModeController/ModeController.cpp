#include "ModeController.h"

ModeController::ModeController(uint16_t num_leds, uint16_t transition_delay_ms)
    : num_leds(num_leds),
      transition_delay_ms(transition_delay_ms),
      is_transitioning(false),
      using_snapshot(false),
      transition_start_time(0)
{
    // Pre-allocate buffers to avoid runtime fragmentation
    buffer_current.resize(num_leds, CRGB::Black);
    buffer_old.resize(num_leds, CRGB::Black);
    buffer_snapshot.resize(num_leds, CRGB::Black);

    // Initialize with a default mode
    current_mode = create_mode(ModeID::SOLID);
}

std::unique_ptr<Mode> ModeController::create_mode(ModeID id) {
    switch (id) {
        case ModeID::RAINBOW: return std::make_unique<Rainbow>();
        case ModeID::SOLID:   return std::make_unique<Solid>();
        default:              return std::make_unique<Solid>(); // Fallback
    }
}

void ModeController::set_mode(ModeID new_mode_id) {
    if (is_transitioning) {
        // EDGE CASE: Mode changed while already changing.
        // We capture the current blended output (which happens to be living on the
        // actual LED strip buffer, but we can recreate it via the snapshot buffer).
        // Since update() populates the final output, we assume the snapshot buffer
        // now contains the frozen frame of the interrupted transition.
        using_snapshot = true;
    } else {
        // NORMAL TRANSITION
        using_snapshot = false;
        // Move current mode to old mode to keep it rendering during transition
        old_mode = std::move(current_mode);
    }

    // Instantiate the new mode dynamically
    current_mode = create_mode(new_mode_id);

    // Start transition
    is_transitioning = true;
    transition_start_time = millis();
}

void ModeController::update(CRGB* output_buffer) {
    if (!current_mode) return;

    // 1. Always render the current target mode
    current_mode->render(buffer_current.data(), num_leds);

    if (!is_transitioning) {
        // If IDLE, just copy the current buffer to the output
        std::copy(buffer_current.begin(), buffer_current.end(), output_buffer);
        return;
    }

    // 2. Handle Transition Interpolation
    uint32_t elapsed = millis() - transition_start_time;
    if (elapsed >= transition_delay_ms) {
        // Transition complete
        is_transitioning = false;
        using_snapshot = false;
        old_mode.reset(); // Free old mode memory
        std::copy(buffer_current.begin(), buffer_current.end(), output_buffer);
        return;
    }

    // Calculate 8-bit blend fraction (0 to 255)
    fract8 blend_amount = (elapsed * 255) / transition_delay_ms;

    // Render old mode if we aren't using a static snapshot
    if (!using_snapshot && old_mode) {
        old_mode->render(buffer_old.data(), num_leds);
    }

    // 3. Interpolate the buffers
    for (uint16_t i = 0; i < num_leds; i++) {
        CRGB old_color = using_snapshot ? buffer_snapshot[i] : buffer_old[i];

        // FastLED blend function interpolates between two CRGB values
        output_buffer[i] = blend(old_color, buffer_current[i], blend_amount);
    }

    // Continually update the snapshot buffer during transitions so it's
    // ready for the edge case if interrupted on the next frame.
    std::copy(output_buffer, output_buffer + num_leds, buffer_snapshot.begin());
}

std::string ModeController::get_current_mode_name() const {
    return current_mode ? current_mode->get_name() : "None";
}

std::vector<ModeParameter> ModeController::get_current_mode_params() const {
    if (current_mode) return current_mode->get_params();
    return {};
}

bool ModeController::set_current_mode_param(const std::string& name, int value) {
    if (current_mode) return current_mode->set_param(name, value);
    return false;
}

std::array<uint8_t, 3> ModeController::hsv_to_rgb(const std::array<uint8_t, 3> hsv) {
    // DBG_PRINTF(ModeController, "hsv_to_rgb input: %u, %u, %u\n", hsv[0], hsv[1], hsv[2]);
    CRGB rgb;
    hsv2rgb_rainbow(CHSV(hsv[0], hsv[1], hsv[2]), rgb);
    return {rgb.r, rgb.g, rgb.b};
}

std::array<uint8_t, 3> ModeController::rgb_to_hsv(const std::array<uint8_t, 3> rgb) {
    // DBG_PRINTF(ModeController, "rgb_to_hsv input: %u, %u, %u\n", rgb[0], rgb[1], rgb[2]);
    CHSV hsv = rgb2hsv_approximate(CRGB(rgb[0], rgb[1], rgb[2]));
    return {hsv.h, hsv.s, hsv.v};
}