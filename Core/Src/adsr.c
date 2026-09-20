#include "main.h"

void adsrEnvStart(flag *adsrTick, adsr_t *adsr, flag *releaseFlag,
                  oscillator_t *oscillator) {
  if (*adsrTick == 1) {
    *adsrTick = 0;
    switch (adsr->state) {

    case ATTACK:
      if (*releaseFlag == 1) {
        *releaseFlag = 0;
        adsr->state = RELEASE;
      } else {
        adsr->value += 0.1f / adsr->attack;
        if (adsr->value >= 1.0f) {
          adsr->value = 1.0f;
          adsr->state = DECAY;
        }
      }
      break;

    case DECAY:
      if (*releaseFlag == 1) {
        *releaseFlag = 0;
        adsr->state = RELEASE;
      } else {
        adsr->value -= (1 - adsr->sustain) / (10 * adsr->decay);
        if (adsr->value <= adsr->sustain) {
          adsr->value = adsr->sustain;
          adsr->state = SUSTAIN; // <-- also: you never transition to SUSTAIN
                                 // currently, see below
        }
      }
      break;

    case SUSTAIN:
      if (*releaseFlag == 1) {
        *releaseFlag = 0;
        adsr->state = RELEASE;
      } else {
        adsr->value = adsr->sustain;
      }
      break;

    case RELEASE:
      adsr->value -= 0.1f / adsr->release;
      if (adsr->value <= 0.0f) {
        adsr->value = 0.0f;
        for (uint8_t k = 0; k < 8; k++) {
          oscillator[k].step = 0;
          oscillator[k].accumulator = 0;
        }
      }
      break;
    }
  }
}
