#include <TFT_eSPI.h>
#include <TFT_Touch.h>
#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2CSen5x.h>
#include <SensirionI2CScd4x.h>
#include "Adafruit_SHT31.h"

#include <SoftwareSerial.h>
SoftwareSerial pmsSerial(16, 17);

// The used commands use up to 48 bytes. On some Arduino's the default buffer
// space is not large enough
#define MAXBUF_REQUIREMENT 48

#if (defined(I2C_BUFFER_LENGTH) &&                 \
     (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
    (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif

bool enableHeater = false;
uint8_t loopCnt = 0;

float t ;
float h ;

uint16_t error;
char errorMessage[256];

uint16_t error1;
char errorMessage1[256];

// Read Measurement
float massConcentrationPm1p0;
float massConcentrationPm2p5;
float massConcentrationPm4p0;
float massConcentrationPm10p0;
float ambientHumidity;
float ambientTemperature;
float vocIndex;
float noxIndex;

// Read Measurement
uint16_t co2 = 0;
float temperature = 0.0f;
float humidity = 0.0f;
bool isDataReady = false;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
SensirionI2CSen5x sen5x;
SensirionI2CScd4x scd4x;

TFT_eSPI tft = TFT_eSPI();  // Invoke library

// These are the pins used to interface between the 2046 touch controller and Arduino Pro
#define DOUT 39  /* Data out pin (T_DO) of touch screen */
#define DIN  32  /* Data in pin (T_DIN) of touch screen */
#define DCS  33  /* Chip select pin (T_CS) of touch screen */
#define DCLK 25  /* Clock pin (T_CLK) of touch screen */

/* Create an instance of the touch screen library */
TFT_Touch touch = TFT_Touch(DCS, DCLK, DIN, DOUT);

TaskHandle_t Task1;
void TaskAnalogRead( void *pvParameters );

void printModuleVersions() {
  uint16_t error;
  char errorMessage[256];

  unsigned char productName[32];
  uint8_t productNameSize = 32;

  error = sen5x.getProductName(productName, productNameSize);

  if (error) {
    Serial.print("Error trying to execute getProductName(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("ProductName:");
    Serial.println((char*)productName);
  }

  uint8_t firmwareMajor;
  uint8_t firmwareMinor;
  bool firmwareDebug;
  uint8_t hardwareMajor;
  uint8_t hardwareMinor;
  uint8_t protocolMajor;
  uint8_t protocolMinor;

  error = sen5x.getVersion(firmwareMajor, firmwareMinor, firmwareDebug,
                           hardwareMajor, hardwareMinor, protocolMajor,
                           protocolMinor);
  if (error) {
    Serial.print("Error trying to execute getVersion(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("Firmware: ");
    Serial.print(firmwareMajor);
    Serial.print(".");
    Serial.print(firmwareMinor);
    Serial.print(", ");

    Serial.print("Hardware: ");
    Serial.print(hardwareMajor);
    Serial.print(".");
    Serial.println(hardwareMinor);
  }
}

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber1(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("Serial: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}

void printSerialNumber() {
  uint16_t error;
  char errorMessage[256];
  unsigned char serialNumber[32];
  uint8_t serialNumberSize = 32;

  error = sen5x.getSerialNumber(serialNumber, serialNumberSize);
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("SerialNumber:");
    Serial.println((char*)serialNumber);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(27, 22);
  pmsSerial.begin(9600);

  tft.init();
  tft.setRotation(1); /* Landscape orientation, flipped */
  touch.setCal(526, 3443, 750, 3377, 320, 240, 1);
  tft.fillScreen(0x0);

  while (!Serial)
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    //    while (1) delay(1);
  }

  Serial.print("Heater Enabled State: ");
  if (sht31.isHeaterEnabled())
    Serial.println("ENABLED");
  else
    Serial.println("DISABLED");

  sen5x.begin(Wire);
  scd4x.begin(Wire);

  uint16_t error;
  char errorMessage[256];

  error = sen5x.deviceReset();
  if (error) {
    Serial.print("Error trying to execute deviceReset(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  // Print SEN55 module information if i2c buffers are large enough
#ifdef USE_PRODUCT_INFO
  printSerialNumber();
  printModuleVersions();
#endif

  // set a temperature offset in degrees celsius
  // Note: supported by SEN54 and SEN55 sensors
  // By default, the temperature and humidity outputs from the sensor
  // are compensated for the modules self-heating. If the module is
  // designed into a device, the temperature compensation might need
  // to be adapted to incorporate the change in thermal coupling and
  // self-heating of other device components.
  //
  // A guide to achieve optimal performance, including references
  // to mechanical design-in examples can be found in the app note
  // “SEN5x – Temperature Compensation Instruction” at www.sensirion.com.
  // Please refer to those application notes for further information
  // on the advanced compensation settings used
  // in `setTemperatureOffsetParameters`, `setWarmStartParameter` and
  // `setRhtAccelerationMode`.
  //
  // Adjust tempOffset to account for additional temperature offsets
  // exceeding the SEN module's self heating.
  float tempOffset = -3;
  error = sen5x.setTemperatureOffsetSimple(tempOffset);
  if (error) {
    Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("Temperature Offset set to ");
    Serial.print(tempOffset);
    Serial.println(" deg. Celsius (SEN54/SEN55 only");
  }

  // Start Measurement
  error = sen5x.startMeasurement();
  if (error) {
    Serial.print("Error trying to execute startMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  // stop potentially previously started measurement
  error1 = scd4x.stopPeriodicMeasurement();
  if (error1) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error1, errorMessage1, 256);
    Serial.println(errorMessage1);
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;

  error1 = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error1) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error1, errorMessage1, 256);
    Serial.println(errorMessage1);
  } else {
    printSerialNumber1(serial0, serial1, serial2);
  }

  // Start Measurement
  error1 = scd4x.startPeriodicMeasurement();
  if (error1) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error1, errorMessage1, 256);
    Serial.println(errorMessage1);
  }

  Serial.println("Waiting for first measurement... (5 sec)");

  // Display Menu
  tft.setCursor(15, 20); // Set the cursor position X,Y
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print("BCM-Factory Test");

  tft.setCursor(50, 90); // Set the cursor position X,Y
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print("PM2.5");

  tft.setCursor(50, 125); // Set the cursor position X,Y
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print("PM10");

  tft.setCursor(50, 165); // Set the cursor position X,Y
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print("Temp");
  tft.setCursor(245, 165); // Set the cursor position X,Y
  tft.print("C");

  tft.setCursor(50, 205); // Set the cursor position X,Y
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print("Humi");
  tft.setCursor(245, 205); // Set the cursor position X,Y
  tft.print("%");

  tft.setCursor(50, 50); // Set the cursor position X,Y
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print("CO2");

  // This variant of task creation can also specify on which core it will be run (only relevant for multi-core ESPs)
  xTaskCreatePinnedToCore(
    TaskAnalogRead,   /* Task function. */
    "Analog Read",     /* name of task. */
    20000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    1);          /* pin task to core 0 */
}

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms5003data data;

/* Main program */
void loop()
{

  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
  } else {
    Serial.println("Failed to read temperature");
  }

  if (! isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = "); Serial.println(h);
  } else {
    Serial.println("Failed to read humidity");
  }

  delay(1000);

  // Toggle heater enabled state every 30 seconds
  // An ~3.0 degC temperature increase can be noted when heater is enabled
  if (loopCnt >= 30) {
    enableHeater = !enableHeater;
    sht31.heater(enableHeater);
    Serial.print("Heater Enabled State: ");
    if (sht31.isHeaterEnabled())
      Serial.println("ENABLED");
    else
      Serial.println("DISABLED");

    loopCnt = 0;
  }
  loopCnt++;

  Serial.print("MassConcentrationPm1p0:");
  Serial.print(massConcentrationPm1p0);
  Serial.print("\t");
  Serial.print("MassConcentrationPm2p5:");
  Serial.print(massConcentrationPm2p5);
  Serial.print("\t");
  Serial.print("MassConcentrationPm4p0:");
  Serial.print(massConcentrationPm4p0);
  Serial.print("\t");
  Serial.print("MassConcentrationPm10p0:");
  Serial.print(massConcentrationPm10p0);
  Serial.print("\t");
  Serial.print("AmbientHumidity:");
  if (isnan(ambientHumidity)) {
    Serial.print("n/a");
  } else {
    Serial.print(ambientHumidity);
  }
  Serial.print("\t");
  Serial.print("AmbientTemperature:");
  if (isnan(ambientTemperature)) {
    Serial.print("n/a");
  } else {
    Serial.print(ambientTemperature);
  }
  Serial.print("\t");
  Serial.print("VocIndex:");
  if (isnan(vocIndex)) {
    Serial.print("n/a");
  } else {
    Serial.print(vocIndex);
  }
  Serial.print("\t");
  Serial.print("NoxIndex:");
  if (isnan(noxIndex)) {
    Serial.println("n/a");
  } else {
    Serial.println(noxIndex);
  }

  Serial.print("Co2:");
  Serial.print(co2);
  Serial.print("\t");
  Serial.print("Temperature:");
  Serial.print(temperature);
  Serial.print("\t");
  Serial.print("Humidity:");
  Serial.println(humidity);

}


void TaskAnalogRead(void *pvParameters) { // This is a task.
  (void) pvParameters;
  // Check if the given analog pin is usable - if not - delete this task

  for (;;) {

    tft.setTextSize(3);
    tft.setCursor(150, 50); // Set the cursor position X,Y
    tft.setTextColor(TFT_BLACK);
    tft.println(co2);
    // tft.setTextFont(GLCD);
    tft.setTextSize(3);
    tft.setCursor(150, 85);
    tft.setTextColor(TFT_BLACK);
    tft.println(massConcentrationPm2p5);
    // tft.setTextFont(GLCD);
    tft.setTextSize(3);
    tft.setCursor(150, 120);
    tft.setTextColor(TFT_BLACK);
    tft.println(massConcentrationPm10p0);
    //
    tft.setTextSize(3);
    tft.setCursor(150, 160);
    tft.setTextColor(TFT_BLACK);
    tft.println(ambientTemperature);
    // tft.setTextFont(GLCD);
    tft.setTextSize(3);
    tft.setCursor(150, 200);
    tft.setTextColor(TFT_BLACK);
    tft.println(ambientHumidity);
    
    error = sen5x.readMeasuredValues(
        massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
        massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
        noxIndex);

    error1 = scd4x.readMeasurement(co2, temperature, humidity);

    tft.setTextSize(3);
    tft.setCursor(150, 50); // Set the cursor position X,Y
    tft.setTextColor(TFT_PINK);
    tft.println(co2);

    tft.setTextSize(3);
    tft.setCursor(150, 85);
    tft.setTextColor(TFT_ORANGE);
    tft.println(massConcentrationPm2p5);
    // tft.setTextFont(GLCD);
    tft.setTextSize(3);
    tft.setCursor(150, 120);
    tft.setTextColor(TFT_ORANGE);
    tft.println(massConcentrationPm10p0);
    //
    // tft.setTextFont(GLCD);
    tft.setTextSize(3);
    tft.setCursor(150, 160);
    tft.setTextColor(TFT_RED);
    tft.println(ambientTemperature);
    // tft.setTextFont(GLCD);
    tft.setTextSize(3);
    tft.setCursor(150, 200);
    tft.setTextColor(TFT_GREEN);
    tft.println(ambientHumidity);
    //
    delay(1000);
    //
    //        // tft.setTextFont(GLCD);
    //        tft.setTextSize(4);
    //        tft.setCursor(150, 120);
    //        tft.setTextColor(TFT_BLACK);
    //        tft.println(average1);
  }
}
