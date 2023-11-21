# M5Unified
### M5Stack Series unified library .


#### How To Use
[Please see examples/Basic/HowToUse](examples/Basic/HowToUse/HowToUse.ino)


## Supported framework
 - ESP-IDF
 - Arduino for ESP32

## Supported device (ESP32)
 - M5Stack BASIC / GRAY / GO / FIRE
 - M5Stack Core2 /Core2 v1.1 / Tough
 - M5Stick C / CPlus / CPlus2
 - M5Stack CoreInk
 - M5Station
 - M5Paper
 - M5ATOM Lite / Matrix / ECHO / PSRAM / U
 - M5STAMP PICO

## Supported device (ESP32S3)
 - M5Stack CoreS3
 - M5ATOMS3 / S3Lite / S3U
 - M5STAMPS3
 - M5Dial
 - M5DinMeter
 - M5Capsule
 - M5Cardputer

## Supported device (ESP32C3)
 - M5STAMPC3 / C3U


## Supported device (external display)
 - Unit LCD
 - Unit OLED
 - Unit Mini OLED
 - Unit RCA
 - Unit GLASS
 - Unit GLASS2
 - ATOM Display (with M5ATOM Lite / Matrix / PSRAM / S3 / S3Lite)
 - Module Display (with M5Stack / Core2 / Tough)
 - Module RCA (with M5Stack / Core2 / Tough)

## Supported device (external speaker)
 - SPK HAT (with M5StickC / CPlus / M5Stack CoreInk)
 - SPK HAT2 (with M5StickCPlus)
 - ATOMIC SPK (with M5ATOM Lite / PSRAM / ATOMS3 / S3Lite)
 - Module Display (with M5Stack / Core2 / Tough)
 - Module RCA (with M5Stack / Core2 / Tough)

## Supported device (external unit)
 - Unit RTC
 - Unit IMU


# H/W infomation

### ESP32 GPIO list
|                    |M5Stack<BR>BASIC<BR>GRAY           |M5Stack<BR>GO/FIRE                 |M5Stack<BR>Core2(AWS)<BR>Tough         |M5Stick<BR>C/CPlus            |M5Stick<BR>CPlus2          |M5Stack<BR>CoreInk      |M5Paper                |M5Station              |M5ATOM<BR>Lite/Matrix<BR>ECHO/U<BR>PSRAM |M5STAMP<BR>PICO     |                    |
|:------------------:|:---------------------------------:|:---------------------------------:|:-------------------------------------:|:----------------------------:|:-------------------------:|:----------------------:|:---------------------:|:---------------------:|:---------------------------------------:|:------------------:|:------------------:|
|GPIO 0<BR>`ADC2_CH1`|`M-Bus`<BR>IIS_MK                  |`M-Bus`<BR>IIS_MK                  |`M-Bus`<BR>**SPK_LRCK<BR>PDM_C**(Core2)|`HAT`<BR>`PAD`<BR>**PDM_C**   |`HAT`<BR>**PDM_C**         |**EPD_RST**             | ---                   | ---                   | ---                                     |                    |GPIO 0<BR>`ADC2_CH1`|
|GPIO 1<BR>`USB_TX`  |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**                  |**Serial**                    |**Serial**                 |**Serial**              |**Serial**             |**Serial**             |**Serial**                               |**Serial**          |GPIO 1<BR>`USB_TX`  |
|GPIO 2<BR>`ADC2_CH2`|`M-Bus`<BR>                        |`M-Bus`<BR>                        |`M-Bus`<BR>**SPK_D**                   |`PAD`<BR>**Beep**(CPlus)      |**Beep**                   |**Beep**                |**PW_Hold**            |**ReadEn**             | ---                                     | ---                |GPIO 2<BR>`ADC2_CH2`|
|GPIO 3<BR>`USB_RX`  |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**              |`M-Bus`<BR>**Serial**                  |**Serial**                    |**Serial**                 |**Serial**              |**Serial**             |**Serial**             |**Serial**                               |**Serial**          |GPIO 3<BR>`USB_RX`  |
|GPIO 4<BR>`ADC2_CH0`|**TF_CS**                          |**TF_CS**                          |**TF_CS**                              | ---                          |**PW_Hold**                |**EPD_BUSY**            |**TF_CS**              |**RGB LED**            | ---                                     | ---                |GPIO 4<BR>`ADC2_CH0`|
|GPIO 5              |`M-Bus`                            |`M-Bus`                            |**LCD_CS**                             |**LCD_CS**                    |**LCD_CS**                 |**BTN_HAT**             |**EXT_5V**             |**LCD_CS**             |`Bus`(P)<BR>**PDM_C**(U)                 | ---                |GPIO 5              |
|GPIO 9              | ---                               | ---                               | ---                                   |**InfraRed**                  | ---                       |**EPD_CS**              | ---                   | ---                   | ---                                     | ---                |GPIO 9              |
|GPIO10              | ---                               | ---                               | ---                                   |**LED**                       | ---                       |**LED**                 | ---                   | ---                   | ---                                     | ---                |GPIO10              |
|GPIO12<BR>`ADC2_CH5`|`M-Bus`<BR>IIS_SK                  |`M-Bus`<BR>IIS_SK                  |**SPK_BCLK**                           | ---                          |**LCD_RST**                |**PW_Hold**             |**SPI_MOSI**           |**USB_PW**             |**InfraRed**                             | ---                |GPIO12<BR>`ADC2_CH5`|
|GPIO13<BR>`ADC2_CH4`|`M-Bus`<BR>IIS_WS                  |`M-Bus`<BR>IIS_WS                  |`M-Bus`<BR>RXD2                        |**SPI_SCLK**                  |**SPI_SCLK**               |`MI-Bus`<BR>RXD2        |**SPI_MISO**           |`PORT.C1`              | ---                                     | ---                |GPIO13<BR>`ADC2_CH4`|
|GPIO14<BR>`ADC2_CH6`|**LCD_CS**                         |**LCD_CS**                         |`M-Bus`<BR>TXD2                        | ---                          |**LCD_D/C**                |`MI-Bus`<BR>TXD2        |**SPI_SCLK**           |`PORT.C1`              | ---                                     | ---                |GPIO14<BR>`ADC2_CH6`|
|GPIO15<BR>`ADC2_CH3`|`M-Bus`<BR>IIS_OUT                 |`M-Bus`<BR>**RGB LED**             |**LCD_D/C**                            |**SPI_MOSI**                  |**SPI_MOSI**               |**EPD_D/C**             |**EPD_CS**             |**LCD_RST**            | ---                                     | ---                |GPIO15<BR>`ADC2_CH3`|
|GPIO16<BR>`PSRAM`   |`M-Bus`<BR>RXD2                    |`M-Bus`<BR>`PORT.C`<BR>RXD2        | ---                                   | ---                          |---                        | ---                    | ---                   |`PORT.C2`<BR>RXD2      | ---                                     | ---                |GPIO16<BR>`PSRAM`   |
|GPIO17<BR>`PSRAM`   |`M-Bus`<BR>TXD2                    |`M-Bus`<BR>`PORT.C`<BR>TXD2        | ---                                   | ---                          |---                        | ---                    | ---                   |`PORT.C2`<BR>TXD2      | ---                                     | ---                |GPIO17<BR>`PSRAM`   |
|GPIO18              |`M-Bus`<BR>**SPI_SCLK**            |`M-Bus`<BR>**SPI_SCLK**            |**SPI_SCLK**                           |**LCD_RST**                   |---                        |`MI-Bus`<BR>**SPI_SCLK**|`PORT.C`               |**SPI_SCLK**           | ---                                     |                    |GPIO18              |
|GPIO19              |`M-Bus`<BR>**SPI_MISO**            |`M-Bus`<BR>**SPI_MISO**            |`M-Bus`                                | ---                          |**LED**<BR>**InfraRed**    |**RTC_INT**             |`PORT.C`               |LCD_D/C                |`Bus`<BR>**SPK_C**(ECHO)<BR>***PDM_D**(U)|                    |GPIO19              |
|GPIO21              |`M-Bus`<BR>`PORT.A`<BR>**I2C0_SDA**|`M-Bus`<BR>`PORT.A`<BR>**I2C0_SDA**|**I2C1_SDA**                           |**I2C1_SDA**                  |**I2C1_SDA**               |`MI-Bus`<BR>**I2C1_SDA**|**I2C1_SDA**           |**I2C1_SDA**           |`Bus`<BR>**I2C1_SCL**                    |                    |GPIO21              |
|GPIO22              |`M-Bus`<BR>`PORT.A`<BR>**I2C0_SCL**|`M-Bus`<BR>`PORT.A`<BR>**I2C0_SCL**|**I2C1_SCL**                           |**I2C1_SCL**                  |**I2C1_SCL**               |`MI-Bus`<BR>**I2C1_SCL**|**I2C1_SCL**           |**I2C1_SCL**           |`Bus`<BR>**SPK_D**(ECHO)                 |                    |GPIO22              |
|GPIO23              |`M-Bus`<BR>**SPI_MOSI**            |`M-Bus`<BR>**SPI_MOSI**            |**SPI_MOSI**                           |**LCD_D/C**                   | ---                       |`MI-Bus`<BR>**SPI_MOSI**|**EPD_RST**            |**SPI_MOSI**           |`Bus`<BR>**PDM_D**(ECHO)                 | ---                |GPIO23              |
|GPIO25<BR>`DAC1`    |`M-Bus`<BR>**SPK_DAC**             |`M-Bus`<BR>**SPK_DAC**             |`M-Bus`<BR>**RGB LED**(AWS)            |`HAT`(CPlus)<BR>`PAD`         |`HAT`                      |`MI-Bus`<BR>`HAT`       |`PORT.A`<BR>I2C0_SDA   |`PORT.B1`              |`Bus`<BR>**I2C1_SDA**                    |                    |GPIO25<BR>`DAC1`    |
|GPIO26<BR>`DAC2`    |`M-Bus`                            |`M-Bus`<BR>`PORT.B`                |`M-Bus`                                |`HAT`<BR>`PAD`                |`HAT`                      |`MI-Bus`<BR>`HAT`       |`PORT.B`               |`PORT.B2`              |`PORT.A`<BR>**I2C0_SDA**                 |                    |GPIO26<BR>`DAC2`    |
|GPIO27<BR>`ADC2_CH7`|**LCD_D/C**                        |**LCD_D/C**                        |`M-Bus`                                |**AXP192 VBUSEN**             |**LCD_BL**                 |**BTN_PWR**             |**EPD_BUSY**           |**IMU_INT**            |**RGB LED**                              |**RGB LED**         |GPIO27<BR>`ADC2_CH7`|
|GPIO32<BR>`ADC1_CH4`|**LCD_BL**                         |**LCD_BL**                         |`M-Bus`<BR>`PORT.A`<BR>I2C0_SDA        |`PORT.A`<BR>I2C0_SDA          |`PORT.A`<BR>I2C0_SDA       |`PORT.A`<BR>I2C0_SDA    |`PORT.A`<BR>I2C0_SCL   |`PORT.A`<BR>I2C0_SDA   |`PORT.A`<BR>**I2C0_SCL**                 |`PORT.A`<BR>I2C0_SDA|GPIO32<BR>`ADC1_CH4`|
|GPIO33<BR>`ADC1_CH5`|**LCD_RST**                        |**LCD_RST**                        |`M-Bus`<BR>`PORT.A`<BR>I2C0_SCL        |`PORT.A`<BR>I2C0_SCL          |`PORT.A`<BR>I2C0_SCL       |`PORT.A`<BR>I2C0_SCL    |`PORT.B`               |`PORT.A`<BR>I2C0_SCL   |`Bus`<BR>**PDM_C**(ECHO)                 |`PORT.A`<BR>I2C0_SCL|GPIO33<BR>`ADC1_CH5`|
|GPIO34<BR>`ADC1_CH6`|`M-Bus`<BR>IIS_IN                  |`M-Bus`<BR>**MIC_ADC**<BR>IIS_IN   |`M-Bus`<BR>**PDM_D**(Core2)            |**PDM_D**                     |**PDM_D**                  |`MI-Bus`<BR>**SPI_MISO**| ---                   | USB Current?          |                                         | ---                |GPIO34<BR>`ADC1_CH6`|
|GPIO35<BR>`ADC1_CH7`|`M-Bus`                            |`M-Bus`                            |`M-Bus`                                |**RTC_INT**                   |**BTN_PWR**                |**BAT_V**               |**BAT_V**              |`PORT.B1`              | ---                                     | ---                |GPIO35<BR>`ADC1_CH7`|
|GPIO36<BR>`ADC1_CH0`|`M-Bus`                            |`M-Bus`<BR>`PORT.B`                |`M-Bus`                                |`HAT`<BR>`PAD`                |`HAT`                      |`MI-Bus`<BR>`HAT`       |**TP_INT**             |`PORT.B2`              | ---                                     | ---                |GPIO36<BR>`ADC1_CH0`|
|GPIO37<BR>`ADC1_CH1`|**BTN_C**                          |**BTN_C**                          | ---                                   |**BTN_A**                     |**BTN_A**                  |**SW_Up**               |**SW_Up**              |**BTN_A**              | ---                                     | ---                |GPIO37<BR>`ADC1_CH1`|
|GPIO38<BR>`ADC1_CH2`|**BTN_B**                          |**BTN_B**                          |`M-Bus`<BR>**SPI_MISO**                |`PAD`                         |**BAT_V**                  |**SW_Press**            |**SW_Press**           |**BTN_B**              | ---                                     | ---                |GPIO38<BR>`ADC1_CH2`|
|GPIO39<BR>`ADC1_CH3`|**BTN_A**                          |**BTN_A**                          |**TP_INT**                             |**BTN_B**                     |**BTN_B**                  |**SW_Down**             |**SW_Down**            |**BTN_C**              |**BTN**                                  |**BTN**             |GPIO39<BR>`ADC1_CH3`|
|                    |M5Stack<BR>BASIC<BR>GRAY           |M5Stack<BR>GO/FIRE                 |M5Stack<BR>Core2(AWS)<BR>Tough         |M5Stick<BR>C/CPlus            |M5Stick<BR>CPlus2          |M5Stack<BR>CoreInk      |M5Paper                |M5Station              |M5ATOM<BR>Lite/Matrix<BR>ECHO/U<BR>PSRAM |M5STAMP<BR>PICO     |                    |


### ESP32C3 GPIO list
|               |M5Stamp<BR>C3            |M5Stamp<BR>C3U                  |               |
|:-------------:|:-----------------------:|:------------------------------:|:-------------:|
|GPIO 0         |`PORT.A`<BR>**I2C0_SCL** |`PORT.A`<BR>**I2C_SCL**         |GPIO 0         |
|GPIO 1         |`PORT.A`<BR>**I2C0_SDA** |`PORT.A`<BR>**I2C_SDA**         |GPIO 1         |
|GPIO 2         |**RGB LED**              |**RGB LED**                     |GPIO 2         |
|GPIO 3         |**BTN_A**                |`Bus`                           |GPIO 3         |
|GPIO 4         |`Bus`                    |`Bus`                           |GPIO 4         |
|GPIO 5         |`Bus`                    |`Bus`                           |GPIO 5         |
|GPIO 6         |`Bus`                    |`Bus`                           |GPIO 6         |
|GPIO 7         |`Bus`                    |`Bus`                           |GPIO 7         |
|GPIO 8         |`Bus`                    |`Bus`                           |GPIO 8         |
|GPIO 9         | ---                     |**BTN_A**                       |GPIO 9         |
|GPIO10         |`Bus`                    |`Bus`                           |GPIO10         |
|GPIO18<BR>`USB`|`PORT.U`<BR>**D-**       |`USB`<BR>`PORT.U`<BR>**D-**     |GPIO18<BR>`USB`|
|GPIO19<BR>`USB`|`PORT.U`<BR>**D+**       |`USB`<BR>`PORT.U`<BR>**D+**     |GPIO19<BR>`USB`|
|GPIO20         |`USB`<BR>**Serial**      |`Bus`<BR>                       |GPIO20         |
|GPIO21         |`USB`<BR>**Serial**      |`Bus`<BR>                       |GPIO21         |
|               |M5Stamp<BR>C3            |M5Stamp<BR>C3U                  |               |


### ESP32S3 GPIO list
|               |M5Stack<BR>CoreS3                |M5ATOMS3 <BR>/ S3Lite    |M5ATOMS3U                | M5STAMPS3                         | M5Dial                   | M5Capsule                | M5Cardputer              |               |
|:-------------:|:-------------------------------:|:-----------------------:|:-----------------------:|:---------------------------------:|:------------------------:|:------------------------:|:------------------------:|:-------------:|
|GPIO 0         |`M-Bus`<BR>**SPK_LRCK**          | ---                     | ---                     | `Bus`<BR>**BTN_A**                | ---                      | ---                      | **BTN_A**                |GPIO 0         |
|GPIO 1         |`PORT.A`<BR>**I2C0_SCL**         |`PORT.A`<BR>**I2C0_SCL** |`PORT.A`<BR>**I2C0_SCL** | `Bus`                             | `PORT.B`                 | ---                      | `PORT.A`<BR>**I2C0_SCL** |GPIO 1         |
|GPIO 2         |`PORT.A`<BR>**I2C0_SDA**         |`PORT.A`<BR>**I2C0_SDA** |`PORT.A`<BR>**I2C0_SDA** | `Bus`                             | `PORT.B`                 | **Beep**                 | `PORT.A`<BR>**I2C0_SDA** |GPIO 2         |
|GPIO 3         |**LCD_CS**                       |vdd3v3                   |vdd3v3                   | `Bus`                             | **Beep**                 | ---                      | **KEY_MATRIX**           |GPIO 3         |
|GPIO 4         |**TF_CS**                        |**InfraRed**             | ---                     | `Bus`                             | **LCD_RS**               | **InfraRed**             | **KEY_MATRIX**           |GPIO 4         |
|GPIO 5         |`M-Bus`                          |`Bus`                    | ---                     | `Bus`                             | **LCD_MOSI**             | ---                      | **KEY_MATRIX**           |GPIO 5         |
|GPIO 6         |`M-Bus`                          |`Bus`                    | ---                     | `Bus`                             | **LCD_SCK**              | **BAT_ADC**              | **KEY_MATRIX**           |GPIO 6         |
|GPIO 7         |`M-Bus`                          |`Bus`                    | ---                     | `Bus`                             | **LCD_CS**               | ---                      | **KEY_MATRIX**           |GPIO 7         |
|GPIO 8         |`M-Bus`<BR>`PORT.B`              |`Bus`                    | ---                     | `Bus`                             | **LCD_RST**              | **I2C1_SDA**             | **KEY_MATRIX**           |GPIO 8         |
|GPIO 9         |`M-Bus`<BR>`PORT.B`              | ---                     | ---                     | `Bus`                             | **LCD_BL**               | ---                      | **KEY_MATRIX**           |GPIO 9         |
|GPIO10         |`M-Bus`                          | ---                     | ---                     | `Bus`                             | **RFID_INT**             | **I2C1_SCL**             | **BAT_ADC**              |GPIO10         |
|GPIO11         |**I2C1_SCL**                     | ---                     | ---                     | `Bus`                             | **I2C1_SDA**             | **TF_CS**                | **KEY_MATRIX**           |GPIO11         |
|GPIO12         |**I2C1_SDA**                     | ---                     |**InfraRed**             | `Bus`                             | **I2C1_SCL**             | **TF_MOSI**              | **TF_CS**                |GPIO12         |
|GPIO13         |`M-Bus`<BR>**SPK_D**             | ---                     | ---                     | `Bus`<BR>`PORT.A`<BR>**I2C0_SDA** | `PORT.A`<BR>**I2C0_SDA** | `PORT.A`<BR>**I2C0_SDA** | **KEY_MATRIX**           |GPIO13         |
|GPIO14         |`M-Bus`<BR>**MIC_IN**            | ---                     |`Bus`                    | `Bus`                             | **TP_INT**               | **TF_CLK**               | **TF_MOSI**              |GPIO14         |
|GPIO15         |**CAM_D6**                       |**LCD_CS**               | ---                     | `Bus`<BR>`PORT.A`<BR>**I2C0_SCL** | `PORT.A`<BR>**I2C0_SCL** | `PORT.A`<BR>**I2C0_SCL** | **KEY_MATRIX**           |GPIO15         |
|GPIO16         |**CAM_D7**                       |**LCD_BL**               | ---                     | `FPC`                             | ---                      | ---                      | ---                      |GPIO16         |
|GPIO17         |`M-Bus`<BR>`PORT.C`              |**LCD_SCLK**             |`Bus`                    | `FPC`                             | ---                      | ---                      | ---                      |GPIO17         |
|GPIO18         |`M-Bus`<BR>`PORT.C`              | ---                     | ---                     | `FPC`                             | ---                      | ---                      | ---                      |GPIO18         |
|GPIO19<BR>`USB`|`USB`<BR>**D--**                 |`USB`<BR>**D--**         |`USB`<BR>**D--**         | `USB`<BR>**D--**                  | `USB`<BR>**D--**         | `USB`<BR>**D--**         | `USB`<BR>**D--**         |GPIO19<BR>`USB`|
|GPIO20<BR>`USB`|`USB`<BR>**D++**                 |`USB`<BR>**D++**         |`USB`<BR>**D++**         | `USB`<BR>**D++**                  | `USB`<BR>**D++**         | `USB`<BR>**D++**         | `USB`<BR>**D++**         |GPIO20<BR>`USB`|
|GPIO21         |**I2C_INT**                      |**LCD_MOSI**             | ---                     | **RGB LED**                       | **RGB LED**              | **RGB LED**              | **RGB_LED**              |GPIO21         |
|GPIO33         |**SPK_WCK**                      |**LCD_DC**               | ---                     | `FPC`                             | ---                      | ---                      | **LCD_RST**              |GPIO33         |
|GPIO34         |**SPK_BCK**                      |**LCD_RST**              | ---                     | `FPC`                             | ---                      | ---                      | **LCD_RS**               |GPIO34         |
|GPIO35         |`M-Bus`<BR>**SPI_MISO<BR>LCD DC**|**RGB LED**              |**RGB LED**              | `FPC`                             | ---                      | ---                      | **LCD_DAT**              |GPIO35         |
|GPIO36         |`M-Bus`<BR>**SPI_SCLK**          | ---                     | ---                     | `FPC`                             | ---                      | ---                      | **LCD_SCK**              |GPIO36         |
|GPIO37         |`M-Bus`<BR>**SPI_MOSI**          | ---                     | ---                     | `FPC`                             | ---                      | ---                      | **LCD_CS**               |GPIO37         |
|GPIO38         |**CAM_HREF**                     |`Bus`<BR>**I2C1_SDA**    |**PDM_DAT**              | `FPC`                             | ---                      | ---                      | **LCD_BL**               |GPIO38         |
|GPIO39         |**CAM_D2**                       |`Bus`<BR>**I2C1_SCL**    |**PDM_CLK**              | `Bus`                             | ---                      | **TF_MISO**              | **TF_MISO**              |GPIO39         |
|GPIO40         |**CAM_D3**                       | ---                     |`Bus`                    | `Bus`                             | **ENCODER_B**            | **MIC_CLK**              | **TF_CLK**               |GPIO40         |
|GPIO41         |**CAM_D4**                       |**BTN_A**                |**BTN_A**                | `Bus`                             | **ENCODER_A**            | **MIC_DAT**              | **SPK_BCLK**             |GPIO41         |
|GPIO42         |**CAM_D5**                       | ---                     |`Bus`                    | `Bus`                             | **BTN_A**                | **BTN_A**                | **SPK_SDATA**            |GPIO42         |
|GPIO43         |`M-Bus`<BR>**SerialTX**          | ---                     | ---                     | `Bus`                             | ---                      | ---                      | **I2S_LRCLK**            |GPIO43         |
|GPIO44         |`M-Bus`<BR>**SerialRX**          | ---                     | ---                     | `Bus`                             | ---                      | ---                      | **InfraRed**             |GPIO44         |
|GPIO45         |**CAM_PCLK**                     | ---                     | ---                     | ---                               | ---                      | ---                      | ---                      |GPIO45         |
|GPIO46         |**CAM_VSYNC**                    | ---                     | ---                     | `Bus`                             | **HOLD**                 | **HOLD**                 | **MIC_DAT**              |GPIO46         |
|GPIO47         |**CAM_D9**                       | ---                     | ---                     | ---                               | ---                      | ---                      | ---                      |GPIO47         |
|GPIO48         |**CAM_D8**                       | ---                     | ---                     | ---                               | ---                      | ---                      | ---                      |GPIO48         |
|               |M5Stack<BR>CoreS3                |M5ATOMS3 <BR>/ S3Lite    |M5ATOMS3U                | M5STAMPS3                         | M5Dial                   | M5Capsule                | M5Cardputer              |               |

### AXP192 usage
|              |M5Stack<BR>Core2   |M5Stack<BR>Tough   |M5Stick<BR>C    |M5Stick<BR>CPlus|  M5Station  |              |
|:------------:|:-----------------:|:-----------------:|:--------------:|:--------------:|:-----------:|:------------:|
|GPIO0<br>LDO0 |BUS PW EN          |BUS PW EN          |MIC VCC         |MIC VCC         |PortA1.A2 EN |GPIO0<br>LDO0 |
| GPIO1        |SYS LED            |TP RST             | ---            | ---            |PortB1 EN    | GPIO1        |
| GPIO2        |SPK EN             |SPK EN             | ---            | ---            |PortB2 EN    | GPIO2        |
| GPIO3        | ---               | ---               | ---            | ---            |PortC1 EN    | GPIO3        |
| GPIO4        |LCD RST<BR>TP RST  |LCD RST            | ---            | ---            |PortC2 EN    | GPIO4        |
| EXTEN        |PORT 5V EN         |PORT 5V EN         |PORT 5V EN      |PORT 5V EN      |PORT 5V EN   | EXTEN        |
| BACKUP       |RTC BAT            |RTC BAT            |RTC BAT         |RTC BAT         | ---         | BACKUP       |
| LDO1         |RTC VDD            |RTC VDD            |RTC VDD         |RTC VDD         |RTC VDD      | LDO1         |
| LDO2         |LCD PW<BR>Periph PW|LCD PW<BR>Periph PW|LCD BL          |LCD BL          | ---         | LDO2         |
| LDO3         |VIB MOTOR          |LCD BL             |LCD PW          |LCD PW          |LCD BL       | LDO3         |
| DCDC1        |ESP32 VDD          |ESP32 VDD          |ESP32 VDD       |ESP32 VDD       |ESP32 VDD    | DCDC1        |
| DCDC2        | ---               | ---               | ---            | ---            | ---         | DCDC2        |
| DCDC3        |LCD BL             | ---               | ---            | ---            | ---         | DCDC3        |

### AXP2101 usage
|           |M5Stack<BR>Core2v1.1  |M5Stack<BR>CoreS3  |           |
|:---------:|:--------------------:|:-----------------:|:---------:|
| ALDO1     | ---                  |VDD 1v8            | ALDO1     |
| ALDO2     |LCD RST               |VDDA 3v3           | ALDO2     |
| ALDO3     |SPK EN                |CAM 3v3            | ALDO3     |
| ALDO4     |Periph PW<BR>TF,TP,LCD|TF 3v3             | ALDO4     |
| BLDO1     |LCD BL                |AVDD               | BLDO1     |
| BLDO2     |PORT 5V EN            |DVDD               | BLDO2     |
| DLDO1/DC1 |VIB MOTOR             |LCD BL             | DLDO1/DC1 |
| DLDO2/DC2 | ---                  | ---               | DLDO2/DC2 |
| BACKUP    |RTC BAT               |RTC BAT            | BACKUP    |


### PinMap

<TABLE>
 <TR>
  <TH></TH>
  <TH width="33%">M5Stack<BR>BASIC/GRAY<BR>GO/FIRE<BR>FACES II</TH>
  <TH width="33%">M5Stack<BR>Core2<BR>Core2AWS<BR>TOUGH</TH>
  <TH width="33%">M5Stack<BR>CoreS3</TH>
 </TR>
 <TR align="center">
  <TD rowspan="2">Bus</TD>
  <TD><IMG src="docs/img/pin_def_core_bus.svg" ><BR>M-Bus</TD>
  <TD><IMG src="docs/img/pin_def_core2_bus.svg"><BR>M-Bus</TD>
  <TD><IMG src="docs/img/pin_def_cores3_bus.svg"><BR>M-Bus</TD>
 </TR>
 <TR align="center">
  <TD colspan="3">
  ※ HPWR=not connected to the ESP32.
   Used by modules capable of supplying 12V power.
  </TD>
 </TR>
</TABLE>

<TABLE>
 <TR>
  <TH></TH>
  <TH width="16%">M5Stack<BR>BASIC/GRAY<BR>GO/FIRE<BR>FACES II</TH>
  <TH width="16%">M5Stack<BR>Core2<BR>Core2AWS<BR>TOUGH</TH>
  <TH width="16%">M5Stack<BR>CoreS3</TH>
  <TH width="16%"> M5Paper </TH>
  <TH width="32%" colspan="2"> M5Station </TH>
 </TR>
 <TR align="center">
  <TD>PortA</TD>
  <TD><IMG src="docs/img/pin_def_core_porta.svg"   title="G,V,21,22"><BR>PortA</TD>
  <TD><IMG src="docs/img/pin_def_core2_porta.svg"  title="G,V,32,33"><BR>PortA</TD>
  <TD><IMG src="docs/img/pin_def_cores3_porta.svg" title="G,V,2,1"><BR>PortA</TD>
  <TD><IMG src="docs/img/pin_def_paper_porta.svg"   title="G,V,25,32"><BR>PortA</TD>
  <TD colspan="2"><IMG src="docs/img/pin_def_station_porta.svg" title="G,V,32,33"><BR>PortA</TD>
 </TR>
 <TR align="center">
  <TD>PortB</TD>
  <TD><IMG src="docs/img/pin_def_core_portb.svg"   title="G,V,26,36"><BR>PortB</TD>
  <TD><IMG src="docs/img/pin_def_core2_portb.svg"  title="G,V,26,36"><BR>PortB</TD>
  <TD><IMG src="docs/img/pin_def_cores3_portb.svg" title="G,V,9,8"><BR>PortB</TD>
  <TD><IMG src="docs/img/pin_def_paper_portb.svg"    title="G,V,26,33"><BR>PortB</TD>
  <TD><IMG src="docs/img/pin_def_station_portb1.svg" title="G,V,25,35"><BR>PortB1</TD>
  <TD><IMG src="docs/img/pin_def_station_portb2.svg" title="G,V,26,36"><BR>PortB2</TD>
 </TR>
 <TR align="center">
  <TD>PortC</TD>
  <TD><IMG src="docs/img/pin_def_core_portc.svg"   title="G,V,17,16"><BR>PortC</TD>
  <TD><IMG src="docs/img/pin_def_core2_portc.svg"  title="G,V,14,13"><BR>PortC</TD>
  <TD><IMG src="docs/img/pin_def_cores3_portc.svg" title="G,V,17,18"><BR>PortC</TD>
  <TD><IMG src="docs/img/pin_def_paper_portc.svg"    title="G,V,18,19"><BR>PortC</TD>
  <TD><IMG src="docs/img/pin_def_station_portc1.svg" title="G,V,14,13"><BR>PortC1</TD>
  <TD><IMG src="docs/img/pin_def_station_portc2.svg" title="G,V,17,16"><BR>PortC2</TD>
 </TR>
 <TR align="center">
  <TD>PortD</TD>
  <TD><IMG src="docs/img/pin_def_core_portd.svg"   title="G,V,35,34"><BR>PortD</TD>
  <TD><IMG src="docs/img/pin_def_core2_portd.svg"  title="G,V,35,34"><BR>PortD</TD>
  <TD></TD>
  <TD></TD>
  <TD colspan="2"></TD>
 </TR>
 <TR align="center">
  <TD>PortE</TD>
  <TD><IMG src="docs/img/pin_def_core_porte.svg"  title="G,V,13,5" ><BR>PortE</TD>
  <TD><IMG src="docs/img/pin_def_core2_porte.svg" title="G,V,19,27"><BR>PortE / 485<BR>TOUGH485:12V</TD>
  <TD></TD>
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

<TABLE>
 <TR>
  <TH></TH>
  <TH>ATOMS3<BR>/S3Lite</TH>
 </TR>
 <TR align="center">
  <TD>PortA</TD>
  <TD><IMG src="docs/img/pin_def_atom_s3_porta.svg" title="G,V,2,1"></TD>
 </TR>
 <TR align="center">
  <TD>Bus</TD>
  <TD><IMG src="docs/img/pin_def_atom_s3.svg"></TD>
 </TR>
</TABLE>



### SPI device

|                |M5Stack<BR>BASIC<BR>GRAY<BR>GO/FIRE|M5Stack<BR>Core2<BR>Tough      |M5Stick<BR>C                 |M5Stick<BR>CPlus               |M5Stack<BR>CoreInk                |M5Paper                       |                |
|:--------------:|:---------------------------------:|:-----------------------------:|:---------------------------:|:-----------------------------:|:--------------------------------:|:----------------------------:|:--------------:|
| Display        |`ILI9342C`<BR>320×240<BR>CS:G14   |`ILI9342C`<BR>320×240<BR>CS:G5|`ST7735S`<BR>80×160<BR>CS:G5|`ST7789V2`<BR>135×240<BR>CS:G5|`GDEW0154M09`<BR>200×200<BR>CS:G9|`IT8951`<BR>960×540<BR>CS:G15| Display        |
| TF Card        |CS:4                               |CS:4                           | ---                         | ---                           | ---                              |CS:4                          | TF Card        |


### I2C device

|                             |M5Stack<BR>BASIC/GRAY<BR>GO/FIRE   |M5Stack<BR>Core2      |M5Stack<BR>Tough    |M5Stack<BR>CoreS3  |M5Stick<BR>C<BR>CPlus|M5Stack<BR>CoreInk |M5Paper              |ATOM<BR>Matrix  |M5Station                    |                             |
|:---------------------------:|:---------------------------------:|:--------------------:|:------------------:|:-----------------:|:-------------------:|:-----------------:|:-------------------:|:--------------:|:---------------------------:|:---------------------------:|
|Touch<BR>Panel               | ---                               |`FT6336U`<BR>38h      |`CHSC6540`<BR>2Eh   |`FT5xxx`<BR>38h    | ---                 | ---               |`GT911`<BR>14h or 5Dh| ---            | ---                         |Touch<BR>Panel               |
|RTC                          | ---                               |`BM8563`<BR>51h       |`BM8563`<BR>51h     |`BM8563`<BR>51h    |`BM8563`<BR>51h      |`BM8563`<BR>51h    |`BM8563`<BR>51h      | ---            |`BM8563`<BR>51h              |RTC                          |
|Power<BR>Manage              |`IP5306`<BR>75h                    |`AXP192`<BR>34h       |`AXP192`<BR>34h     |`AXP2101`<BR>34h   |`AXP192`<BR>34h      | ---               | ---                 | ---            |`AXP192`<BR>34h              |Power<BR>Manage              |
|IMU                          |`MPU6886`<BR>68h                   |`MPU6886`<BR>68h (Ext)| ---                |`BMI270`<BR>69h    |`MPU6886`<BR>68h     | ---               | ---                 |`MPU6886`<BR>68h|`MPU6886`<BR>68h (opt)       |IMU                          |
|IMU<BR>(old lot)             |`SH200Q`<BR>6Ch                    | ---                  | ---                | ---               |`SH200Q`<BR>6Ch      | ---               | ---                 | ---            | ---                         |IMU<BR>(old lot)             |
|ENV                          | ---                               | ---                  | ---                |`LTR553ALS`<BR>23h | ---                 | ---               |`SHT30`<BR>44h       | ---            | ---                         |ENV                          |
|EEPROM                       | ---                               | ---                  | ---                | ---               | ---                 | ---               |`FM24C02`<BR>50h     | ---            | ---                         |EEPROM                       |
|Camera                       | ---                               | ---                  | ---                |`GC0308`<BR>21h    | ---                 | ---               | ---                 | ---            | ---                         |Camera                       |
|Speaker                      | ---                               | ---                  | ---                |`AW88298`<BR>36h   | ---                 | ---               | ---                 | ---            | ---                         |Speaker                      |
|Microphone                   | ---                               | ---                  | ---                |`ES7210`<BR>40h    | ---                 | ---               | ---                 | ---            | ---                         |Microphone                   |
|GPIO Expander                | ---                               | ---                  | ---                |`AW9523B`<BR>58h   | ---                 | ---               | ---                 | ---            | ---                         |GPIO Expander                |
|Current<BR>Voltage<BR>Monitor| ---                               | ---                  | ---                | ---               | ---                 | ---               | ---                 | ---            |`INA3221`<BR>40h/41h<BR>(opt)|Current<BR>Voltage<BR>Monitor|


