#ifndef PowerMeter_H
#define PowerMeter_H

enum PmStatus {
  STATUS_INIT = 1,
  STATUS_OK,
  STATUS_COM_ERROR,
  STATUS_GENERIC_ERROR
};

class PowerMeter {
  private:
    bool enabled;
    PmStatus status;
    float voltage;
    float current;
    float activePower;
    float importactiveenergy;
    float exportactiveenergy;
    float maxtotspd;

  public:
    PowerMeter();
    void setValues(float volt, float curr, float apower, float kwhimp, float kwhexp, float maxspd);
    void setEnabled(bool en);
    void setStatus(PmStatus  st);
    
    PmStatus getStatus();
    bool isEnabled();
    
    float getVoltage();
    float getCurrent();
    float getActivePower();
    float getImportActiveEnergy ();
    float getExportActiveEnergy ();
    float getMaxTotSpd ();
    
};

#endif
