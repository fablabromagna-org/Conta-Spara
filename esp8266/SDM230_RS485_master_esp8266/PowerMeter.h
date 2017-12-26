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

  public:
    PowerMeter();
    void setValues(float volt, float curr, float apower);
    void setEnabled(bool en);
    void setStatus(PmStatus  st);
    
    PmStatus getStatus();
    bool isEnabled();
    
    float getVoltage();
    float getCurrent();
    float getActivePower();
    
};

#endif
