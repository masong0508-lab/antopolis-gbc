/* GBC end-of-game records. Keep battery-save byte layout identical.
   The updater is in ROM bank 3 and runs only after a finished match. */
#ifndef ANTOPOLIS_RESULTS_CALC_H
#define ANTOPOLIS_RESULTS_CALC_H
#include <stdint.h>
typedef struct {
  uint8_t w[3], l[3];
  uint16_t t[3], s[3];
} ResultsRecord;
uint8_t results_record_update(ResultsRecord *rec, uint8_t difficulty,
                              uint8_t victory, uint16_t seconds,
                              uint16_t score) BANKED;
#endif
