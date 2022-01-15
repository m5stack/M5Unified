# M5Unified
### M5Stack Series unified library .

# Support framework
 - ESP-IDF
 - Arduino for ESP32

# Support device
 - M5Stack BASIC / GRAY / GO / FIRE
 - M5Stack Core2 / Tough
 - M5Stick C / CPlus
 - M5Stack CoreInk
 - M5Paper


# H/W infomation

### ESP32 GPIO list
|                    |M5Stack<BR>BASIC<BR>GRAY           |M5Stack<BR>GO/FIRE                 |M5Stack<BR>Core2<BR>Tough              |M5Stick<BR>C/CPlus            |M5Stack<BR>CoreInk      |M5Paper                |M5Station              |M5ATOM                  |                    |
|:------------------:|:---------------------------------:|:---------------------------------:|:-------------------------------------:|:----------------------------:|:----------------------:|:---------------------:|:---------------------:|:----------------------:|:------------------:|
|GPIO 0<BR>`ADC2_CH1`|`M-Bus`<BR>IIS_MK                  |`M-Bus`<BR>IIS_MK                  |`M-Bus`<BR>**SPK_LRCK<BR>PDM_C**(Core2)|`HAT`<BR>`PAD`<BR>**PDM_C**   |**EPD_RST**             | ---                   |                       |                        |GPIO 0<BR>`ADC2_CH1`|
|GPIO 1<BR>`USB_TX`  |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**                  |**Serial**                    |**Serial**              |**Serial**             |**Serial**             |**Serial**              |GPIO 1<BR>`USB_TX`  |
|GPIO 2<BR>`ADC2_CH2`|`M-Bus`<BR>                        |`M-Bus`<BR>                        |`M-Bus`<BR>**SPK D**                   |`PAD`<BR>**Beep**(CPlus)      |**Beep**                |**PW_Hold**            | REn?                  |                        |GPIO 2<BR>`ADC2_CH2`|
|GPIO 3<BR>`USB_RX`  |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**                  |**Serial**                    |**Serial**              |**Serial**             |**Serial**             |**Serial**              |GPIO 3<BR>`USB_RX`  |
|GPIO 4<BR>`ADC2_CH0`|**TF_CS**                          |**TF_CS**                          |**TF_CS**                              | ---                          |**EPD_BUSY**            |**TF_CS**              |**NEOPIXEL**           |                        |GPIO 4<BR>`ADC2_CH0`|
|GPIO 5              |`M-Bus`                            |`M-Bus`                            |**LCD_CS**                             |**LCD_CS**                    |**BTN_HAT**             |**EXT_5V**             |**LCD_CS**             |                        |GPIO 5              |
|GPIO 9              | ---                               | ---                               | ---                                   |**InfraRed**                  |**EPD_CS**              | ---                   | ---                   |                        |GPIO 9              |
|GPIO10              | ---                               | ---                               | ---                                   |**LED**                       |**LED**                 | ---                   | ---                   |                        |GPIO10              |
|GPIO12<BR>`ADC2_CH5`|`M-Bus`<BR>IIS_SK                  |`M-Bus`<BR>IIS_SK                  |**SPK BCLK**                           | ---                          |**PW_Hold**             |**SPI_MOSI**           | USB?                  |**InfraRed**            |GPIO12<BR>`ADC2_CH5`|
|GPIO13<BR>`ADC2_CH4`|`M-Bus`<BR>IIS_WS                  |`M-Bus`<BR>IIS_WS                  |`M-Bus`<BR>RXD2                        |**SPI_SCLK**                  |`MI-Bus`<BR>RXD2        |**SPI_MISO**           |`PORT.C1`              |                        |GPIO13<BR>`ADC2_CH4`|
|GPIO14<BR>`ADC2_CH6`|**LCD_CS**                         |**LCD_CS**                         |`M-Bus`<BR>TXD2                        | ---                          |`MI-Bus`<BR>TXD2        |**SPI_SCLK**           |`PORT.C1`              |                        |GPIO14<BR>`ADC2_CH6`|
|GPIO15<BR>`ADC2_CH3`|`M-Bus`<BR>IIS_OUT                 |`M-Bus`<BR>**NEOPIXEL**            |**LCD_D/C**                            |**SPI_MOSI**                  |**EPD_D/C**             |**EPD_CS**             |**LCD_RST**            |                        |GPIO15<BR>`ADC2_CH3`|
|GPIO16<BR>`PSRAM`   |`M-Bus`<BR>RXD2                    |`M-Bus`<BR>`PORT.C`<BR>RXD2        | ---                                   | ---                          | ---                    | ---                   |`PORT.C2`<BR>RXD2      |                        |GPIO16<BR>`PSRAM`   |
|GPIO17<BR>`PSRAM`   |`M-Bus`<BR>TXD2                    |`M-Bus`<BR>`PORT.C`<BR>TXD2        | ---                                   | ---                          | ---                    | ---                   |`PORT.C2`<BR>TXD2      |                        |GPIO17<BR>`PSRAM`   |
|GPIO18              |`M-Bus`<BR>**SPI_SCLK**            |`M-Bus`<BR>**SPI_SCLK**            |**SPI_SCLK**                           |**LCD_RST**                   |`MI-Bus`<BR>**SPI_SCLK**|`PORT.C`               |**SPI_SCLK**           |                        |GPIO18              |
|GPIO19              |`M-Bus`<BR>**SPI_MISO**            |`M-Bus`<BR>**SPI_MISO**            |`M-Bus`                                | ---                          |**RTC_INT**             |`PORT.C`               |LCD_D/C                |`Bus`<BR>**SPK_C**(ECHO)|GPIO19              |
|GPIO21              |`M-Bus`<BR>`PORT.A`<BR>**I2C0_SDA**|`M-Bus`<BR>`PORT.A`<BR>**I2C0_SDA**|**I2C1_SDA**                           |**I2C1_SDA**                  |`MI-Bus`<BR>**I2C1_SDA**|**I2C1_SDA**           |**I2C1_SDA**           |`Bus`<BR>**I2C1_SCL**   |GPIO21              |
|GPIO22              |`M-Bus`<BR>`PORT.A`<BR>**I2C0_SCL**|`M-Bus`<BR>`PORT.A`<BR>**I2C0_SCL**|**I2C1_SCL**                           |**I2C1_SCL**                  |`MI-Bus`<BR>**I2C1_SCL**|**I2C1_SCL**           |**I2C1_SCL**           |`Bus`<BR>**SPK_D**(ECHO)|GPIO22              |
|GPIO23              |`M-Bus`<BR>**SPI_MOSI**            |`M-Bus`<BR>**SPI_MOSI**            |**SPI_MOSI**                           |**LCD_D/C**                   |`MI-Bus`<BR>**SPI_MOSI**|**EPD_RST**            |**SPI_MOSI**           |`Bus`<BR>**PDM_D**(ECHO)|GPIO23              |
|GPIO25<BR>`DAC1`    |`M-Bus`<BR>**SPK_DAC**             |`M-Bus`<BR>**SPK_DAC**             |`M-Bus`                                |`HAT`(CPlus)<BR>`PAD`         |`MI-Bus`<BR>`HAT`       |`PORT.A`<BR>I2C0_SDA   |`PORT.B1`              |`Bus`<BR>**I2C1_SDA**   |GPIO25<BR>`DAC1`    |
|GPIO26<BR>`DAC2`    |`M-Bus`                            |`M-Bus`<BR>`PORT.B`                |`M-Bus`                                |`HAT`<BR>`PAD`                |`MI-Bus`<BR>`HAT`       |`PORT.B`               |`PORT.B2`              |`PORT.A`<BR>**I2C0_SDA**|GPIO26<BR>`DAC2`    |
|GPIO27<BR>`ADC2_CH7`|**LCD_D/C**                        |**LCD_D/C**                        |`M-Bus`                                |**AXP192 VBUSEN**             |**BTN_PWR**             |**EPD_BUSY**           |**IMU_INT**            |**NEOPIXEL**            |GPIO27<BR>`ADC2_CH7`|
|GPIO32<BR>`ADC1_CH4`|**LCD_BL**                         |**LCD_BL**                         |`M-Bus`<BR>`PORT.A`<BR>I2C0_SDA        |`PORT.A`<BR>I2C0_SDA          |`PORT.A`<BR>I2C0_SDA    |`PORT.A`<BR>I2C0_SCL   |`PORT.A`<BR>SDA        |`PORT.A`<BR>**I2C0_SCL**|GPIO32<BR>`ADC1_CH4`|
|GPIO33<BR>`ADC1_CH5`|**LCD_RST**                        |**LCD_RST**                        |`M-Bus`<BR>`PORT.A`<BR>I2C0_SCL        |`PORT.A`<BR>I2C0_SCL          |`PORT.A`<BR>I2C0_SCL    |`PORT.B`               |`PORT.A`<BR>SCL        |`Bus`<BR>**PDM_C**(ECHO)|GPIO33<BR>`ADC1_CH5`|
|GPIO34<BR>`ADC1_CH6`|`M-Bus`<BR>IIS_IN                  |`M-Bus`<BR>**MIC_ADC**<BR>IIS_IN   |`M-Bus`<BR>**PDM_D**(Core2)            |**PDM_D**                     |`MI-Bus`<BR>**SPI_MISO**| ---                   | USB Current?          |                        |GPIO34<BR>`ADC1_CH6`|
|GPIO35<BR>`ADC1_CH7`|`M-Bus`                            |`M-Bus`                            |`M-Bus`                                |**RTC_INT**                   |**BAT_V**               |**BAT_V**              |`PORT.B1`              |                        |GPIO35<BR>`ADC1_CH7`|
|GPIO36<BR>`ADC1_CH0`|`M-Bus`                            |`M-Bus`<BR>`PORT.B`                |`M-Bus`                                |`HAT`<BR>`PAD`                |`MI-Bus`<BR>`HAT`       |**TP_INT**             |`PORT.B2`              |                        |GPIO36<BR>`ADC1_CH0`|
|GPIO37<BR>`ADC1_CH1`|**BTN_C**                          |**BTN_C**                          | ---                                   |**BTN_A**                     |**SW_Up**               |**SW_Up**              |**BTN_A**              |                        |GPIO37<BR>`ADC1_CH1`|
|GPIO38<BR>`ADC1_CH2`|**BTN_B**                          |**BTN_B**                          |`M-Bus`<BR>**SPI_MISO**                |`PAD`                         |**SW_Press**            |**SW_Press**           |**BTN_B**              |                        |GPIO38<BR>`ADC1_CH2`|
|GPIO39<BR>`ADC1_CH3`|**BTN_A**                          |**BTN_A**                          |**TP_INT**                             |**BTN_B**                     |**SW_Down**             |**SW_Down**            |**BTN_C**              |**BTN**                 |GPIO39<BR>`ADC1_CH3`|
|                    |M5Stack<BR>BASIC<BR>GRAY           |M5Stack<BR>GO/FIRE                 |M5Stack<BR>Core2<BR>Tough              |M5Stick<BR>C/CPlus            |M5Stack<BR>CoreInk      |M5Paper                |M5Station              |M5ATOM                  |                    |

### AXP192 IO list
|              |M5Stack<BR>Core2   |M5Stack<BR>Tough   |M5Stick<BR>C    |M5Stick<BR>CPlus|              |
|:------------:|:-----------------:|:-----------------:|:--------------:|:--------------:|:------------:|
|GPIO0<br>LDO0 |BUS PW EN          |BUS PW EN          |MIC VCC         |MIC VCC         |GPIO0<br>LDO0 |
| GPIO1        |SYS LED            |TP RST             | ---            | ---            | GPIO1        |
| GPIO2        |SPK EN             |SPK EN             | ---            | ---            | GPIO2        |
| GPIO3        | ---               | ---               | ---            | ---            | GPIO3        |
| GPIO4        |LCD RST<BR>TP RST  |LCD RST            | ---            | ---            | GPIO4        |
| EXTEN        |PORT 5V EN         |PORT 5V EN         |PORT 5V EN      |PORT 5V EN      | EXTEN        |
| BACKUP       |RTC BAT            |RTC BAT            |RTC BAT         |RTC BAT         | BACKUP       |
| LDO1         |RTC VDD            |RTC VDD            |RTC VDD         |RTC VDD         | LDO1         |
| LDO2         |LCD PW<BR>Periph PW|LCD PW<BR>Periph PW|LCD BL          |LCD BL          | LDO2         |
| LDO3         |VIB MOTOR          |LCD BL             |LCD PW          |LCD PW          | LDO3         |
| DCDC1        |ESP32 VDD          |ESP32 VDD          |ESP32 VDD       |ESP32 VDD       | DCDC1        |
| DCDC2        | ---               | ---               | ---            | ---            | DCDC2        |
| DCDC3        |LCD BL             | ---               | ---            | ---            | DCDC3        |


### PinMap

|               |M5Stack<BR>BASIC<BR>GRAY<BR>GO/FIRE<BR>FACES II|M5Stack<BR>Core2<BR>Tough |M5Stick<BR>C            |M5Stick<BR>CPlus        |M5Stack<BR>CoreInk     |M5Paper                |                |
|:-------------:|:----------------------------:|:------------------------:|:----------------------:|:----------------------:|:---------------------:|:---------------------:|:--------------:|
|  PortA        |<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">G21</TD></TR><TR><TD bgcolor="#ffffff">G22</TD></TR></TABLE>|<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">G32</TD></TR><TR><TD bgcolor="#ffffff">G33</TD></TR></TABLE>|<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">G32</TD></TR><TR><TD bgcolor="#ffffff">G33</TD></TR></TABLE>|<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">G32</TD></TR><TR><TD bgcolor="#ffffff">G33</TD></TR></TABLE>|<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">G32</TD></TR><TR><TD bgcolor="#ffffff">G33</TD></TR></TABLE>|<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">G25</TD></TR><TR><TD bgcolor="#ffffff">G32</TD></TR></TABLE>|PortA|
|  PortB        |<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">G26</TD></TR><TR><TD bgcolor="#ffffff">G36</TD></TR></TABLE>(GO/FIRE/FACES II)| --- | --- | --- | --- |<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">G26</TD></TR><TR><TD bgcolor="#ffffff">G33</TD></TR></TABLE>|PortB|
|  PortC        |<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">G17</TD></TR><TR><TD bgcolor="#ffffff">G16</TD></TR></TABLE>(GO/FIRE/FACES II)| --- | --- | --- | --- |<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">G18</TD></TR><TR><TD bgcolor="#ffffff">G19</TD></TR></TABLE>|PortC|
|  HAT          |---|---|<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5Vout</TD></TR><TR><TD bgcolor="#efefef">G26</TD></TR><TR><TD bgcolor="#dfdfdf">G36</TD></TR><TR><TD bgcolor="#efefef">G0</TD></TR><TR><TD bgcolor="#ffbfbf">BAT</TD></TR><TR><TD bgcolor="#ffff7f">3V3</TD></TR><TR><TD bgcolor="#ff9f9f">5Vin</TD></TR></TABLE>|<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5Vout</TD></TR><TR><TD bgcolor="#efefef">G26</TD></TR><TR><TD bgcolor="#dfdfdf">G25/G36</TD></TR><TR><TD bgcolor="#efefef">G0</TD></TR><TR><TD bgcolor="#ffbfbf">BAT</TD></TR><TR><TD bgcolor="#ffff7f">3V3</TD></TR><TR><TD bgcolor="#ff9f9f">5Vin</TD></TR></TABLE>|<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD></TR><TR><TD bgcolor="#ff7f7f">5Vout</TD></TR><TR><TD bgcolor="#efefef">G26</TD></TR><TR><TD bgcolor="#dfdfdf">G36</TD></TR><TR><TD bgcolor="#efefef">G25</TD></TR><TR><TD bgcolor="#ffbfbf">BAT</TD></TR><TR><TD bgcolor="#ffff7f">3V3</TD></TR><TR><TD bgcolor="#ff9f9f">5Vin</TD></TR></TABLE>|---|HAT|
|M-Bus<BR>MI-Bus|<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD><TD bgcolor="#9fff9f">G35</TD></TR><TR><TD bgcolor="#7f7f7f">GND</TD><TD bgcolor="#9fff9f">G36</TD></TR><TR><TD bgcolor="#7f7f7f">GND</TD><TD bgcolor="#9f9f9f">EN</TD></TR><TR><TD bgcolor="#cfcfff">G23</TD><TD bgcolor="#ffcfcf">G25</TD></TR><TR><TD bgcolor="#cfcfff">G19</TD><TD bgcolor="#ffcfcf">G26</TD></TR><TR><TD bgcolor="#cfcfff">G18</TD><TD bgcolor="#ff7f7f">3V3</TD></TR><TR><TD bgcolor="#cfffcf">G3</TD><TD bgcolor="#cfffcf">G1</TD></TR><TR><TD bgcolor="#cfffcf">G16</TD><TD bgcolor="#cfffcf">G17</TD></TR><TR><TD bgcolor="#9f9f9f">G21</TD><TD bgcolor="#9f9f9f">G22</TD></TR><TR><TD bgcolor="#9f9fff">G2</TD><TD bgcolor="#9f9fff">G5</TD></TR><TR><TD bgcolor="#efefef">G12</TD><TD bgcolor="#efefef">G13</TD></TR><TR><TD bgcolor="#efefef">G15</TD><TD bgcolor="#efefef">G0</TD></TR><TR><TD bgcolor="#ffff7f">HPWR</TD><TD bgcolor="#efefef">G34</TD></TR><TR><TD bgcolor="#ffff7f">HPWR</TD><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#ffff7f">HPWR</TD><TD bgcolor="#ffbfbf">BAT</TD></TR></TABLE>|<TABLE><TR><TD bgcolor="#7f7f7f">GND</TD><TD bgcolor="#9fff9f">G35</TD></TR><TR><TD bgcolor="#7f7f7f">GND</TD><TD bgcolor="#9fff9f">G36</TD></TR><TR><TD bgcolor="#7f7f7f">GND</TD><TD bgcolor="#9f9f9f">EN</TD></TR><TR><TD bgcolor="#cfcfff">G23</TD><TD bgcolor="#ffcfcf">G25</TD></TR><TR><TD bgcolor="#cfcfff">G38</TD><TD bgcolor="#ffcfcf">G26</TD></TR><TR><TD bgcolor="#cfcfff">G18</TD><TD bgcolor="#ff7f7f">3V3</TD></TR><TR><TD bgcolor="#cfffcf">G3</TD><TD bgcolor="#cfffcf">G1</TD></TR><TR><TD bgcolor="#cfffcf">G13</TD><TD bgcolor="#cfffcf">G14</TD></TR><TR><TD bgcolor="#9f9f9f">G21</TD><TD bgcolor="#9f9f9f">G22</TD></TR><TR><TD bgcolor="#9f9fff">G32</TD><TD bgcolor="#9f9fff">G33</TD></TR><TR><TD bgcolor="#efefef">G27</TD><TD bgcolor="#efefef">G19</TD></TR><TR><TD bgcolor="#efef7f">G2</TD><TD bgcolor="#efef7f">G0</TD></TR><TR><TD bgcolor="#7f7f7f">NC</TD><TD bgcolor="#efef7f">G34</TD></TR><TR><TD bgcolor="#7f7f7f">NC</TD><TD bgcolor="#ff7f7f">5V</TD></TR><TR><TD bgcolor="#7f7f7f">NC</TD><TD bgcolor="#ffbfbf">BAT</TD></TR></TABLE>|---|---|<TABLE><TR><TD bgcolor="#ffcfcf">G25</TD><TD bgcolor="#cfcfff">G23</TD></TR><TR><TD bgcolor="#ffcfcf">G26</TD><TD bgcolor="#cfcfff">G34</TD></TR><TR><TD bgcolor="#ffcfcf">G36</TD><TD bgcolor="#cfcfff">G18</TD></TR><TR><TD bgcolor="#9f9f9f">G22</TD><TD bgcolor="#9f9f9f">G21</TD></TR><TR><TD bgcolor="#cfffcf">G14</TD><TD bgcolor="#cfffcf">G13</TD></TR><TR><TD bgcolor="#ff7f7f">3V3</TD><TD bgcolor="#9f9f9f">RST</TD></TR><TR><TD bgcolor="#ff7f7f">5Vout</TD><TD bgcolor="#9f9f9f">5Vin</TD></TR><TR><TD bgcolor="#ffbfbf">BAT</TD><TD bgcolor="#7f7f7f">GND</TD></TR></TABLE>|---|M-Bus<BR>MI-Bus|




### SPI device

|                |M5Stack<BR>BASIC<BR>GRAY<BR>GO/FIRE|M5Stack<BR>Core2<BR>Tough      |M5Stick<BR>C                 |M5Stick<BR>CPlus               |M5Stack<BR>CoreInk                |M5Paper                       |                |
|:--------------:|:---------------------------------:|:-----------------------------:|:---------------------------:|:-----------------------------:|:--------------------------------:|:----------------------------:|:--------------:|
| Display        |`ILI9342C`<BR>320×240<BR>CS:G14   |`ILI9342C`<BR>320×240<BR>CS:G5|`ST7735S`<BR>80×160<BR>CS:G5|`ST7789V2`<BR>135×240<BR>CS:G5|`GDEW0154M09`<BR>200×200<BR>CS:G9|`IT8951`<BR>960×540<BR>CS:G15| Display        |
| TF Card        |CS:4                               |CS:4                           | ---                         | ---                           | ---                              |CS:4                          | TF Card        |


### I2C device

|                |M5Stack<BR>BASIC/GRAY<BR>GO/FIRE   |M5Stack<BR>Core2      |M5Stack<BR>Tough    |M5Stick<BR>C<BR>CPlus|M5Stack<BR>CoreInk |M5Paper              |                |
|:--------------:|:---------------------------------:|:--------------------:|:------------------:|:-------------------:|:-----------------:|:-------------------:|:--------------:|
|Touch<BR>Panel  | ---                               |`FT6336U`<BR>38h      |`CHSC6540`<BR>2Eh   | ---                 | ---               |`GT911`<BR>14h or 5Dh|Touch<BR>Panel  |
|RTC             | ---                               |`BM8563`<BR>51h       |`BM8563`<BR>51h     |`BM8563`<BR>51h      |`BM8563`<BR>51h    |`BM8563`<BR>51h      |RTC             |
|Power<BR>Manage |`IP5306`<BR>75h                    |`AXP192`<BR>34h       |`AXP192`<BR>34h     |`AXP192`<BR>34h      | ---               | ---                 |Power<BR>Manage |
|IMU             |`MPU6886`<BR>68h                   |`MPU6886`<BR>68h (Ext)| ---                |`MPU6886`<BR>68h     | ---               | ---                 |IMU             |
|IMU<BR>(old lot)|`SH200Q`<BR>6Ch                    | ---                  | ---                |`SH200Q`<BR>6Ch      | ---               | ---                 |IMU<BR>(old lot)|
|ENV             | ---                               | ---                  | ---                | ---                 | ---               |`SHT30`<BR>44h       |ENV             |
|EEPROM          | ---                               | ---                  | ---                | ---                 | ---               |`FM24C02`<BR>50h     |EEPROM          |


