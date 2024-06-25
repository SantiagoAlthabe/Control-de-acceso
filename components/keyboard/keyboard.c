#include "keyboard.h"
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define NUM_ROW 4
#define NUM_COL 4

#define COL_1 GPIO_NUM_33
#define COL_2 GPIO_NUM_32
#define COL_3 GPIO_NUM_21
#define COL_4 GPIO_NUM_0

#define ROW_1 GPIO_NUM_4
#define ROW_2 GPIO_NUM_13
#define ROW_3 GPIO_NUM_12
#define ROW_4 GPIO_NUM_15

gpio_num_t rows[NUM_ROW] = { ROW_1, ROW_2, ROW_3, ROW_4 };
gpio_num_t cols[NUM_COL] = { COL_1, COL_2, COL_3, COL_4 };

#define GPIO_INPUT_COLUMNS ((1ULL<<COL_1) | (1ULL<<COL_2) | (1ULL<<COL_3) | (1ULL<<COL_4))
#define GPIO_OUTPUT_ROWS ((1ULL<<ROW_1) | (1ULL<<ROW_2) | (1ULL<<ROW_3) | (1ULL<<ROW_4))

// Keypad layout: [row][col]
char const KEYS[NUM_ROW][NUM_COL] = {
    { '1', '2' , '3' ,'A' },
    { '4', '5' , '6' ,'B' },
    { '7', '8' , '9' ,'C' },
    { '*', '0' , '#' ,'D' },
};

static char _last_key = 0;
static char _last_checked_key = 0; // Para almacenar la última tecla verificada
static TickType_t _last_key_time = 0; // Para almacenar el tiempo de la última verificación

static void _columns_config(void) {
    gpio_config_t col_config;
    col_config.pin_bit_mask = GPIO_INPUT_COLUMNS;
    col_config.intr_type = GPIO_INTR_DISABLE; // interrupt type: falling and rising edge
    col_config.mode = GPIO_MODE_INPUT;
    col_config.pull_up_en = GPIO_PULLUP_ENABLE;
    col_config.pull_down_en = GPIO_PULLUP_DISABLE;
    gpio_config(&col_config);
}

static void _rows_config(void) {
    gpio_config_t row_config;
    row_config.pin_bit_mask = GPIO_OUTPUT_ROWS;
    row_config.intr_type = GPIO_INTR_DISABLE;   // interrupt disable
    row_config.mode = GPIO_MODE_OUTPUT;
    row_config.pull_up_en = GPIO_PULLUP_DISABLE;
    row_config.pull_down_en = GPIO_PULLUP_DISABLE;
    gpio_config(&row_config);
}

void keyboard_init() {
    printf("Iniciando keyboard \n");
    _columns_config();    
    _rows_config();
    for (int i = 0; i < NUM_ROW; i++) gpio_set_level(rows[i], 1);
}

char keyboard_get_char() {
    return _last_key;
}

int keyboard_check() {
    const TickType_t debounce_delay = 300  / portTICK_PERIOD_MS; // Tiempo de debounce

    for (uint8_t row = 0; row < NUM_ROW; row++) {
        gpio_set_level(rows[row], 0);
        vTaskDelay(1 / portTICK_PERIOD_MS);  // Pequeño retraso para permitir la estabilización
        for (uint8_t col = 0; col < NUM_COL; col++) {
            if (!gpio_get_level(cols[col])) {
                char current_key = KEYS[row][col];
                TickType_t current_time = xTaskGetTickCount();

                if (current_key != _last_checked_key || (current_time - _last_key_time) > debounce_delay) {
                    _last_key = current_key;
                    _last_checked_key = current_key;
                    _last_key_time = current_time;
                    gpio_set_level(rows[row], 1);  // Restaurar el nivel de la fila
                    return 1;
                }
            }
        }
        gpio_set_level(rows[row], 1);
    }
    return 0;
}
