/**
  ******************************************************************************
  * @file    OPAMP/OPAMP_Calibration/Inc/main.h
  * @author  MCD Application Team
  * @brief   Header for main.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "stm32h735g_discovery.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* User can use this section to tailor COMPx instance under use and associated
   resources */

/* ## Definition of OPAMP related resources ################################### */
/* Definition of OPAMPx clock resources */
#define OPAMPx                            OPAMP2
#define OPAMPx_CLK_ENABLE()               __HAL_RCC_OPAMP_CLK_ENABLE()
#define OPAMPx_NON_INVERTING_INPUT_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOE_CLK_ENABLE()
#define OPAMPx_OUTPUT_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOE_CLK_ENABLE()

#define OPAMPx_FORCE_RESET()              __HAL_RCC_OPAMP_FORCE_RESET()
#define OPAMPx_RELEASE_RESET()            __HAL_RCC_OPAMP_RELEASE_RESET()
   
/* Definition of OPAMPx channels pins */
#define OPAMP2_NON_INVERTING_INPUT_PIN        GPIO_PIN_9
#define OPAMP2_NON_INVERTING_INPUT_GPIO_PORT  GPIOE
#define OPAMP2_OUTPUT_PIN                     GPIO_PIN_7
#define OPAMP2_OUTPUT_GPIO_PORT               GPIOE
   
/* ## Definition of DAC related resources ################################### */
/* Definition of DACx clock resources */
#define DACx                            DAC1
#define DACx_CLK_ENABLE()               __HAL_RCC_DAC12_CLK_ENABLE()
#define DACx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define DACx_FORCE_RESET()              __HAL_RCC_DAC12_FORCE_RESET()
#define DACx_RELEASE_RESET()            __HAL_RCC_DAC12_RELEASE_RESET()

/* Definition of DACx channels */
#define DACx_CHANNELa                   DAC_CHANNEL_2

/* Definition of DACx channels pins */
#define DACx_CHANNELa_PIN               GPIO_PIN_5
#define DACx_CHANNELa_GPIO_PORT         GPIOA

/* Definition of DACx DMA resources */
#define DACx_CHANNELa_DMA_CLK_ENABLE()           __HAL_RCC_DMA2_CLK_ENABLE()
#define DACx_CHANNELa_DMA                        DMA2_Stream6

#define DACx_CHANNELa_DMA_IRQn                   DMA2_Stream6_IRQn
#define DACx_CHANNELa_DMA_IRQHandler             DMA2_Stream6_IRQHandler


/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
