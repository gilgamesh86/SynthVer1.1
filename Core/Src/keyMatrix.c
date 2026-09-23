#include "keyMatrix.h"
#include "core_cm4.h"
#include "main.h"
#include "math.h"
#include "usbd_cdc_if.h"
#include <stdint.h>

void delay_us(uint32_t us) {
  uint32_t start = DWT->CYCCNT;
  uint32_t cycles = us * (SystemCoreClock / 1000000UL);
  while ((DWT->CYCCNT - start) < cycles) {
  }
}

int _write(int file, char *ptr, int len) {
  CDC_Transmit_FS((uint8_t *)ptr, len);
  return len;
}

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
#define DEBOUNCE_SAMPLES 20
uint8_t pressed[8][6] = {0};
uint8_t prevState[8][6] = {0};
uint32_t lastKeyTime[8][6] = {0};
int32_t baseStep = 0;
uint8_t stableCount[8][6] = {0};

typedef struct {
  uint8_t i, j;
} key_t;
static key_t heldStack[48];
static uint8_t heldCount = 0;
static key_t activeKey = {0xFF, 0xFF};

static void triggerNote(uint8_t i, uint8_t j, voices_t voiceCount,
                        oscillator_t *oscillator, adsr_t *adsr,
                        int8_t detuneCents) {
  baseStep = phaseTable[i][j];
  if (voiceCount == 1) {
    oscillator[0].step = phaseTable[i][j];
  } else {
    for (int8_t k = 0; k < voiceCount; k++) {
      float factor = (float)(2 * k - (voiceCount - 1)) / (voiceCount - 1);
      float detunePower = powf(2, ((float)detuneCents * factor / 1200));
      oscillator[k].step = baseStep * detunePower;
    }
  }
  adsr->value = 0;
  adsr->state = ATTACK;
}

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

      delay_us(5);

      for (uint8_t j = 0; j < 6; j++) {
        uint8_t reading = HAL_GPIO_ReadPin(rows[j].port, rows[j].pin);
        pressed[i][j] = reading;

        if (reading != prevState[i][j]) {
          stableCount[i][j]++;
          if (stableCount[i][j] >= DEBOUNCE_SAMPLES) {
            prevState[i][j] = reading;
            stableCount[i][j] = 0;
            lastKeyTime[i][j] = HAL_GetTick();

            if (reading) {
              printf("key pressed: i=%d j=%d\r\n", i, j);
              if (heldCount < 48) {
                heldStack[heldCount++] = (key_t){i, j};
              }
              activeKey = (key_t){i, j};
              triggerNote(i, j, voiceCount, oscillator, adsr, detuneCents);

            } else {
              for (uint8_t k = 0; k < heldCount; k++) {
                if (heldStack[k].i == i && heldStack[k].j == j) {
                  for (uint8_t m = k; m < heldCount - 1; m++) {
                    heldStack[m] = heldStack[m + 1];
                  }
                  heldCount--;
                  break;
                }
              }
              if (activeKey.i == i && activeKey.j == j) {
                if (heldCount > 0) {
                  activeKey = heldStack[heldCount - 1];
                  triggerNote(activeKey.i, activeKey.j, voiceCount, oscillator,
                              adsr, detuneCents);
                } else {
                  activeKey = (key_t){0xFF, 0xFF};
                  *releaseFlag = 1;
                }
              }
            }
          }
        } else {
          stableCount[i][j] = 0;
        }
      }
    }
  }
}
