#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <Arduino.h>

class SerialPort {
public:
    explicit SerialPort                         ();

    bool                    begin               (uint32_t baud_rate=115200);
    void                    print               (const String& message);
    void                    println             (const String& message);
    bool                    has_line            () const;
    String                  read_line           ();
    String                  get_string          (const String message="");
    int                     get_int             ();
    bool                    get_confirmation    ();
    bool                    prompt_user_yn      (const String message, uint16_t timeout=10000);
    void                    print_spacer        ();

private:
    unsigned long           baud_rate_;
    void                    flush_input         ();
};

#endif  // SERIAL_PORT_H
