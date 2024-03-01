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
//#define SERVICE2_UUID         "e90ee8e3-d369-4d14-9c8e-a4b62fe36d1f"
//#define stepper_selector_UUID "9be16d54-761b-11ec-90d6-0242ac120003"
#define start_stop_UUID       "7cca561a-761b-11ec-90d6-0242ac120003"
//#define acc_UUID              "7cca5872-761b-11ec-90d6-0242ac120003"
#define speed_UUID            "7cca599e-761b-11ec-90d6-0242ac120003"
#define loop_UUID             "7cca5aca-761b-11ec-90d6-0242ac120003"
//#define starting_point_UUID   "7cca5be2-761b-11ec-90d6-0242ac120003"
#define step_UUID             "f6452482-8136-49e7-a663-f8036fc133a5"
//#define pause_UUID            "e2551434-7fb1-4f50-b1c1-c0d990614c92"


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
      
      if (inputString == "sON") {
        functionMenuSel = 0; //slider
        play = true; 
        layer = 3;
        if (debugM) Serial.println(F("ON"));
      }
      else if (inputString == "tON") {
        functionMenuSel = 1; //TimeLapse
        play = true; 
        layer = 3;
        if (debugM) Serial.println(F("ON"));
      }
      else if (inputString == "mON") {
        functionMenuSel = 2; //Stopmotion
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
      String firstValue;
      String secondValue; 
      
      if (rxValue.length() > 0) {
        for (int i = 0; i < rxValue.length(); i++){
          inputString += rxValue[i];
        }
        
        int index = inputString.indexOf(";");
        firstValue = inputString.substring(0, index);
        secondValue = inputString.substring(index+1);

        if (debugM) Serial.println(F("*********"));
        if (debugM) Serial.print(F("Received Value speed: "));
        if (debugM) Serial.println(firstValue);
        if (debugM) Serial.print(F("Received Value acc: "));
        if (debugM) Serial.println(secondValue);
        if (debugM) Serial.println(F("*********"));
      }

      int inputConversion = firstValue.toInt();
      if (inputConversion >= Slider.variables[0].minVal && inputConversion <= Slider.variables[0].maxVal) {
        Slider.variables[0].val = inputConversion;
        input_speed = inputConversion;
      }
      else if (debugM) Serial.println(F("invalid value"));

      double inputConversion1 = secondValue.toDouble();
      if (inputConversion1 >= Slider.variables[1].minVal && inputConversion1 <= Slider.variables[1].maxVal){
       Slider.variables[1].val = inputConversion1;
       input_acc = inputConversion1;
      }
      else if (debugM) Serial.println(F("invalid value"));

      conversion ();
    }
};

/******/

class stepCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *step_Characteristic) {
      std::string rxValue = step_Characteristic->getValue();
      String inputString;
      String firstValue;
      String secondValue; 
      
      if (rxValue.length() > 0) {
        if (debugM) Serial.println(F("*********"));
        for (int i = 0; i < rxValue.length(); i++){
          inputString += rxValue[i];
        }

        int index = inputString.indexOf(";");
        firstValue = inputString.substring(0, index);
        secondValue = inputString.substring(index+1);

        if (debugM) Serial.println(F("*********"));
        if (debugM) Serial.print(F("Received Value step: "));
        if (debugM) Serial.println(firstValue);
        if (debugM) Serial.print(F("Received Value pause: "));
        if (debugM) Serial.println(secondValue);
        if (debugM) Serial.println(F("*********"));
      }

      int inputConversion = firstValue.toInt();
      if (inputConversion >= TimeLapse.variables[0].minVal && inputConversion <= TimeLapse.variables[0].maxVal){
        TimeLapse.variables[0].val = inputConversion;
        input_step = inputConversion;
      }
      else if (debugM) Serial.println(F("invalid value"));

      int inputConversion1 = secondValue.toInt();
      if (inputConversion1 >= TimeLapse.variables[1].minVal && inputConversion1 <= TimeLapse.variables[1].maxVal){
        TimeLapse.variables[1].val = inputConversion1;
        TL_pause = inputConversion1;
      }
      else if (debugM) Serial.println(F("invalid value"));

      conversion ();
    }
};


class loopCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *loop_Characteristic) {
      std::string rxValue = loop_Characteristic->getValue();
      String inputString;
      String firstValue;
      String secondValue; 
      
      if (rxValue.length() > 0) {
        if (debugM) Serial.println(F("*********"));
        for (int i = 0; i < rxValue.length(); i++){
          inputString += rxValue[i];
        }

        int index = inputString.indexOf(";");
        firstValue = inputString.substring(0, index);
        secondValue = inputString.substring(index+1);

        if (debugM) Serial.println(F("*********"));
        if (debugM) Serial.print(F("Received Value loop: "));
        if (debugM) Serial.println(firstValue);
        if (debugM) Serial.print(F("Received Value startPoint: "));
        if (debugM) Serial.println(secondValue);
        if (debugM) Serial.println(F("*********"));
      }

      int inputConversion = firstValue.toInt();
      if (inputConversion >= Settings.variables[0].minVal && inputConversion <= Settings.variables[0].maxVal){
        Settings.variables[0].val = inputConversion;
        input_loop = inputConversion;
      }
      else if (debugM) Serial.println(F("invalid value"));

      int inputConversion1 = secondValue.toInt();
      if (inputConversion1 >= Settings.variables[0].minVal && inputConversion1 <= Settings.variables[0].maxVal){
        input_startPoint = inputConversion1;
      }
      else if (debugM) Serial.println(F("invalid value"));

      conversion ();
    }
};

/*
class pauseCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pause_Characteristic) {
      std::string rxValue = pause_Characteristic->getValue();
      String inputString;
      
      if (rxValue.length() > 0) {
        if (debugM) Serial.println(F("*********"));
        if (debugM) Serial.print(F("Received Value pause: "));
        for (int i = 0; i < rxValue.length(); i++){
          inputString += rxValue[i];
        }
        if (debugM) Serial.print(inputString);
        if (debugM) Serial.println();
        if (debugM) Serial.println(F("*********"));
      }

      int inputConversion = inputString.toInt();
      if (inputConversion >= TimeLapse.variables[1].minVal && inputConversion <= TimeLapse.variables[1].maxVal){
        TimeLapse.variables[1].val = inputConversion;
        TL_pause = inputConversion;
      }
      else if (debugM) Serial.println(F("invalid value"));
      conversion ();
    }
};*/

/******/
/*
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
*/

/*class starting_point_Callbacks: public BLECharacteristicCallbacks {
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
*/