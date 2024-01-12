# ESP32 Code for the "Mouse To Camera Hack"
Displays frames captured by an optical mouse sensor on a simple web user interface. Works with all optical mice with ADNS 2610 sensor. 
## Parts Needed
- Waveshare ESP32 S3 Mini module
- Raspberry Pi wide angle camera lens f=1.7mm or similar
- Lens adapter, see [example](cad/)
- Microsoft Code with PlatformIO extension
## Usage
- Remove controller chip or extract Pin 3 and 4 of the sensor.
- Connect pin 3 of sensor to pin 2 of the ESP32 module (SDIO)
- Connect pin 4 of sensor to pin 1 of the ESP32 module (Clock)
- Enter your SSID and passkey into [main.cpp](code/src/main.cpp)  
`const char *ssid = "your ssid";`  
`const char *password = "your password";`
- Compile and flash the sketch
- Open the local file [code/data/index.htm](code/data/index.htm) in your browser.

