#include "FastAccelStepper.h"
#include <TMCStepper.h>

//____________________________________________________________________STEPPER
#define STALL_VALUE       120 // [0... 255] comparison value to detect stall -  the higher the value, the higher the sensitivity
#define TOFF_VALUE        4 // [1... 15]
//#define STALL_VALUE      80 // StallGuard4 threshold [0... 255] level for stall detection. It compensates for
                            // motor specific characteristics and controls sensitivity. A higher value gives a higher
                            // sensitivity. A higher value makes StallGuard4 more sensitive and requires less torque to
                            // indicate a stall. The double of this value is compared to SG_RESULT.
                            // The stall output becomes active if SG_RESULT fall below this value.
                            
#define stepperCurrent  720 // mA
#define MICROSTEPS        4

#define SERIAL_PORT Serial2 // TMC2208/TMC2224 HardwareSerial port (RX2 & TX2)
#define R_SENSE 0.11  // SilentStepStick series use 0.11     Watterott TMC5160 uses 0.075
#define DRIVER_ADDRESS 0b00 // TMC2209 Driver address according to MS1 and MS2

// Select your stepper driver type
TMC2209Stepper driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDRESS);
//TMC2209Stepper driver(SW_RX, SW_TX, R_SENSE, DRIVER_ADDRESS);

using namespace TMC2209_n;

#define dirPinStepper 18
#define enablePinStepper 7
#define stepPinStepper 13 //5

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;

bool    play = false;
int     speed_us;
int     acc;
int     loop_steps;
bool    run_cont = true;
double  TL_steps;
double  pulley_diam; 
bool    direction;
int     HOME = 0;
int     END ;

int homing_speed = 900; //us
int homing_acc = 100000;


void setMovement() {

  stepper->setSpeedInUs(speed_us);  // the parameter is us/step !!!
  //stepper->setSpeedInHz(500);       // 500 steps/s
  stepper->setAcceleration(acc);
}
