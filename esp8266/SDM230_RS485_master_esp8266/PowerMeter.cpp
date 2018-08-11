#include "PowerMeter.h"
#include <Arduino.h>

PowerMeter::PowerMeter(){
  status = STATUS_INIT;
  enabled = true;
}

void PowerMeter::setValues(float volt, float curr, float apower, float ia_energy, float ea_energy, float mt_spd) 
{
  voltage = volt;
  current = curr;
  activePower = apower;
  import_active_energy = ia_energy;
  export_active_energy = ea_energy;
  max_tot_spd = mt_spd;
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

bool PowerMeter::getJsonMeasures(char *val_json)
{
    int intstatus = (status == STATUS_OK) ? 0 : 1;
    sprintf(val_json, "{\"status\":%d,\"tens\":%.2f,\"curr\":%.2f,\"apower\":%.2f,\"imp_ae\":%.2f,\"exp_ae\":%.2f,\"spd\":%.2f}", 
                        intstatus, 
                        voltage, 
                        current, 
                        activePower,
                        import_active_energy,
                        export_active_energy,
                        max_tot_spd 
                        );

//    sprintf(val_json, "{\"status\":%d,\n\"tens\":%f,\n\"curr\":%f,\n\"apower\":%f}", 
//                        intstatus, 
//                        voltage, 
//                        current, 
//                        activePower
//                        );
   
   return true;
    
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

float PowerMeter::getImportActiveEnergy(){
  return import_active_energy;
}

float PowerMeter::getExportActiveEnergy(){
  return export_active_energy;
}

float PowerMeter::getMaxTotSpd(){
  return max_tot_spd;
}

