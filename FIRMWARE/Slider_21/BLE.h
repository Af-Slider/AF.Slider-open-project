#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

volatile bool ON = true;
hw_timer_t * timer = NULL;      //used also in Stepper!!


/*____________________________________________________________________BLE
 * See the following for generating UUIDs:
 * https://www.uuidgenerator.net/
 * 
 * CHARACTERISTICS and SERVICES for BLE 
 * 
  */
#define BT_LED 2

#define SERVICE_UUID          "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define stepper_selector_UUID "9be16d54-761b-11ec-90d6-0242ac120003"
#define start_stop_UUID       "7cca561a-761b-11ec-90d6-0242ac120003"
#define acc_UUID              "7cca5872-761b-11ec-90d6-0242ac120003"
#define speed_UUID            "7cca599e-761b-11ec-90d6-0242ac120003"
#define loop_UUID             "7cca5aca-761b-11ec-90d6-0242ac120003"
#define starting_point_UUID   "7cca5be2-761b-11ec-90d6-0242ac120003"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      digitalWrite(BT_LED, HIGH);
      if (debugM) Serial.println(F("CONNECTED")); 
      isConnected = true;
      layer = -1;
      updateData = true;
    };

    void onDisconnect(BLEServer* pServer) {
      digitalWrite(BT_LED, LOW);
      if (debugM) Serial.println(F("DISCONNECTED"));     
      pServer->getAdvertising()->start();
      isConnected = false;
      layer = 0;
      updateData = true;
    }
};

class StartStopCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *start_stop_Characteristic) {
      std::string rxValue = start_stop_Characteristic->getValue();
      String inputString;
      
      if (rxValue.length() > 0) {
        if (debugM) Serial.println(F("*********"));
        if (debugM) Serial.print(F("Received Value: "));
        for (int i = 0; i < rxValue.length(); i++){
          inputString += rxValue[i];
        }
        if (debugM) Serial.print(inputString);
        if (debugM) Serial.println();
        if (debugM) Serial.println(F("*********"));
      }
      
      if (inputString == "ON") {
        play = true; 
        layer = 3;
        if (debugM) Serial.println(F("ON"));
      }
      else if (inputString == "OFF") {
        play = false;  
        layer = -1;
        stepper->stopMove();
        if (debugM) Serial.println(F("OFF")); 
      }
      else if (debugM) Serial.println(F("unknown value"));
      updateData = true;
    }
};

class speedCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *speed_Characteristic) {
      std::string rxValue = speed_Characteristic->getValue();
      String inputString;
      
      if (rxValue.length() > 0) {
        if (debugM) Serial.println(F("*********"));
        if (debugM) Serial.print(F("Received Value speed: "));
        for (int i = 0; i < rxValue.length(); i++){
          inputString += rxValue[i];
        }
        if (debugM) Serial.print(inputString);
        if (debugM) Serial.println();
        if (debugM) Serial.println(F("*********"));
      }

      int inputConversion = inputString.toInt();
      if (inputConversion >= Slider.variables[0].minVal && inputConversion <= Slider.variables[0].maxVal){
        Slider.variables[0].val = inputConversion;
        input_speed = inputConversion;
      }
      else if (debugM) Serial.println(F("invalid value"));
      conversion ();
    }
};

class accCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *acc_Characteristic) {
      std::string rxValue = acc_Characteristic->getValue();
      String inputString;
      
      if (rxValue.length() > 0) {
        if (debugM) Serial.println(F("*********"));
        if (debugM) Serial.print(F("Received Value acc: "));
        for (int i = 0; i < rxValue.length(); i++){
          inputString += rxValue[i];
        }
        if (debugM) Serial.print(inputString);
        if (debugM) Serial.println();
        if (debugM) Serial.println(F("*********"));
      }

      double inputConversion = inputString.toDouble();
      if (inputConversion >= Slider.variables[1].minVal && inputConversion <= Slider.variables[1].maxVal){
       Slider.variables[1].val = inputConversion;
       input_acc = inputConversion;
      }
      else if (debugM) Serial.println(F("invalid value"));
      conversion ();
    }
};

class loop_Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *loop_Characteristic) {
      std::string rxValue = loop_Characteristic->getValue();
      String inputString;
      
      if (rxValue.length() > 0) {
        if (debugM) Serial.println(F("*********"));
        if (debugM) Serial.print(F("Received Value loop: "));
        for (int i = 0; i < rxValue.length(); i++){
          inputString += rxValue[i];
        }
        if (debugM) Serial.print(inputString);
        if (debugM) Serial.println();
        if (debugM) Serial.println(F("*********"));
      }

      int inputConversion = inputString.toInt();
      if (inputConversion >= Settings.variables[0].minVal && inputConversion <= Settings.variables[0].maxVal){
        Settings.variables[0].val = inputConversion;
        input_loop = inputConversion;
      }
      else if (debugM) Serial.println(F("invalid value"));
      conversion ();
    }
};

class starting_point_Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *starting_point_Characteristic) {
      std::string rxValue = starting_point_Characteristic->getValue();
      String inputString;
      
      if (rxValue.length() > 0) {
        if (debugM) Serial.println(F("*********"));
        if (debugM) Serial.print(F("Received Value start point: "));
        for (int i = 0; i < rxValue.length(); i++){
          inputString += rxValue[i];
        }
        if (debugM) Serial.print(inputString);
        if (debugM) Serial.println();
        if (debugM) Serial.println(F("*********"));
      }

      int inputConversion = inputString.toInt();
      if (inputConversion >= Settings.variables[0].minVal && inputConversion <= Settings.variables[0].maxVal){
        input_startPoint = inputConversion;
      }
      else if (debugM) Serial.println(F("invalid value"));
      conversion ();
    }
};
