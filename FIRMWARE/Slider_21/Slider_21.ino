/*
 * Firmware developed by Fulvio Airaghi and based on several open source library under MIT licence. 
 * The present code is distributed by MIT licence as well, WITHOUT the permision of commercial use. 
 * 
 * 
 * Firmware version 1.3
 */
#define FW 1.3

#include "OLED_encoder.h"
#include "BLE.h"

unsigned long print_timer;
unsigned int TL_timer;

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

const char* ssid = "Slider_wifi";
const char* password = "Afslider";

AsyncWebServer server(80);

void setup() { 
  
  pulley_diam = 12.20; // diameter of the pulley, in mm
  
  Serial.begin(500000); 


//____________________________________________________________________OTA server
/*
 * Vedi "settings --> Firmware update"
 */

  
//____________________________________________________________________BLE

  BLEDevice::init("AF.Slider");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Define characteristics for this service, and assign read/write properties
  
  BLECharacteristic *start_stop_Characteristic = pService->createCharacteristic(
                                         start_stop_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  start_stop_Characteristic->setCallbacks(new StartStopCallbacks());
  
  BLECharacteristic *speed_Characteristic = pService->createCharacteristic(
                                         speed_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  speed_Characteristic->setCallbacks(new speedCallbacks());

  BLECharacteristic *acc_Characteristic = pService->createCharacteristic(
                                         acc_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  acc_Characteristic->setCallbacks(new accCallbacks());

  BLECharacteristic *loop_Characteristic = pService->createCharacteristic(
                                         loop_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  loop_Characteristic->setCallbacks(new loop_Callbacks());

  BLECharacteristic *starting_point_Characteristic = pService->createCharacteristic(
                                         starting_point_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  starting_point_Characteristic->setCallbacks(new starting_point_Callbacks());
  
  
  pServer->setCallbacks(new MyServerCallbacks());

  start_stop_Characteristic->setValue("Hello World");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();

//____________________________________________________________________OLED & MENU & ENCODER setup
  Slider.title = "Slider";
  Slider.variables[0].title = "speed";
  Slider.variables[0].val = 20;
  Slider.variables[0].maxVal = 105;
  Slider.variables[0].minVal = 0;
  Slider.variables[0].unit = "mm/s";
  Slider.variables[1].title = "acceleration";
  Slider.variables[1].multi = 0.1;
  Slider.variables[1].val = 2.5;
  Slider.variables[1].maxVal = 10;
  Slider.variables[1].minVal = 0.2;
  Slider.variables[1].unit = "s";
  Slider.variables[2].title = "Back";
  Slider.variables[2].val = -1;                              //used to avoid progress bar print
  Slider.variables[3].title = "Play";

  TimeLapse.title = "TimeLapse";
  TimeLapse.variables[0].title = "step";
  TimeLapse.variables[0].multi = 0.1;  
  TimeLapse.variables[0].val = 3;
  TimeLapse.variables[0].maxVal = 400;
  TimeLapse.variables[0].minVal = 0;
  TimeLapse.variables[0].unit = "mm";
  TimeLapse.variables[1].title = "pause";
  TimeLapse.variables[1].multi = 0.1;  
  TimeLapse.variables[1].val = 4.2;
  TimeLapse.variables[1].maxVal = 360;
  TimeLapse.variables[1].minVal = 0;
  TimeLapse.variables[1].unit = "s";
  TimeLapse.variables[2].title = "Back";
  TimeLapse.variables[2].val = -1;
  TimeLapse.variables[3].title = "Play";

  StopMotion.title = "StopMotion";
  StopMotion.variables[0].title = "step";
  StopMotion.variables[0].multi = 0.1;  
  StopMotion.variables[0].val = 3;
  StopMotion.variables[0].maxVal = 400;
  StopMotion.variables[0].minVal = 0;
  StopMotion.variables[0].unit = "mm";
  StopMotion.variables[1].val = -1;                            
  StopMotion.variables[2].title = "Back";
  StopMotion.variables[2].val = -1;
  StopMotion.variables[3].title = "Play";
  
  Settings.title = "Settings";
  Settings.variables[0].title = "Loop"; 
  Settings.variables[0].val = 500;
  Settings.variables[0].maxVal = 1200;
  Settings.variables[0].minVal = 0;
  Settings.variables[0].unit = " (mm)"; 
  Settings.variables[1].title = "No-stop loop";
  Settings.variables[1].val = 1;
  Settings.variables[1].maxVal = 1;
  Settings.variables[1].minVal = 0;
  Settings.variables[2].title = "homing";
  Settings.variables[2].val = 0;
  Settings.variables[2].maxVal = 1;
  Settings.variables[2].minVal = 0;
  Settings.variables[3].title = "Firmware update";
  Settings.variables[4].title = "Back";
  Settings.variables[4].val = -1;

  pinMode (DT, INPUT_PULLUP);
  pinMode (CLK, INPUT_PULLUP);
  pinMode (SW, INPUT_PULLUP);
  pinMode (BT_LED, OUTPUT);
  pinMode (33, OUTPUT);
  
  attachInterrupt(DT, UpdateEncoder, CHANGE);
  attachInterrupt(CLK, UpdateEncoder, CHANGE);
  attachInterrupt(SW, up, FALLING);

  display.flipScreenVertically();
  display.init();

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(Roboto_Light_12);
  display.drawString(128/2, (64/2 - 12), "AF.Slider");
  display.display();
  delay (1000); 
  
//____________________________________________________________________Stepper
  Serial2.begin(500000);        // SW UART drivers
  
  driver.begin();               // SPI: Init CS pins and possible SW SPI pins
  driver.toff(TOFF_VALUE);      // Sets the slow decay time (off time) [1... 15] --> 0 motor off, enything else motor on
  driver.TCOOLTHRS(0xFFFFF);    // 20bit max - Lower threshold velocity for switching on smart energy CoolStep and StallGuard to DIAG output
  driver.blank_time(24);        // Comparator blank time. This time needs to safely cover the switching event and the duration of the ringing on the sense resistor
  driver.semin(10);             // CoolStep lower threshold [0... 15]. If SG_RESULT goes below this threshold, CoolStep increases the current to both coils.
                                // 0: disable CoolStep
  driver.semax(1);              // CoolStep upper threshold [0... 15]. If SG is sampled equal to or above this threshold enough times,
                                // CoolStep decreases the current to both coils.
  driver.rms_current(stepperCurrent, 0.3);  //RMS current, hold current multiplier, sense resistor value - (required, optional, optional)
  driver.microsteps(MICROSTEPS);
  driver.sedn(0b101);           // Sets the number of StallGuard2 readings above the upper threshold necessary for each current decrement of the motor current
  
  driver.SGTHRS(STALL_VALUE);  // StallGuard4 threshold [0... 255] level for stall detection.
                                // A higher value gives a higher sensitivity. A higher value makes StallGuard4 more sensitive and requires less torque to
                                // indicate a stall.

  engine.init();
  stepper = engine.stepperConnectToPin(stepPinStepper);
  if (stepper) {
    stepper->setDirectionPin(dirPinStepper, 0, 0); //setDirectionPin(uint8_t dirPin, bool dirHighCountsUp, uint16_t dir_change_delay_us)
    stepper->setEnablePin(enablePinStepper);
    stepper->setAutoEnable(true);

    stepper->setSpeedInUs(homing_speed);  // the parameter is us/step !!!
    stepper->setAcceleration(homing_speed); // the parameter is steps/s²
  }
  
//___________________________________________________________________test driver comunication

  Serial.println(F("\nTesting connection..."));
  uint8_t result = driver.test_connection();

  if (result) {
    Serial.println("failed!");
    Serial.print("Likely cause: ");

    switch(result) {
        case 1: Serial.println("loose connection"); break;
        case 2: Serial.println("no power"); break;
    }
    Serial.println("Fix the problem and reset board.");

    // We need this delay or messages above don't get fully printed out
    delay(100);
    //abort();
    //while (1);
  }
  Serial.println("OK");

  
//___________________________________________________________________Homing routine

  delay(500);

  direction = 0;
  driver.shaft(!direction);
  
//  Serial.println(F("start homing"));
  homing();
  
//  Serial.println(F("fine homing"));
//  Serial.print(F("current Position: "));
//  Serial.println(stepper->getCurrentPosition());

  assignVal();
  delay(200);
  
  stepper->setAcceleration(1000);
  stepper->setSpeedInUs(600); 
  direction = 1;
  driver.shaft(direction);
  stepper->moveTo(HOME, 1);
  Settings.variables[2].val = 0;
  
//  Serial.print(F("steps tot: "));
//  Serial.println(END);
//  Serial.print(F("lunghezza loop: "));
//  Serial.println(Settings.variables[0].val);

  double temp = Settings.variables[0].val;
  char txString[8];
  dtostrf(temp, 1, 2, txString);

  loop_Characteristic->setValue(txString);

  timing = millis();
  input_speed = Slider.variables[0].val;
  conversion();
  
}//___________________________________________________________________end setup


void loop() {

  if (updateData) {
    OLED_print();
    assignVal();
    updateData = false;
  }

  if (start_wifi){
    if (WiFi.status() != WL_CONNECTED) firmware_update();
  }

  if (goingHome && stepper->getCurrentPosition() != HOME){
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(128/2, (64/2 - 12), "going home");
    display.display();  
    goingHome = false;
    delay(1000);
    OLED_print();
  }

  if (revaluate && play){
    bool store_direction = direction;
    max_frame = floor (END / TL_steps);
    if (direction = 0) frame_count = floor (stepper->getCurrentPosition() / TL_steps) - 1;
    else frame_count = ceil (stepper->getCurrentPosition() / TL_steps) - 1;
    int prevPos = TL_steps * frame_count;
    driver.shaft(!direction);
    stepper->moveTo(prevPos,1);
    direction = store_direction;
    driver.shaft(direction);
    delay(50);
    revaluate = false;
  }
  
  if (play){

    int selector;

    if ( menuList[functionMenuSel]->title == "Slider" ) selector = 0;
    if ( menuList[functionMenuSel]->title == "TimeLapse" ) selector = 1;
    if ( menuList[functionMenuSel]->title == "StopMotion" ) selector = 2;
    if ( menuList[functionMenuSel]->title == "Settings" ) selector = 3;

    max_frame = floor (END / TL_steps);
    if (selector == 1){
      if (stepper->getCurrentPosition() != 0 && frame_count == 0) frame_count = floor (stepper->getCurrentPosition() / TL_steps);
    }
    switch (selector){
      
      // Slider
      case 0: 
      if (stepper->getCurrentPosition() == END) direction = 1; 
      else if (stepper->getCurrentPosition() == HOME) direction = 0; 
      driver.shaft(direction);
      
        if (run_cont){
          if (!stepper->isRunning()){
            if (direction) stepper->moveTo(HOME);
            else stepper->moveTo(END);
          }
        }
        else {
          if (direction) {
            stepper->moveTo(HOME);
            if (stepper->getCurrentPosition() == HOME) play = false, layer = 1, OLED_print();
          }
          else {
            stepper->moveTo(END);
            if (stepper->getCurrentPosition() == END) play = false, layer = 1, OLED_print();
          }
        }
    
        if (millis() - print_timer > 50){
          OLED_print();
          print_timer = millis();
        }
      break;

      //Time Lapse
      case 1:
        if (!run_cont) {
          if (direction && frame_count == 0) play = false, layer = 1, OLED_print();
          if (!direction && frame_count == max_frame) play = false, layer = 1, OLED_print();
        }
        if (frame_count == max_frame) direction = 1;
        else if (frame_count == 0) direction = 0;
        driver.shaft(direction);
        
        
        if (millis() - TL_timer >= TL_pause * 1000) {
          if (direction) stepper->moveTo(stepper->getCurrentPosition() - TL_steps, 1), frame_count--;
          else stepper->moveTo(stepper->getCurrentPosition() + TL_steps, 1), frame_count++;
          TL_timer = millis();
          
          /** CORRECT WITHOUT DELAY*/
          digitalWrite(33, HIGH);
          delay(50);
          digitalWrite(33, LOW);
          /** CORRECT WITHOUT DELAY*/
          
          // Serial.print("max_frame ");
          // Serial.println(max_frame);
          // Serial.print("frame_count ");
          // Serial.println(frame_count);
        }
      break;

      //Stop Motion
      case 2: 
        if (frame_count == max_frame) direction = 1;
        else if (frame_count == 0) direction = 0;
        driver.shaft(direction);

        if (playSel == 2) {
          if (direction == 1 && stepper->getCurrentPosition() - TL_steps >= HOME) {
            stepper->moveTo(stepper->getCurrentPosition() - TL_steps, 1), frame_count--;
            digitalWrite(33, HIGH);
            delay(50);
            digitalWrite(33, LOW);
          }
          else if (direction == 0 && stepper->getCurrentPosition() + TL_steps <= END) {
            stepper->moveTo(stepper->getCurrentPosition() + TL_steps, 1), frame_count++;
            digitalWrite(33, HIGH);
            delay(50);
            digitalWrite(33, LOW);
          }
          else temp_print();
        }
        else if (playSel == 1) {
          direction = !direction;
          driver.shaft(direction);
          if (!last_direction == 1 && stepper->getCurrentPosition() - TL_steps >= HOME) {
            stepper->moveTo(stepper->getCurrentPosition() - TL_steps, 1), frame_count--;
            digitalWrite(33, HIGH);
            delay(50);
            digitalWrite(33, LOW);
          }
          else if (!last_direction == 0 && stepper->getCurrentPosition() + TL_steps <= END) {
            stepper->moveTo(stepper->getCurrentPosition() + TL_steps, 1), frame_count++;
            digitalWrite(33, HIGH);
            delay(50);
            digitalWrite(33, LOW);
          }
          else temp_print();
          direction = !direction;
          driver.shaft(direction); 
        }
        last_direction = direction;
        play = false;
        
      /*  for PCB with direction pin
       *   
        if      (playSel == 2) {                           //next
          if (direction == 0) stepper->moveTo(stepper->getCurrentPosition() + TL_steps), frame_count++;
          else stepper->moveTo(stepper->getCurrentPosition() - TL_steps), frame_count--;
          //else temp_print();
        }
        else if (playSel == 1) {                           //prev
          if (direction == 1) stepper->moveTo(stepper->getCurrentPosition() + TL_steps), frame_count++;
          else stepper->moveTo(stepper->getCurrentPosition() - TL_steps), frame_count--;
        }

        digitalWrite(33, HIGH);
        delay(50);
        digitalWrite(33, LOW);
        play = false;
        
        */
        
      break;

      //Setting menu
      case 3:
//        direction = 0;
//        driver.shaft(direction);
//        stepper->moveTo(0, 1); 
      break; 
    }
  }

    if (stepper->getCurrentPosition() == stepper->targetPos() && menuList[functionMenuSel]->title != "Slider") {
      OLED_print();
    }

  
  if (menuList[3]->variables[2].val && layer == 1) {
  delay(500);
  direction = 0;
  driver.shaft(!direction);
  
  Serial.println(F("start homing"));
  homing();
  
  Serial.println(F("fine homing"));
  Serial.print(F("current Position: "));
  Serial.println(stepper->getCurrentPosition());

  assignVal();
  delay(200);
  
  stepper->setAcceleration(1000);
  stepper->setSpeedInUs(600); 
  direction = 1;
  driver.shaft(direction);
  stepper->moveTo(HOME, 1);
  Settings.variables[2].val = 0;
  
  Serial.print(F("steps tot: "));
  Serial.println(END);
  Serial.print(F("lunghezza loop: "));
  Serial.println(Settings.variables[0].val);

  double temp = Settings.variables[0].val;
  char txString[8];
  dtostrf(temp, 1, 2, txString);

  }
  
}

void temp_print() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(128/2, 64/2, "LOOP ENDED");
  display.display();
  play = false;
  delay (1000);
  OLED_print();
}

void firmware_update() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  IP_address = WiFi.localIP().toString();
  Serial.print("IP address: ");
  Serial.println(IP_address);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Welcome to AF.Slider firmware update page! Please go to yourIPaddress/update");
  });

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
  server.begin();
  Serial.println("HTTP server started");

}

void homing() {
  
  int shortSum = 0;
  int longSum = 0;
  int readingCounter = 0;
  int readingCounterLong = 0;
  int SG_short_average = 350;
  int SG_long_average = 350; 
  int stallThreshold = 150;
  static uint32_t last_time = 0;
  bool isHoming = true;
  bool endHoming = false;
  int tempPos = 0; 
  
  driver.rms_current(400); // mA
  driver.microsteps(4);

  stepper->setAcceleration(100000);
  stepper->setSpeedInUs(700);
  
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(128/2, 64/2, "homing..." );  
  display.display();

  driver.shaft(direction); 
  stepper->move(300000);
  
  while(isHoming){
    
    if ((SG_short_average < stallThreshold && stepper->getCurrentPosition() > 1400 && endHoming) || (!endHoming && SG_short_average < stallThreshold && stepper->getCurrentPosition() > 100)) { 
      stepper->forceStop();
      Serial.println("stall detected");
      SG_short_average = 350;
      SG_long_average = 350;
      delay(500);
      direction = !direction;
      driver.shaft(direction);
      stepper->move(700,1);
      delay(500);
      if (!endHoming) {
        stepper->setCurrentPosition(0);
        stepper->move(30000);
      }
      else {
        tempPos = stepper->getCurrentPosition();
        int loopConv = (tempPos - 400) * 3.14 * pulley_diam / (200 * MICROSTEPS);
        Settings.variables[0].val = loopConv;
        input_loop = Settings.variables[0].val;
        Settings.variables[0].maxVal = Settings.variables[0].val;
        stepper->setCurrentPosition(500);
        isHoming = false;
      }
      endHoming = true;
      last_time = millis();
    }
  
    if((millis()-last_time) > 0) { // run every 0s
      last_time = millis();
      
      shortSum = shortSum + driver.SG_RESULT();
      longSum = longSum + driver.SG_RESULT();
      readingCounter ++;
      readingCounterLong ++;

      if (readingCounterLong > 18){
        SG_long_average = longSum / 18;
        longSum = 0;
        readingCounterLong = 0;
      }

      if (SG_long_average > 400) stallThreshold = SG_long_average * 0.9;
      else stallThreshold = SG_long_average * 0.7;

      if (readingCounter > 4){
        SG_short_average = shortSum / 4;
        shortSum = 0;
        readingCounter = 0;
      }
      
      // Serial.print("average:");
      // Serial.print(SG_short_average);
//      Serial.print(",");
//      Serial.print("long_average:");
//      Serial.print(SG_long_average);
      // Serial.print(",");
      // Serial.print("Threshold:");
      // Serial.println(stallThreshold);
      // Serial.print("current: ");
      // Serial.println(driver.DRV_STATUS());
      
    }
  }
  setMovement();
}






void menuSel () {   //rotary encoder click
  
  if (millis() - firstPress > 2000 && millis() - firstPress < 9000 && layer < 2 ) {
    Serial.println("Moving to HOME");
    direction = 1;
    driver.shaft(direction);
    stepper->setSpeedInUs(480);
    stepper->moveTo(HOME);
    direction = 0;
    frame_count = 0;
    setMovement();
    goingHome = true;
  }
  
  else if (millis() - firstPress > 80){
    
    if (menuList[functionMenuSel]->variables[subMenuSel].title != "Play" && menuList[functionMenuSel]->variables[subMenuSel].title != "Firmware update"){
      if (layer < 2 ) layer++; 
      else layer--; 
    }
    else {  
      if (layer < 2 ) layer = 3; 
      else {
        if (playSel == 0 || menuList[functionMenuSel]->title != "StopMotion") layer = 1;  //back to function menu (STOP)
        else if (playSel == 1) play = true; //prev button clicked
        else if (playSel == 2) play = true;//next button clicked
      }
    }
    if (layer==2 && menuList[functionMenuSel]->variables[subMenuSel].title == "Back") layer = 0; //back to main manu when "back" variable is clicked
    updateData = true;
  }
  attachInterrupt(SW, up, FALLING);
}

void up () {
  if (millis() - timing > 100) {
    firstPress = millis();
    attachInterrupt(SW, menuSel, RISING);
  }
  timing = millis();
}
