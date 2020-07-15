/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of 
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 * 
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and 
 * accompanying documentation covered by this license (the "Software") to use, reproduce, display, distribute, execute,
 * and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the
 * Software is furnished to do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all 
 * derivative works of the Software, unless such copies or derivative works are solely in the form of 
 * machine-executable object code generated by a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE 
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *********************************************************************************************************************/
/*\title Multicore LED control
 * \abstract One LED is controlled by using three different cores.
 * \description Core 0 is switching on an LED. When the LED flag is set, Core 1 is switching off the LED.
 *              Core 2 is controlling the state of the LED flag.
 *
 * \name Multicore_1_KIT_TC297_TFT
 * \version V1.0.0
 * \board APPLICATION KIT TC2X7 V1.1, KIT_AURIX_TC297_TFT_BC-Step, TC29xTA/TX_BC-step
 * \keywords multicore, LED, AURIX, Multicore_1
 * \documents https://www.infineon.com/aurix-expert-training/Infineon-AURIX_Multicore_1_KIT_TC297_TFT-TR-v01_00_00-EN.pdf
 * \documents https://www.infineon.com/aurix-expert-training/TC29B_iLLD_UM_1_0_1_11_0.chm
 * \lastUpdated 2020-02-11
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hDrv.h"
#include "semphr.h"

IfxCpu_syncEvent g_cpuSyncEvent = 0;

SemaphoreHandle_t BinarySemaphore = NULL;

void vled_blink_107_core0(void *pvParameters)
{
    for (;;)
    {
        led_107_blink();
    }
}

void test_task1(void *pvParameters)
{
    int i = 0;
    Ifx_print("test_task1\r\n");
    while (1)
    {
        IfxPort_setPinState(LED1, IfxPort_State_toggled);

        vTaskDelay(1000);
        i++;
        if (i % 5 == 0)
        {
            Ifx_print("test_task1 give %d BinarySemaphore\r\n", i / 5);
            xSemaphoreGive(BinarySemaphore);
        }
    }
}

void test_task2(void *pvParameters)
{
    int i = 0;
    Ifx_print("test_task2\r\n");
    BinarySemaphore = xSemaphoreCreateBinary();
    if (NULL == BinarySemaphore)
    {
        Ifx_print("BinarySemaphore Failed...\r\n");
    }
    while (1)
    {
        IfxPort_setPinState(LED2, IfxPort_State_toggled);
        //Ifx_print("test_task2\r\n");
        Ifx_print("test_task2 wait %d BinarySemaphore\r\n", i++);
        vTaskDelay(500);
        xSemaphoreTake(BinarySemaphore, portMAX_DELAY);
    }
}

int core0_main(void)
{
    IfxCpu_enableInterrupts();

    /* !!WATCHDOG0 AND SAFETY WATCHDOG ARE DISABLED HERE!!
     * Enable the watchdogs and service them periodically if it is required
     */
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    /* Initialize the LED and the time constants before the CPUs synchronization */
    init_led();
    initTime();
    init_uart_module();
    //gtm_global_init();
    //gtm_tom_init();
    Ifx_print("hello \r\n");
    /* Wait for CPU sync event */
    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    Ifx_print("wait \r\n");

    xTaskCreate(test_task1, "test_task1", 1024, NULL, 3, NULL);

    xTaskCreate(test_task2, "test_task2", 1024, NULL, 4, NULL);

    Ifx_print("start \r\n");
    // Start the scheduler
    vTaskStartScheduler();

    // The following line should never be reached.
    while (1)
        ;
    return (1);
}
