/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - VELA FLIGHT SOFTWARE (Phase 10: TinyML)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "protocol.h"
#include "ai_model.h" /* Auto-generated AI Weights from Python */
#include <string.h>
#include <math.h>

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Definitions for FreeRTOS Tasks */
osThreadId_t commsTaskHandle;
const osThreadAttr_t commsTask_attributes = {
  .name = "commsTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t telemetryTaskHandle;
const osThreadAttr_t telemetryTask_attributes = {
  .name = "telemetryTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t aiTaskHandle;
const osThreadAttr_t aiTask_attributes = {
  .name = "aiTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

osThreadId_t housekeepingTaskHandle;
const osThreadAttr_t housekeepingTask_attributes = {
  .name = "housekeepingTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Definitions for FreeRTOS Queue */
osMessageQueueId_t telemQueueHandle;
const osMessageQueueAttr_t telemQueue_attributes = {
  .name = "telemQueue"
};

/* Watchdog & Global State Flags */
volatile uint8_t commsAlive = 0;
volatile uint8_t telemetryAlive = 0;

/* Global AI Inference Variables */
volatile float live_temp = 0.0f;
volatile float live_rad = 0.0f;
volatile float live_att = 0.0f;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void StartCommsTask(void *argument);
void StartTelemetryTask(void *argument);
void StartAITask(void *argument);
void StartHousekeepingTask(void *argument);

/* USER CODE BEGIN PFP */
void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};
  HAL_MPU_Disable();

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x20010000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256B;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
/* USER CODE END PFP */

int main(void)
{
  HAL_Init();
  MPU_Config();

  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  osKernelInitialize();

  telemQueueHandle = osMessageQueueNew (5, sizeof(TelemetryFrame_t), &telemQueue_attributes);

  commsTaskHandle = osThreadNew(StartCommsTask, NULL, &commsTask_attributes);
  telemetryTaskHandle = osThreadNew(StartTelemetryTask, NULL, &telemetryTask_attributes);
  aiTaskHandle = osThreadNew(StartAITask, NULL, &aiTask_attributes);
  housekeepingTaskHandle = osThreadNew(StartHousekeepingTask, NULL, &housekeepingTask_attributes);

  osKernelStart();

  while (1)
  {
  }
}

/* ------------------------------------------------------------------------- */
/*                             FREERTOS TASKS                                */
/* ------------------------------------------------------------------------- */

void StartCommsTask(void *argument)
{
  uint8_t rxByte;
  TelemetryFrame_t frame;

  for(;;)
  {
    if (HAL_UART_Receive(&huart2, &rxByte, 1, 10) == HAL_OK)
    {
      if (rxByte == 0x55)
      {
        if (HAL_UART_Receive(&huart2, &rxByte, 1, 10) == HAL_OK && rxByte == 0xAA)
        {
          frame.sync = 0xAA55;
          HAL_UART_Receive(&huart2, (uint8_t*)&frame.type, sizeof(TelemetryFrame_t) - 2, 50);
          
          osMessageQueuePut(telemQueueHandle, &frame, 0, 0);
          commsAlive = 1;
        }
      }
    }
    osDelay(1);
  }
}

void StartTelemetryTask(void *argument)
{
  TelemetryFrame_t rxFrame;

  for(;;)
  {
    if (osMessageQueueGet(telemQueueHandle, &rxFrame, NULL, osWaitForever) == osOK)
    {
      /* Unpack for the AI Task */
      live_temp = rxFrame.payload.temperature;
      live_rad  = (float)rxFrame.payload.radiation;
      live_att  = rxFrame.payload.attitude;

      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      telemetryAlive = 1;
    }
  }
}

/**
 * @brief Edge AI Anomaly Detection Task
 * Runs continuous TinyML inference using the Python-trained statistical model.
 */
void StartAITask(void *argument)
{
  float z_temp, z_rad, z_att, max_anomaly_score;

  for(;;)
  {
    /* 1. Inference: Calculate deviation from learned normal bounds */
    z_temp = fabs((live_temp - AI_TEMP_MEAN) / AI_TEMP_STD);
    z_rad  = fabs((live_rad - AI_RAD_MEAN) / AI_RAD_STD);
    z_att  = fabs((live_att - AI_ATT_MEAN) / AI_ATT_STD);

    /* 2. Find the highest deviation */
    max_anomaly_score = z_temp;
    if (z_rad > max_anomaly_score) max_anomaly_score = z_rad;
    if (z_att > max_anomaly_score) max_anomaly_score = z_att;

    /* 3. Decision: Trigger AI Safe Mode if deviation exceeds 3-Sigma threshold */
    if (max_anomaly_score > AI_Z_THRESHOLD)
    {
       /*
        * Anomaly detected!
        * Execute 3 rapid warning flashes overriding the normal telemetry blink.
        */
       for(int i = 0; i < 6; i++) {
           HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
           osDelay(50);
       }
    }

    osDelay(500); /* Run inference at 2Hz */
  }
}

void StartHousekeepingTask(void *argument)
{
  for(;;)
  {
    osDelay(2000);

    if (commsAlive == 0 || telemetryAlive == 0)
    {
      /* Flash SOS Pattern */
      for (int i = 0; i < 20; i++)
      {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(50);
      }
      /* Hard Reboot */
      NVIC_SystemReset();
    }

    commsAlive = 0;
    telemetryAlive = 0;
  }
}

/* ------------------------------------------------------------------------- */
/*                       AUTO-GENERATED CUBEIDE BOILERPLATE                  */
/* ------------------------------------------------------------------------- */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
