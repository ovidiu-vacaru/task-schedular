/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
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

#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "led.h"



void task1_handler(void);
void task2_handler(void);
void task3_handler(void);
void task4_handler(void);
void init_tasks_stack(void);
void enable_processor_faults(void);
void switch_sp_to_psp(void);



void init_systick_timer(uint32_t tick_hz);
__attribute__((naked)) void init_schedular_stack(uint32_t schedular_top_of_stack);

/* some stack memory calculations */

uint32_t psp_of_tasks[MAX_TASKS] = {T1_STACK_START,T2_STACK_START, T3_STACK_START, T4_STACK_START };
uint32_t task_handlers[MAX_TASKS];
uint8_t current_task = 0;

int main(void)
{

	enable_processor_faults();
    /* Loop forever */
	init_schedular_stack(SCHED_STACK_START);

	task_handlers[0] = (uint32_t)&task1_handler;
	task_handlers[1] = (uint32_t)&task2_handler; // Was task_handlers[2]
	task_handlers[2] = (uint32_t)&task3_handler; // Was task_handlers[3]
	task_handlers[3] = (uint32_t)&task4_handler; // Was task_handlers[4]


	init_tasks_stack();

	led_init_all();

	init_systick_timer(TICK_HZ);

	switch_sp_to_psp();

	task1_handler();
	for(;;);
}

void task1_handler(void){
	while(1){
		led_on(LED_GREEN);
		delay(DELAY_COUNT_1S);
		led_off(LED_GREEN);
		delay(DELAY_COUNT_1S);
	}

}
void task2_handler(void){
	while(1){
		led_on(LED_ORANGE);
		delay(DELAY_COUNT_500MS);
		led_off(LED_ORANGE);
		delay(DELAY_COUNT_500MS);
	}
}
void task3_handler(void){
	while(1){
		led_on(LED_BLUE);
		delay(DELAY_COUNT_250MS);
		led_off(LED_BLUE);
		delay(DELAY_COUNT_250MS);
	}

}
void task4_handler(void){
	while(1){
		led_on(LED_RED);
		delay(DELAY_COUNT_125MS);
		led_off(LED_RED);
		delay(DELAY_COUNT_125MS);
	}
}



void init_systick_timer(uint32_t tick_hz){

	uint32_t *pSRVR = (uint32_t*)  0xE000E014;
	uint32_t *pSCSR = (uint32_t*)  0xE000E010;
	uint32_t count_value = SYSTICK_TIMER_CLK / tick_hz -1;

	// Clear the value of SVR
	*pSRVR &= ~(0x00FFFFFF);
	// Load the value in to SVR
	*pSRVR |= count_value;

	//do some settings

	*pSCSR |= ( 1 << 1); // enable SysTick exception request.
	*pSCSR |= ( 1 << 2);
	*pSCSR |= ( 1 << 0); // enable the counter


}


void init_tasks_stack(void){

	uint32_t *pPSP;

	for (int i =0 ; i < MAX_TASKS ; i++){
		pPSP = (uint32_t*) psp_of_tasks[i];
		pPSP--;
		*pPSP = DUMMY_XPSR; // 0x01000000

		pPSP--; // PC
		*pPSP = task_handlers[i]; //

		pPSP--; // LR
		*pPSP = 0xFFFFFFFD; //

		for( int j = 0; j < 13; j++){
			pPSP--;
			*pPSP = 0;
		}

		psp_of_tasks[i] = (uint32_t)pPSP;

	}

}

__attribute__((naked)) void init_schedular_stack(uint32_t schedular_top_of_stack){
	__asm volatile("MSR MSP, R0");
	__asm volatile("BX LR");
}


void update_next_task(void){
	current_task++;
	current_task %= MAX_TASKS;
}



uint32_t get_psp_value(void){
	return psp_of_tasks[current_task];
}

void save_psp_value(uint32_t current_psp_value){
	psp_of_tasks[current_task] = current_psp_value;
}


__attribute__((naked)) void switch_sp_to_psp(void){
	//1. Initialize the PSP with TASK1 stack start
    __asm volatile ("PUSH {LR}");
	__asm volatile ("BL get_psp_value");
	__asm volatile ("MSR PSP, R0");
	__asm volatile ("POP {LR}");
	//2. change SP to PSP using CONTROL REGISTER

	__asm volatile("MOV R0,#0X02");
	__asm volatile("MSR CONTROL, R0");
	__asm volatile("BX LR");
}


void enable_processor_faults(void){
	uint32_t *pSHCSR = (uint32_t*)0xE000ED24;

	*pSHCSR |= ( 1 << 16); // mem manage
	*pSHCSR |= ( 1 << 17); // bus fault
	*pSHCSR |= ( 1 << 18); // usage fault

}


__attribute__ ((naked)) void SysTick_Handler(void){
	// Save the context of current task
	//1. Get current running task's PSP value
	//2. Using that PSP value store Sf2 (r4+r11)
	//3. Save the current value of PSP

	__asm volatile("MRS R0, PSP");
	__asm volatile("STMDB R0!, {R4-R11}");

	__asm volatile("PUSH {LR}");

	__asm volatile("BL save_psp_value");


	/* Retreive the context of the task */
	//1. Decide next task to run
	__asm volatile("BL update_next_task");

	//2. get its past PSP value
	__asm volatile("BL get_psp_value");

	//3 Using that PSP value retrieve sf2(r4+R11)
	__asm volatile("LDMIA R0!, {R4-R11}");

	// update psp and exit

    __asm volatile("MSR PSP, R0"); // Corrected from "MSR, R0, PSP"

    __asm volatile("POP {LR}");

    __asm volatile("BX LR");



}

void HardFault_Handler(void){
	printf("Exception : Hardfault\n");
}

void MemManage_Handler(void){
	printf("Exception : MemManage\n");
}

void BusFault_Handler(void){
	printf("Exception : BusFault\n");
}
