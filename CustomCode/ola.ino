const byte PIN_IMU_POWER = 27;
const byte PIN_PWR_LED = 29; 
const byte PIN_STAT_LED = 19;
const byte PIN_IMU_INT = 37;
const byte PIN_IMU_CHIP_SELECT = 44;
const byte PIN_MICROSD_CHIP_SELECT = 23;
const byte PIN_MICROSD_POWER = 15;
const byte PIN_SPI_SCK = 5;
const byte PIN_SPI_CIPO = 6;
const byte PIN_SPI_COPI = 7;

#include "ICM_20948.h"  
#include <SPI.h> 
#include <String.h> 
#include <SdFat.h>
#include<settings.h>


struct struct_online {
  bool microSD = false;
  bool dataLogging = false;
  bool serialLogging = false;
  bool IMU = false;
  bool serialOutput = false;
} online;



#define SD_CONFIG SdSpiConfig(PIN_MICROSD_CHIP_SELECT, SHARED_SPI, SD_SCK_MHZ(24)) // 24MHz
#define USE_SPI       
#define SERIAL_PORT Serial
#define SPI_PORT SPI
#define CS_PIN PIN_IMU_CHIP_SELECT 
#define SPI_FREQ 5000000 // You can override the default SPI frequency

ICM_20948_SPI myICM;  // If using SPI create an ICM_20948_SPI object
SdFat sd;
File csvFile;
float cTime, accX, accY, accZ, gyroX, gyroY, gyroZ;
int count = 0;
#define FILE_BASE_NAME "WIPAD"  
char fileName[13] = FILE_BASE_NAME "00.csv";

void setup() {

    beginIMU(false);

  SERIAL_PORT.begin(115200);
//  while(!SERIAL_PORT){};

  pinMode(PIN_PWR_LED, OUTPUT); 
  pinMode(PIN_STAT_LED, OUTPUT); 

#ifdef USE_SPI
  SPI_PORT.begin();
  beginSD();
//  fileName = setupSD(csvFile, sd, fileName); 

  Serial.print("Initializing SD card...");
  if (!sd.begin(PIN_MICROSD_CHIP_SELECT)) {
    Serial.println("Card failed, or not present");
    // Blink Red Error ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    digitalWrite(PIN_PWR_LED, HIGH);                                       
    float startTime = millis();
    while (1);
  }
  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  //char fileName[13] = FILE_BASE_NAME "00.csv";
  Serial.println("card initialized.");
  while (sd.exists(fileName))
  {
      if (fileName[BASE_NAME_SIZE + 1] != '9')
      {
        fileName[BASE_NAME_SIZE + 1]++;
      }
      else if (fileName[BASE_NAME_SIZE] != '9')
      {
        fileName[BASE_NAME_SIZE + 1] = '0';
        fileName[BASE_NAME_SIZE]++;
      }
      else
      {
        Serial.println("Can't create file name");
        // Blink Red Error ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        digitalWrite(PIN_PWR_LED, HIGH);                                       
      }
  }
  csvFile = sd.open(fileName, FILE_WRITE);
  csvFile.print("currTime");       csvFile.print(",");

  csvFile.print("accelX");         csvFile.print(",");
  csvFile.print("accelY");         csvFile.print(","); 
  csvFile.print("accelZ");         csvFile.print(",");

  csvFile.print("gyroX");          csvFile.print(",");
  csvFile.print("gyroY");          csvFile.print(","); 
  csvFile.print("gyroZ");          

  csvFile.println();
  csvFile.close();

  
  pinMode(PIN_IMU_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_IMU_CHIP_SELECT, HIGH); //Be sure IMU is deselected
  
  
  enableCIPOpullUp(); // Enable CIPO pull-up on the OLA
#if defined(ARDUINO_ARCH_MBED)
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); // Do a fake transaction
  SPI.endTransaction();
  enableCIPOpullUp(); // Re-enable the CIPO pull-up
#endif

#else
  WIRE_PORT.begin();
  WIRE_PORT.setClock(400000);
#endif
  
  
  //Reset ICM by power cycling it
  imuPowerOff();
  delay(10);
  imuPowerOn(); // Enable power for the OLA IMU
  delay(100); // Wait for the IMU to power up

  

  bool initialized = false;
  while( !initialized ){
    
    myICM.begin( CS_PIN, SPI_PORT, SPI_FREQ ); 
    SERIAL_PORT.print( F("Initialization of the sensor returned: ") );
    SERIAL_PORT.println( myICM.statusString() );
    if( myICM.status != ICM_20948_Stat_Ok ){
      SERIAL_PORT.println( "Trying again..." );
      delay(500);
    }else{
      initialized = true;
    }
  }

  myICM.setSampleMode((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), ICM_20948_Sample_Mode_Continuous);
  if (myICM.status != ICM_20948_Stat_Ok)
  {
    SERIAL_PORT.print(F("setSampleMode returned: "));
    SERIAL_PORT.println(myICM.statusString());
  }

  ICM_20948_fss_t myFSS;
  myFSS.a = gpm4; 
  myFSS.g = dps2000; 
  myICM.setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myFSS);

  ICM_20948_dlpcfg_t myDLPcfg;    
  myDLPcfg.a = acc_d5bw7_n8bw3; 
  myDLPcfg.g = gyr_d5bw7_n8bw9; 

}

void loop() {

  count = count + 1;
  csvFile = sd.open(fileName, FILE_WRITE);

  float startTime = millis();
  while ( ((millis() - startTime)/1000) <= 5)
  {
    if (myICM.dataReady())
    {
        myICM.getAGMT();              
        cTime = (millis() - startTime);
        gyroX = myICM.gyrX();   gyroY = myICM.gyrY();   gyroZ = myICM.gyrZ();
        accX  = myICM.accX();   accY  = myICM.accY();   accZ  = myICM.accZ();

        Write_SDcard();           
        SERIAL_PORT.print(gyroZ); 
        SERIAL_PORT.println();
    }
  }
  if(count%2 == 1){ // Blink Blue
    digitalWrite(PIN_PWR_LED, LOW);
    digitalWrite(PIN_STAT_LED, HIGH);
  }
  else{ // Blink Red
    digitalWrite(PIN_PWR_LED, HIGH);
    digitalWrite(PIN_STAT_LED, LOW);
  }
  csvFile.close();
}

void microSDPowerOn()
{
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  digitalWrite(PIN_MICROSD_POWER, LOW);
}

void beginSD()
{
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  pin_config(PinName(PIN_MICROSD_POWER), g_AM_HAL_GPIO_OUTPUT); // Make sure the pin does actually get re-configured
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  pin_config(PinName(PIN_MICROSD_CHIP_SELECT), g_AM_HAL_GPIO_OUTPUT); // Make sure the pin does actually get re-configured
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected

  // If the microSD card is present, it needs to be powered on otherwise the IMU will fail to start
  // (The microSD card will pull the SPI pins low, preventing communication with the IMU)

  // For reasons I don't understand, we seem to have to wait for at least 1ms after SPI.begin before we call microSDPowerOn.
  // If you comment the next line, the Artemis resets at microSDPowerOn when beginSD is called from wakeFromSleep...
  // But only on one of my V10 red boards. The second one I have doesn't seem to need the delay!?
  delay(5);

  microSDPowerOn();

  //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
  //Max current is 200mA average across 1s, peak 300mA
  for (int i = 0; i < 10; i++) //Wait
  {
    // checkBattery();
    delay(1);
  }

  if (sd.begin(SD_CONFIG) == false) // Try to begin the SD card using the correct chip select
  {
    SERIAL_PORT.println(F("SD init failed (first attempt). Trying again...\r\n"));
    for (int i = 0; i < 250; i++) //Give SD more time to power up, then try again
    {
      // checkBattery();
      delay(1);
    }
    if (sd.begin(SD_CONFIG) == false) // Try to begin the SD card using the correct chip select
    {
      if (true)
      {
        SERIAL_PORT.println(F("SD init failed (second attempt). Is card present? Formatted?"));
        SERIAL_PORT.println(F("Please ensure the SD card is formatted correctly using https://www.sdcard.org/downloads/formatter/"));
      }
      digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
      online.microSD = false;
      return;
    }
  }

  //Change to root directory. All new file creation will be in root.
  if (sd.chdir() == false)
  {
    if (true)
    {
      SERIAL_PORT.println(F("SD change directory failed"));
    }
    online.microSD = false;
    return;
  }

  online.microSD = true;
}

void imuPowerOn()
{
  pinMode(PIN_IMU_POWER, OUTPUT);
  digitalWrite(PIN_IMU_POWER, HIGH);
}
void imuPowerOff()
{
  pinMode(PIN_IMU_POWER, OUTPUT);
  digitalWrite(PIN_IMU_POWER, LOW);
}

void Write_SDcard()
{
  if (csvFile){
    csvFile.print(String(cTime));    csvFile.print(",");
    
    csvFile.print(String(accX));        csvFile.print(",");
    csvFile.print(String(accY));        csvFile.print(","); 
    csvFile.print(String(accZ));        csvFile.print(",");

    csvFile.print(String(gyroX));       csvFile.print(",");
    csvFile.print(String(gyroY));       csvFile.print(","); 
    csvFile.print(String(gyroZ));         
    csvFile.println();                  //End of Row move to next row
  }
}

#if defined(ARDUINO_ARCH_MBED) // updated for v2.1.0 of the Apollo3 core
bool enableCIPOpullUp()
{
  am_hal_gpio_pincfg_t cipoPinCfg = g_AM_BSP_GPIO_IOM0_MISO;
  cipoPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K;
  pin_config(PinName(PIN_SPI_CIPO), cipoPinCfg);
  return (true);
}
#else
bool enableCIPOpullUp()
{
  ap3_err_t retval = AP3_OK;
  am_hal_gpio_pincfg_t cipoPinCfg = AP3_GPIO_DEFAULT_PINCFG;
  cipoPinCfg.uFuncSel = AM_HAL_PIN_6_M0MISO;
  cipoPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K;
  cipoPinCfg.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA;
  cipoPinCfg.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL;
  cipoPinCfg.uIOMnum = AP3_SPI_IOM;
  padMode(MISO, cipoPinCfg, &retval);
  return (retval == AP3_OK);
}
#endif

