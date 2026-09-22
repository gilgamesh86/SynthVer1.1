#include "oscillators.h"
#include "main.h"
#include "sinewave.h"

void unisonFill(voices_t number, position_t half, uint16_t *mainBuff,
                adsr_t *adsr, oscillator_t *oscillator, wave_t wave) {

  switch (wave) {
  case SAWTOOTH:
    if (number == ONE_VOICE) {
      for (uint16_t i = 0; i < 128; i++) {
        int16_t sample =
            (int16_t)(oscillator[0].accumulator >> 18) * adsr->value;
        mainBuff[(2 * i) + half] = sample;
        mainBuff[(2 * i) + 1 + half] = sample;
        oscillator[0].accumulator += oscillator[0].step;
      }

    } else if (number <= EIGHT_VOICE) {
      int32_t inputBuffer[128] = {0};
      for (uint8_t i = 0; i < (1 << number); i++) {
        for (uint8_t j = 0; j < 128; j++) {
          inputBuffer[j] +=
              (int32_t)((oscillator[i].accumulator >> 18)) * adsr->value;
          oscillator[i].accumulator += oscillator[i].step;
        }
      }
      for (uint8_t j = 0; j < 128; j++) {
        mainBuff[(2 * j) + half] = inputBuffer[j] >> number;
        mainBuff[(2 * j) + 1 + half] = inputBuffer[j] >> number;
        inputBuffer[j] = 0;
      }
    }
    break;

  case SINE:
    if (number == ONE_VOICE) {
      static int16_t index;
      for (int i = 0; i < 128; i++) {
        index = oscillator[0].accumulator >> 20;
        mainBuff[(2 * i) + half] = sineWave[index];
        mainBuff[(2 * i) + half + 1] = sineWave[index];
        oscillator[0].accumulator += oscillator[0].step;
      }
    }

    break;
  }
}
