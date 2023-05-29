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

void beginSD()
{
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
  delay(1);

  microSDPowerOn();
}

void microSDPowerOn()
{
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  digitalWrite(PIN_MICROSD_POWER, LOW);
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
