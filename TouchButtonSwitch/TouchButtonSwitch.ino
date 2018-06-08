/*
  Konnekting - TouchButtonSwitch (0xCDDC)
  Created by Christian Dürnberger (Chrid), Mai 2018
  https://github.com/ChriD
*/


/* TODO:
  # add temperature and humidity/temp sensor
*/

#include "Arduino.h"
#include "Wire.h"
#include "KonnektingDevice.h"
#include "TouchButtonSwitch.h"
#include "src/CY8TouchSwitch.h"


// we do use the same firmware for 4x or 6x touch switches
// so we have to define which type of sitch is being compiled
#define SWITCHTYPE        4

// used for debugging
// should not be activated on productive environment
#define KDEBUG

// for testing purposes without having a bcu attached we have to skip
// the knx code to test the device. For this we define the NOBCU
//#define NOBCU

#define DEBUGSERIAL       Serial
#define KNX_SERIAL        Serial1
#define PROG_LED_PIN      6
#define PROG_BUTTON_PIN   PC13
#define TC_INTERRUPTPIN   PA8

// the differnt type of switches we do have
#define SWITCHTYPE_4X     4
#define SWITCHTYPE_6X     6

// we do have some global vars
CY8TouchSwitch  *touchSwitch;
bool            startupRecalibrationNeeded = true;
uint64_t        mainLoopEndTime;


void setup()
{

  #ifdef KDEBUG
    DEBUGSERIAL.begin(115200);
    while (!DEBUGSERIAL)
    {
    }
    Debug.setPrintStream(&DEBUGSERIAL);
    Debug.println(F("KONNEKTING TouchButtonSwitch"));
  #endif

  touchSwitch = new CY8TouchSwitch();
  touchSwitch->setup();

  // add buttons for the given type of switch (4x or 6x)
  // TODO: @@@
  touchSwitch->addButton(3, D3, true);
  touchSwitch->addButton(4, D4, true);
  touchSwitch->addButton(5, D11, true);
  touchSwitch->addButton(6, D9, true);

  touchSwitch->changeMode(TS_MODE_STARTUP1, true);
  delay(150);

  // startup I2C
  Wire.begin();

  touchSwitch->changeMode(TS_MODE_STARTUP2, true);
  delay(150);

  // after we have changed the mode of the switch to SETUP, we start setting up the
  // knx device stuff
  #ifndef NOBCU
    knxDeviceSetup();
  #endif

  touchSwitch->changeMode(TS_MODE_STARTUP3, true);
  delay(150);

  // setup the interrupt method for the touch controller
  pinMode(TC_INTERRUPTPIN, INPUT_PULLDOWN);
  attachInterrupt(TC_INTERRUPTPIN, touchControllerInterrupt, FALLING);

  // setup the prog button interrupt for rising edge
  pinMode(PROG_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PROG_BUTTON_PIN), progButtonPressed, RISING);

  // set the callbacks we do like to send to the KNX-Bus
  touchSwitch->setTouchEventCallback(touchEvent);
  touchSwitch->setGestureEventCallback(gestureEvent);
  touchSwitch->setProximityEventCallback(proximityEvent);

  //std::bind(&CY8TouchSwitch::sensorStateEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)

  touchSwitch->changeMode(TS_MODE_STARTUP4, true);
  delay(150);

  // be sure we do set back to the normal state after rebooting
  touchSwitch->changeMode(TS_MODE_NORMAL, true);
}



// if the prog button is pressed we have to switch the device into the prog mode
// this happens if the user is pressing the prog button
void progButtonPressed()
{
  Konnekting.toggleProgState();
}



void progLed (bool state){
  if(state)
    touchSwitch->changeMode(TS_MODE_PROG, false);
  else
    touchSwitch->changeMode(TS_MODE_NORMAL, false);
}



void knxDeviceSetup()
{
  //Konnekting.init(KNX_SERIAL, PROG_BUTTON_PIN, PROG_LED_PIN, MANUFACTURER_ID, DEVICE_ID, REVISION);
  Konnekting.init(KNX_SERIAL, &progLed, MANUFACTURER_ID, DEVICE_ID, REVISION);
  if (!Konnekting.isFactorySetting())
  {
    //typeTemp = (int) Konnekting.getUINT8Param(PARAM_tempSendUpdate);
    //temperature polling interval (ms)
    //intervalTempUser = (long) Konnekting.getUINT32Param(PARAM_tempPollingTime)*1000;
    //minimal difference between previous and current temperature [°C]
    //diffTempUser = (float) Konnekting.getUINT8Param(PARAM_tempDiff)*0.1;
    //intervalHumdUser = (long) Konnekting.getUINT32Param(PARAM_rhPollingTime)*1000; //humidity polling interval (ms)
  }
  Debug.println(F("KNX-Device settings loaded"));
}



// the touch controller will trigger an interrupt when the state of a sensor changes
// we have to reroute the interrupt to the touchSwitch class for further processing
void touchControllerInterrupt()
{
  touchSwitch->interrupt();
}


void touchEvent(uint8_t _sensorId, uint8_t _event, uint8_t _count)
{
  // send the touch event to the knx bus
  //Knx.write(getComObjIndex(COMOBJ_abStatusMovement), 0);
  //Knx.write(getComObjIndex(COMOBJ_abStatusClosePos), DPT1_001_on);
  //Debug.print(" COMOBJ_abStatusMovementClosePos=1", _group);
}


void proximityEvent(uint8_t _sensorId, uint8_t _event)
{
}


void gestureEvent(uint8_t _event)
{

}


// this method will be called when a com object is updated
void knxEvents(byte _index)
{
  /*
  switch (index)
  {
      case COMOBJ_btnShortInput:
            boolShortClick = Knx.read(COMOBJ_btnShortInput);
            Debug.println(F("KNX event: boolShortClick: %d"),boolShortClick);
            break;
        case COMOBJ_btnLongInput:
            boolLongClick = Knx.read(COMOBJ_btnLongInput);
            Debug.println(F("KNX event: boolLongClick: %d"),boolLongClick);
            break;
        case COMOBJ_binD0ValueInput:
            inputStateD0 = Knx.read(COMOBJ_binD0ValueInput);
            Debug.println(F("KNX event: inputStateD0: %d"),inputStateD0);
            break;
        case COMOBJ_binD1ValueInput:
            inputStateD1 = Knx.read(COMOBJ_binD1ValueInput);
            Debug.println(F("KNX event: inputStateD1: %d"),inputStateD1);
            break;
        case COMOBJ_nightMode:
            nightMode = (bool) Knx.read(COMOBJ_nightMode);
            Debug.println(F("KNX event: nightMode: %d"),nightMode);
            break;
        default:
          break;
  }
  */
}




void loop()
{
  // we have to call the knx.task form the konnekting library faster then ~ 400us to not miss any telegram
  // in next versions this should be obsolete but for now we have to stay with itself
  // ATTENTION: Currently we do have some loops which will go above 400us due I2C handling, we have to ceck in
  // production mode if this will lead to problems
  #ifndef NOBCU
    Knx.task();
  #endif

  #ifdef KDEBUG
    uint64_t now = micros();
    if(mainLoopEndTime && now > mainLoopEndTime && now - mainLoopEndTime > 400)
      Debug.println(F("ATTENTION: Main Loop exceeds KNX timing requirement! %uus"), (now - mainLoopEndTime));
    //Debug.println(F("MainLoop %u"), (now - mainLoopEndTime));
  #endif

  // the switch itself must loop eyerytime, no matter of KNX is active or not
  // while KNX connector is not active, the switch is in a special mode and will not raise any touch events
  touchSwitch->task();

  // be sure to reset the touch controller after resetting the main controller and be sure
  // that there is time to "set up" the frontboard finish (reset for recalibration)
  if (startupRecalibrationNeeded &&  millis() > 10000)
  {
    startupRecalibrationNeeded = false;
    touchSwitch->resetTouchController();
  }

  if (Konnekting.isReadyForApplication())
  {
    // TODO: switch to normal mode
  }
  else
  {
    // TODO: set switch auto to PRG mode
  }


  mainLoopEndTime = micros();
}

