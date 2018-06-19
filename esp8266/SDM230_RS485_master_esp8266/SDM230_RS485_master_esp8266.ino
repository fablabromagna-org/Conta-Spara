/*

  SDM230_RS485_master_esp8266.ino 
  ===============================
  
  Interrogazione dei misuratori SDM230 via ModBus e scrittura topic MQTT

  Device ESP8266 - NodeMCU ESP12 V2

  Il debug viene eseguito attraverso la Serial1, collegata al pin GPIO2

  
  Vengono interrogati 8 misuratori SDM230, con indirizzi modbus contigui da 1 a 7 (ora non più)

  I risultati delle letture vengono messi in una serie di topic MQTT organizzati
  secondo la seguente struttura (tutta da modificare)

  {base}/misuratori/
            generale/
                  abilitato     // 0/1
                  stato         // 0:OK, 1:no_comm
                  tensione      // volts
                  corrente      // ampere
                  potenza       // Watt
                  ...
            
            servizi/
                  abilitato     
                  stato
                  tensione    
                  corrente    
                  potenza
                  ...

            appartamento1/
                  abilitato     
                  stato
                  tensione
                  corrente
                  potenza
                  ...

            appartamento2/
                  abilitato     
                  stato
                  tensione
                  corrente
                  potenza
                  ...

            appartamento5/
                  abilitato     
                  stato
                  tensione
                  corrente
                  potenza
                  ...
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ModbusMaster.h>
#include <PubSubClient.h>
#include "PowerMeter.h"

/**************** Configuration from external file *******************/
#include "myconfig.h"

/************************ WiFi Access Point *********************************/
#ifndef MYCONFIG
#define WLAN_SSID       "ssid"
#define WLAN_PASS       "passwd"

#define MQTT_SERVER      "iot.eclipse.org"
#define MQTT_SERVERPORT  1883                   // use 8883 for SSL
#define MQTT_USERNAME    ""
#define MQTT_KEY         ""
#endif
/*****************************************************************************/

const char* mqtt_clientID = "contaspara";

#define NR_MISURATORI 10

PowerMeter misuratori[NR_MISURATORI];


/*!
  We're using a MAX485-compatible RS485 Transceiver.
  Rx/Tx is hooked up to the hardware serial port at 'Serial'.
  The Data Enable and Receiver Enable pins are hooked up as follows:
*/
#define MAX485_DE      12
#define MAX485_RE_NEG  14

#define POLLING_TIME 5000


// Istanze degli oggetti ModbusMaster 
#define MODBUS_NR_DEVICES NR_MISURATORI
ModbusMaster mbus_node;


/************************ MQTT topics *********************************/
const char* mqtt_topic_base = "contaspara/misuratori";
const char* mqtt_topic_misuratore[MODBUS_NR_DEVICES] = {
  "/app1",
  "/center",
  "/vuoto1",
  "/app4M",
  "/app2",
  "/vuoto2",
  "/app3",
  "/sergen",
  "/app4A",
  "/app5"
  }; 
  
const char* mqtt_topic_abilitato = "/abilitato"; 
const char* mqtt_topic_stato = "/stato"; 
const char* mqtt_topic_tensione = "/tensione"; 
const char* mqtt_topic_corrente = "/corrente"; 
const char* mqtt_topic_potenza = "/potenza";
const char* mqtt_topic_kwhimport = "/kwhimport";
const char* mqtt_topic_kwhexport = "/kwhexport";
const char* mqtt_topic_maxtotspd = "/maxtotspd";    
 

WiFiClient espClient;
PubSubClient mqtt_client(espClient);
char val[50];

bool state = true;
u32 polling_time_prev =0;
int mis_idx = 0;
int modbus_add[MODBUS_NR_DEVICES] = {11,12,13,14,21,22,23,31,32,33};


void setup()
{
  Serial.begin(19200);
  delay(1000);
  Serial1.begin(19200);
  delay(1000);

  //=== Inizializzo il WiFi ===  
  setup_wifi();


  mqtt_client.setServer(MQTT_SERVER, MQTT_SERVERPORT);
  mqtt_client.setCallback(mqtt_callback);
  
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);


  // Modbus communication runs at 2400 baud
  Serial.begin(2400, SERIAL_8N2);
 
  


  // @@@@ DEBUG : disabilito temporaneamente i misuratori assenti
  //misuratori[0].setEnabled(false);
  //misuratori[1].setEnabled(false);
  misuratori[2].setEnabled(false);
  misuratori[3].setEnabled(false);
  //misuratori[4].setEnabled(false);
  misuratori[5].setEnabled(false);
  //misuratori[6].setEnabled(false);
  //misuratori[7].setEnabled(false);   
  misuratori[8].setEnabled(false);
  //misuratori[9].setEnabled(false);

  delay(5000);
}

void loop()
{
  uint8_t result, result2;
  int mb_status = 0;
  float volts; 
  float current;
  float active_power;
  float import_active_energy;
  float export_active_energy;
  float max_tot_spd;

 //  //=== Ad ogni loop controlla che il collegamento al broker mqtt sia ok ===
  if (!mqtt_client.connected()) {
    mqtt_connect();
  }
  mqtt_client.loop();   // processa eventiali callback
  
  if ( (millis()- polling_time_prev) > POLLING_TIME) {
   
    
    //=== Ad ogni loop interrogo solo un misuratore, in modo da non bloccare per troppo tempo il programma ===
    if (misuratori[mis_idx].isEnabled()) {  
      Serial1.print("- Polling Misuratore:  ");
      Serial1.println(mis_idx);

      mbus_node.begin(modbus_add[mis_idx], Serial);
      mbus_node.preTransmission(preTransmission);
      mbus_node.postTransmission(postTransmission);

     
      // Read 16 registers starting at 30001)
      result = mbus_node.readInputRegisters(0x00, 16 );
      if (result == mbus_node.ku8MBSuccess)
      {
        Serial1.println("prima lettura ok");
        misuratori[mis_idx].setStatus(STATUS_OK);
        volts = decodeFloat(mbus_node.getResponseBuffer(0x00),  mbus_node.getResponseBuffer(0x01));
        current = decodeFloat(mbus_node.getResponseBuffer(0x06),  mbus_node.getResponseBuffer(0x07));
        active_power = decodeFloat(mbus_node.getResponseBuffer(0x0c),  mbus_node.getResponseBuffer(0x0d));
      }
      else {
        Serial1.println("prima lettura ERRORE");
        misuratori[mis_idx].setStatus(STATUS_COM_ERROR);
        volts = 0;
        current = 0;
        active_power = 0;
      }
      
      Serial1.print("Tensione V : ");
      Serial1.println(volts);
      Serial1.print("Corrente A : ");
      Serial1.println(current);
      Serial1.print("Potenza  W : ");
      Serial1.println(active_power);
      
      // Read 2 registers starting at 30073)
      result2 = mbus_node.readInputRegisters(0x48, 4 );
      if (result2 == mbus_node.ku8MBSuccess)
      {
        Serial1.println("seconda lettura ok");
        misuratori[mis_idx].setStatus(STATUS_OK);
        import_active_energy = decodeFloat(mbus_node.getResponseBuffer(0x00),  mbus_node.getResponseBuffer(0x01));
        export_active_energy = decodeFloat(mbus_node.getResponseBuffer(0x02),  mbus_node.getResponseBuffer(0x03));
        //max_tot_spd = decodeFloat(mbus_node.getResponseBuffer(0x0e),  mbus_node.getResponseBuffer(0x0f));
        // Per ora non lo leggo
        max_tot_spd = 0;
      }
      else {
        Serial1.println("seconda lettura ERRORE");
        misuratori[mis_idx].setStatus(STATUS_COM_ERROR);
        import_active_energy = 0;
        export_active_energy = 0;
        max_tot_spd = 0;
      }
      
      Serial1.print("Import Kwh : ");
      Serial1.println(import_active_energy);
      Serial1.print("Export Kwh : ");
      Serial1.println(export_active_energy);
      Serial1.print("Max SPD  W : ");
      Serial1.println(max_tot_spd);
      
      
      misuratori[mis_idx].setValues(volts, current, active_power, import_active_energy, export_active_energy, max_tot_spd);
      
      // pubblica i valori via MQTT
      char str_topic[128];
      // itoa((misuratori[mis_idx].getStatus() == STATUS_OK)?0:1,  val, 10 );
      // sprintf(str_topic, "%s%s%s", mqtt_topic_base, mqtt_topic_misuratore[mis_idx], mqtt_topic_stato);
      // mqtt_client.publish(str_topic , val);

      // TODO: trovare un modo migliore per costruire e json e convertire i float
      // TODO: aggiungere i nuovi valori letti .... (GPMISSI)
      char val_json[128];
      int status = (misuratori[mis_idx].getStatus() == STATUS_OK) ? 0 : 1;
      sprintf(val_json, "{\"status\":%d \"tens\":%s,\"curr\":%s,\"apower\":%s}", 
                            String(status).c_str(), 
                            String(volts).c_str(), 
                            String(current).c_str(), 
                            String(active_power).c_str());
      mqtt_client.publish(str_topic , val_json);


      
    }  

    // incremento il contatore del misuratore da interrogare
    if (mis_idx < NR_MISURATORI) {
      mis_idx++;  
    }
    else {
      mis_idx = 0;
      polling_time_prev = millis();
    }
  }

  delay(100);
}

// -----------------------------------------------------------------
// Connessione alla rete WiFi
// -----------------------------------------------------------------
void setup_wifi() {
  Serial1.println(); Serial1.println();
  Serial1.print("Connecting to ");
  Serial1.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial1.print(".");
  }
  Serial1.println();

  Serial1.println("WiFi connected");
  Serial1.println("IP address: "); Serial1.println(WiFi.localIP());


}


// -----------------------------------------------------------------
// Connessione al broker mqtt
// -----------------------------------------------------------------
void mqtt_connect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial1.print("Attempting MQTT connection...");
    if (mqtt_client.connect(mqtt_clientID, MQTT_USERNAME, MQTT_KEY)) {
      Serial1.println("connected");
      // Once connected, publish an announcement...
      // mqtt_client.publish(outTopicAnn, clientID);

      //===  resubscribe ===
      String str_topic;
      for (int mis_idx=0; mis_idx < NR_MISURATORI; mis_idx++) {
        str_topic = String(mqtt_topic_base) + String(mqtt_topic_misuratore[mis_idx]) + String(mqtt_topic_abilitato) ;
        mqtt_client.subscribe(str_topic.c_str());
        Serial1.print("Subscribe: ");
        Serial1.println(str_topic.c_str());
      }
      
    } else {
      Serial1.print("failed, rc=");
      Serial1.print(mqtt_client.state());
      Serial1.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


// -----------------------------------------------------------------
// Funzione di callback chiamata quando ricevo dati dal broker mqtt
// -----------------------------------------------------------------
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial1.print("Message arrived [");
  Serial1.print(topic);
  Serial1.print("] ");

  //=== individua il misuratore di destinazione ===
  String str_topic;
  for (int mis_idx=0; mis_idx < NR_MISURATORI; mis_idx++) {
    str_topic = String(mqtt_topic_base) + String(mqtt_topic_misuratore[mis_idx]) + String(mqtt_topic_abilitato) ;
   
    if (strcmp(str_topic.c_str(), topic) == 0) {
      
      misuratori[mis_idx].setEnabled((payload[0]=='0')?false:true);

      Serial1.print("@@@@ PM Enable -> [");
      Serial1.println(misuratori[mis_idx].isEnabled());
      Serial1.println("]");

      break;
    }
  }
}


// -------------------------------------------------------------
// Funzioni di conversione per valori Modbus, da Word a Float
// -------------------------------------------------------------
union Pun {float f; uint32_t u;};
float decodeFloat(const uint16_t reg0, uint16_t reg1)
{
    union Pun pun;

    pun.u = ((uint32_t)reg0 << 16) | reg1;
    return pun.f;
}

// -------------------------------------------------------------
// Pilotaggio dei pin di controllo del convertitore RS485
// Servono a commutare la linea 485 da trasmissione a ricezione
// -------------------------------------------------------------
void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  delay(10);
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}


