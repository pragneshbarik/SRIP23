////#include "ICM_20948.h" 
//#include <SdFat.h>
////#include <SD.h>
////#include <SPI.h> 
//#define FILE_BASE_NAME "WIPAD"  
//char fileName[13] = FILE_BASE_NAME "00.csv";
//
//
//void beginSD()
//{
//  pinMode(PIN_MICROSD_POWER, OUTPUT);
//  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
//  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
//  delay(1);
//
////  microSDPowerOn();
//  pinMode(PIN_MICROSD_POWER, OUTPUT);
//  digitalWrite(PIN_MICROSD_POWER, LOW);
//}
////
////void microSDPowerOn()
////{
////  pinMode(PIN_MICROSD_POWER, OUTPUT);
////  digitalWrite(PIN_MICROSD_POWER, LOW);
////}
//
//
//String setupSD(File csvFile, SdFat sd, String fileName)
//{
//  
//  Serial.print("Initializing SD card...");
//    if (!sd.begin(PIN_MICROSD_CHIP_SELECT)) {
//      Serial.println("Card failed, or not present");
//      // Blink Red Error ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//      digitalWrite(PIN_PWR_LED, HIGH);                                       
//      float startTime = millis();
//      while (1);
//    }
//    const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
//    //char fileName[13] = FILE_BASE_NAME "00.csv";
//    Serial.println("card initialized.");
//    while (sd.exists(fileName))
//    {
//        if (fileName[BASE_NAME_SIZE + 1] != '9')
//        {
//          fileName[BASE_NAME_SIZE + 1]++;
//        }
//        else if (fileName[BASE_NAME_SIZE] != '9')
//        {
//          fileName[BASE_NAME_SIZE + 1] = '0';
//          fileName[BASE_NAME_SIZE]++;
//        }
//        else
//        {
//          Serial.println("Can't create file name");
//          // Blink Red Error ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//          digitalWrite(PIN_PWR_LED, HIGH);                                       
//        }
//    }
//    csvFile = sd.open(fileName, FILE_WRITE);
//    csvFile.print("currTime");       csvFile.print(",");
//  
//    csvFile.print("accelX");         csvFile.print(",");
//    csvFile.print("accelY");         csvFile.print(","); 
//    csvFile.print("accelZ");         csvFile.print(",");
//  
//    csvFile.print("gyroX");          csvFile.print(",");
//    csvFile.print("gyroY");          csvFile.print(","); 
//    csvFile.print("gyroZ");          
//  
//    csvFile.println();
//    csvFile.close();
//
//    return fileName;
//}
//
//void Write_SDcard(File csvFile, float cTime, float accX, float accY, float accZ, float gyroX, float gyroY, float gyroZ)
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
