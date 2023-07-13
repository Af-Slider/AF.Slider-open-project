//#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"
#include <SH1106.h>
#include "myfont.h"
//#include "EEPROM.h"
#include "TMC220xStepper.h"

#define SW 25 //4
#define DT 26 //2
#define CLK 27 //15

SH1106 display(0x3c, SDA, SCL);   

bool    isConnected = false;
String  caseSel = "";
int     input_speed = 100; // (mm/s)
double  input_acc = 2; // (s) - acelleration total time 
double  input_step = 10; //step for TM and SM (mm)
double  last_inputStep = input_step;
double  TL_pause = 2; //puase between steps fot TM and SM (s)
int     frame_count = 0; //counter to keep track for TM and SM
int     last_direction;
int     input_loop ;
int     input_startPoint;
int     max_frame;  //maximum number of shoot that can be taken
String  IP_address = "00:00:00:00";
bool    start_wifi = false;
bool    goingHome = false;
bool    revaluate = false;

volatile int lastEncoded = 0;
//long lastencoderValue = 0;
int lastMSB = 0;
int lastLSB = 0;

volatile bool updateData = true;
volatile unsigned long timing = 0; //timer generico per encoder
unsigned long firstPress = 0; //timer per pressButton
String lastFunction;

int8_t layer = 0;
int8_t functionMenuSel;
int8_t subMenuSel;
int8_t playSel;
int8_t valueSel;
volatile int8_t sumVal;

typedef struct subMenu{
  String title;     //name of the variable
  double multi = 1; //multiplier to obtain the basic unit (for example 1 for the step - integer - and 0.1 for the time - double)
  double val;       //variable value
  double maxVal;    //variable max value
  double minVal;    //variable min value
  String unit;      //variable dimension unit
};

typedef struct functionMenu{
  String title;
  subMenu variables[5];
};

functionMenu Slider;
functionMenu TimeLapse;
functionMenu StopMotion;
functionMenu Settings;

functionMenu *menuList[4] = { &Slider, &TimeLapse, &StopMotion, &Settings};



void OLED_print(){

  /*
   * Layer 0 --> main menu
   *  Layer 1 --> single function menu (slider, timelapse, etc)
   *  Layer 2 --> variable value selection
   *    Layer 3 --> "play" screen
   */
  
  display.clear();
  int lineSpace = 24;

    if(layer == 0){                                                       //print first layer menu 
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      for (int i; i<4; i++){
        display.drawString(12, (i*16), menuList[i]->title );              // (x pos, y pos, string) --> text align set the distribution of the text starting from the x pos
      }
      display.drawString(0, (functionMenuSel*16), ">" );
    }

    if(layer == 1 || layer == 2){                                          //print second layer menu 
      if (menuList[functionMenuSel]->title != "Settings"){                 //print any sub menu except the "settings" sub menu
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        for (int i; i<3; i++){
          display.drawString(12, (i*lineSpace), menuList[functionMenuSel]->variables[i].title ); //print varaible names 
          if (menuList[functionMenuSel]->variables[i].val != -1){     //print progress bar and value if required
            display.setTextAlignment(TEXT_ALIGN_RIGHT); 
            display.drawString(128, (i*lineSpace), (String)(menuList[functionMenuSel]->variables[i].val + menuList[functionMenuSel]->variables[i].unit));  
            display.setTextAlignment(TEXT_ALIGN_LEFT);
            double progress = (menuList[functionMenuSel]->variables[i].val / menuList[functionMenuSel]->variables[i].maxVal ) * 100;
            display.drawProgressBar(12, ((i*lineSpace)+15), 112, 8, progress);
          }
          
        }
        if (layer == 1) display.drawString(0, (subMenuSel*lineSpace), ">"); //print "inactive" cursor 
        else  {                                                             //print "active" cursor
          display.setFont(Roboto_Bold_12);
          display.drawString(0, (subMenuSel*lineSpace), "*"); 
          display.setFont(Roboto_Light_12);
        }

          if (subMenuSel == 3) display.setFont(Roboto_Bold_12);
          display.setTextAlignment(TEXT_ALIGN_RIGHT);
          display.drawString(126, 2*lineSpace, menuList[functionMenuSel]->variables[3].title);
          display.setTextAlignment(TEXT_ALIGN_LEFT);
          display.setFont(Roboto_Light_12);

      }
      
      else {                                                                //print "settings" sub menu
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        lineSpace = 12;
        for (int i; i<5; i++){
          display.drawString(12, (i*lineSpace), menuList[functionMenuSel]->variables[i].title + menuList[functionMenuSel]->variables[i].unit); 
          
          if (menuList[functionMenuSel]->variables[i].title != "Back"){
            if (menuList[functionMenuSel]->variables[i].title == "Loop"){ 
              display.setTextAlignment(TEXT_ALIGN_RIGHT); 
              display.drawString(128, (i*lineSpace), (String)((int)menuList[functionMenuSel]->variables[i].val) );  
              display.setTextAlignment(TEXT_ALIGN_LEFT);
            }
            else if (menuList[functionMenuSel]->variables[i].title != "Firmware update"){    
              if (menuList[functionMenuSel]->variables[i].val) display.setTextAlignment(TEXT_ALIGN_RIGHT), display.drawString(128, ((i*lineSpace)), "ON"), display.setTextAlignment(TEXT_ALIGN_LEFT);
              else display.setTextAlignment(TEXT_ALIGN_RIGHT), display.drawString(128, ((i*lineSpace)), "OFF"), display.setTextAlignment(TEXT_ALIGN_LEFT);       
            }
          }
        }
        if (layer == 1) display.drawString(0, (subMenuSel*lineSpace), ">");   //print "inactive" cursor on layer 1
        else  {                                                               //print "active" cursor on layer 2
          display.setFont(Roboto_Bold_12);
          display.drawString(0, (subMenuSel*lineSpace), "*"); 
          display.setFont(Roboto_Light_12);
        }
        lineSpace = 24;
      }
    }

    if(layer == 3){    
      if (menuList[functionMenuSel]->title != "Settings"){                      //print any "play" screen but stop-motion
        int progress = ((float) (stepper->getCurrentPosition()) / (float) END) * 100;
        display.drawProgressBar(13, 12, 100, 10, progress);                     //draw slider position
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        if (menuList[functionMenuSel]->title != "StopMotion" || playSel == 0) display.setFont(Roboto_Bold_12);
        display.drawString(128/2, 28, "STOP");
        display.setFont(Roboto_Light_12);
        display.setTextAlignment(TEXT_ALIGN_LEFT);
      }
      
      if (menuList[functionMenuSel]->title == "StopMotion") {                  //diplay StopMotion play screen
        if (playSel == 1) display.setFont(Roboto_Bold_12);
        display.drawString(13, 48, "PREV");
        display.setFont(Roboto_Light_12);
        
        if (playSel == 2) display.setFont(Roboto_Bold_12);
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        display.drawString(113, 48, "NEXT");
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(Roboto_Light_12);
      }

      else if (menuList[functionMenuSel]->title == "Settings"){                 //print firmware update screen
        if (IP_address == "00:00:00:00") {
          display.setTextAlignment(TEXT_ALIGN_CENTER);
          display.drawString(128/2, 64/2 - 8, "Connecting");
          display.drawString(128/2, 64/2 + 8, "to Wi-Fi");
        }
        else {
          display.setTextAlignment(TEXT_ALIGN_LEFT);
          display.drawString(1, 0, "version");
          display.drawString(1, 16, "IP");
          display.drawString(1, 48, "> Back");
          
          display.setTextAlignment(TEXT_ALIGN_RIGHT);
          display.drawString(126, 0, (String)FW);
          display.drawString(126, 16, IP_address); 
        }
    }
    
    }

    if(layer == -1){                                                            //print BLE connected screen
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(Roboto_Bold_12);
      display.drawString(128/2, 30, "Bluetooth");
      display.drawString(128/2, 44, "connected");
      display.setFont(Roboto_Light_12);
    }
    
    
  display.display();
}

void UpdateValue () { //rotary encoder turn
  
  switch (layer) {
    case 0:
      functionMenuSel += sumVal;
      if (functionMenuSel > 3) functionMenuSel = 0;
      if (functionMenuSel < 0) functionMenuSel = 3;
      break;
      
    case 1:
      subMenuSel += sumVal;
      if (menuList[functionMenuSel]->title != "Settings"){
        if (subMenuSel > 3) subMenuSel = 0;
        if (subMenuSel < 0) subMenuSel = 3;
      }
      else {
        if (subMenuSel > 4) subMenuSel = 0;
        if (subMenuSel < 0) subMenuSel = 4;
      }
      break;

    case 2:
      if (millis() - timing > 50) {
      menuList[functionMenuSel]->variables[subMenuSel].val += sumVal*menuList[functionMenuSel]->variables[subMenuSel].multi;
      }
      else menuList[functionMenuSel]->variables[subMenuSel].val += sumVal*menuList[functionMenuSel]->variables[subMenuSel].multi*10;

      if (menuList[functionMenuSel]->variables[subMenuSel].val < menuList[functionMenuSel]->variables[subMenuSel].minVal) menuList[functionMenuSel]->variables[subMenuSel].val = menuList[functionMenuSel]->variables[subMenuSel].minVal;
      else if (menuList[functionMenuSel]->variables[subMenuSel].val > menuList[functionMenuSel]->variables[subMenuSel].maxVal) menuList[functionMenuSel]->variables[subMenuSel].val = menuList[functionMenuSel]->variables[subMenuSel].maxVal;
        
      break;

    case 3:
      playSel += sumVal;
      if (playSel > 2) playSel = 0;
      if (playSel < 0) playSel = 2;
      break;
  }
    updateData=true;
    caseSel =  menuList[functionMenuSel]->variables[subMenuSel].title; //assign caseSel value
    timing=millis();
}


void UpdateEncoder() {
  
  int MSB = digitalRead(DT); //MSB = most significant bit
  int LSB = digitalRead(CLK); //LSB = least significant bit

  int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  //if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  //if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;
  
  if (sum == 0b1000){
    if (millis() - timing>25) {
      sumVal = 1;
      UpdateValue();
    }

  }
  if (sum == 0b0010) {
    if (millis() - timing>25) {
      sumVal = -1;
      UpdateValue();
    }

  }

  lastEncoded = encoded; //store this value for next time
  //play = false;

}

void conversion () {
  
  int speed_step_s = ((input_speed / (3.14 * pulley_diam)) * 200 * MICROSTEPS);
  
  speed_us = 1E6 / speed_step_s;
  acc = (float) speed_step_s / (float) input_acc;
  loop_steps = 200 * MICROSTEPS * input_loop / (3.14 * pulley_diam);
  END = loop_steps;

  if (debugM) Serial.print("END = ");
  if (debugM) Serial.println(END);

  int compare = 200 * MICROSTEPS * input_startPoint / (3.14 * pulley_diam); 
  
  if (HOME != compare) {
    bool store_direction = direction;
    HOME = compare;
    if (debugM) Serial.print(F("compare: "));
    if (debugM) Serial.println(compare);
    if (debugM) Serial.print(F("current position: "));
    if (debugM) Serial.println(stepper->getCurrentPosition());
    if (stepper->getCurrentPosition() > compare) direction = 1;
    else direction = 0;
    driver.shaft(direction); 
    if (!play) stepper->moveTo(HOME, 1);
    direction = store_direction;
    driver.shaft(direction);
  }

  
  TL_steps = 200 * MICROSTEPS * input_step / (3.14 * pulley_diam);
  int currentPos = TL_steps * frame_count;
  if (last_inputStep != input_step && menuList[functionMenuSel]->title != "Slider") revaluate = true;
  if (lastFunction != menuList[functionMenuSel]->title && menuList[functionMenuSel]->title != "Slider") revaluate = true;
  if (menuList[functionMenuSel]->title == "Slider") revaluate = false;
  last_inputStep = input_step;
  lastFunction = menuList[functionMenuSel]->title;
  setMovement();
  
}


void assignVal () {
    
  if (caseSel == "speed") input_speed = menuList[functionMenuSel]->variables[subMenuSel].val;    
  else if (caseSel == "acceleration") input_acc = menuList[functionMenuSel]->variables[subMenuSel].val;
  else if (caseSel == "step") input_step = menuList[functionMenuSel]->variables[subMenuSel].val;
  else if (caseSel == "pause") TL_pause = menuList[functionMenuSel]->variables[subMenuSel].val;
  else if (caseSel == "Loop") input_loop = menuList[functionMenuSel]->variables[subMenuSel].val;
  else if (caseSel == "No-stop loop") run_cont = menuList[functionMenuSel]->variables[subMenuSel].val;
  //else if (debugM) Serial.println (F("no valid input found"));
  
  conversion();

  if (layer == 3 && menuList[functionMenuSel]->title != "StopMotion") setMovement(), play = true;
  if (layer == 3 && menuList[functionMenuSel]->title == "Settings") setMovement(), start_wifi = true;
  else if (layer != 3) play = false, start_wifi = false, stepper->stopMove();

}
