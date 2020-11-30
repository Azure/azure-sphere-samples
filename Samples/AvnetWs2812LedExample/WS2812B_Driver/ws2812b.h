#include <inttypes.h>

typedef struct {
	uint8_t b1;
	uint8_t b2;
	uint8_t b3;
}WS_Color;

typedef struct {
	WS_Color Green;
	WS_Color Red;
	WS_Color Blue;
}WS_Pixel;


/* Initialize Pixel Strip 
   count : number of pixels
   spi : SPI to be used; 0 for ISU0, 1 for ISU1 
   returns -1 if something fails */
int WS_PixelStrip_Init(int count, int spi);

/* Sets the Color for the pixel at the given index 
   use index = -1 to set the same color for all pixels */
void WS_PixelStrip_SetColor(int index, uint8_t red, uint8_t green, uint8_t blue);

/* Sends the signal with the current properties for all of the pixels in the strip */
void WS_PixelStrip_Show();




