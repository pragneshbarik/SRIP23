# 1 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino"
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
const float pi = 3.1416;

# 15 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 2

# 17 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 2
# 18 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 2
# 19 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 2
# 20 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 2







ICM_20948_SPI myICM; // If using SPI create an ICM_20948_SPI object
SdFat sd;
File csvFile;
//===Variable Declaration===============================================================
float passThres = 90.123;
float LPThres = 90.123;
int indHS, indLP, indMS, indTO, indZC, j = 0;
float maxA, minA = 0.0;
float toTime, CS, startTime, wz, wz_prev, wz_pprev, flagForce, thisMS, hsTime, msTime, lpTime, hsms, rtAngDS;
float thisHS, thisTO, prevCS, thisZC, calDS, thisLP, LL;
int countH, flagLP, flagMS, flagZC, flagTO;
float cTime,
    accX, accY, accZ, gyroX, gyroY, gyroZ;
int count = 0;

char fileName[13] = "WIPAD" "00.csv";

//==Queue for tracking previous gyroscope values========================

class GyroQueue
{
public:
  double values[15];
  int front;
  int rear;
  int count;

  GyroQueue()
  {
    front = 0;
    rear = -1;
    count = 0;
  }

  // Setter Functions
  void push(double value)
  {
    if (count == 15)
    {
      front = (front + 1) % 15;
      count--;
    }

    rear = (rear + 1) % 15;
    values[rear] = value;
    count++;
  }

  void set(double value, int id)
  {
    if (id < 15)
      values[id] = value;
  }

  // Getter Functions
  double get(int id)
  {
    if (id < 15)
      return values[id];
    else
      return std::numeric_limits<double>::min();
  }

  double getMaximum()
  {
    double max = std::numeric_limits<double>::min();
    for (int i = 0; i < count; i++)
    {
      int index = (front + i) % 15;
      if (values[index] > max)
      {
        max = values[index];
      }
    }
    return max;
  }

  double getMinimum()
  {
    double min = std::numeric_limits<double>::max();
    for (int i = 0; i < count; i++)
    {
      int index = (front + i) % 15;
      if (values[index] < min)
      {
        min = values[index];
      }
    }
    return min;
  }
};

GyroQueue q;

void setup()
{

  Serial.begin(115200);
  //  while(!SERIAL_PORT){};

  pinMode(PIN_PWR_LED, OUTPUT);
  pinMode(PIN_STAT_LED, OUTPUT);


  SPI.begin();
  beginSD();
  //  fileName = setupSD(csvFile, sd, fileName);

  Serial.print("Initializing SD card...");
  if (!sd.begin(PIN_MICROSD_CHIP_SELECT))
  {
    Serial.println("Card failed, or not present");
    // Blink Red Error ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    digitalWrite(PIN_PWR_LED, HIGH);
    float startTime = millis();
    while (1)
      ;
  }
  const uint8_t BASE_NAME_SIZE = sizeof("WIPAD") - 1;
  // char fileName[13] = FILE_BASE_NAME "00.csv";
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
  csvFile = sd.open(fileName, (
# 166 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 3
                             2 /* +1 == FREAD|FWRITE */ 
# 166 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino"
                             | 
# 166 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 3
                             0x0200 /* open with file create */ 
# 166 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino"
                             | 
# 166 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 3
                             0x4000 /* non blocking I/O (POSIX style) */ 
# 166 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino"
                             /*|< Open at EOF.*/));
  csvFile.print("currTime");
  csvFile.print(",");

  // csvFile.print("accelX");
  // csvFile.print(",");
  // csvFile.print("accelY");
  // csvFile.print(",");
  // csvFile.print("accelZ");
  // csvFile.print(",");

  // csvFile.print("gyroX");
  // csvFile.print(",");
  // csvFile.print("gyroY");
  // csvFile.print(",");
  csvFile.print("gyroZ");
  csvFile.print(",");

  csvFile.print("indHS");
  csvFile.print(",");
  csvFile.print("indLP");
  csvFile.print(",");
  csvFile.print("indMS");
  csvFile.print(",");
  csvFile.print("indTO");
  csvFile.print(",");
  csvFile.print("indZC");

  csvFile.println();
  csvFile.close();

  pinMode(PIN_IMU_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_IMU_CHIP_SELECT, HIGH); // Be sure IMU is deselected

  enableCIPOpullUp(); // Enable CIPO pull-up on the OLA

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); // Do a fake transaction
  SPI.endTransaction();
  enableCIPOpullUp(); // Re-enable the CIPO pull-up







  // Reset ICM by power cycling it
  imuPowerOff();
  delay(10);
  imuPowerOn(); // Enable power for the OLA IMU
  delay(100); // Wait for the IMU to power up

  bool initialized = false;
  while (!initialized)
  {

    myICM.begin(PIN_IMU_CHIP_SELECT, SPI, 5000000 /* You can override the default SPI frequency*/);
    Serial.print((reinterpret_cast<const __FlashStringHelper *>(("Initialization of the sensor returned: "))));
    Serial.println(myICM.statusString());
    if (myICM.status != ICM_20948_Stat_Ok)
    {
      Serial.println("Trying again...");
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
    Serial.print((reinterpret_cast<const __FlashStringHelper *>(("setSampleMode returned: "))));
    Serial.println(myICM.statusString());
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
  csvFile = sd.open(fileName, (
# 257 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 3
                             2 /* +1 == FREAD|FWRITE */ 
# 257 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino"
                             | 
# 257 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 3
                             0x0200 /* open with file create */ 
# 257 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino"
                             | 
# 257 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 3
                             0x4000 /* non blocking I/O (POSIX style) */ 
# 257 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino"
                             /*|< Open at EOF.*/));

  startTime = millis();
  while (((millis() - startTime) / 1000) <= 5)
  {
    if (myICM.dataReady())
    {
      myICM.getAGMT();
      cTime = (millis() - startTime);
      gyroX = myICM.gyrX();
      gyroY = myICM.gyrY();
      gyroZ = myICM.gyrZ();
      accX = myICM.accX();
      accY = myICM.accY();
      accZ = myICM.accZ();
      wz = gyroZ;
      q.push(wz);

      // HEEL STRIKE DETECTION
      if (wz >= passThres && wz_prev <= passThres)
      {
        countH = countH + 1;
        flagForce = 0;
      }
      if (wz <= passThres && wz_prev >= passThres)
      {
        countH = countH + 1;
      }
      if (wz_prev <= wz_pprev && wz_prev <= wz && wz_prev < 0 && countH > 0)
      {
        if (countH % 2 == 0)
        {
          hsTime = cTime;
          countH = 0;
          flagLP = 1;
          flagMS = 1;
          flagZC = 1;
          thisHS = wz_prev;
          indHS = 1;
          hsms = 200 * (hsTime - msTime);
          rtAngDS = (180 / pi) * atan((wz_prev - thisMS) / (hsTime - msTime));
        }
      }

      // ZERO CROSSING 1 DETECTION
      if (wz_prev < 0 && wz > 0 && flagZC == 1 && cTime - hsTime > 0.04)
      {
        thisZC = wz;
        flagZC = 0;
        indZC = 1;
      }

      // LAMBDA PEAK DETECTION
      if (wz_prev >= wz_pprev && wz_prev >= wz && wz_pprev > 0.8 * thisHS && wz_pprev < passThres && flagLP == 1 && cTime - hsTime > 0.18)
      {
        lpTime = cTime;
        flagLP = 0;
        flagTO = 1;
        thisLP = wz_prev;
        indLP = 1;
        flagForce = 1;
        flagZC = 0;
        //======================Classifier Algorithm Invoke at Lambda-Peak Detection ======================
        if (wz > 6.23)
        {
          CS = 4 * 100 + 400; // Upstairs
        }
        if (hsms > calDS)
        {
          CS = 3 * 100 + 400; // Downstairs
        }
        else if (wz < 0)
        {
          if ((prevCS == 800 || prevCS == 700 || prevCS == 400))
          {
            CS = -1 * 100 + 400; // Transition
          }
          if (thisHS < LL)
          {
            CS = 1 * 100 + 400; // Overground
          }
        }
      }

      // TOE-OFF DETECTION

      if (wz_prev <= wz_pprev && wz_prev <= wz && wz_prev < 0.9 * thisHS && flagTO == 1)
      {
        if (cTime - lpTime > 0.21)
        {
          toTime = cTime;
          flagTO = 0;
          flagMS = 1;
          thisTO = wz_prev;
          indTO = 1;
        }
      }

      // MIDSWING DETECTION
      if (wz_prev >= wz_pprev && wz_prev >= wz && wz_prev > LPThres && flagMS == 1)
      {
        msTime = cTime;
        flagMS = 0;
        thisMS = wz_prev;
        indMS = 1;
      }

      // STATIONARY DETECTION
      if (abs(q.getMaximum() - q.getMinimum()) <= 3 && q.getMaximum() > -8 && q.getMinimum() > -8)
      {
        CS = 0 * 100 + 400; // Stationary
      }

      prevCS = CS;
      wz_pprev = wz_prev;
      wz_prev = wz;

      Write_SDcard();
      Serial.print(gyroZ);
      Serial.print(",");
      Serial.print(indHS);
      Serial.print(",");
      Serial.print(indLP);
      Serial.print(",");
      Serial.print(indMS);
      Serial.print(",");
      Serial.print(indTO);
      Serial.print(",");
      Serial.print(indZC);

      Serial.println();
    }
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
  csvFile.close();
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

void Write_SDcard()
{
  if (csvFile)
  {
    csvFile.print((cTime));
    csvFile.print(",");

    // csvFile.print((accX));
    // csvFile.print(",");
    // csvFile.print((accY));
    // csvFile.print(",");
    // csvFile.print((accZ));
    // csvFile.print(",");

    // csvFile.print((gyroX));
    // csvFile.print(",");
    // csvFile.print((gyroY));
    // csvFile.print(",");
    csvFile.print((gyroZ));
    csvFile.print(",");
    csvFile.print(String(indHS));
    csvFile.print(",");
    csvFile.print(String(indLP));
    csvFile.print(",");
    csvFile.print(String(indMS));
    csvFile.print(",");
    csvFile.print(String(indTO));
    csvFile.print(",");
    csvFile.print(String(indZC));
    csvFile.println(); // End of Row move to next row
  }
}


bool enableCIPOpullUp()
{
  am_hal_gpio_pincfg_t cipoPinCfg = g_AM_BSP_GPIO_IOM0_MISO;
  cipoPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K;
  pin_config(PinName(PIN_SPI_CIPO), cipoPinCfg);
  return (true);
}
