/**
 * @file       scheduler.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */
/*================================ include ==================================*/
    
#include "scheduler.h"

#include "cpu.h"
#include "leds.h"

#include "virtual_timer.h"

/*================================ define ===================================*/

#define TASK_BUFFER_SIZE            ( 16 )

#define SCHEDULER_LED_ON_TICKS      ( 64 )
#define SCHEDULER_LED_OFF_TICKS     ( 32704 )

/*================================ typedef ==================================*/

typedef struct task_list_t {
    task_cb_t callback;
    task_prio_t priority;
    struct task_list_t* next_task;
} task_list_t;

typedef struct {
    task_list_t task_buffer[TASK_BUFFER_SIZE];
    task_list_t* task_head;
} scheduler_vars_t;

/*=============================== variables =================================*/

static scheduler_vars_t scheduler_vars;

/*=============================== prototypes ================================*/

static void scheduler_toggle_led(void);

/*================================= public ==================================*/

void scheduler_init() {
    // Initialize the memory of the scheduler variables
    memset(&scheduler_vars, 0, sizeof(scheduler_vars_t));
}

void scheduler_start(void) {
    task_list_t* current_task = NULL;

    // Push the task that controls the system led
    scheduler_push(scheduler_toggle_led, TASK_PRIO_MIN);

    // Enable the CPU interrupts
    cpu_enable_interrupts();

    // The scheduler loops forever
    while (true) {
        // If there is a task to be executed
        while (scheduler_vars.task_head != NULL) {
            // Point to the next task to be executed
            current_task = scheduler_vars.task_head;

            // Update the pointer to the next task to be executed
            scheduler_vars.task_head = current_task->next_task;

            // Execute the current task
            current_task->callback();

            // Empty the task container of the task that has just been executed
            current_task->callback = NULL;
            current_task->priority = TASK_PRIO_NONE;
            current_task->next_task = NULL;
        }
        cpu_wait();
    }
}

void scheduler_push(task_cb_t callback, task_prio_t priority) {
    task_list_t* current_task = NULL;
    task_list_t** task_walker = NULL;

    // Disable interrupts
    cpu_disable_interrupts();

    // Find a position in the task buffer
    current_task = &scheduler_vars.task_buffer[0];
    while (current_task->callback != NULL && current_task <= &scheduler_vars.task_buffer[TASK_BUFFER_SIZE - 1]) {
       current_task++;
    }

    // The task list has overflown, this should never happpen!
    if (current_task > &scheduler_vars.task_buffer[TASK_BUFFER_SIZE - 1]) {
       // Turn on the error LED
       leds_error_on();
       // Lock the scheduler
       while(true);
    }

    // Fill in the current task information
    current_task->callback = callback;
    current_task->priority = priority;

    // Find a position in the task queue
    task_walker = &scheduler_vars.task_head;
    while (*task_walker != NULL && (*task_walker)->priority < current_task->priority) {
        task_walker = (task_list_t**) &((*task_walker)->next_task);
    }

    // Insert the task in the given queue position
    current_task->next_task = *task_walker;
    *task_walker = current_task;

    // Reenable interrupts
    cpu_enable_interrupts();
}

/*================================ private ==================================*/

static void scheduler_toggle_led(void) {
    static uint8_t led_status = false;
    if (led_status == true) {
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, SCHEDULER_LED_OFF_TICKS, scheduler_toggle_led, TASK_PRIO_MIN);
    } else {
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, SCHEDULER_LED_ON_TICKS, scheduler_toggle_led, TASK_PRIO_MIN);
    }
    leds_system_toggle();
    led_status = !led_status;
}
