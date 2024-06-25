#ifndef MAIN_ST7789_H_
#define MAIN_ST7789_H_

#include "driver/spi_master.h"
#include "fontx.h"

/**
 * @brief Macro para convertir valores RGB a formato RGB565.
 */
#define rgb565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

#define RED    rgb565(255,   0,   0) // 0xf800
#define GREEN  rgb565(  0, 255,   0) // 0x07e0
#define BLUE   rgb565(  0,   0, 255) // 0x001f
#define BLACK  rgb565(  0,   0,   0) // 0x0000
#define WHITE  rgb565(255, 255, 255) // 0xffff
#define GRAY   rgb565(128, 128, 128) // 0x8410
#define YELLOW rgb565(255, 255,   0) // 0xFFE0
#define CYAN   rgb565(  0, 156, 209) // 0x04FA
#define PURPLE rgb565(128,   0, 128) // 0x8010
#define ORANGE rgb565(255,   196,   100) 

typedef enum {DIRECTION0, DIRECTION90, DIRECTION180, DIRECTION270} DIRECTION;

typedef enum {
	SCROLL_RIGHT = 1,
	SCROLL_LEFT = 2,
	SCROLL_DOWN = 3,
	SCROLL_UP = 4,
} SCROLL_TYPE_t;

typedef struct {
	uint16_t _width;              /**< Ancho del LCD */
	uint16_t _height;             /**< Alto del LCD */
	uint16_t _offsetx;            /**< Offset X del LCD */
	uint16_t _offsety;            /**< Offset Y del LCD */
	uint16_t _font_direction;     /**< Dirección de la fuente */
	uint16_t _font_fill;          /**< Habilitación del relleno de fuente */
	uint16_t _font_fill_color;    /**< Color de relleno de la fuente */
	uint16_t _font_underline;     /**< Habilitación del subrayado de fuente */
	uint16_t _font_underline_color; /**< Color del subrayado de la fuente */
	int16_t _dc;                  /**< Pin de control de datos/comando */
	int16_t _bl;                  /**< Pin de control de retroiluminación */
	spi_device_handle_t _SPIHandle; /**< Maneja la interfaz SPI */
	bool _use_frame_buffer;       /**< Indicador de uso de buffer de frame */
	uint16_t *_frame_buffer;      /**< Puntero al buffer de frame */
} TFT_t;

/**
 * @brief Configura la velocidad del reloj SPI.
 * 
 * @param speed Velocidad en Hz.
 */
void spi_clock_speed(int speed);

/**
 * @brief Inicializa la interfaz SPI para la comunicación con el LCD.
 * 
 * @param dev Puntero a la estructura TFT_t que contiene la configuración del dispositivo.
 * @param GPIO_MOSI GPIO para la línea MOSI.
 * @param GPIO_SCLK GPIO para la línea SCLK.
 * @param GPIO_CS GPIO para la línea CS.
 * @param GPIO_DC GPIO para la línea DC.
 * @param GPIO_RESET GPIO para la línea de reinicio.
 * @param GPIO_BL GPIO para la línea de retroiluminación.
 */
void spi_master_init(TFT_t * dev, int16_t GPIO_MOSI, int16_t GPIO_SCLK, int16_t GPIO_CS, int16_t GPIO_DC, int16_t GPIO_RESET, int16_t GPIO_BL);

/**
 * @brief Envía un byte de datos a través de SPI.
 * 
 * @param SPIHandle Maneja la interfaz SPI.
 * @param Data Puntero a los datos a enviar.
 * @param DataLength Longitud de los datos.
 * @return true si la escritura fue exitosa.
 * @return false si la escritura falló.
 */
bool spi_master_write_byte(spi_device_handle_t SPIHandle, const uint8_t* Data, size_t DataLength);

/**
 * @brief Envía un comando a través de SPI.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param cmd Comando a enviar.
 * @return true si la escritura fue exitosa.
 * @return false si la escritura falló.
 */
bool spi_master_write_command(TFT_t * dev, uint8_t cmd);

/**
 * @brief Envía un byte de datos a través de SPI.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param data Dato a enviar.
 * @return true si la escritura fue exitosa.
 * @return false si la escritura falló.
 */
bool spi_master_write_data_byte(TFT_t * dev, uint8_t data);

/**
 * @brief Envía una palabra de datos a través de SPI.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param data Dato a enviar.
 * @return true si la escritura fue exitosa.
 * @return false si la escritura falló.
 */
bool spi_master_write_data_word(TFT_t * dev, uint16_t data);

/**
 * @brief Envía una dirección a través de SPI.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param addr1 Dirección inicial.
 * @param addr2 Dirección final.
 * @return true si la escritura fue exitosa.
 * @return false si la escritura falló.
 */
bool spi_master_write_addr(TFT_t * dev, uint16_t addr1, uint16_t addr2);

/**
 * @brief Envía un color a través de SPI.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param color Color a enviar.
 * @param size Tamaño del color.
 * @return true si la escritura fue exitosa.
 * @return false si la escritura falló.
 */
bool spi_master_write_color(TFT_t * dev, uint16_t color, uint16_t size);

/**
 * @brief Envía múltiples colores a través de SPI.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param colors Puntero a los colores a enviar.
 * @param size Tamaño de los colores.
 * @return true si la escritura fue exitosa.
 * @return false si la escritura falló.
 */
bool spi_master_write_colors(TFT_t * dev, uint16_t * colors, uint16_t size);

/**
 * @brief Introduce un retardo en milisegundos.
 * 
 * @param ms Milisegundos a esperar.
 */
void delayMS(int ms);

/**
 * @brief Inicializa la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param width Ancho de la pantalla.
 * @param height Alto de la pantalla.
 * @param offsetx Offset horizontal.
 * @param offsety Offset vertical.
 */
void lcdInit(TFT_t * dev, int width, int height, int offsetx, int offsety);

/**
 * @brief Dibuja un píxel en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param x Coordenada X del píxel.
 * @param y Coordenada Y del píxel.
 * @param color Color del píxel.
 */
void lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Dibuja múltiples píxeles en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param x Coordenada X de inicio.
 * @param y Coordenada Y de inicio.
 * @param size Tamaño del array de colores.
 * @param colors Array de colores.
 */
void lcdDrawMultiPixels(TFT_t * dev, uint16_t x, uint16_t y, uint16_t size, uint16_t * colors);

/**
 * @brief Dibuja un rectángulo relleno en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param x1 Coordenada X de la esquina superior izquierda.
 * @param y1 Coordenada Y de la esquina superior izquierda.
 * @param x2 Coordenada X de la esquina inferior derecha.
 * @param y2 Coordenada Y de la esquina inferior derecha.
 * @param color Color de relleno.
 */
void lcdDrawFillRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief Apaga la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 */
void lcdDisplayOff(TFT_t * dev);

/**
 * @brief Enciende la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 */
void lcdDisplayOn(TFT_t * dev);

/**
 * @brief Llena la pantalla LCD con un color.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param color Color de relleno.
 */
void lcdFillScreen(TFT_t * dev, uint16_t color);

/**
 * @brief Dibuja una línea en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param x1 Coordenada X de inicio.
 * @param y1 Coordenada Y de inicio.
 * @param x2 Coordenada X de fin.
 * @param y2 Coordenada Y de fin.
 * @param color Color de la línea.
 */
void lcdDrawLine(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief Dibuja un rectángulo en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param x1 Coordenada X de la esquina superior izquierda.
 * @param y1 Coordenada Y de la esquina superior izquierda.
 * @param x2 Coordenada X de la esquina inferior derecha.
 * @param y2 Coordenada Y de la esquina inferior derecha.
 * @param color Color del rectángulo.
 */
void lcdDrawRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief Dibuja un rectángulo rotado en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param xc Coordenada X del centro.
 * @param yc Coordenada Y del centro.
 * @param w Ancho del rectángulo.
 * @param h Alto del rectángulo.
 * @param angle Ángulo de rotación en grados.
 * @param color Color del rectángulo.
 */
void lcdDrawRectAngle(TFT_t * dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color);

/**
 * @brief Dibuja un triángulo en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param xc Coordenada X del centro.
 * @param yc Coordenada Y del centro.
 * @param w Base del triángulo.
 * @param h Altura del triángulo.
 * @param angle Ángulo de rotación en grados.
 * @param color Color del triángulo.
 */
void lcdDrawTriangle(TFT_t * dev, uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color);

/**
 * @brief Dibuja un polígono regular en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param xc Coordenada X del centro.
 * @param yc Coordenada Y del centro.
 * @param n Número de lados del polígono.
 * @param r Radio del polígono.
 * @param angle Ángulo de rotación en grados.
 * @param color Color del polígono.
 */
void lcdDrawRegularPolygon(TFT_t *dev, uint16_t xc, uint16_t yc, uint16_t n, uint16_t r, uint16_t angle, uint16_t color);

/**
 * @brief Dibuja un círculo en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param x0 Coordenada X del centro.
 * @param y0 Coordenada Y del centro.
 * @param r Radio del círculo.
 * @param color Color del círculo.
 */
void lcdDrawCircle(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * @brief Dibuja un círculo relleno en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param x0 Coordenada X del centro.
 * @param y0 Coordenada Y del centro.
 * @param r Radio del círculo.
 * @param color Color de relleno del círculo.
 */
void lcdDrawFillCircle(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * @brief Dibuja un rectángulo con bordes redondeados en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param x1 Coordenada X de la esquina superior izquierda.
 * @param y1 Coordenada Y de la esquina superior izquierda.
 * @param x2 Coordenada X de la esquina inferior derecha.
 * @param y2 Coordenada Y de la esquina inferior derecha.
 * @param r Radio de las esquinas redondeadas.
 * @param color Color del rectángulo.
 */
void lcdDrawRoundRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint16_t color);

/**
 * @brief Dibuja una flecha en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param x0 Coordenada X de la base.
 * @param y0 Coordenada Y de la base.
 * @param x1 Coordenada X de la punta.
 * @param y1 Coordenada Y de la punta.
 * @param w Ancho de la flecha.
 * @param color Color de la flecha.
 */
void lcdDrawArrow(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint16_t color);

/**
 * @brief Dibuja una flecha rellena en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param x0 Coordenada X de la base.
 * @param y0 Coordenada Y de la base.
 * @param x1 Coordenada X de la punta.
 * @param y1 Coordenada Y de la punta.
 * @param w Ancho de la flecha.
 * @param color Color de la flecha.
 */
void lcdDrawFillArrow(TFT_t * dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint16_t color);

/**
 * @brief Establece la dirección de la fuente.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param direction Dirección de la fuente (0, 90, 180, 270 grados).
 */
void lcdSetFontDirection(TFT_t * dev, uint16_t direction);

/**
 * @brief Establece el color de relleno de la fuente.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param color Color de relleno de la fuente.
 */
void lcdSetFontFill(TFT_t * dev, uint16_t color);

/**
 * @brief Deshabilita el relleno de la fuente.
 * 
 * @param dev Puntero a la estructura TFT_t.
 */
void lcdUnsetFontFill(TFT_t * dev);

/**
 * @brief Establece el color del subrayado de la fuente.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param color Color del subrayado de la fuente.
 */
void lcdSetFontUnderLine(TFT_t * dev, uint16_t color);

/**
 * @brief Deshabilita el subrayado de la fuente.
 * 
 * @param dev Puntero a la estructura TFT_t.
 */
void lcdUnsetFontUnderLine(TFT_t * dev);

/**
 * @brief Apaga la retroiluminación de la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 */
void lcdBacklightOff(TFT_t * dev);

/**
 * @brief Enciende la retroiluminación de la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 */
void lcdBacklightOn(TFT_t * dev);

/**
 * @brief Desactiva la inversión de colores en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 */
void lcdInversionOff(TFT_t * dev);

/**
 * @brief Activa la inversión de colores en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 */
void lcdInversionOn(TFT_t * dev);

/**
 * @brief Configura el desplazamiento de la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 * @param scroll Tipo de desplazamiento.
 * @param start Posición inicial del desplazamiento.
 * @param end Posición final del desplazamiento.
 */
void lcdWrapArround(TFT_t * dev, SCROLL_TYPE_t scroll, int start, int end);

/**
 * @brief Finaliza la operación de dibujo en la pantalla LCD.
 * 
 * @param dev Puntero a la estructura TFT_t.
 */
void lcdDrawFinish(TFT_t *dev);


void LCD_DrawChar(TFT_t *dev, uint16_t x, uint16_t y, char c, FontDef *font, uint16_t color);
void LCD_DrawString(TFT_t *dev, uint16_t x, uint16_t y, const char *str, FontDef *font, uint16_t color);
#endif /* MAIN_ST7789_H_ */
