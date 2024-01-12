/* Copyright (C) 2024  Doctor Volt
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses></https:>.
*/

#include "Arduino.h"
#include "esp32-hal.h"
#include <ArduinoJson.h>
#include <driver/timer.h>

#define SCK_PIN GPIO_NUM_1  /* Clock (pin 4 of ADNS 2610) */
#define SDIO_PIN GPIO_NUM_2 /* Bidirectional serial data (pin 3 of ADNS 2610)*/
#define REG_CONFIG 0        /* Configuration register */
#define REG_CONFIG_WAKEUP 1
#define REG_CONFIG_RESET 128
#define REG_SQUAL 4      /* Surface Quality */
#define REG_DATA 8       /* Pixel data register */
#define REG_DATA_VALID 6 /* Set if pixel data is valid */
#define CMD_SCAN 1
#define CMD_ACK 3
#define LED 21

#define INTENS 32 /*Brightness of LED*/
#define RED 0, INTENS, 0         
#define YELLOW INTENS, INTENS, 0 
#define GREEN INTENS, 0, 0       
#define BLUE 0, 0, INTENS        

//#define TEST /*send a simple test pattern*/

const char *ssid = "******";
const char *password = "******";
hw_timer_t *scn_timer = NULL;
bool connected = false;
int speed = 1000;

void web_init(const char *ssid, const char *password);
void web_write(const char *data);
void web_cleanup();

void reg_write(uint8_t reg, uint8_t val)
{
    uint16_t to_write = reg;
    to_write <<= 8;
    to_write |= 0x8000 + val;
    // Serial.printf("reg: %d, val: %d, to_write: %d\n", reg, val, to_write);
    pinMode(SDIO_PIN, OUTPUT);
    gpio_set_level(SDIO_PIN, HIGH);
    gpio_set_level(SCK_PIN, HIGH);
    // Register number and value to sensor
    for (int b = 0; b < 16; b++)
    {
        gpio_set_level(SCK_PIN, LOW);
        gpio_set_level(SDIO_PIN, (to_write << b) & 0x8000);
        gpio_set_level(SCK_PIN, HIGH);
    }
    gpio_set_level(SDIO_PIN, LOW);
    pinMode(SDIO_PIN, INPUT);
}

uint8_t reg_read(uint8_t reg)
{
    pinMode(SDIO_PIN, OUTPUT);
    gpio_set_level(SDIO_PIN, HIGH);
    gpio_set_level(SCK_PIN, HIGH);
    // Register number to sensor
    for (int b = 0; b < 8; b++)
    {
        gpio_set_level(SCK_PIN, LOW);
        gpio_set_level(SDIO_PIN, (reg << b) & 0x80);
        gpio_set_level(SCK_PIN, HIGH);
        gpio_set_level(SCK_PIN, HIGH); // Extend a bit
    }
    gpio_set_level(SDIO_PIN, LOW);
    pinMode(SDIO_PIN, INPUT);
    delayMicroseconds(80); // Give sensor some time to prepare response
    uint8_t val = 0;
    // Clock in value
    for (int b = 7; b >= 0; b--)
    {
        gpio_set_level(SCK_PIN, LOW);
        gpio_set_level(SCK_PIN, LOW);
        gpio_set_level(SCK_PIN, HIGH);
        val |= gpio_get_level(SDIO_PIN) << b;
        gpio_set_level(SCK_PIN, HIGH);
    }
    return val;
}

void send_image()
{
    // Serial.println("-->send_image()");
    neopixelWrite(LED, BLUE);
    DynamicJsonDocument jdoc(8192);
    jdoc["squal"] = reg_read(REG_SQUAL);
    JsonArray jarray = jdoc.createNestedArray("data");
    reg_write(REG_DATA, 1); //Capture frame 
    delay(10);
    for (int i = 0; i < 324; i++) //Dump frame buffer pixel by pixel
    {
        uint8_t pixval = 0;
        uint32_t time = millis();
#ifdef TEST
        pixval = i;
        delayMicroseconds(100);
#else        
        pixval = reg_read(REG_DATA);
#endif
        jarray.add(pixval & 0x3F);
        delayMicroseconds(speed);
    }
    char buf[1400];
    memset(buf, 0, 1400);
    neopixelWrite(LED, GREEN);
    serializeJson(jdoc, buf);
    serializeJson(jdoc, Serial);
    web_write(buf);
    neopixelWrite(LED, YELLOW);
}

TaskHandle_t Task1;
void handleWebMessage(const char *data, size_t len)
{
    // Serial.println(data);
    StaticJsonDocument<200> jdoc;
    deserializeJson(jdoc, data);
    switch ((int)jdoc["cmd"])
    {
    case CMD_ACK:
        send_image();
        break;
    }
    speed = 1500 - 100 * (int)jdoc["speed"];
    Serial.println(speed);
}

void on_connect()
{
    neopixelWrite(LED, GREEN);
    reg_write(REG_CONFIG, REG_CONFIG_RESET);
    delay(100);
    reg_write(REG_CONFIG, REG_CONFIG_WAKEUP);
    send_image();
    connected = true;
}
void on_disconnect()
{
    neopixelWrite(LED, RED);
    connected = false;
}

void setup()
{
    pinMode(SCK_PIN, OUTPUT);
    digitalWrite(LED, HIGH);
    neopixelWrite(LED, RED);
    Serial.begin(115200);
    delay(1000);
    web_init(ssid, password);
}

void loop()
{
    web_cleanup();
}