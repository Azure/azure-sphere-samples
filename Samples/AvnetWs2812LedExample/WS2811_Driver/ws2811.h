#include <inttypes.h>

typedef struct {
	uint16_t b1;
	uint16_t b2;
	uint16_t b3;
}WS11_Color;

typedef struct {
	WS11_Color Red;
	WS11_Color Green;
	WS11_Color Blue;
}WS11_Pixel;


/* Initialize Pixel Strip 
   count : number of pixels
   spi : SPI to be used; 0 for ISU0, 1 for ISU1 
   returns -1 if something fails */
int WS11_PixelStrip_Init(int count, int spi);

/* Sets the Color for the pixel at the given index 
   use index = -1 to set the same color for all pixels */
void WS11_PixelStrip_SetColor(int index, uint8_t red, uint8_t green, uint8_t blue);

/* Sends the signal with the current properties for all of the pixels in the strip */
void WS11_PixelStrip_Show();




