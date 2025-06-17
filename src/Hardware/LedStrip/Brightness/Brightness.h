#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

#include <memory>
#include "../../../Debug.h"
#include "../AsyncTimer/AsyncTimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class Brightness {

public:
    Brightness                              (uint16_t transition_delay, uint8_t initial_brightness, uint8_t state);
    ~Brightness                             ();

    uint8_t         get_start_value         () const;
    uint8_t         get_current_value       () const;
    uint8_t         get_target_value        () const;
    void            set_brightness          (uint8_t new_brightness);
    void            turn_on                 ();
    void            turn_off                ();
    uint8_t         get_dimmed_color        (uint8_t color) const;
    bool            get_state               () const;
    uint8_t         get_last_brightness     () const;
private:
    std::unique_ptr<AsyncTimer<uint8_t>>    timer;
    uint8_t                                 state;
    uint8_t                                 last_brightness;
    SemaphoreHandle_t                       internal_mutex;
};

#endif // BRIGHTNESS_H
