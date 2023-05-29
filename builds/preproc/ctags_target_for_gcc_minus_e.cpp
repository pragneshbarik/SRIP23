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

# 13 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 2
# 14 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 2
# 15 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 2
# 16 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino" 2







ICM_20948_SPI myICM; // If using SPI create an ICM_20948_SPI object
SdFat sd;
File csvFile;
float cTime, accX, accY, accZ, gyroX, gyroY, gyroZ;
int count, count_h = 0;
float passThresh = 75.123;
float MS_Thresh = 75.123;
float UP_Thresh = 31.123;
float WZ_prev, WZ_pprev, last_heel, last_HS_time = 0.0;
int flag_HO,flag_MS,flag_ZC,flag_toe,flag_Int = 0;
int cs, fa = -100;
float HS, LP, TO, ZC, MS = 0.0;
int timePacket = 120;

float aySeq[50];
int count_aySeq = 0;

//Constants for the AFO
const int M = 3;
const float nu = 5;
const float eta = 1;
const float pi = 3.14;
const float f_min = 1.3;
//Initialize the variables for the AFO
float F;
float th_d = 0.00, th_cap = 0.00;
float start;
float Y[2 * M + 2] = {0, 0, 0, 2 * pi * f_min, 0, 0, 0, 0};
float dt = 0.0;
float phi_GC, phi_HS = 0.0;




char fileName[13] = "WIPAD" "00.csv";

void setup() {

  Serial.begin(115200);
//  while(!SERIAL_PORT){};

  pinMode(PIN_PWR_LED, OUTPUT);
  pinMode(PIN_STAT_LED, OUTPUT);


  SPI.begin();
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
  digitalWrite(PIN_IMU_CHIP_SELECT, HIGH); //Be sure IMU is deselected


  enableCIPOpullUp(); // Enable CIPO pull-up on the OLA

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); // Do a fake transaction
  SPI.endTransaction();
  enableCIPOpullUp(); // Re-enable the CIPO pull-up
# 133 "E:\\Projects\\Artemis\\CustomCode\\sketch_may29a\\sketch_may29a.ino"
  //Reset ICM by power cycling it
  imuPowerOff();
  delay(10);
  imuPowerOn(); // Enable power for the OLA IMU
  delay(100); // Wait for the IMU to power up



  bool initialized = false;
  while( !initialized ){

    myICM.begin( PIN_IMU_CHIP_SELECT, SPI, 5000000 /* You can override the default SPI frequency*/ );
    Serial.print( (reinterpret_cast<const __FlashStringHelper *>(("Initialization of the sensor returned: "))) );
    Serial.println( myICM.statusString() );
    if( myICM.status != ICM_20948_Stat_Ok ){
      Serial.println( "Trying again..." );
      delay(500);
    }else{
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

void loop() {

  count = count + 1;
//  csvFile = sd.open(fileName, FILE_WRITE);

  float startTime = millis();
  while ( ((millis() - startTime)/1000) <= timePacket)
  {
    if (myICM.dataReady())
    {
        myICM.getAGMT();
        cTime = (millis() - startTime);
        gyroX = myICM.gyrX(); gyroY = myICM.gyrY(); gyroZ = -myICM.gyrZ();
        accX = myICM.accX(); accY = myICM.accY(); accZ = myICM.accZ();

//      Heel-Strike Detection =================================================================================
        if (gyroZ >= passThresh && WZ_prev <= passThresh){
          count_h = count_h + 1;
        }
        if (gyroZ <= passThresh && WZ_prev >= passThresh){
          count_h = count_h + 1;
        }
        if (WZ_prev <= WZ_pprev && WZ_prev <= gyroZ && WZ_prev < 0 && count_h > 0){
          if (count_h%2 == 0){
            last_HS_time = (millis() - startTime)/1000;
            last_heel = gyroZ;
            count_h = 0;
            flag_HO = 1;
            flag_MS = 1;
            flag_ZC = 1;
            HS = gyroZ;

            phi_HS = Y[0];

          }
        }
        else{
          HS = 0;
        }
//      Heel-OFF Detection ====================================================================================
//      Classifier Decision Making Event ----------------------------------------------------------------------
        if (WZ_prev >= WZ_pprev && WZ_prev >= gyroZ && WZ_pprev > last_heel && WZ_pprev < passThresh && flag_HO == 1){
          if((millis() - startTime)/1000 - last_HS_time > 0.175){

            flag_HO = 0;
            flag_toe = 1;
            flag_ZC = 1;
            LP = gyroZ;

            if (WZ_prev > UP_Thresh){
              cs = 900;
            }
            else if (fa == 200){
              cs = 800;
            }
            else if (WZ_prev < 0){
              if (fa == 0){
                cs = 600;
              }
            }
          }
        }
        else{
          LP = 0;
        }

//      TOE-OFF Detection =====================================================================================
        if (WZ_prev <= WZ_pprev && WZ_prev <= gyroZ && WZ_pprev < 0.66*last_heel && flag_toe == 1){
          flag_toe = 0;
          flag_ZC = 1;
          TO = gyroZ;
        }
        else{
          TO = 0;
        }
//      Zero-Crossing =========================================================================================
        if (WZ_pprev < 0 && WZ_prev < 0 && gyroZ >= 0 && flag_ZC == 1){
          flag_ZC = 0;
          ZC = gyroZ;
          if (flag_Int == 0){
            flag_Int = 1;
          }
          else if (flag_Int == 1){
            flag_Int = 0;
          }
        }
        else{
          ZC = 0;
        }

//      Append
        if (flag_Int == 1){
          aySeq[count_aySeq] = -accY/10.0 + 98.1;
          count_aySeq = count_aySeq + 1;
        }
//      MID-SWING ==========================================================
        if (WZ_prev >= WZ_pprev && WZ_prev >= gyroZ && WZ_pprev > MS_Thresh && flag_MS == 1){
          flag_Int = 0;
          MS = gyroZ;
          count_aySeq = 0;
          int maxVal = aySeq[0];

          for (int itr = 0; itr < (sizeof(aySeq) / sizeof(aySeq[0])); itr++) {

            maxVal = max(aySeq[itr],maxVal);
          }
          if (maxVal > 125){
            fa = 200;
          }
          else{
            fa = 0;
          }

          memset(aySeq, 0, sizeof(aySeq));
        }
        else{
          MS = 0;
        }

        WZ_pprev = WZ_prev;
        WZ_prev = gyroZ;
        start = millis();
        AFO();
        phi_GC=((Y[0]-phi_HS)*100.0)/(4*3.1415926535897932384626433832795);

        Serial.print(gyroZ);
        Serial.print(",");
        Serial.print(th_cap);
        Serial.print(",");
        Serial.print(phi_GC);
//        SERIAL_PORT.print(",");
//        SERIAL_PORT.print(TO); 
//        SERIAL_PORT.print(",");
//        SERIAL_PORT.print(ZC);
//        SERIAL_PORT.print(",");
//        SERIAL_PORT.print(MS);  
        Serial.println();

    }
    dt=(millis()-start)/1000.0;
  }
  if(count%2 == 1){ // Blink Blue
    digitalWrite(PIN_PWR_LED, LOW);
    digitalWrite(PIN_STAT_LED, HIGH);
  }
  else{ // Blink Red
    digitalWrite(PIN_PWR_LED, HIGH);
    digitalWrite(PIN_STAT_LED, LOW);
  }
//  csvFile.close();
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
void AFO()
{

  // Calculate the Pelvis Acceleration from the Analog Accelerometer data
  // Obtained by checking accelerometer reading for 1 g and -1 g and using equation y=mx+c;x=(y-c)/m;
  th_d=gyroZ;//*9.81;//-13.21;

  th_cap = Y[2 * M + 1]; //Initializing th_cap  to beta
  //Serial.println(th_cap);
  for (int i = 0; i <= M - 1; i++)
  {
    th_cap = th_cap + Y[M + 1 + i] * sin(Y[i]);
  }

  F = th_d - th_cap;

  //Solving the differential equations
  //Diff eqns of phi

  for (int j = 0; j <= M - 1; j++)
  {
    Y[j] = Y[j] + dt * ((j + 1) * Y[M] + eta * F * cos(Y[j]));
  }
  //Diff eqn of omega
  Y[M] = Y[M] + dt * eta * F * cos(Y[0]);

  //Diff eqns of alpha
  for (int k = 0; k <= M - 1; k++)
  {
    Y[M + 1 + k] = Y[M + 1 + k] + dt * (nu * F * sin(Y[k]));
  }
  //Diff eqn of Beta
  Y[2 * M + 1] = Y[2 * M + 1] + dt * (nu * F);



  //Y[0]=fmod(Y[0],4*PI);

  if (Y[M] < 2 * pi * f_min)
  {
    Y[M] = 2 * pi * f_min;
  }



}
//void Write_SDcard()
//{
//  if (csvFile){
//    csvFile.print(String(cTime));    csvFile.print(",");
//    
//    csvFile.print(String(accX));        csvFile.print(",");
//    csvFile.print(String(accY));        csvFile.print(","); 
//    csvFile.print(String(accZ));        csvFile.print(",");
//
//    csvFile.print(String(gyroX));       csvFile.print(",");
//    csvFile.print(String(gyroY));       csvFile.print(","); 
//    csvFile.print(String(gyroZ));         
//    csvFile.println();                  //End of Row move to next row
//  }
//}


bool enableCIPOpullUp()
{
  am_hal_gpio_pincfg_t cipoPinCfg = g_AM_BSP_GPIO_IOM0_MISO;
  cipoPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K;
  pin_config(PinName(PIN_SPI_CIPO), cipoPinCfg);
  return (true);
}
