/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

typedef volatile uint8_t flag;

typedef struct {
  volatile uint32_t accumulator;
  volatile uint32_t step;
} oscillator_t;

typedef enum { FIRST_HALF = 0, SECOND_HALF = 256 } position_t;

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
} pin_t;

typedef struct {
  uint8_t state;
  uint16_t attack;
  uint16_t decay;
  float sustain;
  uint16_t release;
  float value;
} adsr_t;

typedef enum { ATTACK, DECAY, SUSTAIN, RELEASE } state_t;

typedef enum {
  ONE_VOICE,
  TWO_VOICE,
  FOUR_VOICE,
  EIGHT_VOICE
} voices_t; // this way so that i can just shift by this numbers to divide
            // instead of divide

typedef enum { SINE, SAWTOOTH } wave_t;

typedef enum {
  WAVE_TYPE,
  UNISON,
  DETUNE,
  ATK,
  DEC,
  SUS,
  REL,
  PADDING // padding cause 8 is good number
} uiStates;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
