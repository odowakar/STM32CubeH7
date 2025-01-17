 /**
  ******************************************************************************
  * @file    USB_Device/AUDIO_Standalone/Src/main.c 
  * @author  MCD Application Team
  * @brief   USB device Audio demo main file 
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------ */
#include "main.h"

/** @addtogroup STM32H7xx_HAL_Applications
  * @{
  */

/** @addtogroup AUDIO_Standalone
  * @{
  */

/* Private typedef ----------------------------------------------------------- */
/* Private define ------------------------------------------------------------ */
/* Private macro ------------------------------------------------------------- */
/* Private variables --------------------------------------------------------- */
USBD_HandleTypeDef USBD_Device;
extern PCD_HandleTypeDef hpcd;
/* Private function prototypes ----------------------------------------------- */
static void MPU_Config(void);
/* Private functions --------------------------------------------------------- */

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Configure the MPU attributes as Write Through */
  MPU_Config();
  
   /* STM32H7xx HAL library initialization:
     - Systick timer is configured by default as source of time base, but user
       can eventually implement his proper time base source (a general purpose
       timer for application or other time source), keeping in mind that Time base
       duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
       handled in milliseconds basis.
     - Set NVIC Group Priority to 4
     - Low Level Initialization: global MSP (MCU Support Package) initialization
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  HAL_PWREx_EnableUSBVoltageDetector();
  
  /* Init Device Library */
  USBD_Init(&USBD_Device, &AUDIO_Desc, 0);

  /* Add Supported Class */
  USBD_RegisterClass(&USBD_Device, USBD_AUDIO_CLASS);

  /* Add Interface callbacks for AUDIO Class */
  USBD_AUDIO_RegisterInterface(&USBD_Device, &USBD_AUDIO_fops);

  /* Start Device Process */
  USBD_Start(&USBD_Device);

  while (1)
  {
  }
}

/**
  * @brief  Clock Config.
  * @param  hsai: might be required to set audio peripheral predivider if any.
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @note   This API is called by BSP_AUDIO_OUT_Init() and BSP_AUDIO_OUT_SetFrequency()
  *         Being __weak it can be overwritten by the application     
  * @retval None
  */
void AUDIO_OUT_ClockConfig(SAI_HandleTypeDef * hsai, uint32_t AudioFreq,
                               void *Params)
{
  RCC_PeriphCLKInitTypeDef rcc_ex_clk_init_struct;

  HAL_RCCEx_GetPeriphCLKConfig(&rcc_ex_clk_init_struct);

  /* Set the PLL configuration according to the audio frequency */
  if ((AudioFreq == AUDIO_FREQUENCY_11K) || (AudioFreq == AUDIO_FREQUENCY_22K)
      || (AudioFreq == AUDIO_FREQUENCY_44K))
  {
    /* SAI clock config */
    /* Configure PLLSAI prescalers */
    /* PLL2_VCO Input = HSE_VALUE/PLL2M = 1 Mhz */
    /* PLL2_VCO Output = PLL2_VCO Input * PLL2N = 429 Mhz */
    /* SAI_CLK_x = PLL2_VCO Output/PLL2P = 429/38 = 11.267 Mhz */
    rcc_ex_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
    rcc_ex_clk_init_struct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
    rcc_ex_clk_init_struct.PLL2.PLL2P = 38;
    rcc_ex_clk_init_struct.PLL2.PLL2Q = 1;
    rcc_ex_clk_init_struct.PLL2.PLL2R = 1;
    rcc_ex_clk_init_struct.PLL2.PLL2N = 429;
    rcc_ex_clk_init_struct.PLL2.PLL2FRACN = 0;
    rcc_ex_clk_init_struct.PLL2.PLL2M = 25;
    HAL_RCCEx_PeriphCLKConfig(&rcc_ex_clk_init_struct);
  }
  else                          /* AUDIO_FREQUENCY_8K, AUDIO_FREQUENCY_16K, *
                                 * AUDIO_FREQUENCY_48K, AUDIO_FREQUENCY_96K */
  {
    /* SAI clock config */
    /* Configure PLLSAI prescalers */
    /* PLL2_VCO Input = HSE_VALUE/PLL2M = 1 Mhz */
    /* PLL2_VCO Output = PLL2_VCO Input * PLL2N = 344 Mhz */
    /* SAI_CLK_x = PLL2_VCO Output/PLL2P = 344/7 = 49.333 Mhz */
    rcc_ex_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
    rcc_ex_clk_init_struct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
    rcc_ex_clk_init_struct.PLL2.PLL2P = 7;
    rcc_ex_clk_init_struct.PLL2.PLL2Q = 1;
    rcc_ex_clk_init_struct.PLL2.PLL2R = 1;
    rcc_ex_clk_init_struct.PLL2.PLL2N = 344;
    rcc_ex_clk_init_struct.PLL2.PLL2FRACN = 0;
    rcc_ex_clk_init_struct.PLL2.PLL2M = 25;
    HAL_RCCEx_PeriphCLKConfig(&rcc_ex_clk_init_struct);
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 400000000 (CPU Clock)
  *            HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 160
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            PLL3_M                         = 25
  *            PLL3_N                         = 336
  *            PLL3_P                         = 2
  *            PLL3_Q                         = 7
  *            PLL3_R                         = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */

void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
  
  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
 
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  
  /* PLL1 for System Clock */
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
 
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOMEDIUM;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  
  /* PLL3 for USB Clock */
  PeriphClkInitStruct.PLL3.PLL3M = 25;
  PeriphClkInitStruct.PLL3.PLL3N = 336;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.PLL3.PLL3P = 2;
  PeriphClkInitStruct.PLL3.PLL3R = 2;
  PeriphClkInitStruct.PLL3.PLL3Q = 7;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOMEDIUM;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0;

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
  
  /* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
  RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);
  
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1; 
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  
  /* Activate CSI clock is mandatory for I/O Compensation Cell */
  __HAL_RCC_CSI_ENABLE() ;
  
  /* Enable SYSCFG clock is mandatory for I/O Compensation Cell */
  __HAL_RCC_SYSCFG_CLK_ENABLE() ;
  
  /* Enables the I/O Compensation Cell */
  HAL_EnableCompensationCell();
}

/**
  * @brief  Configure the MPU attributes as Write Through for D1 AXI SRAM.
  * @note   The Base Address is 0x24000000 since this memory interface is the AXI.
  *         The Region Size is 512KB, it is related to D1 AXI SRAM  memory size.
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as Normal Non Cacheable for SRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x24000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t * file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line
   * number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file, 
   * * line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
