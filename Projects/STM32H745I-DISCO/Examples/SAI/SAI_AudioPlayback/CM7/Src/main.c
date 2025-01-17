/**
  ******************************************************************************
  * @file    SAI/SAI_AudioPlayback/CM7/Src/main.c
  * @author  MCD Application Team
  * @brief   This example describes how to use SAI PDM feature to realize
  *          audio play.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup SAI_AudioPlayback
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
typedef enum {
  BUFFER_OFFSET_NONE = 0,
  BUFFER_OFFSET_HALF,
  BUFFER_OFFSET_FULL,
} BUFFER_StateTypeDef;

/* Private define ------------------------------------------------------------*/
#define AUDIO_FREQUENCY       SAI_AUDIO_FREQUENCY_16K
#define AUDIO_CHANNEL_NUMBER  2U
#define AUDIO_BUFFER_SIZE     256U
#define AUDIO_PCM_CHUNK_SIZE  32U

/* Private macro -------------------------------------------------------------*/
/* Audio PDM FS frequency = 128KHz = 8 * 16KHz = 8 * FS_Freq */
#define AUDIO_PDM_GET_FS_FREQUENCY(FS)  (FS * 8)

/* Private variables ---------------------------------------------------------*/

/* SAI input Handle */
SAI_HandleTypeDef  SaiInputHandle;

/* SAI output Handle */
SAI_HandleTypeDef  SaiOutputHandle;
static int32_t WM8994_Probe(void);
/* Audio codec Handle */
static AUDIO_Drv_t  *Audio_Drv = NULL;
void *Audio_CompObj = NULL;
WM8994_Init_t codec_init;
/* Buffer containing the PDM samples */
#if defined ( __CC_ARM )  /* !< ARM Compiler */
  /* Buffer location should aligned to cache line size (32 bytes) */
  ALIGN_32BYTES (uint16_t audioPdmBuf[AUDIO_BUFFER_SIZE]) __attribute__((section(".RAM_D3")));
#elif defined ( __ICCARM__ )  /* !< ICCARM Compiler */
  #pragma location=0x38000000
  /* Buffer location should aligned to cache line size (32 bytes) */
  ALIGN_32BYTES (uint16_t audioPdmBuf[AUDIO_BUFFER_SIZE]);
#elif defined ( __GNUC__ )  /* !< GNU Compiler */
  /* Buffer location should aligned to cache line size (32 bytes) */
  ALIGN_32BYTES (uint16_t audioPdmBuf[AUDIO_BUFFER_SIZE]) __attribute__((section(".RAM_D3")));
#endif

/* Buffer containing the PCM samples
   Buffer location should aligned to cache line size (32 bytes) */
ALIGN_32BYTES (uint16_t audioPcmBuf[AUDIO_BUFFER_SIZE]);

/* PDM Filters params */
PDM_Filter_Handler_t  PDM_FilterHandler[2];
PDM_Filter_Config_t   PDM_FilterConfig[2];

/* Pointer to PCM data */
__IO uint32_t pcmPtr;

/* Buffer status variable */
__IO BUFFER_StateTypeDef bufferStatus = BUFFER_OFFSET_NONE;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Playback_Init(void);
static void AUDIO_IN_PDMToPCM_Init(uint32_t AudioFreq, uint32_t ChannelNumber);
static void AUDIO_IN_PDMToPCM(uint16_t *PDMBuf, uint16_t *PCMBuf, uint32_t ChannelNumber);
static void CPU_CACHE_Enable(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* System Init, System clock, voltage scaling and L1-Cache configuration are done by CPU1 (Cortex-M7) 
     in the meantime Domain D2 is put in STOP mode(Cortex-M4 in deep-sleep)
  */

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32H7xx HAL library initialization:
       - Systick timer is configured by default as source of time base, but user
         can eventually implement his proper time base source (a general purpose
         timer for example or other time source), keeping in mind that Time base
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 400 MHz */
  SystemClock_Config();

  /* When system initialization is finished, Cortex-M7 could wakeup (when needed) the Cortex-M4  by means of 
     HSEM notification or by any D2 wakeup source (SEV,EXTI..)   */

  /* Configure LED1 */
  BSP_LED_Init(LED1);

  /* Configure LED2 */
  BSP_LED_Init(LED2);

  /* Initialize playback */
  Playback_Init();
  WM8994_Init_t codec_init;
  
  codec_init.Resolution   = 0;
  
  /* Fill codec_init structure */
  codec_init.Frequency    = 16000;
  codec_init.InputDevice  = WM8994_IN_NONE;
  codec_init.OutputDevice = AUDIO_OUT_DEVICE_HEADPHONE;
  
  /* Convert volume before sending to the codec */
  codec_init.Volume       = VOLUME_OUT_CONVERT(60);
  
  /* Start the playback */
  if(Audio_Drv->Init(Audio_CompObj, &codec_init) != 0)
  {
    Error_Handler();
  }
  
  /* Start the playback */
  if(Audio_Drv->Play(Audio_CompObj) < 0)
  {
    Error_Handler();
  }
  
  /* Start the PDM data reception process */
  if(HAL_OK != HAL_SAI_Receive_DMA(&SaiInputHandle, (uint8_t*)audioPdmBuf, AUDIO_BUFFER_SIZE))
  {
    Error_Handler();
  }

  /* Start the PCM data transmission process */
  if(HAL_OK != HAL_SAI_Transmit_DMA(&SaiOutputHandle, (uint8_t *)audioPcmBuf, AUDIO_BUFFER_SIZE))
  {
    Error_Handler();
  }

  /* Initialize Rx buffer status */
  bufferStatus &= BUFFER_OFFSET_NONE;

  /* Start loopback */
  while(1)
  {
    /* Wait Rx half transfert event */
    while((bufferStatus &  BUFFER_OFFSET_HALF) != BUFFER_OFFSET_HALF);

    /* Convert the first half of PDM data to PCM */
    AUDIO_IN_PDMToPCM((uint16_t*)&audioPdmBuf[0], &audioPcmBuf[pcmPtr], AUDIO_CHANNEL_NUMBER);

    /* Update pcmPtr */
    pcmPtr += AUDIO_PCM_CHUNK_SIZE;

    /* Initialize Rx buffer status */
    bufferStatus &= BUFFER_OFFSET_NONE;

    /* Wait Rx transfert complete event */
    while((bufferStatus &  BUFFER_OFFSET_FULL) != BUFFER_OFFSET_FULL);

    /* Convert the second half of PDM data to PCM */
    AUDIO_IN_PDMToPCM((uint16_t*)&audioPdmBuf[AUDIO_BUFFER_SIZE/2], &audioPcmBuf[pcmPtr], AUDIO_CHANNEL_NUMBER);

    /* Update pcmPtr */
    pcmPtr += AUDIO_PCM_CHUNK_SIZE;
    if(pcmPtr >= AUDIO_BUFFER_SIZE)
    {
      pcmPtr = 0;
    }

    /* Initialize Rx buffer status */
    bufferStatus &= BUFFER_OFFSET_NONE;

    /* Toggle LED1 */
    BSP_LED_Toggle(LED1);
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
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

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

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

/* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  Playback initialization
  * @param  None
  * @retval None
  */
static void Playback_Init(void)
{
  RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct;

  /* Configure PLLSAI1 prescalers */
  /* PLLSAI_VCO: VCO_512M
     SAI_CLK(first level) = PLLSAI_VCO/PLLSAIP = 512/125 = 4.096 Mhz */
  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI2;
  RCC_PeriphCLKInitStruct.Sai23ClockSelection = RCC_SAI2CLKSOURCE_PLL2;
  RCC_PeriphCLKInitStruct.PLL2.PLL2P = 125;
  RCC_PeriphCLKInitStruct.PLL2.PLL2Q = 1;
  RCC_PeriphCLKInitStruct.PLL2.PLL2R = 1;
  RCC_PeriphCLKInitStruct.PLL2.PLL2N = 512;
  RCC_PeriphCLKInitStruct.PLL2.PLL2FRACN = 0;
  RCC_PeriphCLKInitStruct.PLL2.PLL2M = 25;

  if(HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure PLLSAI4A prescalers */
  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI4A;
  RCC_PeriphCLKInitStruct.Sai4AClockSelection = RCC_SAI4ACLKSOURCE_PLL2;
  if(HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* SAI PDM Input init */
  __HAL_SAI_RESET_HANDLE_STATE(&SaiInputHandle);

  SaiInputHandle.Instance                 = AUDIO_IN_SAI_PDMx;
  SaiInputHandle.Init.AudioMode           = SAI_MODEMASTER_RX;
  SaiInputHandle.Init.Synchro             = SAI_ASYNCHRONOUS;
  SaiInputHandle.Init.SynchroExt          = SAI_SYNCEXT_DISABLE;
  SaiInputHandle.Init.OutputDrive         = SAI_OUTPUTDRIVE_DISABLE;
  SaiInputHandle.Init.NoDivider           = SAI_MASTERDIVIDER_DISABLE;
  SaiInputHandle.Init.FIFOThreshold       = SAI_FIFOTHRESHOLD_HF;
  SaiInputHandle.Init.AudioFrequency      = AUDIO_PDM_GET_FS_FREQUENCY(AUDIO_FREQUENCY);
  SaiInputHandle.Init.Mckdiv              = 0;
  SaiInputHandle.Init.MckOverSampling     = SAI_MCK_OVERSAMPLING_DISABLE;
  SaiInputHandle.Init.MonoStereoMode      = SAI_STEREOMODE;
  SaiInputHandle.Init.CompandingMode      = SAI_NOCOMPANDING;
  SaiInputHandle.Init.TriState            = SAI_OUTPUT_NOTRELEASED;
  SaiInputHandle.Init.PdmInit.Activation  = ENABLE;
  SaiInputHandle.Init.PdmInit.MicPairsNbr = 2;
  SaiInputHandle.Init.PdmInit.ClockEnable = SAI_PDM_CLOCK2_ENABLE;
  SaiInputHandle.Init.Protocol            = SAI_FREE_PROTOCOL;
  SaiInputHandle.Init.DataSize            = SAI_DATASIZE_16;
  SaiInputHandle.Init.FirstBit            = SAI_FIRSTBIT_LSB;
  SaiInputHandle.Init.ClockStrobing       = SAI_CLOCKSTROBING_FALLINGEDGE;

  SaiInputHandle.FrameInit.FrameLength       = 32;
  SaiInputHandle.FrameInit.ActiveFrameLength = 1;
  SaiInputHandle.FrameInit.FSDefinition      = SAI_FS_STARTFRAME;
  SaiInputHandle.FrameInit.FSPolarity        = SAI_FS_ACTIVE_HIGH;
  SaiInputHandle.FrameInit.FSOffset          = SAI_FS_FIRSTBIT;

  SaiInputHandle.SlotInit.FirstBitOffset = 0;
  SaiInputHandle.SlotInit.SlotSize       = SAI_SLOTSIZE_DATASIZE;
  SaiInputHandle.SlotInit.SlotNumber     = 2;
  SaiInputHandle.SlotInit.SlotActive     = SAI_SLOTACTIVE_1;

  /* DeInit SAI PDM input */
  HAL_SAI_DeInit(&SaiInputHandle);

  /* Init SAI PDM input */
  if(HAL_OK != HAL_SAI_Init(&SaiInputHandle))
  {
    Error_Handler();
  }

  /* Enable SAI to generate clock used by audio driver */
  __HAL_SAI_ENABLE(&SaiInputHandle);

  /* SAI PCM Output init */
  __HAL_SAI_RESET_HANDLE_STATE(&SaiOutputHandle);

  SaiOutputHandle.Instance            = AUDIO_OUT_SAIx;
  SaiOutputHandle.Init.AudioMode      = SAI_MODEMASTER_TX;
  SaiOutputHandle.Init.Synchro        = SAI_ASYNCHRONOUS;
  SaiOutputHandle.Init.OutputDrive    = SAI_OUTPUTDRIVE_ENABLE;
  SaiOutputHandle.Init.NoDivider      = SAI_MASTERDIVIDER_ENABLE;
  SaiOutputHandle.Init.FIFOThreshold  = SAI_FIFOTHRESHOLD_1QF;
  SaiOutputHandle.Init.AudioFrequency = AUDIO_FREQUENCY;
  SaiOutputHandle.Init.Protocol       = SAI_FREE_PROTOCOL;
  SaiOutputHandle.Init.DataSize       = SAI_DATASIZE_16;
  SaiOutputHandle.Init.FirstBit       = SAI_FIRSTBIT_MSB;
  SaiOutputHandle.Init.ClockStrobing  = SAI_CLOCKSTROBING_FALLINGEDGE;

  SaiOutputHandle.FrameInit.FrameLength       = 128;
  SaiOutputHandle.FrameInit.ActiveFrameLength = 64;
  SaiOutputHandle.FrameInit.FSDefinition      = SAI_FS_CHANNEL_IDENTIFICATION;
  SaiOutputHandle.FrameInit.FSPolarity        = SAI_FS_ACTIVE_LOW;
  SaiOutputHandle.FrameInit.FSOffset          = SAI_FS_BEFOREFIRSTBIT;

  SaiOutputHandle.SlotInit.FirstBitOffset = 0;
  SaiOutputHandle.SlotInit.SlotSize       = SAI_SLOTSIZE_DATASIZE;
  SaiOutputHandle.SlotInit.SlotNumber     = 4;
  SaiOutputHandle.SlotInit.SlotActive     = (SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_2);

  /* DeInit SAI PCM input */
  HAL_SAI_DeInit(&SaiOutputHandle);

  /* Init SAI PCM input */
  if(HAL_OK != HAL_SAI_Init(&SaiOutputHandle))
  {
    Error_Handler();
  }

  /* Enable SAI to generate clock used by audio driver */
  __HAL_SAI_ENABLE(&SaiOutputHandle);
    WM8994_Probe();

  /* Init PDM Filters */
  AUDIO_IN_PDMToPCM_Init(AUDIO_FREQUENCY, AUDIO_CHANNEL_NUMBER);
}
/**
  * @brief  Register Bus IOs if component ID is OK
  * @retval error status
  */
static int32_t WM8994_Probe(void)
{
  int32_t ret = BSP_ERROR_NONE;
  WM8994_IO_t              IOCtx;
  static WM8994_Object_t   WM8994Obj;
  uint32_t id;

  /* Configure the audio driver */
  IOCtx.Address     = AUDIO_I2C_ADDRESS;
  IOCtx.Init        = BSP_I2C4_Init;
  IOCtx.DeInit      = BSP_I2C4_DeInit;
  IOCtx.ReadReg     = BSP_I2C4_ReadReg16;
  IOCtx.WriteReg    = BSP_I2C4_WriteReg16;
  IOCtx.GetTick     = BSP_GetTick;

  if(WM8994_RegisterBusIO (&WM8994Obj, &IOCtx) != WM8994_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }
  else
  {
    /* Reset the codec */
    if(WM8994_Reset(&WM8994Obj) != WM8994_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(WM8994_ReadID(&WM8994Obj, &id) != WM8994_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else if(id != WM8994_ID)
    {
      ret = BSP_ERROR_UNKNOWN_COMPONENT;
    }
    else
    {
      Audio_Drv = (AUDIO_Drv_t *) &WM8994_Driver;
      Audio_CompObj = &WM8994Obj;
    }
  }
  return ret;
}
/**
* @brief  Init PDM Filters.
* @param  AudioFreq: Audio sampling frequency
* @param  ChannelNumber: Number of audio channels in the PDM buffer.
* @retval None
*/
static void AUDIO_IN_PDMToPCM_Init(uint32_t AudioFreq, uint32_t ChannelNumber)
{
  uint32_t index = 0;

  /* Enable CRC peripheral to unlock the PDM library */
  __HAL_RCC_CRC_CLK_ENABLE();

  for(index = 0; index < ChannelNumber; index++)
  {
    /* Init PDM filters */
    PDM_FilterHandler[index].bit_order  = PDM_FILTER_BIT_ORDER_MSB;
    PDM_FilterHandler[index].endianness = PDM_FILTER_ENDIANNESS_LE;
    PDM_FilterHandler[index].high_pass_tap = 2122358088;
    PDM_FilterHandler[index].out_ptr_channels = ChannelNumber;
    PDM_FilterHandler[index].in_ptr_channels  = ChannelNumber;
    PDM_Filter_Init((PDM_Filter_Handler_t *)(&PDM_FilterHandler[index]));

    /* Configure PDM filters */
    PDM_FilterConfig[index].output_samples_number = AudioFreq/1000;
    PDM_FilterConfig[index].mic_gain = 24;
    PDM_FilterConfig[index].decimation_factor = PDM_FILTER_DEC_FACTOR_64;
    PDM_Filter_setConfig((PDM_Filter_Handler_t *)&PDM_FilterHandler[index], &PDM_FilterConfig[index]);
  }
}

/**
  * @brief  Convert audio format from PDM to PCM.
  * @param  PDMBuf: Pointer to PDM buffer data
  * @param  PCMBuf: Pointer to PCM buffer data
  * @param  ChannelNumber: PDM Channels number.
  * @retval None
*/
static void AUDIO_IN_PDMToPCM(uint16_t *PDMBuf, uint16_t *PCMBuf, uint32_t ChannelNumber)
{
  uint32_t index = 0;

  /* Invalidate Data Cache to get the updated content of the SRAM*/
  SCB_InvalidateDCache_by_Addr((uint32_t *)PDMBuf, AUDIO_BUFFER_SIZE);

  for(index = 0; index < ChannelNumber; index++)
  {
    PDM_Filter(&((uint8_t*)(PDMBuf))[index], (uint16_t*)&(PCMBuf[index]), &PDM_FilterHandler[index]);
  }

  /* Clean Data Cache to update the content of the SRAM */
  SCB_CleanDCache_by_Addr((uint32_t*)PCMBuf, AUDIO_PCM_CHUNK_SIZE*2);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* LED2 On in error case */
  BSP_LED_On(LED2);
  while (1)
  {
  }
}


/**
  * @brief  Rx Transfer completed callbacks.
  * @param  hsai : pointer to a SAI_HandleTypeDef structure that contains
  *                the configuration information for SAI module.
  * @retval None
  */
void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_SAI_RxCpltCallback could be implemented in the user file
   */
  bufferStatus |= BUFFER_OFFSET_FULL;
}

/**
  * @brief  Rx Transfer Half completed callbacks
  * @param  hsai : pointer to a SAI_HandleTypeDef structure that contains
  *                the configuration information for SAI module.
  * @retval None
  */
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_SAI_RxHalfCpltCallback could be implenetd in the user file
   */
  bufferStatus |= BUFFER_OFFSET_HALF;
}

/**
* @brief  CPU L1-Cache enable.
* @param  None
* @retval None
*/
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

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
