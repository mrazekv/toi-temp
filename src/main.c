#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h" 

#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

#define GPIO_DS18B20_0       (4)
#define MAX_DEVICES          (8)
#define DS18B20_RESOLUTION   (DS18B20_RESOLUTION_12_BIT)
#define SAMPLE_PERIOD        (1000)   // milliseconds


void onewire_task(void *pvParameter)
{
    // Stable readings require a brief period before communication
    vTaskDelay(2000.0 / portTICK_PERIOD_MS);

    // Create a 1-Wire bus, using the RMT timeslot driver
    OneWireBus * owb;
    owb_rmt_driver_info rmt_driver_info;
    owb = owb_rmt_initialize(&rmt_driver_info, GPIO_DS18B20_0,
                             RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(owb, true);  // enable CRC check for ROM code

    // Find all connected devices
    ESP_LOGI("one-wire", "Find devices:\n");
    OneWireBus_ROMCode device_rom_codes[MAX_DEVICES] = {0};
    int num_devices = 0;
    OneWireBus_SearchState search_state = {0};
    bool found = false;
    owb_search_first(owb, &search_state, &found);
    while (found)
    {
        char rom_code_s[17];
        owb_string_from_rom_code(search_state.rom_code, rom_code_s,
                                 sizeof(rom_code_s));
        printf("  %d : %s\n", num_devices, rom_code_s);
        device_rom_codes[num_devices] = search_state.rom_code;
        ++num_devices;
        owb_search_next(owb, &search_state, &found);
    }
    ESP_LOGI("one-wire", "Found %d device%s", num_devices, 
                         num_devices == 1 ? "" : "s");



    // Create DS18B20 devices on the 1-Wire bus
    DS18B20_Info * devices[MAX_DEVICES] = {0};
    for (int i = 0; i < num_devices; ++i)
    {
        DS18B20_Info * ds18b20_info = ds18b20_malloc();  
        // heap allocation
        devices[i] = ds18b20_info;

        if (num_devices == 1)
        {
            ESP_LOGI("one-wire", "Single device optimisations enabled");
            ds18b20_init_solo(ds18b20_info, owb);          
            // only one device on bus
        }
        else
        {
            ds18b20_init(ds18b20_info, owb, device_rom_codes[i]); 
            // associate with bus and device
        }
        ds18b20_use_crc(ds18b20_info, true);           
        // enable CRC check on all reads
        ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION);
    }

    // Read temperatures from all sensors sequentially
    while (1)
    {
        ESP_LOGI("one-wire", "Temperature readings (degrees C):");
        ds18b20_convert_all(owb);

        // In this application all devices use the same resolution,
        // so use the first device to determine the delay
        ds18b20_wait_for_conversion(devices[0]);
        
        for (int i = 0; i < num_devices; ++i)
        {
            float temp = 0;
            ds18b20_read_temp(devices[i], &temp);
            ESP_LOGI("one-wire", "  %d: %.3f\n", i, temp);

            /* @TODO: hodnotu z čidla temp uložte do nějaké sdílené 
            proměnné a navrhněte mechanismus komunikace mezi procesy, 
            který může být založen jen na sdílené proměnné */
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
        // @TODO: zvolte vhodnou periodu obnovy
    }
}


void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    xTaskCreate(onewire_task, "one_wire_task", 2048, NULL, 5, NULL);
}