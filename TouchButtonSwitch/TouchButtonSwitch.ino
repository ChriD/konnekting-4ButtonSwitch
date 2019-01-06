
/*
  Konnekting - TouchButtonSwitch (0xCDDC)
  Created by Christian Dürnberger (ChriD), 2019
  https://github.com/ChriD
*/


#include "Arduino.h"
#include "KonnektingDevice.h"
#include "TouchButtonSwitch.h"
#include "src/TouchSwitch_4X_V1.h"


// simple version definition for the library
#define LIB_VERSION             "1.0"

// if this define is uncommented, debugging via serial is activated. This should not be active on productive environment!
#define KDEBUG

// for testing purposes without having an active bcu attached we have to skip
// the knx connection and task codes to test the device. This can be done by setting this define
#define BCUDISABLED

// This is the time in miliseconds how long the button should wait after startup/setup to allow touches
// There has to be a enough time for the user to put the frontplate on the switch before recalibration of
// the touch sensor has been done
#define STARTUP_IDLETIME        10000

// define seriald for the Adafruit Itsy-Bitsy M0
// SERIAL_BCU is mandatory to work, the SERIAL_DBG will onlky be used in DEBUG Mode
#define SERIAL_DBG              SERIAL_PORT_USBVIRTUAL
#define SERIAL_BCU              Serial1

// so our touch swutch device is the TouchSwitch_CODENAME_4X_V1 switch wich has 4 touch buttons and 2 proximity
// sensors on top and on the button of the device. So we use the approiate class for that switch
BaseSwitch   *baseSwitch = new TouchSwitch_4X_V1();


uint16_t counter = 0;




void onButtonStateChanged(uint16_t _buttonId, uint16_t _state)
{
  SERIAL_DBG.print(_buttonId);
  SERIAL_DBG.print(" : ");
  SERIAL_DBG.print(_state);
  SERIAL_DBG.print("\n");
}

void onButtonAction(uint16_t _buttonId, uint16_t _type, uint16_t _value)
{
  SERIAL_DBG.print(_buttonId);
  SERIAL_DBG.print(" : ");
  SERIAL_DBG.print(_type);
  SERIAL_DBG.print(" | ");
  SERIAL_DBG.print(_value);
  SERIAL_DBG.print("\n");
}


void onProximityAlert(uint16_t _buttonId, boolean _isProximity, uint16_t _proximityValue, uint16_t _proximityLevel)
{
  counter++;
  SERIAL_DBG.print(counter);
  SERIAL_DBG.print(" # ");
  SERIAL_DBG.print(_buttonId);
  SERIAL_DBG.print(" : ");
  SERIAL_DBG.print(_isProximity);
  SERIAL_DBG.print(" | ");
  SERIAL_DBG.print(_proximityValue);
  SERIAL_DBG.print(" | ");
  SERIAL_DBG.print(_proximityLevel);
  SERIAL_DBG.print("\n");
}



void setup()
{
  // when starting we set the SETUP MODE on the switch with the first _proximityLevel
  // so in most cases some led will be set to a specific color or one of four led is illuminating
  // this is a nice to have to go into troublehooting without having a serial debugger online
  // SETUP MODE 1 is at the very beginning and indicates as power on LED
  baseSwitch->setMode(SWITCH_MODE::SETUP, 1);

  // start the debugging serial and wait for it to be present before doing anything
  // of course we do only activate the serial if we have enabled debugging
  #ifdef KDEBUG
    SERIAL_DBG.begin(115200);
    while (!SERIAL_DBG) {}
    Debug.setPrintStream(&SERIAL_DBG);
    Debug.println(F("KONNEKTING TouchButtonSwitch Library %u"), LIB_VERSION);
  #endif

  baseSwitch->attachCallbackOnButtonAction(makeFunctor((CallbackFunction_ButtonAction*)0,&onButtonAction));
  baseSwitch->attachCallbackOnProximityAlert(makeFunctor((CallbackFunction_ProximityAlert*)0,&onProximityAlert));

  // setup the connection to the KNX-BUS
  #ifndef BCUDISABLED
    initKNX()
    // if we have already an application programm (!isFactorySetting), get the values from the EEPROM
    // and apply them to the switch
    if (!Konnekting.isFactorySetting())
      baseSwitch->initParameters(); // TODO: @@@ How to use Konnekting var? Parm via address?
  #endif

  // SETUP MODE 2 is after connection with the BCU is established
  baseSwitch->setMode(SWITCH_MODE::SETUP, 2);

  // after we have a connection to the bus, we do setup the touch switch and start the calibration
  // of all the touch areas and proximity sensors
  if(!baseSwitch->setup())
  {
    // TODO: add some error code?!
    Debug.println(F("Error initializing Touch Switch"));
  }

  // SETUP MODE 3 is after the setup of the switch has been done
  baseSwitch->setMode(SWITCH_MODE::SETUP, 3);

  baseSwitch->startCalibration();

  // SETUP MODE 4 is after setup is done and calibration is STARTUP_IDLETIME
  // this mode will stay till the STARTUP_IDLETIME is reached
  baseSwitch->setMode(SWITCH_MODE::SETUP, 4);
}


// this method does the initialization of the KONNEKTING Library
// if there is a problem connecting to the bus, the method will not return!
void initKNX()
{
  Konnekting.init(SERIAL_BCU, &progLed, MANUFACTURER_ID, DEVICE_ID, REVISION);
}


// if the prog button is pressed we have to switch the device into the prog mode
// we have to debounce the button because we do not have hardware debounce
volatile unsigned long progButtonDebounceTime = 0;
void progButtonPressed()
{
  unsigned long tempTime = millis();
  if(tempTime >= progButtonDebounceTime)
  {
    if(tempTime - progButtonDebounceTime < 200)
      return;
  }
  progButtonDebounceTime = tempTime;
  Konnekting.toggleProgState();
}

void progLed (bool _isPrg){
  if(_isPrg)
  {
    //touchSwitch->changeMode(TS_MODE_PROG, false);
  }
  else
  {
    //touchSwitch->changeMode(TS_MODE_NORMAL, false);
  }
}


uint64_t loopSum    = 0;
uint64_t loopCount  = 0;


void loop()
{
  #ifdef KDEBUG
    uint64_t startLoop  = micros();
    uint64_t endLoop    = 0;
  #endif

  // we have to call the knx.task form the konnekting library faster then ~ 400us to not miss any telegram
  // in next versions this should be obsolete but for now we have to stay with this pitfall
  #ifndef BCUDISABLED
    Knx.task();
  #endif

  // let the button do its apllication loop tasks
  baseSwitch->task();

  // give some info about the average duration of the loop
  #ifdef KDEBUG
    endLoop = micros();
    loopCount++;
    loopSum += endLoop - startLoop;
    if(loopSum > 1000*1000)
    {
      Debug.println(F("Main Loop timing: %uus"), (loopSum/loopCount));
      loopCount = 0;
      loopSum   = 0;
    }
    if(endLoop - startLoop > 400)
      Debug.println(F("ATTENTION: Main Loop exceeds KNX timing requirement! %uus"), (endLoop - startLoop));
  #endif
}
