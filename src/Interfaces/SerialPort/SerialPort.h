#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <Arduino.h>

class SerialPort {
public:
    explicit SerialPort                         (unsigned long baud_rate = 115200);

    void                    print               (const String& message);
    void                    println             (const String& message);
    bool                    has_line            () const;
    String                  read_line           ();
    String                  get_string          ();
    int                     get_int             ();
    bool                    get_confirmation    ();
    void                    print_spacer        ();

private:
    unsigned long           baud_rate_;
    void                    flush_input         ();
};

#endif  // SERIAL_PORT_H
