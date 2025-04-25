#include <cstdio>
#include <cstring>

#include "i2chost.hpp"

extern "C" void app_main() {
    // create i2c scan runtime
    brick_i2c_init();
    xTaskCreatePinnedToCore(
        brick_task_i2c_scan_devices,
        "brick_task_i2c_scan_devices",
        2048,
        nullptr,
        1,
        nullptr,
        tskNO_AFFINITY
    );

    //create bluetooth runtime

    //create lua runtime
}
