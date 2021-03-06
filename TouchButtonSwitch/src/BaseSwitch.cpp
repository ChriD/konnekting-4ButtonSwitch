/*
  BaseSwitch.cpp
  Created by Christian Dürnberger, 2019
  Released into the public domain.
*/

#include "Arduino.h"
#include "BaseSwitch.h"


BaseSwitch::BaseSwitch()
{
  for(int8_t i=0; i<=SWITCH_MAX_BUTTONCOUNT; i++)
    this->buttons[i] = NULL;

  this->mode      = SWITCH_MODE::UNDEFINED;
  this->modeLevel = 0;

  this->lastTaskRunTime = 0;

  this->callback_onButtonAction   = NULL;
  this->callback_onProximityAlert = NULL;
  this->callback_onModeChange     = NULL;
  this->callback_onEnvDataUpdated = NULL;

  this->maxButtonIdx = -1;

  this->envSensorsSettings.temperature            = false;
  this->envSensorsSettings.temperatureAdj         = 0.0;
  this->envSensorsSettings.temperaturePeriod      = 1000*60*3;
  this->envSensorsSettings.temperatureWarningHigh = 300;
  this->envSensorsSettings.temperatureWarningLow  = -300;
  this->envSensorsSettings.humidity               = false;
  this->envSensorsSettings.humidityAdj            = 0.0;
  this->envSensorsSettings.humidityPeriod         = 1000*60*30;
  this->envSensorsSettings.pressure               = 0;

  this->curEnvData.temperature          = 0;
  this->curEnvData.humidity             = 0;
  this->curEnvData.pressure             = 0;

  this->lightningSettings.stdR          = 255;
  this->lightningSettings.stdG          = 0;
  this->lightningSettings.stdB          = 255;
  this->lightningSettings.stdBrightness = 10;

  this->speakerSettings.clickFeedbackEnabled    = true;
  this->speakerSettings.clickFeedbackFrequency  = 4000;
  this->speakerSettings.clickFeedbackDuration   = 250;

}


BaseSwitch::~BaseSwitch()
{
}


boolean BaseSwitch::setup()
{
  boolean ret;
  // let the child classes init the buttons (add the buttons)
  this->initButtons();
  // when init is done we need to setup all the buttons for duty
  ret = this->setupButtons();

  return ret;
}


void BaseSwitch::attachCallbackOnButtonAction(const CallbackFunction_ButtonAction &_callback)
{
  this->callback_onButtonAction = _callback;
}

//
void BaseSwitch::attachCallbackOnProximityAlert(const CallbackFunction_ProximityAlert &_callback)
{
  this->callback_onProximityAlert = _callback;
}


void BaseSwitch::attachCallbackOnModeChange(const CallbackFunction_ModeChange &_callback)
{
  this->callback_onModeChange = _callback;
}

void BaseSwitch::attachCallbackOnEnvDataUpdated(const CallbackFunction_EnvDataUpdated &_callback)
{
  this->callback_onEnvDataUpdated = _callback;
}


uint16_t BaseSwitch::addButton(Button* _button, uint16_t _buttonId)
{
  this->maxButtonIdx++;
  this->buttons[this->maxButtonIdx] = _button;
  // set a button id if not specified by the user
  if(!_buttonId)
    this->buttons[this->maxButtonIdx]->parmId(this->maxButtonIdx + 1);
  else
    this->buttons[this->maxButtonIdx]->parmId(_buttonId);
  return this->maxButtonIdx;
}


Button* BaseSwitch::getButtonByIndex(uint16_t _index)
{
  if(_index > this->maxButtonIdx)
    return NULL;
  return this->buttons[_index];
}

Button* BaseSwitch::getButtonById(uint16_t _id)
{
  for(uint8_t i=0; i<=this->maxButtonIdx; i++)
  {
    if(this->buttons[i]->parmId() == _id)
      return this->buttons[i];
  }
  return NULL;
}


void BaseSwitch::initButtons()
{
  // TODO: @@@ has to be overwritten
}


boolean BaseSwitch::setupButtons()
{
  boolean ret = true;
  for(uint8_t i=0; i<=this->maxButtonIdx; i++)
  {
    // setup the callbacks for the button. All buttons use the same callback method
    // we can distinguish between the buttons due to the button ID provided by the user or the 'addButton' method
    this->buttons[i]->attachCallbackOnButtonAction(makeFunctor((CallbackFunction_ButtonAction*)0, *this, &BaseSwitch::onButtonAction));
    this->buttons[i]->attachCallbackOnButtonStateChanged(makeFunctor((CallbackFunction_ButtonStateChanged*)0, *this, &BaseSwitch::onButtonStateChanged));
    this->buttons[i]->attachCallbackOnProximityAlert(makeFunctor((CallbackFunction_ProximityAlert*)0, *this, &BaseSwitch::onProximityAlert));
    // do the setup routine for the button
    ret = ret && this->buttons[i]->setup();
  }
  return ret;
}

void BaseSwitch::setButtonParameters(uint16_t _buttonId, ButtonParms _parameters)
{
  Button* button = this->getButtonById(_buttonId);
  if(!button)
    return;
  button->setButtonParameters(_parameters);
}


void BaseSwitch::startCalibration()
{
  // start calibration on all buttons, no matter if they are calibrateable or not
  // the method is in the base class and if no calibration is needed it wont be overwritten
  for(uint8_t i=0; i<=this->maxButtonIdx; i++)
  {
    this->buttons[i]->startCalibration();
  }
}


void BaseSwitch::onButtonAction(uint16_t _id, uint16_t _type, uint16_t _data)
{
  if(this->callback_onButtonAction)
    this->callback_onButtonAction(_id, _type, _data);

}

void BaseSwitch::onButtonStateChanged(uint16_t _id, uint16_t _state)
{
  // we do not expose anything here...
  // we attached to the callback because we may use this in future
}

void BaseSwitch::onProximityAlert(uint16_t _id, boolean _isProximity, uint16_t _proximityValue, uint16_t _proximityLevel)
{
  if(this->callback_onProximityAlert)
    this->callback_onProximityAlert(_id, _isProximity, _proximityValue, _proximityLevel);
}


void BaseSwitch::setMode(SWITCH_MODE _mode, uint16_t _modeLevel)
{
  SWITCH_MODE prevMode      = this->mode;
  uint16_t    prevModeLevel = this->modeLevel;
  this->mode      = _mode;
  this->modeLevel = _modeLevel;
  if(this->callback_onModeChange)
    this->callback_onModeChange(prevMode, prevModeLevel, _mode, _modeLevel);
}


void BaseSwitch::requestEnvironmentData()
{
  // has to be overwritten.
  // Should start gathering the environemntal data and if finished is has to call
  // the callback method 'CallbackFunction_EnvironmentDataUpdated'
}


uint64_t BaseSwitch::getPeriod(uint64_t _lastCallTime, bool _useMicros)
{
  uint64_t cur;

  if(_useMicros)
    cur = micros();
  else
    cur = millis();
  // if we had an overflow we have to calculate the period in a special way
  // in fact we have to add "(max)uint16_t - LastCallTime"  to  "cur"
  // but ofr our purpose its okay to only return "cur".
  if(cur < _lastCallTime)
    return (uint64_t)(cur + (UINT64_MAX - _lastCallTime));
  else
    return (uint64_t)(cur - _lastCallTime);
}


void BaseSwitch::task()
{
  for(uint8_t i=0; i<=this->maxButtonIdx; i++)
  {
    this->buttons[i]->task();
  }
  this->lastTaskRunTime = millis();
}


void BaseSwitch::parmEnvSensorsSettings(BaseSwitchEnvSensors _envSensors){
  this->envSensorsSettings = _envSensors;
}
BaseSwitchEnvSensors BaseSwitch::parmEnvSensorsSettings(){
  return this->envSensorsSettings;
}


void BaseSwitch::parmLightningSettings(BaseSwitchLightning _lightningSettings){
  this->lightningSettings = _lightningSettings;
}
BaseSwitchLightning BaseSwitch::parmLightningSettings(){
  return this->lightningSettings;
}


void BaseSwitch::parmSpeakerSettings(BaseSwitchSpeaker _speakerSettings){
  this->speakerSettings = _speakerSettings;
}
BaseSwitchSpeaker BaseSwitch::parmSpeakerSettings(){
  return this->speakerSettings;
}
