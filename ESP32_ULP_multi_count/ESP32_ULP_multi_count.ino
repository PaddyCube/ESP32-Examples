/* ULP Example: multi pulse counting, counting two GPIOs while ESP sleeps
 *  
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   ULP wakes up to run this code at a certain period, determined by the values
   in SENS_ULP_CP_SLEEP_CYCx_REG registers. On each wake up, the program checks
   the input on GPIO_A and GPIO_B. If the values are different from the previous one, the
   program "debounces" the input: on the next debounce_max_count wake ups,
   it expects to see the same value of input.
   If this condition holds true, the program increments edge_count for each GPIO and starts
   waiting for input signal polarity to change again.

   While ULP counts, ESP main core sleeps for a given time (10s). After each timer wake-up, 
   ESP core reads ULP counting values for both pins and print the value to console

   (commented out:
   When the edge counter reaches certain value (set by the main program),
   this program running triggers a wake up from deep sleep. )

   This example is based on the ULP count example provided by Espressif. All credits to them
   https://github.com/espressif/esp-idf/blob/master/examples/system/ulp

   I used the ulptool provided by duff2013 https://github.com/duff2013/ulptool so this example
   was created using Arduino IDE instead of ESP-IDF. Thanks to duff2013 to share this great tool
   with the community
*/


#include <stdio.h>
#include "esp_sleep.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc_periph.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "esp32/ulp.h"// Must have this!!!

// include ulp header you will create
#include "ulp_main.h"// Must have this!!!

// Custom binary loader
#include "ulptool.h"// Must have this!!!

// Unlike the esp-idf always use these binary blob names
extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");


static void init_ulp_program(void);
static void update_pulse_count(void);
static void pin_setup(gpio_num_t gpio_num);

/*--------------------------------------------------
 * setup gets called on each boot and on each wakeup
 *--------------------------------------------------*/
void setup(void)
{
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause != ESP_SLEEP_WAKEUP_TIMER) {
    printf("No TIMER wakeup, initializing ULP\n");
    init_ulp_program();
  } else {
    printf("TIMER wakeup, saving pulse count\n");
    update_pulse_count();
  }

  printf("Entering deep sleep\n\n");

  ESP_ERROR_CHECK( esp_sleep_enable_timer_wakeup(10 * 1000000) );
  esp_deep_sleep_start();
}

/*--------------------------------------------------
 * loop
 *--------------------------------------------------*/
void loop()
{
  // do nothing here
}

/*--------------------------------------------------
 * This function loads ULP programm to ULP-co processor
 *--------------------------------------------------*/
static void init_ulp_program(void)
{
  esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
                                  (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
  ESP_ERROR_CHECK(err);

  /* GPIO used for pulse counting. */
  gpio_num_t gpio_num_a = GPIO_NUM_0;
  assert(rtc_gpio_desc[gpio_num_a].reg && "GPIO used for pulse counting must be an RTC IO");

  gpio_num_t gpio_num_b = GPIO_NUM_2;
  assert(rtc_gpio_desc[gpio_num_b].reg && "GPIO used for pulse counting must be an RTC IO");

  /* Initialize some variables used by ULP program.
     Each 'ulp_xyz' variable corresponds to 'xyz' variable in the ULP program.
     These variables are declared in an auto generated header file,
     'ulp_main.h', name of this file is defined in component.mk as ULP_APP_NAME.
     These variables are located in RTC_SLOW_MEM and can be accessed both by the
     ULP and the main CPUs.

     Note that the ULP reads only the lower 16 bits of these variables.
  */
  ulp_debounce_counter_a = 2;
  ulp_debounce_counter_b = 2;
  ulp_debounce_max_count = 3;
  ulp_next_edge_a = 0;
  ulp_next_edge_b = 0;
  ulp_io_number_a = rtc_gpio_desc[gpio_num_a].rtc_num; /* map from GPIO# to RTC_IO# */
  ulp_io_number_b = rtc_gpio_desc[gpio_num_b].rtc_num; /* map from GPIO# to RTC_IO# */
  ulp_edge_count_to_wake_up = 10;

  /* Initialize selected GPIO as RTC IO, enable input, disable pullup and pulldown */
  pin_setup(gpio_num_a);
  pin_setup(gpio_num_b);

  /* Disconnect GPIO12 and GPIO15 to remove current drain through
     pullup/pulldown resistors.
     GPIO12 may be pulled high to select flash voltage.
  */
  rtc_gpio_isolate(GPIO_NUM_12);
  rtc_gpio_isolate(GPIO_NUM_15);
  esp_deep_sleep_disable_rom_logging(); // suppress boot messages

  /* Set ULP wake up period to T = 20ms.
     Minimum pulse width has to be T * (ulp_debounce_counter + 1) = 80ms.
  */
  ulp_set_wakeup_period(0, 20000);

  /* Start the program */
  err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
  ESP_ERROR_CHECK(err);
}

/*--------------------------------------------------
 * This function shows last pulse counts
 *--------------------------------------------------*/
static void update_pulse_count(void)
{


  /* ULP program counts signal edges, convert that to the number of pulses */
  uint32_t pulse_count_from_ulp_a = (ulp_edge_count_a & UINT16_MAX) / 2;
  /* In case of an odd number of edges, keep one until next time */
  ulp_edge_count_a = ulp_edge_count_a % 2;
  printf("Pulse count from ULP A: %5d\n", pulse_count_from_ulp_a);

  /* ULP program counts signal edges, convert that to the number of pulses */
  uint32_t pulse_count_from_ulp_b = (ulp_edge_count_b & UINT16_MAX) / 2;
  /* In case of an odd number of edges, keep one until next time */
  ulp_edge_count_b = ulp_edge_count_b % 2;
  printf("Pulse count from ULP B: %5d\n", pulse_count_from_ulp_b);

}

/*--------------------------------------------------
 * This function prepares RTC_GPIO for ULP program
 *--------------------------------------------------*/
static void pin_setup(gpio_num_t gpio_num)
{
  rtc_gpio_init(gpio_num);
  rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(gpio_num);
  rtc_gpio_pullup_dis(gpio_num);
  rtc_gpio_hold_en(gpio_num);
}
