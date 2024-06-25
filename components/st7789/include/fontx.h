#ifndef FONT24_H
#define FONT24_H

#include <stdint.h>
// Estructura para la definición de la fuente
typedef struct {
    const uint8_t *table;  // Tabla de datos de la fuente
    uint16_t width;        // Ancho de un carácter
    uint16_t height;       // Altura de un carácter
} FontDef;

// Declaración de la tabla de la fuente de 24px
extern const uint8_t Font24_Table[];
#endif // FONT24_H