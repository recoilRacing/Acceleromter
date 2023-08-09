// (c) Michael Schoeffler 2017, http://www.mschoeffler.de

#define EEPROM_EMULATION_SIZE     (19 * 1024)
#define FLASH_DEBUG       0

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include <FlashStorage_SAMD.h>

#include "Wire.h" // This library allows you to communicate with I2C devices.

const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.
const float g = 17000/9.81;
const float rot = 131;
const float frequencyInHz = 50;
const int arraySize = 1.4 * frequencyInHz;
const int arraySizeOrganised = 1.3 * frequencyInHz + 1;
const int runtime = frequencyInHz * 1.3;
int countdown;
uint16_t address;

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data

char tmp_str[7]; // temporary variable used in convert function

char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

String dataStorage[arraySize];
String dataStorageOrganised;
int posInDataStorage;

void setup() {
  Serial.begin(115200);

  while (!Serial);

  delay(200);

  Serial.print(F("\nStart FlashStoreAndRetrieve on "));
  Serial.println(BOARD_NAME);
  Serial.println(FLASH_STORAGE_SAMD_VERSION);

  Serial.print("EEPROM length: ");
  Serial.println(EEPROM.length());

  address = 0;
  String data;

  // Read the content of emulated-EEPROM
  EEPROM.get(address, data);

  if (!EEPROM.getCommitASAP())
  {
    Serial.println("CommitASAP not set. Need commit()");
    EEPROM.commit();
  }

  // Print the current number on the serial monitor
  Serial.println("Data:");
  Serial.println(data);

  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDR); //I2C address of the MPU
  Wire.write(0x1B); //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4)
  Wire.write(0); //Setting the gyro to full scale +/- 250deg./s
  Wire.endTransmission();
  Wire.beginTransmission(MPU_ADDR); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5)
  Wire.write(3); //Setting the accel to +/- 16g
  Wire.endTransmission();
  countdown = 0;
  posInDataStorage = 0;
  Serial.println("Finished Initialisation process");
  delay(1000);

  while(true){
    if (countdown <= runtime){
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
    Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
    Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
    
    // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
    accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
    accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
    accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
    temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
    gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
    gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
    gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

    
    if (float(accelerometer_y)/g > 20 && countdown == 0){
      countdown = 1;
    } else if (countdown > 0){
      countdown++;
    }

    if(posInDataStorage < arraySize){
      dataStorage[posInDataStorage] = String((float(accelerometer_x)/g)) + ";" + String((float(accelerometer_y)/g)) + ";" + String((float(accelerometer_z)/g));
      posInDataStorage++;
    } else {
      posInDataStorage = 0;
      dataStorage[posInDataStorage] = String((float(accelerometer_x)/g)) + ";" + String((float(accelerometer_y)/g)) + ";" + String((float(accelerometer_z)/g));
      posInDataStorage++;
    }

    // delay
    delay(1000/frequencyInHz);
  } else if (countdown == runtime+1){
    dataStorageOrganised = "t;ax(t);ay(t);az(t)\n";
    for (int i = 0; i < arraySize; i++){
      dataStorageOrganised += String(float(i)/frequencyInHz) + ";" + dataStorage[int(i+posInDataStorage)%arraySize] + "\n";
    }
    EEPROM.put(address, (String) dataStorageOrganised);

    if (!EEPROM.getCommitASAP())
    {
      Serial.println("CommitASAP not set. Need commit()");
      EEPROM.commit();
    }

    Serial.println("Done writing to emulated EEPROM. You can reset now");
    countdown++;
  }
  }
}

void loop(){}