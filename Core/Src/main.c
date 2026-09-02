/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cordic.h"
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "i2s.h"
#include "rng.h"
#include "stm32g4xx_hal_gpio.h"
#include "tim.h"
#include <math.h>
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
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

typedef enum { ONE_VOICE, TWO_VOICE, FOUR_VOICE, EIGHT_VOICE } voices_t;

typedef enum { SINE, SAW } wave_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* ---------------------------buffer------------------------------------------*/

volatile uint16_t mainBuff[512] = {0};

/*----------------------------flags-------------------------------------------*/

flag scan = 0;
flag adsrTick = 0;
flag releaseFlag = 0;
flag ledFlag = 0;
flag gachaFlag = 0;
/*----------------------------LUTs--------------------------------------------*/

const pin_t rows[6] = {{GPIOA, GPIO_PIN_5}, {GPIOA, GPIO_PIN_4},
                       {GPIOA, GPIO_PIN_3}, {GPIOA, GPIO_PIN_2},
                       {GPIOA, GPIO_PIN_1}, {GPIOA, GPIO_PIN_0}};

const pin_t columns[8] = {{GPIOB, GPIO_PIN_10}, {GPIOB, GPIO_PIN_11},
                          {GPIOB, GPIO_PIN_2},  {GPIOB, GPIO_PIN_1},
                          {GPIOB, GPIO_PIN_0},  {GPIOC, GPIO_PIN_4},
                          {GPIOA, GPIO_PIN_7},  {GPIOA, GPIO_PIN_6}};

uint32_t phaseTable[8][6] = {
    {15624207, 24801882, 39370534, 62496826, 99207528, 157482134},
    {16553270, 26276679, 41711627, 66213081, 105106715, 166846509},
    {17537579, 27839171, 44191930, 70150316, 111356685, 176767719},
    {18580418, 29494575, 46819719, 74321671, 117978298, 187278874},
    {19685267, 31248413, 49603764, 78741067, 124993653, 198415056},
    {20855814, 33106541, 52553357, 83423255, 132426162, 210213429},
    {22095965, 35075158, 55678342, 88383859, 140300631, 222713370},
    {23409859, 37160835, 58989149, 93639437, 148643341, 235956596},
};

const int16_t aaaaSample[326] = {
    510,    510,    690,    690,    888,    888,    1212,   1212,   1541,
    1541,   1849,   1849,   2017,   2017,   2086,   2086,   1918,   1918,
    1719,   1719,   1436,   1436,   1027,   1027,   490,    490,    -71,
    -71,    -535,   -535,   -930,   -930,   -1392,  -1392,  -2016,  -2016,
    -2694,  -2694,  -3331,  -3331,  -3920,  -3920,  -4525,  -4525,  -5009,
    -5009,  -5231,  -5231,  -5121,  -5121,  -4758,  -4758,  -4292,  -4292,
    -3787,  -3787,  -3231,  -3231,  -2583,  -2583,  -1833,  -1833,  -1011,
    -1011,  -145,   -145,   868,    868,    2037,   2037,   3196,   3196,
    4223,   4223,   5073,   5073,   5822,   5822,   6493,   6493,   7026,
    7026,   7395,   7395,   7569,   7569,   7620,   7620,   7489,   7489,
    7150,   7150,   6602,   6602,   5938,   5938,   5237,   5237,   4411,
    4411,   3524,   3524,   2585,   2585,   1642,   1642,   702,    702,
    -201,   -201,   -998,   -998,   -1679,  -1679,  -2261,  -2261,  -2723,
    -2723,  -3076,  -3076,  -3341,  -3341,  -3423,  -3423,  -3311,  -3311,
    -3010,  -3010,  -2554,  -2554,  -1992,  -1992,  -1425,  -1425,  -886,
    -886,   -424,   -424,   -31,    -31,    349,    349,    698,    698,
    997,    997,    1144,   1144,   1100,   1100,   848,    848,    472,
    472,    34,     34,     -453,   -453,   -1057,  -1057,  -1822,  -1822,
    -2706,  -2706,  -3736,  -3736,  -4916,  -4916,  -6140,  -6140,  -7263,
    -7263,  -8228,  -8228,  -8971,  -8971,  -9619,  -9619,  -10240, -10240,
    -10765, -10765, -11140, -11140, -11075, -11075, -10460, -10460, -9281,
    -9281,  -7366,  -7366,  -4893,  -4893,  -2270,  -2270,  -133,   -133,
    1072,   1072,   1772,   1772,   2713,   2713,   4424,   4424,   6866,
    6866,   9551,   9551,   11982,  11982,  13921,  13921,  15185,  15185,
    15317,  15317,  14234,  14234,  12693,  12693,  11558,  11558,  11130,
    11130,  10743,  10743,  9699,   9699,   7890,   7890,   5328,   5328,
    2591,   2591,   -89,    -89,    -2595,  -2595,  -4592,  -4592,  -6022,
    -6022,  -7088,  -7088,  -8471,  -8471,  -10385, -10385, -12386, -12386,
    -13848, -13848, -14484, -14484, -14268, -14268, -13383, -13383, -12084,
    -12084, -10473, -10473, -8892,  -8892,  -7686,  -7686,  -6822,  -6822,
    -5814,  -5814,  -4320,  -4320,  -2374,  -2374,  -392,   -392,   1295,
    1295,   2839,   2839,   4320,   4320,   5678,   5678,   6622,   6622,
    7000,   7000,   7171,   7171,   7306,   7306,   7278,   7278,   6921,
    6921,   6246,   6246,   5524,   5524,   4918,   4918,   4457,   4457,
    3964,   3964,   3315,   3315,   2574,   2574,   1870,   1870,   1360,
    1360,   994,    994,    642,    642,    325,    325,    166,    166,
    182,    182};

/*----------------------------random------------------------------------------*/

oscillator_t oscillator[8] = {0};
adsr_t adsr = {ATTACK, 5000, 500, 0.5, 500, 0};

uint8_t pressed[8][6] = {0};
uint8_t prevState[8][6] = {0};
uint32_t lastKeyTime[8][6] = {0};
uint32_t baseStep = 0;
uint8_t voiceFactor = FOUR_VOICE;
uint8_t voiceCount = 0;
int8_t detuneCents = 25;

uint32_t randomNumber = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void blink(void) {
  if (ledFlag == 1) {
    ledFlag = 0;
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);
  }
}

void fillSine(position_t half) {
  static int32_t inputBuffer[128] = {0};
  static int32_t outputBuffer[128] = {0};
  for (uint8_t i = 0; i < 128; i++) {
    inputBuffer[i] = oscillator[0].accumulator;
    oscillator[0].accumulator += oscillator[0].step;
  }
  HAL_CORDIC_Calculate(&hcordic, inputBuffer, outputBuffer, 128, HAL_MAX_DELAY);

  for (uint8_t i = 0; i < 128; i++) {
    mainBuff[(2 * i) + half] = (int16_t)(outputBuffer[i] >> 18) * adsr.value;
    mainBuff[(2 * i) + 1 + half] =
        (int16_t)(outputBuffer[i] >> 18) * adsr.value;
  }
}

void fillSaw(position_t half) {
  for (uint16_t i = 0; i < 128; i++) {
    int16_t sample = (int16_t)(oscillator[0].accumulator >> 18) * adsr.value;
    mainBuff[(2 * i) + half] = sample;
    mainBuff[(2 * i) + 1 + half] = sample;
    oscillator[0].accumulator += oscillator[0].step;
  }
}

void unisonFill(voices_t number, position_t half) {
  if (number == ONE_VOICE) {
    for (uint16_t i = 0; i < 128; i++) {
      int16_t sample = (int16_t)(oscillator[0].accumulator >> 18) * adsr.value;
      mainBuff[(2 * i) + half] = sample;
      mainBuff[(2 * i) + 1 + half] = sample;
      oscillator[0].accumulator += oscillator[0].step;
    }
  } else if (number <= EIGHT_VOICE) {
    int32_t inputBuffer[128] = {0};
    for (uint8_t i = 0; i < (1 << number); i++) {
      for (uint8_t j = 0; j < 128; j++) {
        inputBuffer[j] +=
            (int32_t)((oscillator[i].accumulator >> 18)) * adsr.value;
        oscillator[i].accumulator += oscillator[i].step;
      }
    }
    for (uint8_t j = 0; j < 128; j++) {
      mainBuff[(2 * j) + half] = inputBuffer[j] >> number;
      mainBuff[(2 * j) + 1 + half] = inputBuffer[j] >> number;
      inputBuffer[j] = 0;
    }
  }
}

void tetoMode(position_t half) {
  uint8_t i2 = 0;
  for (int i = 0; i < 128; i++) {
    i2 = ((uint64_t)(uint32_t)oscillator[0].accumulator * 163) >> 32;
    mainBuff[(2 * i) + half] = (int16_t)(aaaaSample[2 * i2] * adsr.value);
    mainBuff[(2 * i) + 1 + half] =
        (int16_t)(aaaaSample[(2 * i2) + 1] * adsr.value);
    oscillator[0].accumulator += oscillator[0].step;
  }
}

void scanMatrix(void) {
  voiceCount = 1 << voiceFactor;
  if (scan == 1) {
    scan = 0;
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
              adsr.value = 0;
              adsr.state = ATTACK;
            } else {

              releaseFlag = 1;
            }
          }
        }
      }
    }
  }
}

void adsrEnvStart(void) {
  if (adsrTick == 1) {
    adsrTick = 0;
    switch (adsr.state) {

    case ATTACK:
      if (releaseFlag == 1) {
        releaseFlag = 0;
        adsr.state = RELEASE;
      } else if (adsr.value <= 1) {
        adsr.value += 0.1f / adsr.attack;
      } else {
        adsr.value = 1;
        adsr.state = DECAY;
      }
      break;

    case DECAY:
      if (releaseFlag == 1) {
        releaseFlag = 0;
        adsr.state = RELEASE;
      } else if (adsr.value >= adsr.sustain) {
        adsr.value -= (1 - adsr.sustain) / (10 * adsr.decay);
      } else {
        adsr.value = adsr.sustain;
      }
      break;

    case SUSTAIN:
      if (releaseFlag == 1) {
        releaseFlag = 0;
        adsr.state = RELEASE;
      } else {
        adsr.value = adsr.sustain;
      }
      break;

    case RELEASE:
      if (adsr.value <= 0) {
        adsr.value = 0;
        for (uint8_t k = 0; k < 8; k++) {
          oscillator[k].step = 0;
          oscillator[k].accumulator = 0;
        }

      } else {
        adsr.value -= 0.1f / adsr.release;
      }
      break;
    }
  }
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */
  SystemCoreClockUpdate();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S2_Init();
  MX_CORDIC_Init();
  MX_TIM4_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_RNG_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  HAL_RNG_GenerateRandomNumber(&hrng, &randomNumber);
  if (randomNumber % 20 == 1) {
    gachaFlag = 1;
  } else {
    gachaFlag = 0;
  }
  CORDIC_ConfigTypeDef cordic;
  cordic.Function = CORDIC_FUNCTION_SINE;
  cordic.Scale = CORDIC_SCALE_0;
  cordic.InSize = CORDIC_INSIZE_32BITS;
  cordic.OutSize = CORDIC_OUTSIZE_32BITS;
  cordic.Precision = CORDIC_PRECISION_6CYCLES;
  cordic.NbRead = CORDIC_NBREAD_1;
  cordic.NbWrite = CORDIC_NBWRITE_1;
  if (HAL_CORDIC_Configure(&hcordic, &cordic) != HAL_OK) {
    Error_Handler();
  }

  HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t *)mainBuff, 512);

  HAL_TIM_Base_Start_IT(&htim4);

  HAL_TIM_Base_Start_IT(&htim6);

  HAL_TIM_Base_Start_IT(&htim7);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {

    if (gachaFlag == 1) {
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
    } else {
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
    }
    scanMatrix();

    adsrEnvStart();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
  if (gachaFlag == 1) {
    tetoMode(FIRST_HALF);
  } else {
    // fillSaw(FIRST_HALF);
    unisonFill(voiceFactor, FIRST_HALF);
  }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
  if (gachaFlag == 1) {
    tetoMode(SECOND_HALF);
  } else {
    // fillSaw(SECOND_HALF);
    unisonFill(voiceFactor, SECOND_HALF);
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM4) {
    scan = 1;
  }

  if (htim->Instance == TIM6) {
    adsrTick = 1;
  }

  if (htim->Instance == TIM7) {
    ledFlag = 1;
  }
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
