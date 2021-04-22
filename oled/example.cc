#include "pico/stdlib.h"
#include "oled.h"

extern "C" const uint8_t splash1_data[];

int main()
{
	//init_i2c();

	int h = 32;
	if(1) h = 64;
	init_display(h);

	ssd1306_print("HELLO PICO...\n"); // demonstrate some text
	show_scr();
	sleep_ms(2000);
	fill_scr(0); // empty the screen

	drawBitmap(0, 0, splash1_data, 64, 64, 1);
	show_scr();



	sleep_ms(2000);
	fill_scr(0);
    draw_pixel(10, 10, 1);
    show_scr();
    sleep_ms(2000);

    fill_scr(0);
    setCursorx(0);
	setCursory(0);
	ssd1306_print("Cursor at 0,0");
	show_scr();
    sleep_ms(2000);
    setCursorx(1);
    setCursory(1);
    ssd1306_print("Cursor at 1,1");
    show_scr();
    sleep_ms(2000);
    setCursorx(10);
    setCursory(5);
    ssd1306_print("Cursor at 10,5");
	show_scr();

	for(;;);
	return 0;
}
