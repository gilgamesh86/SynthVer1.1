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
#include "i2s.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef volatile uint8_t flag;

typedef struct {
  volatile uint32_t accumulator;
  volatile uint32_t step;
} oscillator;

typedef enum { FIRST_HALF = 0, SECOND_HALF = 256 } position;

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
} pin_t;

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

/*----------------------------random------------------------------------------*/

oscillator oscillator1 = {0, 0};

uint8_t pressed[8][6] = {0};
uint8_t prevState[8][6] = {0};
uint32_t lastKeyTime[8][6] = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void fillBuffer(position half) {
  static int32_t inputBuffer[128] = {0};
  static int32_t outputBuffer[128] = {0};
  for (uint8_t i = 0; i < 128; i++) {
    inputBuffer[i] = oscillator1.accumulator;
    oscillator1.accumulator += oscillator1.step;
  }
  HAL_CORDIC_Calculate(&hcordic, inputBuffer, outputBuffer, 128, HAL_MAX_DELAY);

  for (uint8_t i = 0; i < 128; i++) {
    mainBuff[(2 * i) + half] = (int16_t)(outputBuffer[i] >> 18);
    mainBuff[(2 * i) + 1 + half] = (int16_t)(outputBuffer[i] >> 18);
  }
}

void scanMatrix(void) {

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
            oscillator1.step = phaseTable[i][j];
          } else {
            oscillator1.step = 0;
          }
        }
      }
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
int main(void)
{

  /* USER CODE BEGIN 1 */
  SystemCoreClockUpdate();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
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
  /* USER CODE BEGIN 2 */
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
  HAL_I2S_Transmit_DMA(&hi2s2, mainBuff, 512);
  HAL_TIM_Base_Start_IT(&htim4);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {

    if (scan == 1) {
      scan = 0;
      scanMatrix();
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
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
  fillBuffer(FIRST_HALF);
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
  fillBuffer(SECOND_HALF);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM4) {
    scan = 1;
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
