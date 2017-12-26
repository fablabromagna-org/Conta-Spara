#include "PowerMeter.h"

PowerMeter::PowerMeter(){
  status = STATUS_INIT;
  enabled = true;
}

void PowerMeter::setValues(float volt, float curr, float apower)
{
  voltage = volt;
  current = curr;
  activePower = apower;

}

void PowerMeter::setEnabled(bool en)
{
  enabled = en; 
}

void PowerMeter::setStatus(PmStatus  st){
  status = st;
}

bool PowerMeter::isEnabled()
{
  return enabled;
}

    
PmStatus PowerMeter::getStatus()
{
  return status;
}
    
float PowerMeter::getVoltage()
{
  return voltage;
}

float PowerMeter::getCurrent() 
{
  return current;
}

float PowerMeter::getActivePower()
{
  return activePower;
}

