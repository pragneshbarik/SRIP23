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

ICM_20948_SPI myICM; // If using SPI create an ICM_20948_SPI object
SdFat sd;
File csvFile;
float cTime, accX, accY, accZ, gyroX, gyroY, gyroZ;
int count, count_h = 0;
float passThresh = 75.123;
float MS_Thresh = 75.123;
float UP_Thresh = 31.123;
float WZ_prev, WZ_pprev, last_heel, last_HS_time = 0.0;
int flag_HO, flag_MS, flag_ZC, flag_toe, flag_Int = 0;
int cs, fa = -100;
float HS, LP, TO, ZC, MS = 0.0;
int timePacket = 120;

float aySeq[50];
int count_aySeq = 0;

// Constants for the AFO
const int M = 3;
const float nu = 5;
const float eta = 1;
const float pi = 3.14;
const float f_min = 1.3;
// Initialize the variables for the AFO
float F;
float th_d = 0.00, th_cap = 0.00;
float start;
float Y[2 * M + 2] = {0, 0, 0, 2 * pi *f_min, 0, 0, 0, 0};
float dt = 0.0;
float phi_GC, phi_HS = 0.0;

#define FILE_BASE_NAME "WIPAD"
char fileName[13] = FILE_BASE_NAME "00.csv";

void setup()
{

  SERIAL_PORT.begin(115200);
  //  while(!SERIAL_PORT){};

  pinMode(PIN_PWR_LED, OUTPUT);
  pinMode(PIN_STAT_LED, OUTPUT);

#ifdef USE_SPI
  SPI_PORT.begin();
  //  beginSD();
  ////  fileName = setupSD(csvFile, sd, fileName);
  //
  //  Serial.print("Initializing SD card...");
  //  if (!sd.begin(PIN_MICROSD_CHIP_SELECT)) {
  //    Serial.println("Card failed, or not present");
  //    // Blink Red Error ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //    digitalWrite(PIN_PWR_LED, HIGH);
  //    float startTime = millis();
  //    while (1);
  //  }
  //  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  //  //char fileName[13] = FILE_BASE_NAME "00.csv";
  //  Serial.println("card initialized.");
  //  while (sd.exists(fileName))
  //  {
  //      if (fileName[BASE_NAME_SIZE + 1] != '9')
  //      {
  //        fileName[BASE_NAME_SIZE + 1]++;
  //      }
  //      else if (fileName[BASE_NAME_SIZE] != '9')
  //      {
  //        fileName[BASE_NAME_SIZE + 1] = '0';
  //        fileName[BASE_NAME_SIZE]++;
  //      }
  //      else
  //      {
  //        Serial.println("Can't create file name");
  //        // Blink Red Error ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //        digitalWrite(PIN_PWR_LED, HIGH);
  //      }
  //  }
  //  csvFile = sd.open(fileName, FILE_WRITE);
  //  csvFile.print("currTime");       csvFile.print(",");
  //
  //  csvFile.print("accelX");         csvFile.print(",");
  //  csvFile.print("accelY");         csvFile.print(",");
  //  csvFile.print("accelZ");         csvFile.print(",");
  //
  //  csvFile.print("gyroX");          csvFile.print(",");
  //  csvFile.print("gyroY");          csvFile.print(",");
  //  csvFile.print("gyroZ");
  //
  //  csvFile.println();
  //  csvFile.close();

  pinMode(PIN_IMU_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_IMU_CHIP_SELECT, HIGH); // Be sure IMU is deselected

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

  // Reset ICM by power cycling it
  imuPowerOff();
  delay(10);
  imuPowerOn(); // Enable power for the OLA IMU
  delay(100);   // Wait for the IMU to power up

  bool initialized = false;
  while (!initialized)
  {

    myICM.begin(CS_PIN, SPI_PORT, SPI_FREQ);
    SERIAL_PORT.print(F("Initialization of the sensor returned: "));
    SERIAL_PORT.println(myICM.statusString());
    if (myICM.status != ICM_20948_Stat_Ok)
    {
      SERIAL_PORT.println("Trying again...");
      delay(500);
    }
    else
    {
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

void loop()
{

  count = count + 1;
  //  csvFile = sd.open(fileName, FILE_WRITE);

  float startTime = millis();
  while (((millis() - startTime) / 1000) <= timePacket)
  {
    if (myICM.dataReady())
    {
      myICM.getAGMT();
      cTime = (millis() - startTime);
      gyroX = myICM.gyrX();
      gyroY = myICM.gyrY();
      gyroZ = -myICM.gyrZ();
      accX = myICM.accX();
      accY = myICM.accY();
      accZ = myICM.accZ();

      //      Heel-Strike Detection =================================================================================
      if (gyroZ >= passThresh && WZ_prev <= passThresh)
      {
        count_h = count_h + 1;
      }
      if (gyroZ <= passThresh && WZ_prev >= passThresh)
      {
        count_h = count_h + 1;
      }
      if (WZ_prev <= WZ_pprev && WZ_prev <= gyroZ && WZ_prev < 0 && count_h > 0)
      {
        if (count_h % 2 == 0)
        {
          last_HS_time = (millis() - startTime) / 1000;
          last_heel = gyroZ;
          count_h = 0;

          HS = gyroZ;

          phi_HS = Y[0];
        }
      }
      else
      {
        HS = 0;
      }

      WZ_pprev = WZ_prev;
      WZ_prev = gyroZ;
      start = millis();
      AFO();
      phi_GC = ((Y[0] - phi_HS) * 100.0) / (4 * PI);

      SERIAL_PORT.print(gyroZ);
      SERIAL_PORT.print(",");
      SERIAL_PORT.print(th_cap);
      SERIAL_PORT.print(",");
      SERIAL_PORT.print(phi_GC);
      //        SERIAL_PORT.print(",");
      //        SERIAL_PORT.print(TO);
      //        SERIAL_PORT.print(",");
      //        SERIAL_PORT.print(ZC);
      //        SERIAL_PORT.print(",");
      //        SERIAL_PORT.print(MS);
      SERIAL_PORT.println();
    }
    dt = (millis() - start) / 1000.0;
  }
  if (count % 2 == 1)
  { // Blink Blue
    digitalWrite(PIN_PWR_LED, LOW);
    digitalWrite(PIN_STAT_LED, HIGH);
  }
  else
  { // Blink Red
    digitalWrite(PIN_PWR_LED, HIGH);
    digitalWrite(PIN_STAT_LED, LOW);
  }
  //  csvFile.close();
}

void beginSD()
{
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); // Be sure SD is deselected
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
void AFO()
{

  // Calculate the Pelvis Acceleration from the Analog Accelerometer data
  // Obtained by checking accelerometer reading for 1 g and -1 g and using equation y=mx+c;x=(y-c)/m;
  th_d = gyroZ; //*9.81;//-13.21;

  th_cap = Y[2 * M + 1]; // Initializing th_cap  to beta
  // Serial.println(th_cap);
  for (int i = 0; i <= M - 1; i++)
  {
    th_cap = th_cap + Y[M + 1 + i] * sin(Y[i]);
  }

  F = th_d - th_cap;

  // Solving the differential equations
  // Diff eqns of phi

  for (int j = 0; j <= M - 1; j++)
  {
    Y[j] = Y[j] + dt * ((j + 1) * Y[M] + eta * F * cos(Y[j]));
  }
  // Diff eqn of omega
  Y[M] = Y[M] + dt * eta * F * cos(Y[0]);

  // Diff eqns of alpha
  for (int k = 0; k <= M - 1; k++)
  {
    Y[M + 1 + k] = Y[M + 1 + k] + dt * (nu * F * sin(Y[k]));
  }
  // Diff eqn of Beta
  Y[2 * M + 1] = Y[2 * M + 1] + dt * (nu * F);

  // Y[0]=fmod(Y[0],4*PI);

  if (Y[M] < 2 * pi * f_min)
  {
    Y[M] = 2 * pi * f_min;
  }
}
// void Write_SDcard()
//{
//   if (csvFile){
//     csvFile.print(String(cTime));    csvFile.print(",");
//
//     csvFile.print(String(accX));        csvFile.print(",");
//     csvFile.print(String(accY));        csvFile.print(",");
//     csvFile.print(String(accZ));        csvFile.print(",");
//
//     csvFile.print(String(gyroX));       csvFile.print(",");
//     csvFile.print(String(gyroY));       csvFile.print(",");
//     csvFile.print(String(gyroZ));
//     csvFile.println();                  //End of Row move to next row
//   }
// }

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
