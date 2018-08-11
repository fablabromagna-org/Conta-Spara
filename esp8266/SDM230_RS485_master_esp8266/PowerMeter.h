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
    float voltage, current, activePower, import_active_energy, export_active_energy, max_tot_spd;
    
    
  public:
    PowerMeter();
    void setValues(float volt, float curr, float apower, float import_active_energy, float export_active_energy, float max_tot_spd);
    void setEnabled(bool en);
    void setStatus(PmStatus  st);
    
    PmStatus getStatus();
    bool isEnabled();
    
    float getVoltage();
    float getCurrent();
    float getActivePower();
    float getImport_active_energy();
    float getExport_active_energy();
    float getMax_tot_spd();
    bool getJsonMeasures(char *val_json);
    float getImportActiveEnergy();
    float getExportActiveEnergy();
    float getMaxTotSpd();
  
};

#endif
