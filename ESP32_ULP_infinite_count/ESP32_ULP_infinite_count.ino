#include "esp32/ulp.h"// Must have this!!!

// include ulp header you will create
#include "ulp_main.h"// Must have this!!!

// Custom binary loader
#include "ulptool.h"// Must have this!!!

// Unlike the esp-idf always use these binary blob names
extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

static void init_run_ulp(uint32_t usec);

void setup() {
    Serial.begin(115200);
    delay(1000);
    init_run_ulp(100 * 1000); // 100 msec
}

void loop() {
    // ulp variables data is the lower 16 bits
    Serial.printf("ulp count: %u\n", ulp_count & 0xFFFF);
    delay(1000);
}

static void init_run_ulp(uint32_t usec) {
    // initialize ulp variable
    ulp_count = 0;
    ulp_set_wakeup_period(0, usec);
    // use this binary loader instead
    esp_err_t err = ulptool_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    // ulp coprocessor will run on its own now
    err = ulp_run((&ulp_entry - RTC_SLOW_MEM) / sizeof(uint32_t));
}
