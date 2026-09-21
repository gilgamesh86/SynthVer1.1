#include "main.h"

extern const pin_t rows[6];
extern const pin_t columns[8];
extern const uint32_t phaseTable[8][6];

void scanMatrix(voices_t voiceFactor, flag *scan, oscillator_t *oscillator,
                adsr_t *adsr, int8_t detuneCents, flag *releaseFlag);
