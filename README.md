# GP2040-CL GP2040CE WITH ST7789 RGB SCREEN SUPPORT
# GP2040-CL 一个加了st7789型号的全彩屏幕支持的gp2040ce固件

The monochrome OLED display was way too dull, so I added support for a color screen driver. Right now, it only includes hardware support for the ST7789.
I recommend using SPI1. For the wiring, please follow the official Raspberry Pi pico pinout diagram. Make sure not to connect the SPI1 pins to the SPI0 pins.
I’ve also added a nyan-cat screensaver and an extremely flashy boot animation (you’ll see where the “flashy” part is 😏). Text color is now fully customizable, complete with a color picker palette.
//------------------------------ Especially Important ----------------------------------//
Please strictly follow the official pinout diagram and connect using the correct SPI1 or SPI0 pins (MOSI, TX, SCK, etc.).
Beginner Recommended SPI1 Wiring:
First, give up the buttons on pins 10 and 11. You can reassign those buttons to pins 15, 21, and 22.

GND → GND
VCC → 3V3
SCK → 10
TX/MOSI/SDA → 11
RES → 14
DC → 26
CS → 27

If SPI0 does not work, try SPI1 instead.

Notes (for clarity):

“TX/MOSI/SDA” refers to the MOSI pin (data output from Pico).
This wiring is for SPI1 on the Raspberry Pi Pico.

单色的oled显示实在太单调了所以我加了个彩色屏幕驱动的支持，目前只是添加了st7789硬件的支持
建议使用spi1,具体接线请按照树莓派官方给出pico的引脚图示意接线，spi1协议的协议请不要接到spi0的引脚上
（设定接线引脚时可以先拖拽选中后再输入你要配置的引脚）
屏保添加了彩虹猫，并添加了极其酷炫的开机动画（酷炫在哪），文字颜色可以自定义并提供了取色器调色盘


//------------------------------尤其重要----------------------------------//
请严格按照官方的引脚线路图spi1或者spi0的MOSI TX SCK等引脚接线
新手建议SPI1引脚接线：首先放弃引脚10和11的按键，可以把这些按键分配到15,21,22上去
GND=》GND
VCC=》3V3
SCK=>10
TX/MOSI/SDA=>11
RES=>14
DC=>26
CS=>27
如果spi0不行那就试试spi12

