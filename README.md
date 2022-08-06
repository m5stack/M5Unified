# M5Unified
### M5Stack Series unified library .

## Supported development environment

- ArduinoIDE
- PlatformIO

Note: Using CMake ESP-IDF projects is not supported.

## Supported framework
 - ESP-IDF
 - Arduino for ESP32

## Supported device
 - M5Stack BASIC / GRAY / GO / FIRE
 - M5Stack Core2 / Tough
 - M5Stick C / CPlus
 - M5Stack CoreInk
 - M5Paper
 - M5ATOM Lite / Matrix / ECHO / PSRAM / U
 - M5STAMP PICO / C3 / C3U

## Supported device (external display)
 - Unit LCD (with no display model)
 - Unit OLED (with no display model)
 - ATOM Display (with M5ATOM Lite / Matrix / PSRAM)

## Supported device (external speaker)
 - SPK HAT (with M5StickC / CPlus / M5Stack CoreInk)
 - ATOMIC SPK (with M5ATOM Lite / PSRAM)


# H/W infomation

### ESP32 GPIO list
|                    |M5Stack<BR>BASIC<BR>GRAY           |M5Stack<BR>GO/FIRE                 |M5Stack<BR>Core2<BR>Tough              |M5Stick<BR>C/CPlus            |M5Stack<BR>CoreInk      |M5Paper                |M5Station              |M5ATOM<BR>Lite/Matrix<BR>ECHO/U<BR>PSRAM |M5STAMP<BR>PICO     |                    |
|:------------------:|:---------------------------------:|:---------------------------------:|:-------------------------------------:|:----------------------------:|:----------------------:|:---------------------:|:---------------------:|:---------------------------------------:|:------------------:|:------------------:|
|GPIO 0<BR>`ADC2_CH1`|`M-Bus`<BR>IIS_MK                  |`M-Bus`<BR>IIS_MK                  |`M-Bus`<BR>**SPK_LRCK<BR>PDM_C**(Core2)|`HAT`<BR>`PAD`<BR>**PDM_C**   |**EPD_RST**             | ---                   | ---                   | ---                                     |                    |GPIO 0<BR>`ADC2_CH1`|
|GPIO 1<BR>`USB_TX`  |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**                  |**Serial**                    |**Serial**              |**Serial**             |**Serial**             |**Serial**                               |**Serial**          |GPIO 1<BR>`USB_TX`  |
|GPIO 2<BR>`ADC2_CH2`|`M-Bus`<BR>                        |`M-Bus`<BR>                        |`M-Bus`<BR>**SPK D**                   |`PAD`<BR>**Beep**(CPlus)      |**Beep**                |**PW_Hold**            | REn?                  | ---                                     | ---                |GPIO 2<BR>`ADC2_CH2`|
|GPIO 3<BR>`USB_RX`  |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**                  |**Serial**                    |**Serial**              |**Serial**             |**Serial**             |**Serial**                               |**Serial**          |GPIO 3<BR>`USB_RX`  |
|GPIO 4<BR>`ADC2_CH0`|**TF_CS**                          |**TF_CS**                          |**TF_CS**                              | ---                          |**EPD_BUSY**            |**TF_CS**              |**RGB LED**            | ---                                     | ---                |GPIO 4<BR>`ADC2_CH0`|
|GPIO 5              |`M-Bus`                            |`M-Bus`                            |**LCD_CS**                             |**LCD_CS**                    |**BTN_HAT**             |**EXT_5V**             |**LCD_CS**             |(PSRAM)<BR>**PDM_C**(U)                  | ---                |GPIO 5              |
|GPIO 9              | ---                               | ---                               | ---                                   |**InfraRed**                  |**EPD_CS**              | ---                   | ---                   | ---                                     | ---                |GPIO 9              |
|GPIO10              | ---                               | ---                               | ---                                   |**LED**                       |**LED**                 | ---                   | ---                   | ---                                     | ---                |GPIO10              |
|GPIO12<BR>`ADC2_CH5`|`M-Bus`<BR>IIS_SK                  |`M-Bus`<BR>IIS_SK                  |**SPK BCLK**                           | ---                          |**PW_Hold**             |**SPI_MOSI**           | USB?                  |**InfraRed**                             | ---                |GPIO12<BR>`ADC2_CH5`|
|GPIO13<BR>`ADC2_CH4`|`M-Bus`<BR>IIS_WS                  |`M-Bus`<BR>IIS_WS                  |`M-Bus`<BR>RXD2                        |**SPI_SCLK**                  |`MI-Bus`<BR>RXD2        |**SPI_MISO**           |`PORT.C1`              | ---                                     | ---                |GPIO13<BR>`ADC2_CH4`|
|GPIO14<BR>`ADC2_CH6`|**LCD_CS**                         |**LCD_CS**                         |`M-Bus`<BR>TXD2                        | ---                          |`MI-Bus`<BR>TXD2        |**SPI_SCLK**           |`PORT.C1`              | ---                                     | ---                |GPIO14<BR>`ADC2_CH6`|
|GPIO15<BR>`ADC2_CH3`|`M-Bus`<BR>IIS_OUT                 |`M-Bus`<BR>**RGB LED**             |**LCD_D/C**                            |**SPI_MOSI**                  |**EPD_D/C**             |**EPD_CS**             |**LCD_RST**            | ---                                     | ---                |GPIO15<BR>`ADC2_CH3`|
|GPIO16<BR>`PSRAM`   |`M-Bus`<BR>RXD2                    |`M-Bus`<BR>`PORT.C`<BR>RXD2        | ---                                   | ---                          | ---                    | ---                   |`PORT.C2`<BR>RXD2      | ---                                     | ---                |GPIO16<BR>`PSRAM`   |
|GPIO17<BR>`PSRAM`   |`M-Bus`<BR>TXD2                    |`M-Bus`<BR>`PORT.C`<BR>TXD2        | ---                                   | ---                          | ---                    | ---                   |`PORT.C2`<BR>TXD2      | ---                                     | ---                |GPIO17<BR>`PSRAM`   |
|GPIO18              |`M-Bus`<BR>**SPI_SCLK**            |`M-Bus`<BR>**SPI_SCLK**            |**SPI_SCLK**                           |**LCD_RST**                   |`MI-Bus`<BR>**SPI_SCLK**|`PORT.C`               |**SPI_SCLK**           | ---                                     |                    |GPIO18              |
|GPIO19              |`M-Bus`<BR>**SPI_MISO**            |`M-Bus`<BR>**SPI_MISO**            |`M-Bus`                                | ---                          |**RTC_INT**             |`PORT.C`               |LCD_D/C                |`Bus`<BR>**SPK_C**(ECHO)<BR>***PDM_D**(U)|                    |GPIO19              |
|GPIO21              |`M-Bus`<BR>`PORT.A`<BR>**I2C0_SDA**|`M-Bus`<BR>`PORT.A`<BR>**I2C0_SDA**|**I2C1_SDA**                           |**I2C1_SDA**                  |`MI-Bus`<BR>**I2C1_SDA**|**I2C1_SDA**           |**I2C1_SDA**           |`Bus`<BR>**I2C1_SCL**                    |                    |GPIO21              |
|GPIO22              |`M-Bus`<BR>`PORT.A`<BR>**I2C0_SCL**|`M-Bus`<BR>`PORT.A`<BR>**I2C0_SCL**|**I2C1_SCL**                           |**I2C1_SCL**                  |`MI-Bus`<BR>**I2C1_SCL**|**I2C1_SCL**           |**I2C1_SCL**           |`Bus`<BR>**SPK_D**(ECHO)                 |                    |GPIO22              |
|GPIO23              |`M-Bus`<BR>**SPI_MOSI**            |`M-Bus`<BR>**SPI_MOSI**            |**SPI_MOSI**                           |**LCD_D/C**                   |`MI-Bus`<BR>**SPI_MOSI**|**EPD_RST**            |**SPI_MOSI**           |`Bus`<BR>**PDM_D**(ECHO)                 | ---                |GPIO23              |
|GPIO25<BR>`DAC1`    |`M-Bus`<BR>**SPK_DAC**             |`M-Bus`<BR>**SPK_DAC**             |`M-Bus`                                |`HAT`(CPlus)<BR>`PAD`         |`MI-Bus`<BR>`HAT`       |`PORT.A`<BR>I2C0_SDA   |`PORT.B1`              |`Bus`<BR>**I2C1_SDA**                    |                    |GPIO25<BR>`DAC1`    |
|GPIO26<BR>`DAC2`    |`M-Bus`                            |`M-Bus`<BR>`PORT.B`                |`M-Bus`                                |`HAT`<BR>`PAD`                |`MI-Bus`<BR>`HAT`       |`PORT.B`               |`PORT.B2`              |`PORT.A`<BR>**I2C0_SDA**                 |                    |GPIO26<BR>`DAC2`    |
|GPIO27<BR>`ADC2_CH7`|**LCD_D/C**                        |**LCD_D/C**                        |`M-Bus`                                |**AXP192 VBUSEN**             |**BTN_PWR**             |**EPD_BUSY**           |**IMU_INT**            |**RGB LED**                              |**RGB LED**         |GPIO27<BR>`ADC2_CH7`|
|GPIO32<BR>`ADC1_CH4`|**LCD_BL**                         |**LCD_BL**                         |`M-Bus`<BR>`PORT.A`<BR>I2C0_SDA        |`PORT.A`<BR>I2C0_SDA          |`PORT.A`<BR>I2C0_SDA    |`PORT.A`<BR>I2C0_SCL   |`PORT.A`<BR>SDA        |`PORT.A`<BR>**I2C0_SCL**                 |`PORT.A`<BR>I2C0_SDA|GPIO32<BR>`ADC1_CH4`|
|GPIO33<BR>`ADC1_CH5`|**LCD_RST**                        |**LCD_RST**                        |`M-Bus`<BR>`PORT.A`<BR>I2C0_SCL        |`PORT.A`<BR>I2C0_SCL          |`PORT.A`<BR>I2C0_SCL    |`PORT.B`               |`PORT.A`<BR>SCL        |`Bus`<BR>**PDM_C**(ECHO)                 |`PORT.A`<BR>I2C0_SCL|GPIO33<BR>`ADC1_CH5`|
|GPIO34<BR>`ADC1_CH6`|`M-Bus`<BR>IIS_IN                  |`M-Bus`<BR>**MIC_ADC**<BR>IIS_IN   |`M-Bus`<BR>**PDM_D**(Core2)            |**PDM_D**                     |`MI-Bus`<BR>**SPI_MISO**| ---                   | USB Current?          |                                         | ---                |GPIO34<BR>`ADC1_CH6`|
|GPIO35<BR>`ADC1_CH7`|`M-Bus`                            |`M-Bus`                            |`M-Bus`                                |**RTC_INT**                   |**BAT_V**               |**BAT_V**              |`PORT.B1`              | ---                                     | ---                |GPIO35<BR>`ADC1_CH7`|
|GPIO36<BR>`ADC1_CH0`|`M-Bus`                            |`M-Bus`<BR>`PORT.B`                |`M-Bus`                                |`HAT`<BR>`PAD`                |`MI-Bus`<BR>`HAT`       |**TP_INT**             |`PORT.B2`              | ---                                     | ---                |GPIO36<BR>`ADC1_CH0`|
|GPIO37<BR>`ADC1_CH1`|**BTN_C**                          |**BTN_C**                          | ---                                   |**BTN_A**                     |**SW_Up**               |**SW_Up**              |**BTN_A**              | ---                                     | ---                |GPIO37<BR>`ADC1_CH1`|
|GPIO38<BR>`ADC1_CH2`|**BTN_B**                          |**BTN_B**                          |`M-Bus`<BR>**SPI_MISO**                |`PAD`                         |**SW_Press**            |**SW_Press**           |**BTN_B**              | ---                                     | ---                |GPIO38<BR>`ADC1_CH2`|
|GPIO39<BR>`ADC1_CH3`|**BTN_A**                          |**BTN_A**                          |**TP_INT**                             |**BTN_B**                     |**SW_Down**             |**SW_Down**            |**BTN_C**              |**BTN**                                  |**BTN**             |GPIO39<BR>`ADC1_CH3`|
|                    |M5Stack<BR>BASIC<BR>GRAY           |M5Stack<BR>GO/FIRE                 |M5Stack<BR>Core2<BR>Tough              |M5Stick<BR>C/CPlus            |M5Stack<BR>CoreInk      |M5Paper                |M5Station              |M5ATOM<BR>Lite/Matrix<BR>ECHO/U<BR>PSRAM |M5STAMP<BR>PICO     |                    |


### ESP32C3 GPIO list
|                    |M5Stamp<BR>C3                   |M5Stamp<BR>C3U                  |
|:------------------:|:------------------------------:|:------------------------------:|
|GPIO 0              |`PORT.A`<BR>**I2C0SCL**         |`PORT.A`<BR>**I2C_SCL**         |
|GPIO 1              |`PORT.A`<BR>**I2C0SDA**         |`PORT.A`<BR>**I2C_SDA**         |
|GPIO 2              |**RGB LED**                     |**RGB LED**                     |
|GPIO 3              |**BTN_A**                       |`Bus`                           |
|GPIO 4              |`Bus`                           |`Bus`                           |
|GPIO 5              |`Bus`                           |`Bus`                           |
|GPIO 6              |`Bus`                           |`Bus`                           |
|GPIO 7              |`Bus`                           |`Bus`                           |
|GPIO 8              |`Bus`                           |`Bus`                           |
|GPIO 9              | ---                            |**BTN_A**                       |
|GPIO10              |`Bus`                           |`Bus`                           |
|GPIO18              |`PORT.U`<BR>**D-**              |`USB`<BR>`PORT.U`<BR>**D-**     |
|GPIO19              |`PORT.U`<BR>**D+**              |`USB`<BR>`PORT.U`<BR>**D+**     |
|GPIO20              |`USB`<BR>**Serial**             |`Bus`<BR>                       |
|GPIO21              |`USB`<BR>**Serial**             |`Bus`<BR>                       |
|                    |M5Stamp<BR>C3                   |M5Stamp<BR>C3U                  |


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

<TABLE>
 <TR>
  <TH></TH>
  <TH width="24%">M5Stack<BR>BASIC/GRAY<BR>GO/FIRE<BR>FACES II</TH>
  <TH width="24%">M5Stack<BR>Core2<BR>Core2AWS<BR>TOUGH</TH>
  <TH width="16%"> M5Paper </TH>
  <TH width="32%" colspan="2"> M5Station </TH>
 </TR>
 <TR align="center">
  <TD>PortA</TD>
  <TD><IMG src="docs/img/pin_def_core_porta.svg"    title="G,V,21,22"><BR>PortA</TD>
  <TD><IMG src="docs/img/pin_def_core2_porta.svg"   title="G,V,32,33"><BR>PortA</TD>
  <TD><IMG src="docs/img/pin_def_paper_porta.svg"   title="G,V,25,32"><BR>PortA</TD>
  <TD colspan="2"><IMG src="docs/img/pin_def_station_porta.svg" title="G,V,32,33"><BR>PortA</TD>
 </TR>
 <TR align="center">
  <TD>PortB</TD>
  <TD><IMG src="docs/img/pin_def_core_portb.svg"     title="G,V,26,36"><BR>PortB</TD>
  <TD><IMG src="docs/img/pin_def_core2_portb.svg"    title="G,V,26,36"><BR>PortB</TD>
  <TD><IMG src="docs/img/pin_def_paper_portb.svg"    title="G,V,26,33"><BR>PortB</TD>
  <TD><IMG src="docs/img/pin_def_station_portb1.svg" title="G,V,25,35"><BR>PortB1</TD>
  <TD><IMG src="docs/img/pin_def_station_portb2.svg" title="G,V,26,36"><BR>PortB2</TD>
 </TR>
 <TR align="center">
  <TD>PortC</TD>
  <TD><IMG src="docs/img/pin_def_core_portc.svg"     title="G,V,17,16"><BR>PortC</TD>
  <TD><IMG src="docs/img/pin_def_core2_portc.svg"    title="G,V,14,13"><BR>PortC</TD>
  <TD><IMG src="docs/img/pin_def_paper_portc.svg"    title="G,V,18,19"><BR>PortC</TD>
  <TD><IMG src="docs/img/pin_def_station_portc1.svg" title="G,V,14,13"><BR>PortC1</TD>
  <TD><IMG src="docs/img/pin_def_station_portc2.svg" title="G,V,17,16"><BR>PortC2</TD>
 </TR>
 <TR align="center">
  <TD>PortD</TD>
  <TD><IMG src="docs/img/pin_def_core_portd.svg"  title="G,V,35,34"><BR>PortD</TD>
  <TD><IMG src="docs/img/pin_def_core2_portd.svg" title="G,V,35,34"><BR>PortD</TD>
  <TD></TD>
  <TD colspan="2"></TD>
 </TR>
 <TR align="center">
  <TD>PortE</TD>
  <TD><IMG src="docs/img/pin_def_core_porte.svg"  title="G,V,13,5" ><BR>PortE</TD>
  <TD><IMG src="docs/img/pin_def_core2_porte.svg" title="G,V,19,27"><BR>PortE / 485<BR>TOUGH485:12V</TD>
  <TD></TD>
  <TD colspan="2"></TD>
 </TR>
 <TR align="center">
  <TD>Bus</TD>
  <TD><IMG src="docs/img/pin_def_core_bus.svg" ><BR>M-Bus</TD>
  <TD><IMG src="docs/img/pin_def_core2_bus.svg"><BR>M-Bus</TD>
  <TD></TD>
  <TD colspan="2"></TD>
 </TR>
</TABLE>

<TABLE>
 <TR>
  <TH></TH>
  <TH width="11%">M5Stick<BR>C</TH>
  <TH width="11%">M5Stick<BR>C Plus</TH>
  <TH width="18%">M5Stack<BR>CoreInk</TH>
  <TH width="18%">M5Stamp<BR>PICO</TH>
  <TH width="18%">M5Stamp<BR>C3</TH>
  <TH width="18%">M5Stamp<BR>C3U</TH>
 </TR>
 <TR align="center">
  <TD>PortA</TD>
  <TD colspan="4"><IMG src="docs/img/pin_def_stickc_porta.svg" title="G,V,32,33"></TD>
  <TD></TD>
  <TD></TD>
 </TR>
 <TR align="center">
  <TD>HAT</TD>
  <TD><IMG src="docs/img/pin_def_stickc_hat.svg"   title="G,5Vout,26,36,0,BAT,3V,5Vin"></TD>
  <TD><IMG src="docs/img/pin_def_stickcplus_hat.svg" title="G,5Vout,26,25/36,0,BAT,3V,5Vin"></TD>
  <TD><IMG src="docs/img/pin_def_coreink_hat.svg" title="G,5Vout,26,36,25,BAT,3V,5Vin"></TD>
  <TD></TD>
  <TD></TD>
  <TD></TD>
 </TR>
 <TR align="center">
  <TD>Bus</TD>
  <TD></TD>
  <TD></TD>
  <TD><IMG src="docs/img/pin_def_coreink_bus.svg"><BR>MI-Bus</TD>
  <TD><IMG src="docs/img/pin_def_stamppico_bus.svg"></TD>
  <TD><IMG src="docs/img/pin_def_stampc3_bus.svg"></TD>
  <TD><IMG src="docs/img/pin_def_stampc3u_bus.svg"></TD>
 </TR>
</TABLE>

<TABLE>
 <TR>
  <TH></TH>
  <TH width="20%">ATOM<BR>Lite</TH>
  <TH width="20%">ATOM<BR>Matrix</TH>
  <TH width="20%">ATOM<BR>ECHO</TH>
  <TH width="20%">ATOM<BR>PSRAM</TH>
  <TH width="16%">ATOM<BR>U</TH>
 </TR>
 <TR align="center">
  <TD>PortA</TD>
  <TD colspan="5"><IMG src="docs/img/pin_def_atom_porta.svg" title="G,V,26,32"></TD>
 </TR>
 <TR align="center">
  <TD>Bus</TD>
  <TD><IMG src="docs/img/pin_def_atom_lite.svg"></TD>
  <TD><IMG src="docs/img/pin_def_atom_matrix.svg"></TD>
  <TD><IMG src="docs/img/pin_def_atom_echo.svg"></TD>
  <TD><IMG src="docs/img/pin_def_atom_psram.svg"></TD>
  <TD><IMG src="docs/img/pin_def_atom_u.svg"></TD>
 </TR>
</TABLE>



### SPI device

|                |M5Stack<BR>BASIC<BR>GRAY<BR>GO/FIRE|M5Stack<BR>Core2<BR>Tough      |M5Stick<BR>C                 |M5Stick<BR>CPlus               |M5Stack<BR>CoreInk                |M5Paper                       |                |
|:--------------:|:---------------------------------:|:-----------------------------:|:---------------------------:|:-----------------------------:|:--------------------------------:|:----------------------------:|:--------------:|
| Display        |`ILI9342C`<BR>320×240<BR>CS:G14   |`ILI9342C`<BR>320×240<BR>CS:G5|`ST7735S`<BR>80×160<BR>CS:G5|`ST7789V2`<BR>135×240<BR>CS:G5|`GDEW0154M09`<BR>200×200<BR>CS:G9|`IT8951`<BR>960×540<BR>CS:G15| Display        |
| TF Card        |CS:4                               |CS:4                           | ---                         | ---                           | ---                              |CS:4                          | TF Card        |


### I2C device

|                |M5Stack<BR>BASIC/GRAY<BR>GO/FIRE   |M5Stack<BR>Core2      |M5Stack<BR>Tough    |M5Stick<BR>C<BR>CPlus|M5Stack<BR>CoreInk |M5Paper              |ATOM<BR>Matrix  |                |
|:--------------:|:---------------------------------:|:--------------------:|:------------------:|:-------------------:|:-----------------:|:-------------------:|:--------------:|:--------------:|
|Touch<BR>Panel  | ---                               |`FT6336U`<BR>38h      |`CHSC6540`<BR>2Eh   | ---                 | ---               |`GT911`<BR>14h or 5Dh| ---            |Touch<BR>Panel  |
|RTC             | ---                               |`BM8563`<BR>51h       |`BM8563`<BR>51h     |`BM8563`<BR>51h      |`BM8563`<BR>51h    |`BM8563`<BR>51h      | ---            |RTC             |
|Power<BR>Manage |`IP5306`<BR>75h                    |`AXP192`<BR>34h       |`AXP192`<BR>34h     |`AXP192`<BR>34h      | ---               | ---                 | ---            |Power<BR>Manage |
|IMU             |`MPU6886`<BR>68h                   |`MPU6886`<BR>68h (Ext)| ---                |`MPU6886`<BR>68h     | ---               | ---                 |`MPU6886`<BR>68h|IMU             |
|IMU<BR>(old lot)|`SH200Q`<BR>6Ch                    | ---                  | ---                |`SH200Q`<BR>6Ch      | ---               | ---                 | ---            |IMU<BR>(old lot)|
|ENV             | ---                               | ---                  | ---                | ---                 | ---               |`SHT30`<BR>44h       | ---            |ENV             |
|EEPROM          | ---                               | ---                  | ---                | ---                 | ---               |`FM24C02`<BR>50h     | ---            |EEPROM          |


