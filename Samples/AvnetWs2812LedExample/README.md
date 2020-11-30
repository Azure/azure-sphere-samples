# WS2812B Led Strips Driver for Azure Sphere

This Project presents a driver for WS2812B Led Strips to be used with Azure Sphere. 

The driver was created with the main ideas from this [post](https://jeelabs.org/book/1450d)

The technique described in the post uses the SPI hardware of a LPC810 MCU in order to send the signal to the WS2812B strip.

## WS2812B signal requirements

From the WS2812B datasheet, each led requires 24bits (3 bytes. Green, Red, Blue) to set the color. These bits are not given as HIGH for "1" and LOW for "0"", rather a signal of 600ns HIGH + 300ns LOW is required for a "1", and a signal of 300ns HIGH + 600ns LOW is required for a "0"". These values have been chosen arbitrarily from the aceptable range.

![](https://raw.githubusercontent.com/judios/WS2812B-Driver-For-Azure-Sphere/master/docs/Signal.PNG)

Therefore the translation 1 = 110, 0 = 100 can be used to send the signal to the WS2812B led strip through the SPI MOSI. Provided that the clock of the SPI is set to achieve 300ns per bit. Which is exactly 3.33Mhz, but can be aproximated to 3.4Mhz.

Since there are 3 spi bits needed per WS2812B bit, 72 spi bits are needed per led. These are broken into 3 sets of 3 bytes, one set per color.

The technique in the post uses two words of 12 bits to set each color of the led. However, Azure Sphere SDK does not allows to change the word size of the SPI, and it is not really needed. One can use three bytes (uint8_t) per color:

```
b1 : 1#0 1#0 1#
b2 : 0 1#0 1#0 1
b3 : #0 1#0 1#0 
```

Sending these three bytes in sequence will produce the 8 WS2812b bits required:
```
b1, b2, b3 :  1#0 1#0 1#0 1#0 1#0 1#0 1#0 1#0
```
Note, the second bit in each triplet is the one carrying the value, the bits 7, 6, and 5 are set in b1; bits 4, and 3 are set in b2; bits 2, 1, 0 are set in b3.

```c
	wscolor->b1 = 0b10010010;
	wscolor->b2 = 0b01001001;
	wscolor->b3 = 0b00100100;
	if (value == 0) return;
	if (0b10000000 & value) wscolor->b1 += 0b01000000;
	if (0b01000000 & value) wscolor->b1 += 0b00001000;
	if (0b00100000 & value) wscolor->b1 += 0b00000001;
	if (0b00010000 & value) wscolor->b2 += 0b00100000;
	if (0b00001000 & value) wscolor->b2 += 0b00000100;
	if (0b00000100 & value) wscolor->b3 += 0b10000000;
	if (0b00000010 & value) wscolor->b3 += 0b00010000;
	if (0b00000001 & value) wscolor->b3 += 0b00000010;
```

## WS2811 Driver

Following the same approach, a driver for the WS2811 ic was created. In this case, a signal of 1200ns HIGH + 1200ns LOW is required for a "1", and a signal of 400ns HIGH + 2000ns LOW is required for a "0". The translation will then need 6 spi bits per WS2811 bit required.

```
1 = 111000
0 = 100000
```

Here, there are 24 spi bits needed for each color. Then, uint16_t type was used instead for b1, b2, and b3


## Credits

After spending some time trying to write a driver for the WS2812b led strips to be used with the Azure Sphere, based on pure software, I realized that the smalest time step I could achieve was in the order of 65 micro seconds. When I was about to give up, I found the post from Jean-Claude Wippler.  

https://jeelabs.org/book/1450d/
