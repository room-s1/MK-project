cat > /home/s1/Документы/esp32/00-test-prj/main/main.c << 'EOF'
#include <stdio.h>
#include <math.h>
#include "esp_chip_info.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"

static const char *TAG = "CHIP_INFO";

static void test_pow_speed(void)
{
    const int ITERATIONS = 10;
    double results[10];
    double total = 0.0;
    
    ESP_LOGI(TAG, "=== Test pow() speed ===");
    ESP_LOGI(TAG, "Iterations: %d", ITERATIONS);
    
    for (int i = 0; i < ITERATIONS; i++) {
        double base = i + 1;
        double exponent = 1.5;
        results[i] = pow(base, exponent);
        total += results[i];
    }
    
    ESP_LOGI(TAG, "=== Values of pow(x, 1.5) ===");
    for (int i = 0; i < ITERATIONS; i++) {
        ESP_LOGI(TAG, "pow(%d, 1.5) = %.4f", i + 1, results[i]);
    }
    
    ESP_LOGI(TAG, "Sum of all values: %.4f", total);
    
    const int SPEED_ITERATIONS = 100000;
    double speed_result = 0.0;
    
    ESP_LOGI(TAG, "=== Speed test with %d iterations ===", SPEED_ITERATIONS);
    
    int64_t start_time = esp_timer_get_time();
    
    for (int i = 1; i <= SPEED_ITERATIONS; i++) {
        speed_result += pow(i, 1.5);
        
        // Сбрасываем watchdog каждые 1000 итераций
        if (i % 1000 == 0) {
            esp_task_wdt_reset();
        }
    }
    
    int64_t end_time = esp_timer_get_time();
    int64_t elapsed_us = end_time - start_time;
    
    ESP_LOGI(TAG, "Total time: %.2f ms", elapsed_us / 1000.0);
    ESP_LOGI(TAG, "Time per pow(): %.2f us", (double)elapsed_us / SPEED_ITERATIONS);
    ESP_LOGI(TAG, "Total result (partial): %.2f", speed_result);
}

void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    ESP_LOGI(TAG, "=== ESP32 Chip Info ===");
    
    const char *chip_model;
    switch(chip_info.model) {
        case CHIP_ESP32:   chip_model = "ESP32"; break;
        case CHIP_ESP32S2: chip_model = "ESP32-S2"; break;
        case CHIP_ESP32S3: chip_model = "ESP32-S3"; break;
        case CHIP_ESP32C3: chip_model = "ESP32-C3"; break;
        default:           chip_model = "Unknown"; break;
    }
    ESP_LOGI(TAG, "Model: %s", chip_model);
    ESP_LOGI(TAG, "Cores: %d", chip_info.cores);
    ESP_LOGI(TAG, "Revision: v%d.%d", 
             chip_info.revision / 100, 
             chip_info.revision % 100);
    
    test_pow_speed();
}
EOF