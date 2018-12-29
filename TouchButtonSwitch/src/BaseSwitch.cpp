/*
  BaseSwitch.cpp
  Created by Christian Dürnberger, 2019
  Released into the public domain.
*/

#include "Arduino.h"
#include "BaseSwitch.h"


BaseSwitch::BaseSwitch()
{
  this->maxButtonIdx = -1;
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


Button* BaseSwitch::addButton(Button* _button, uint16_t _buttonId)
{
  this->maxButtonIdx++;
  this->buttons[this->maxButtonIdx] = _button;
  // set a button id if not specified by the user
  if(!_buttonId)
    this->buttons[this->maxButtonIdx]->parmId(this->maxButtonIdx + 1);
  else
    this->buttons[this->maxButtonIdx]->parmId(_buttonId);
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
    // do the setup routine for the button
    ret = ret && this->buttons[i]->setup();
  }
  return ret;
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


void BaseSwitch::task()
{
  for(uint8_t i=0; i<=this->maxButtonIdx; i++)
  {
    this->buttons[i]->task();
  }
}



