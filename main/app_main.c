#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

// Includes para el teclado matricial
#include "keyboard.h"

// Includes para el rc522
#include <inttypes.h>
#include "rc522.h"

// Includes para el LCD
#include "st7789.h"
#include "fontx.h"

// Includes para el Servo
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

static esp_mqtt_client_handle_t client = NULL; // Variable global para almacenar el cliente MQTT

static const char *TAG = "mqtt-logs";
static const char *TAG1 = "rc522-logs";
static rc522_handle_t scanner;

#define DEVICE_ID "24001" // Cambia este ID en cada dispositivo

// Variable global para almacenar el código de la tarjeta
uint64_t codigo_tarjeta = 0;

// Definimos la fuente con la tabla y sus dimensiones
FontDef Font24 = {Font24_Table, 17, 24};

TFT_t dev;

//------------------------------------------funciones para controlar servo-------------------------------
// Función para inicializar GPIO para MCPWM
static void mcpwm_example_gpio_initialize()
{
    printf("Inicializando GPIO para control de servomotor con MCPWM...\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, 2); // Configura GPIO 2 como PWM0A
}

// Función para convertir ángulo a ancho de pulso
static uint32_t servo_angle_to_duty_us(uint32_t angle)
{
    // Calcula el ancho de pulso en microsegundos
    return (angle * (2400 - 600) / 180) + 600;
}

// Función para mover el servo
void move_servo(uint32_t start_angle, uint32_t end_angle, uint32_t speed)
{
    if (start_angle == end_angle)
        return; // Si los ángulos son iguales, no hacer nada

    int step = (start_angle < end_angle) ? 1 : -1; // Determinar la dirección del movimiento
    uint32_t current_angle = start_angle;

    // Mover el servo gradualmente al ángulo final
    while (current_angle != end_angle)
    {
        uint32_t duty_us = servo_angle_to_duty_us(current_angle);
        mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty_us);
        current_angle += step;
        vTaskDelay(pdMS_TO_TICKS(speed)); // Esperar entre pasos para controlar la velocidad
    }

    // Asegurar que el servo esté en el ángulo final
    uint32_t duty_us = servo_angle_to_duty_us(end_angle);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty_us);

    // Detener la señal PWM después de alcanzar la posición deseada
    //mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);
}

//------------------------------------------funciones para controlar acceso-------------------------------
void access_handler(const char *response, int length)
{
    // Crear una copia de la cadena recibida para asegurarse de que esté terminada en nulo
    char buffer[4]; // Suficiente para tres dígitos y el terminador nulo
    if (length >= sizeof(buffer))
    {
        ESP_LOGI(TAG1, "La longitud del dato es demasiado larga");
        return;
    }
    memcpy(buffer, response, length);
    buffer[length] = '\0'; // Asegurarse de que la cadena esté terminada en nulo

    // Convertir string a entero
    int value = atoi(buffer);

    switch (value)
    {
    case 111:
        ESP_LOGI(TAG1, "Acceso permitido");
        lcdFillScreen(&dev, GREEN);
        LCD_DrawString(&dev, 75, 80, "ACCESO", &Font24, RED);
        LCD_DrawString(&dev, 50, 120, "CONCEDIDO", &Font24, RED);
        vTaskDelay(300);
        lcdFillScreen(&dev, ORANGE);
        LCD_DrawString(&dev, 30, 100, "Bienvenido!", &Font24, RED);
        break;
    case 101:
        ESP_LOGI(TAG1, "Cofre abierto");
        lcdFillScreen(&dev, BLUE);
        LCD_DrawString(&dev, 80, 80, "COFRE", &Font24, RED);
        LCD_DrawString(&dev, 60, 120, "ABIERTO", &Font24, RED);
        move_servo(60, 10, 40);            // Mover el servo de 60 grados a 0
        vTaskDelay(pdMS_TO_TICKS(15000)); // Esperar 15 segundos
        move_servo(10, 60, 40);            // Mover el servo de regreso a 60 grados
        lcdFillScreen(&dev, ORANGE);
        LCD_DrawString(&dev, 30, 100, "Bienvenido!", &Font24, RED);
        break;
    case 100:
        ESP_LOGI(TAG1, "No autorizado");
        lcdFillScreen(&dev, RED);
        LCD_DrawString(&dev, 100, 80, "NO", &Font24, GRAY);
        LCD_DrawString(&dev, 40, 120, "AUTORIZADO", &Font24, GRAY);
        vTaskDelay(300);
        lcdFillScreen(&dev, ORANGE);
        LCD_DrawString(&dev, 30, 100, "Bienvenido!", &Font24, RED);
        break;
    default:
        ESP_LOGI(TAG1, "Código no reconocido");
        break;
    }
}

//------------------------------------------funciones para Mqtt-------------------------------
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char topic[50]; // Buffer para almacenar el tópico

    // Construir el tópico "/cntrlaxs/respuesta/DEVICE_ID"
    snprintf(topic, sizeof(topic), "/cntrlaxs/respuesta/%s", DEVICE_ID);

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        char payload[100];
        snprintf(payload, sizeof(payload), "{\"device_id\":\"%s\",\"status\":\"connected\"}", DEVICE_ID);
        msg_id = esp_mqtt_client_publish(client, "/cntrlaxs/solicitud", payload, 0, 1, 0);
        ESP_LOGI(TAG, "se publicó el mensaje, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, topic, 1);
        ESP_LOGI(TAG, "Suscrito correctamente a %s, msg_id=%d", topic, msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA"); // acá se reciben los msjs mqtt
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        access_handler(event->data, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_publish_message(const char *topic, const char *json_message)
{
    int msg_id = esp_mqtt_client_publish(client, topic, json_message, 0, 1, 0);
    if (msg_id != -1)
    {
        ESP_LOGI(TAG, "Mensaje enviado correctamente, msg_id=%d", msg_id);
    }
    else
    {
        ESP_LOGE(TAG, "Error al enviar mensaje");
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

//----------------------------------------------------------------funciones para rc522----------------------------------------------------------------------------------
static void rc522_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    rc522_event_data_t *data = (rc522_event_data_t *)event_data;

    switch (event_id)
    {
    case RC522_EVENT_TAG_SCANNED:
    {
        rc522_tag_t *tag = (rc522_tag_t *)data->ptr;
        ESP_LOGI(TAG1, "Tarjeta escaneada (sn: %" PRIu64 ")", tag->serial_number);

        codigo_tarjeta = tag->serial_number;                                  // Obtener el valor de la variable global codigo_tarjeta
        char json_message[100];                                               // Buffer para almacenar el mensaje JSON
        snprintf(json_message, sizeof(json_message), "{\"device_id\":\"%s\",\"card\":\"%" PRIu64 "\"}", DEVICE_ID, codigo_tarjeta);

        // Publicar el mensaje a través de MQTT
        mqtt_publish_message("/cntrlaxs/solicitud/card", json_message);
    }
    break;
    }
}

//------------------------------------------funciones para teclado-------------------------------
void keyboard_task(void *pvParameter)
{
    int digit_count = 0;                                         // Contador de dígitos ingresados
    char code_buffer[7] = {0};                                   // Buffer para almacenar el código ingresado (6 dígitos + terminador nulo)
    TickType_t last_key_time = 0;                                // Registro del tiempo del último dígito ingresado
    const TickType_t debounce_delay = 50 / portTICK_PERIOD_MS;   // Tiempo de debounce
    const TickType_t timeout_delay = 15000 / portTICK_PERIOD_MS; // Tiempo de espera de 15 segundos

    while (1)
    {
        if (keyboard_check())
        {
            // Implementar debounce
            if (xTaskGetTickCount() - last_key_time < debounce_delay)
            {
                vTaskDelay(debounce_delay); // Esperar el tiempo de debounce
                continue;
            }

            char key = keyboard_get_char();
            ESP_LOGI(TAG, "Tecla presionada: %c", key);

            // Si se presiona una tecla numérica y no se ha alcanzado el límite de dígitos
            if (isdigit(key) && digit_count < 6)
            {
                // Almacenar el dígito en el buffer
                code_buffer[digit_count++] = key;
                last_key_time = xTaskGetTickCount(); // Actualizar el tiempo del último dígito ingresado
                ESP_LOGI(TAG, "Código ingresado hasta ahora: %s", code_buffer);
            }
            // Si se presiona el botón de cancelar
            else if (key == '*')
            {
                // Reiniciar el buffer y el contador de dígitos
                memset(code_buffer, 0, sizeof(code_buffer));
                digit_count = 0;
                ESP_LOGI(TAG, "Código cancelado");
            }
            // Si se presiona el botón de terminar
            else if (key == '#' && digit_count > 0)
            {
                // Verificar si se ha ingresado el número completo
                if (digit_count == 6)
                {
                    // Publicar el código ingresado a través de MQTT
                    char json_message[100];
                    snprintf(json_message, sizeof(json_message), "{\"device_id\":\"%s\",\"code\":\"%s\"}", DEVICE_ID, code_buffer);
                    mqtt_publish_message("/cntrlaxs/solicitud/code", json_message);
                    ESP_LOGI(TAG, "Código completo ingresado: %s", code_buffer);
                    // Limpiar el buffer y el contador de dígitos
                    memset(code_buffer, 0, sizeof(code_buffer));
                    digit_count = 0;
                    ESP_LOGI(TAG, "Memoria limpia");
                }
            }
        }

        // Verificar si el tiempo de espera ha excedido los 15 segundos
        if (digit_count > 0 && (xTaskGetTickCount() - last_key_time) > timeout_delay)
        {
            // Limpiar el buffer y el contador de dígitos
            memset(code_buffer, 0, sizeof(code_buffer));
            digit_count = 0;
            ESP_LOGI(TAG, "Tiempo de espera excedido, memoria limpia");
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // Pequeño retraso para evitar la sobrecarga de la CPU
    }
}

//------------------------------------------funcion principal-------------------------------
void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    //----------------------------LCD--------------------------
    // Define the GPIOs for your SPI interface
    int16_t GPIO_MOSI = 14;
    int16_t GPIO_SCLK = 27;
    int16_t GPIO_CS = -1;
    int16_t GPIO_DC = 26;
    int16_t GPIO_RESET = 25;
    int16_t GPIO_BL = -1;

    // Initialize the SPI interface
    spi_master_init(&dev, GPIO_MOSI, GPIO_SCLK, GPIO_CS, GPIO_DC, GPIO_RESET, GPIO_BL);

    // Initialize the display with the specified width, height, and offsets
    lcdInit(&dev, 240, 240, 0, 0);

    lcdFillScreen(&dev, BLACK);
    uint16_t xpos = 240 / 2;
    uint16_t ypos = 240 / 2;
    for (int i = 5; i < 240; i = i + 5)
    {
        lcdDrawCircle(&dev, xpos, ypos, i, BLUE);
    }
    lcdDrawFinish(&dev);
    lcdFillScreen(&dev, ORANGE);
    LCD_DrawString(&dev, 30, 100, "Bienvenido!", &Font24, RED);

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    // Inicializa GPIO servo
    mcpwm_example_gpio_initialize();

    // Configura MCPWM
    mcpwm_config_t pwm_config = {
        .frequency = 50, // Frecuencia de 50 Hz para servomotores
        .cmpr_a = 0,     // Ciclo de trabajo inicial del 0%
        .cmpr_b = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    rc522_config_t config = {
        .spi.host = VSPI_HOST,
        .spi.miso_gpio = 18,
        .spi.mosi_gpio = 23,
        .spi.sck_gpio = 19,
        .spi.sda_gpio = 22,
    };

    mqtt_app_start();
    rc522_create(&config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    rc522_start(scanner);

    keyboard_init();
    xTaskCreate(&keyboard_task, "keyboard_task", 2048, NULL, 5, NULL);
}
