#include "keyMatrix.h"
#include "main.h"
#include "math.h"
#include <stdint.h>

const pin_t rows[6] = {{GPIOA, GPIO_PIN_5}, {GPIOA, GPIO_PIN_4},

                       {GPIOA, GPIO_PIN_3}, {GPIOA, GPIO_PIN_2},

                       {GPIOA, GPIO_PIN_1}, {GPIOA, GPIO_PIN_0}};

const pin_t columns[8] = {{GPIOB, GPIO_PIN_10}, {GPIOB, GPIO_PIN_11},
                          {GPIOB, GPIO_PIN_2},  {GPIOB, GPIO_PIN_1},
                          {GPIOB, GPIO_PIN_0},  {GPIOC, GPIO_PIN_4},
                          {GPIOA, GPIO_PIN_7},  {GPIOA, GPIO_PIN_6}};

const uint32_t phaseTable[8][6] = {
    {15624207, 24801882, 39370534, 62496826, 99207528, 157482134},
    {16553270, 26276679, 41711627, 66213081, 105106715, 166846509},
    {17537579, 27839171, 44191930, 70150316, 111356685, 176767719},
    {18580418, 29494575, 46819719, 74321671, 117978298, 187278874},
    {19685267, 31248413, 49603764, 78741067, 124993653, 198415056},
    {20855814, 33106541, 52553357, 83423255, 132426162, 210213429},
    {22095965, 35075158, 55678342, 88383859, 140300631, 222713370},
    {23409859, 37160835, 58989149, 93639437, 148643341, 235956596},

};

uint8_t pressed[8][6] = {0};
uint8_t prevState[8][6] = {0};
uint32_t lastKeyTime[8][6] = {0};
int32_t baseStep = 0;

void scanMatrix(voices_t voiceFactor, flag *scan, oscillator_t *oscillator,
                adsr_t *adsr, int8_t detuneCents, flag *releaseFlag) {
  int8_t voiceCount = 1 << voiceFactor;
  if (*scan == 1) {

    *scan = 0;

    for (uint8_t i = 0; i < 8; i++) {
      for (uint8_t j = 0; j < 8; j++) {
        HAL_GPIO_WritePin(columns[j].port, columns[j].pin, 0);
      }
      HAL_GPIO_WritePin(columns[i].port, columns[i].pin, 1);

      for (uint16_t d = 0; d < 50; d++) {

        __NOP();
      }

      for (uint8_t j = 0; j < 6; j++) {
        pressed[i][j] = HAL_GPIO_ReadPin(rows[j].port, rows[j].pin);
        if (pressed[i][j] != prevState[i][j]) {
          if (HAL_GetTick() - lastKeyTime[i][j] > 20) {
            prevState[i][j] = pressed[i][j];
            lastKeyTime[i][j] = HAL_GetTick();
            if (pressed[i][j]) {
              baseStep = phaseTable[i][j];
              if (voiceCount == 1) {
                oscillator[0].step = phaseTable[i][j];
              } else {
                for (int8_t k = 0; k < voiceCount; k++) {
                  float factor =
                      (float)(2 * k - (voiceCount - 1)) / (voiceCount - 1);
                  float detunePower =
                      powf(2, ((float)detuneCents * factor / 1200));
                  oscillator[k].step = baseStep * detunePower;
                }
              }
              adsr->value = 0;
              adsr->state = ATTACK;
            } else {
              *releaseFlag = 1;
            }
          }
        }
      }
    }
  }
}
