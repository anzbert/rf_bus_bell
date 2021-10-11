/* Libraries used: 
    - Arduino
    - RF433any - by Sébastien Millet (from https://github.com/sebmillet/RF433any or Arduino Package Manager)

    Pins:
RF 433Mhz RECEIVER: D2  (Input)
Test LED:           D9  (Output)
Bell Solenoid:      D10 (Output)
*/

#include "RF433any.h"
#include <Arduino.h>

#define PIN_RFINPUT 2
#define LED_OUT 9
#define TRIGGER_OUT 10

#define ASSERT_OUTPUT_TO_SERIAL

#define assert(cond)                 \
    {                                \
        if (!(cond))                 \
        {                            \
            assert_failed(__LINE__); \
        }                            \
    }

static void assert_failed(int line)
{
#ifdef ASSERT_OUTPUT_TO_SERIAL
    Serial.print("\ntest.ino:");
    Serial.print(line);
    Serial.println(": assertion failed, aborted.");
#endif
    while (1)
        ;
}

char serial_printf_buffer[150];
void serial_printf(const char *msg, ...)
    __attribute__((format(printf, 1, 2)));

// NOTE
//   Assume Serial has been initialized (Serial.begin(...))
void serial_printf(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);

    vsnprintf(serial_printf_buffer, sizeof(serial_printf_buffer), msg, args);
    va_end(args);
    Serial.print(serial_printf_buffer);
}

Track track(PIN_RFINPUT);

bool ring = false;
bool ringBell(bool ring)
{
    if (ring == true)
    {
        digitalWrite(TRIGGER_OUT, HIGH); // Bell solenoid up
        digitalWrite(LED_OUT, HIGH);     // LED on

        delay(400);

        digitalWrite(TRIGGER_OUT, LOW); // Bell solenoid back down
        digitalWrite(LED_OUT, LOW);     // LED off
    }
    return false;
}

void setup()
{
    pinMode(PIN_RFINPUT, INPUT);
    pinMode(TRIGGER_OUT, OUTPUT);
    pinMode(LED_OUT, OUTPUT);
    Serial.begin(115200);
}

void loop()
{
    track.treset();

    while (!track.do_events())
    {
        delay(1);
    }

    Decoder *pdec0 = track.get_data(
        RF433ANY_FD_DECODED | RF433ANY_FD_DEDUP | RF433ANY_FD_NO_ERROR);
    Decoder *pdec = pdec0;

    while (pdec)
    {
        const BitVector *pdata = pdec->get_pdata();
        assert(pdata); // Must be the case (RF433ANY_FD_DECODED in the call to get_data() above).
        char *buf = pdata->to_str();
        assert(buf);
        serial_printf("Received %c(%d): %s -- %d bytes\n", pdec->get_id_letter(),
                      pdata->get_nb_bits(), buf, pdata->get_nb_bytes());
        free(buf);

        // React on code if it consists of 12 bits and 2 bytes, as in that shitty bunnings remote ;)
        if (pdata->get_nb_bits() == 12 && pdata->get_nb_bytes() == 2)
        {
            ring = true;
        }

        pdec = pdec->get_next();
    }
    delete pdec0;

    ring = ringBell(ring);

    track.wait_free_433();
}
