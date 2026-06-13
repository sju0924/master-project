/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32l5xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32l5xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "test_runner.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* USER CODE BEGIN 0 */
extern void HAL_MPU_Disable(void);
extern void HAL_MPU_Enable(uint32_t MPU_Control);
#define MPU_RNR          (*(volatile uint32_t *)0xE000ED98)
#define MPU_RLAR_REG     (*(volatile uint32_t *)0xE000EDA0)
#define MPU_PRIVILEGED_DEFAULT_IT  4U

/* Called after exception return redirects PC here */
static void test_recovery_fn(void) {
    longjmp(g_test_recovery, 1);
}

/*
 * Shared recovery body used by both MemManage and HardFault handlers.
 * Clears fault status registers, resets all dynamic MPU regions, then
 * redirects the exception return to test_recovery_fn so that longjmp
 * resumes the test loop without re-faulting.
 *
 * Exception frame layout (basic Cortex-M frame on MSP):
 *   [0]=R0 [1]=R1 [2]=R2 [3]=R3 [4]=R12 [5]=LR [6]=PC [7]=xPSR
 */
static void fault_recover_body(uint32_t *frame) {
    g_error_detected = 1;

    /* Clear HardFault status (harmless when called from MemManage) */
    SCB->HFSR = SCB->HFSR;
    /* Clear all configurable fault status bits */
    SCB->CFSR = SCB->CFSR;

    HAL_MPU_Disable();
    for (int i = 0; i < 7; i++) {
        MPU_RNR      = (uint32_t)i;
        MPU_RLAR_REG &= ~0x1UL;
    }
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT_IT);

    /* Overwrite stacked PC; clear ICI/IT bits in xPSR to avoid INVSTATE */
    frame[6] = (uint32_t)test_recovery_fn;
    frame[7] = (frame[7] | 0x01000000U) & ~0x0600FC00U;
}

void MemManage_Handler_C(uint32_t *frame) {
    if (!g_test_running) { while (1); }
    fault_recover_body(frame);
}

void HardFault_Handler_C(uint32_t *frame) {
    if (!g_test_running) { while (1); }
    fault_recover_body(frame);
}
/* USER CODE END 0 */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  *
  * Naked wrapper identical in structure to MemManage_Handler: captures MSP,
  * then calls HardFault_Handler_C which clears HFSR/CFSR, resets the MPU,
  * and redirects the stacked PC to test_recovery_fn so longjmp resumes the
  * test loop.  If no test is running the C handler hangs as before.
  */
__attribute__((naked)) void HardFault_Handler(void) {
    __asm volatile(
        ".syntax unified          \n"
        "mrs  r0, msp             \n"  /* r0 = exception frame pointer    */
        "push {lr}                \n"  /* save EXC_RETURN                 */
        "bl   HardFault_Handler_C \n"  /* call C body                     */
        "pop  {pc}                \n"  /* EXC_RETURN → exception return   */
    );
}

/**
  * @brief This function handles Memory management fault.
  *
  * Naked wrapper: captures MSP (= exception frame pointer) before the C
  * compiler can push any local-variable frame, then tail-calls the C handler.
  * On return from the C handler the saved EXC_RETURN value is popped into PC,
  * which triggers a proper exception return, clearing MEMFAULTACT and
  * restoring registers from the (now modified) exception frame.
  */
__attribute__((naked)) void MemManage_Handler(void) {
    __asm volatile(
        ".syntax unified        \n"
        "mrs  r0, msp           \n"  /* r0 = start of exception frame   */
        "push {lr}              \n"  /* save EXC_RETURN                  */
        "bl   MemManage_Handler_C\n" /* call C body with frame ptr in r0 */
        "pop  {pc}              \n"  /* EXC_RETURN → triggers exception return */
    );
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
__attribute__((naked)) void BusFault_Handler(void) {
    __asm volatile(
        ".syntax unified           \n"
        "mrs  r0, msp              \n"
        "push {lr}                 \n"
        "bl   HardFault_Handler_C  \n"
        "pop  {pc}                 \n"
    );
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
__attribute__((naked)) void UsageFault_Handler(void) {
    __asm volatile(
        ".syntax unified           \n"
        "mrs  r0, msp              \n"
        "push {lr}                 \n"
        "bl   HardFault_Handler_C  \n"
        "pop  {pc}                 \n"
    );
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32L5xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32l5xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
