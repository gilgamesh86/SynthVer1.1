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
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "i2s.h"
#include "rng.h"
#include "tim.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "adsr.h"
#include "kasaneTeto.h"
#include "keyMatrix.h"
#include "oscillators.h"
#include "sinewave.h"
#include "ui.h"
#include "usbd_cdc_if.h"
#include <stdint.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
int _write(int file, char *ptr, int len) {
  CDC_Transmit_FS((uint8_t *)ptr, len);
  return len;
}

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

uint16_t mainBuff[512] = {0};

/*----------------------------flags-------------------------------------------*/

flag scan = 0;
flag adsrTick = 0;
flag releaseFlag = 0;
flag ledFlag = 0;
flag gachaFlag = 0;

/*----------------------------random------------------------------------------*/

oscillator_t oscillator[8] = {0};

adsr_t adsr = {ATTACK, 10, 10, 1, 0, 0};
uint8_t voiceCount = 0;
int8_t detuneCents = 20;
int8_t waveType = 0;

volatile uint8_t uiStateCounter = 0;
volatile int8_t updateState = 0;

volatile int8_t uiState = 0;

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

volatile uint32_t fillSineCycles = 0;
volatile uint32_t fillSineMaxCycles = 0;
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
  if (gachaFlag == 1) {
    tetoMode(FIRST_HALF, (int16_t *)mainBuff, oscillator, &adsr);
  } else {

    volatile uint32_t start = DWT->CYCCNT;
    unisonFill(voiceCount, FIRST_HALF, mainBuff, &adsr, oscillator, waveType);
    fillSineCycles = DWT->CYCCNT - start;
    if (fillSineCycles > fillSineMaxCycles)
      fillSineMaxCycles = fillSineCycles;
  }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
  if (gachaFlag == 1) {
    tetoMode(SECOND_HALF, (int16_t *)mainBuff, oscillator, &adsr);
  } else {
    volatile uint32_t start = DWT->CYCCNT;
    unisonFill(voiceCount, SECOND_HALF, mainBuff, &adsr, oscillator, waveType);
    fillSineCycles = DWT->CYCCNT - start;
    if (fillSineCycles > fillSineMaxCycles)
      fillSineMaxCycles = fillSineCycles;
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

#define DEBOUNCE_MS 150

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  static uint32_t lastPB5Tick = 0;
  static uint32_t lastPB6Tick = 0;
  uint32_t now = HAL_GetTick();

  if (GPIO_Pin == GPIO_PIN_5) {
    if (now - lastPB5Tick >= DEBOUNCE_MS) {
      uiStateCounter--;
      lastPB5Tick = now;
      updateState = 1;
    }
  }

  if (GPIO_Pin == GPIO_PIN_6) {
    if (now - lastPB6Tick >= DEBOUNCE_MS) {
      uiStateCounter++;
      lastPB6Tick = now;
      updateState = 1;
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
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // enable trace
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  gamble(&gachaFlag);

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S2_Init();
  MX_TIM4_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_RNG_Init();
  MX_USB_Device_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

  HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t *)mainBuff, 512);
  HAL_TIM_Base_Start_IT(&htim4);
  HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_Base_Start_IT(&htim7);
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  displayInit();
  displayText(TITLE, "Welcome!");
  displayText(BODY1, "press any button");
  displayText(BODY2, "to start");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1) {

    if (gachaFlag == 1) {
      blink();
    }

    adsrEnvStart(&adsrTick, &adsr, &releaseFlag, oscillator);

    scanMatrix(voiceCount, &scan, oscillator, &adsr, detuneCents, &releaseFlag);

    uint8_t uiState = uiStateCounter % 8;
    uint8_t redraw = 0;

    if (updateState == 1) {
      redraw = updateState;
      printf("counter: %d state: %d \r\n", uiStateCounter, uiState);
      updateState = 0;
    }

    switch (uiState) {
    case WAVE_TYPE: {
      static int8_t number = 0;
      if (parameterSet(&htim2, &number, PARAM_INT) || redraw == 1) {
        waveType = number & 1;
        displayClear();
        displayText(TITLE, "Wavetable");
        displayText(BODY1, waveType == 0 ? "SINE" : "SAW");
      }
      break;
    }
    case UNISON: {
      static int8_t number = 0;
      if (parameterSet(&htim2, &number, PARAM_INT) || redraw == 1) {
        voiceCount = number & 3;
        displayClear();
        displayText(TITLE, "Unison");
        displayText(BODY1, "Voices: %d", (1 << voiceCount));
      }
      break;
    }
    case DETUNE: {
      static int8_t number = 0;
      if (parameterSet(&htim2, &number, PARAM_INT) || redraw == 1) {
        detuneCents = number;
        displayClear();
        displayText(TITLE, "Detune");
        displayText(BODY1, "cents: %d", detuneCents);
      }
      break;
    }
    case ATK: {

      static int16_t number = 0;
      if (parameterSet(&htim2, &number, PARAM_INT) || redraw == 1) {
        adsr.attack = number * 10;
        displayClear();
        displayText(TITLE, "Attack");
        displayText(BODY1, "%d ms", adsr.attack);
      }
      break;
    }

    case DEC: {

      static int16_t number = 0;
      if (parameterSet(&htim2, &number, PARAM_INT) || redraw == 1) {
        adsr.decay = number * 10;
        displayClear();
        displayText(TITLE, "Decay");
        displayText(BODY1, "%d ms", adsr.decay);
      }
      break;
    }

    case SUS: {

      static int16_t number = 0;
      if (parameterSet(&htim2, &number, PARAM_INT) || redraw == 1) {
        adsr.sustain = 1.0f - (float)number / 50.0f;
        int8_t fdosentwork = (int)(adsr.sustain * 100);
        displayClear();
        displayText(TITLE, "Sustain");
        if (fdosentwork == 100) {
          displayText(BODY1, "1.00", fdosentwork);
        } else if (fdosentwork < 10) {
          displayText(BODY1, "0.0%d", fdosentwork);
        } else {
          displayText(BODY1, "0.%d", fdosentwork);
        }
      }
      break;
    }

    case REL: {

      static int16_t number = 0;
      if (parameterSet(&htim2, &number, PARAM_INT) || redraw == 1) {
        adsr.release = number * 10;
        displayClear();
        displayText(TITLE, "Release");
        displayText(BODY1, "%d ms", adsr.release);
      }
      break;
    }
    case PADDING: {

      static int16_t number = 0;
      if (parameterSet(&htim2, &number, PARAM_INT) || redraw == 1) {
        adsr.release = number * 10;
        displayClear();
        displayText(TITLE, "More stuff");
        displayText(BODY1, "comming soon!!");
      }
      break;
    }
    }
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

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state
   */
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
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
     file, line) */

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
