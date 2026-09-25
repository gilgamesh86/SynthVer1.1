#include "ui.h"
#include "SH1106.h"
#include "fonts.h"
#include "i2c.h"
#include "stm32g4xx_hal_i2c.h"
#include "stm32g4xx_hal_i2s.h"
#include "stm32g4xx_hal_tim.h"
#include "string.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

void displayInit(void) { SH1106_Init(); }

void displayText(text type, char *str, ...) {
  va_list args;
  va_start(args, str);
  char stringBuff[30] = {0};
  vsprintf(stringBuff, str, args);
  va_end(args);
  if (type == TITLE) {
    SH1106_GotoXY(0, 0);
    SH1106_Puts(stringBuff, &Font_11x18, SH1106_COLOR_WHITE);
    SH1106_UpdateScreen();
  } else if (type == BODY1) {
    SH1106_GotoXY(0, 19);
    SH1106_Puts(stringBuff, &Font_7x10, SH1106_COLOR_WHITE);
    SH1106_UpdateScreen();
  } else if (type == BODY2) {
    SH1106_GotoXY(0, 30);
    SH1106_Puts(stringBuff, &Font_7x10, SH1106_COLOR_WHITE);
    SH1106_UpdateScreen();
  }
}

void displayClear(void) { SH1106_Clear(); }

bool parameterSet(TIM_HandleTypeDef *htim, void *parameter, paramType type) {
  static int16_t prev = 0;
  int16_t curr = (int16_t)__HAL_TIM_GET_COUNTER(htim);
  int16_t delta = curr - prev;

  if (delta == 0)
    return false;

  prev = curr;
  if (type == PARAM_INT) {
    *(int8_t *)parameter += (int8_t)(delta);
  } else {
    *(float *)parameter += delta;
  }
  return true;
}
