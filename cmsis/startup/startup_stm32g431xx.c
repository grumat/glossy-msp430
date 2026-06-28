/*
	This file contains the entry point (Reset_Handler) of your firmware project.
	The reset handled initializes the RAM and calls system library initializers as well as
	the platform-specific initializer and the main() function.
*/

#include <stddef.h>
extern void *_estack;

void Reset_Handler();
void Default_Handler();

#ifndef DEBUG_DEFAULT_INTERRUPT_HANDLERS

#ifdef DEBUG
#define DEBUG_DEFAULT_INTERRUPT_HANDLERS 1
#else
#define DEBUG_DEFAULT_INTERRUPT_HANDLERS 0
#endif

#endif

#if DEBUG_DEFAULT_INTERRUPT_HANDLERS
void __attribute__ ((weak, naked)) NMI_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void NMI_Handler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) HardFault_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void HardFault_Handler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) MemManage_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void MemManage_Handler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) BusFault_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void BusFault_Handler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) UsageFault_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void UsageFault_Handler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) SVC_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SVC_Handler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DebugMon_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DebugMon_Handler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) PendSV_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void PendSV_Handler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) SysTick_Handler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SysTick_Handler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) WWDG_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void WWDG_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) PVD_PVM_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void PVD_PVM_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) RTC_TAMP_LSECSS_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void RTC_TAMP_LSECSS_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) RTC_WKUP_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void RTC_WKUP_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) FLASH_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void FLASH_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) RCC_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void RCC_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) EXTI0_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void EXTI0_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) EXTI1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void EXTI1_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) EXTI2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void EXTI2_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) EXTI3_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void EXTI3_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) EXTI4_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void EXTI4_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA1_Channel1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA1_Channel1_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA1_Channel2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA1_Channel2_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA1_Channel3_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA1_Channel3_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA1_Channel4_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA1_Channel4_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA1_Channel5_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA1_Channel5_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA1_Channel6_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA1_Channel6_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) ADC1_2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void ADC1_2_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) USB_HP_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void USB_HP_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) USB_LP_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void USB_LP_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) FDCAN1_IT0_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void FDCAN1_IT0_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) FDCAN1_IT1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void FDCAN1_IT1_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) EXTI9_5_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void EXTI9_5_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM1_BRK_TIM15_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM1_BRK_TIM15_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM1_UP_TIM16_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM1_UP_TIM16_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM1_TRG_COM_TIM17_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM1_TRG_COM_TIM17_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM1_CC_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM1_CC_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM2_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM3_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM3_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM4_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM4_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) I2C1_EV_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void I2C1_EV_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) I2C1_ER_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void I2C1_ER_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) I2C2_EV_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void I2C2_EV_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) I2C2_ER_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void I2C2_ER_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) SPI1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SPI1_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) SPI2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SPI2_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) USART1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void USART1_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) USART2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void USART2_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) USART3_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void USART3_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) EXTI15_10_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void EXTI15_10_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) RTC_Alarm_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void RTC_Alarm_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) USBWakeUp_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void USBWakeUp_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM8_BRK_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM8_BRK_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM8_UP_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM8_UP_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM8_TRG_COM_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM8_TRG_COM_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM8_CC_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM8_CC_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) LPTIM1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void LPTIM1_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) SPI3_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SPI3_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) UART4_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void UART4_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM6_DAC_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM6_DAC_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) TIM7_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void TIM7_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA2_Channel1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA2_Channel1_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA2_Channel2_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA2_Channel2_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA2_Channel3_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA2_Channel3_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA2_Channel4_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA2_Channel4_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA2_Channel5_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA2_Channel5_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) UCPD1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void UCPD1_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) COMP1_2_3_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void COMP1_2_3_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) COMP4_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void COMP4_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) CRS_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void CRS_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) SAI1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void SAI1_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) FPU_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void FPU_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) RNG_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void RNG_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) LPUART1_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void LPUART1_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) I2C3_EV_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void I2C3_EV_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) I2C3_ER_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void I2C3_ER_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMAMUX_OVR_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMAMUX_OVR_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) DMA2_Channel6_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void DMA2_Channel6_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) CORDIC_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void CORDIC_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}
void __attribute__ ((weak, naked)) FMAC_IRQHandler() 
{
	//If you hit the breakpoint below, one of the interrupts was unhandled in your code. 
	//Define the following function in your code to handle it:
	//	extern "C" void FMAC_IRQHandler();
	__asm("bkpt 255");
	__asm("bx lr");
}

#else
void NMI_Handler()                    __attribute__ ((weak, alias ("Default_Handler")));
void HardFault_Handler()              __attribute__ ((weak, alias ("Default_Handler")));
void MemManage_Handler()              __attribute__ ((weak, alias ("Default_Handler")));
void BusFault_Handler()               __attribute__ ((weak, alias ("Default_Handler")));
void UsageFault_Handler()             __attribute__ ((weak, alias ("Default_Handler")));
void SVC_Handler()                    __attribute__ ((weak, alias ("Default_Handler")));
void DebugMon_Handler()               __attribute__ ((weak, alias ("Default_Handler")));
void PendSV_Handler()                 __attribute__ ((weak, alias ("Default_Handler")));
void SysTick_Handler()                __attribute__ ((weak, alias ("Default_Handler")));
void WWDG_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void PVD_PVM_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void RTC_TAMP_LSECSS_IRQHandler()     __attribute__ ((weak, alias ("Default_Handler")));
void RTC_WKUP_IRQHandler()            __attribute__ ((weak, alias ("Default_Handler")));
void FLASH_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void RCC_IRQHandler()                 __attribute__ ((weak, alias ("Default_Handler")));
void EXTI0_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void EXTI1_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void EXTI2_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void EXTI3_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void EXTI4_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Channel1_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Channel2_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Channel3_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Channel4_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Channel5_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_Channel6_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void ADC1_2_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void USB_HP_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void USB_LP_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void FDCAN1_IT0_IRQHandler()          __attribute__ ((weak, alias ("Default_Handler")));
void FDCAN1_IT1_IRQHandler()          __attribute__ ((weak, alias ("Default_Handler")));
void EXTI9_5_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void TIM1_BRK_TIM15_IRQHandler()      __attribute__ ((weak, alias ("Default_Handler")));
void TIM1_UP_TIM16_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void TIM1_TRG_COM_TIM17_IRQHandler()  __attribute__ ((weak, alias ("Default_Handler")));
void TIM1_CC_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void TIM2_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void TIM3_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void TIM4_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void I2C1_EV_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void I2C1_ER_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void I2C2_EV_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void I2C2_ER_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void SPI1_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void SPI2_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void USART1_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void USART2_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void USART3_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void EXTI15_10_IRQHandler()           __attribute__ ((weak, alias ("Default_Handler")));
void RTC_Alarm_IRQHandler()           __attribute__ ((weak, alias ("Default_Handler")));
void USBWakeUp_IRQHandler()           __attribute__ ((weak, alias ("Default_Handler")));
void TIM8_BRK_IRQHandler()            __attribute__ ((weak, alias ("Default_Handler")));
void TIM8_UP_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void TIM8_TRG_COM_IRQHandler()        __attribute__ ((weak, alias ("Default_Handler")));
void TIM8_CC_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void LPTIM1_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void SPI3_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void UART4_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void TIM6_DAC_IRQHandler()            __attribute__ ((weak, alias ("Default_Handler")));
void TIM7_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Channel1_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Channel2_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Channel3_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Channel4_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Channel5_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void UCPD1_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void COMP1_2_3_IRQHandler()           __attribute__ ((weak, alias ("Default_Handler")));
void COMP4_IRQHandler()               __attribute__ ((weak, alias ("Default_Handler")));
void CRS_IRQHandler()                 __attribute__ ((weak, alias ("Default_Handler")));
void SAI1_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
void FPU_IRQHandler()                 __attribute__ ((weak, alias ("Default_Handler")));
void RNG_IRQHandler()                 __attribute__ ((weak, alias ("Default_Handler")));
void LPUART1_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void I2C3_EV_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void I2C3_ER_IRQHandler()             __attribute__ ((weak, alias ("Default_Handler")));
void DMAMUX_OVR_IRQHandler()          __attribute__ ((weak, alias ("Default_Handler")));
void DMA2_Channel6_IRQHandler()       __attribute__ ((weak, alias ("Default_Handler")));
void CORDIC_IRQHandler()              __attribute__ ((weak, alias ("Default_Handler")));
void FMAC_IRQHandler()                __attribute__ ((weak, alias ("Default_Handler")));
#endif

void * g_pfnVectors[0x76] __attribute__ ((section (".isr_vector"), used)) = 
{
	&_estack,
	&Reset_Handler,
	&NMI_Handler,
	&HardFault_Handler,
	&MemManage_Handler,
	&BusFault_Handler,
	&UsageFault_Handler,
	NULL,
	NULL,
	NULL,
	NULL,
	&SVC_Handler,
	&DebugMon_Handler,
	NULL,
	&PendSV_Handler,
	&SysTick_Handler,
	&WWDG_IRQHandler,
	&PVD_PVM_IRQHandler,
	&RTC_TAMP_LSECSS_IRQHandler,
	&RTC_WKUP_IRQHandler,
	&FLASH_IRQHandler,
	&RCC_IRQHandler,
	&EXTI0_IRQHandler,
	&EXTI1_IRQHandler,
	&EXTI2_IRQHandler,
	&EXTI3_IRQHandler,
	&EXTI4_IRQHandler,
	&DMA1_Channel1_IRQHandler,
	&DMA1_Channel2_IRQHandler,
	&DMA1_Channel3_IRQHandler,
	&DMA1_Channel4_IRQHandler,
	&DMA1_Channel5_IRQHandler,
	&DMA1_Channel6_IRQHandler,
	NULL,
	&ADC1_2_IRQHandler,
	&USB_HP_IRQHandler,
	&USB_LP_IRQHandler,
	&FDCAN1_IT0_IRQHandler,
	&FDCAN1_IT1_IRQHandler,
	&EXTI9_5_IRQHandler,
	&TIM1_BRK_TIM15_IRQHandler,
	&TIM1_UP_TIM16_IRQHandler,
	&TIM1_TRG_COM_TIM17_IRQHandler,
	&TIM1_CC_IRQHandler,
	&TIM2_IRQHandler,
	&TIM3_IRQHandler,
	&TIM4_IRQHandler,
	&I2C1_EV_IRQHandler,
	&I2C1_ER_IRQHandler,
	&I2C2_EV_IRQHandler,
	&I2C2_ER_IRQHandler,
	&SPI1_IRQHandler,
	&SPI2_IRQHandler,
	&USART1_IRQHandler,
	&USART2_IRQHandler,
	&USART3_IRQHandler,
	&EXTI15_10_IRQHandler,
	&RTC_Alarm_IRQHandler,
	&USBWakeUp_IRQHandler,
	&TIM8_BRK_IRQHandler,
	&TIM8_UP_IRQHandler,
	&TIM8_TRG_COM_IRQHandler,
	&TIM8_CC_IRQHandler,
	NULL,
	NULL,
	&LPTIM1_IRQHandler,
	NULL,
	&SPI3_IRQHandler,
	&UART4_IRQHandler,
	NULL,
	&TIM6_DAC_IRQHandler,
	&TIM7_IRQHandler,
	&DMA2_Channel1_IRQHandler,
	&DMA2_Channel2_IRQHandler,
	&DMA2_Channel3_IRQHandler,
	&DMA2_Channel4_IRQHandler,
	&DMA2_Channel5_IRQHandler,
	NULL,
	NULL,
	&UCPD1_IRQHandler,
	&COMP1_2_3_IRQHandler,
	&COMP4_IRQHandler,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&CRS_IRQHandler,
	&SAI1_IRQHandler,
	NULL,
	NULL,
	NULL,
	NULL,
	&FPU_IRQHandler,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&RNG_IRQHandler,
	&LPUART1_IRQHandler,
	&I2C3_EV_IRQHandler,
	&I2C3_ER_IRQHandler,
	&DMAMUX_OVR_IRQHandler,
	NULL,
	NULL,
	&DMA2_Channel6_IRQHandler,
	NULL,
	NULL,
	&CORDIC_IRQHandler,
	&FMAC_IRQHandler,
};

void SystemInit();
void __libc_init_array();
int main();

extern void *_sidata, *_sdata, *_edata;
extern void *_sbss, *_ebss;

void __attribute__((naked, noreturn)) Reset_Handler()
{
	//Normally the CPU should will setup the based on the value from the first entry in the vector table.
	//If you encounter problems with accessing stack variables during initialization, ensure the line below is enabled.
	#if defined(sram_layout) || defined(INITIALIZE_SP_AT_RESET)
	__asm ("ldr sp, =_estack");
	#endif

	void **pSource, **pDest;
	for (pSource = &_sidata, pDest = &_sdata; pDest < &_edata; pSource++, pDest++)
		*pDest = *pSource;

	for (pDest = &_sbss; pDest < &_ebss; pDest++)
		*pDest = 0;

	SystemInit();
	__libc_init_array();
	(void)main();
	for (;;) ;
}

void __attribute__((naked, noreturn)) Default_Handler()
{
	//If you get stuck here, your code is missing a handler for some interrupt.
	//Define a 'DEBUG_DEFAULT_INTERRUPT_HANDLERS' macro via VisualGDB Project Properties and rebuild your project.
	//This will pinpoint a specific missing vector.
	for (;;) ;
}
