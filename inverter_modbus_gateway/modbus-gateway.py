#!/usr/bin/env python3
"""
Modbus-Mqtt Gateway

For the Modbus interface it use Pymodbus library (pymodbus) and is derived from
updating_server.py example

For the MQTT interface it use Paho MQTT library (paho-mqtt)
--------------------------------------------------------------------------
"""

# NOTE:   versione preliminare per la sola gestione dei registri per inverter

# TODO: completare polling assegnando aree registri e scrivendo mqtt
# TODO: gestire lunghezza e offset in client

# TODO: gestire switch sw per copia valori





## --------------------------------------------------------------------------- #
# import the modbus libraries we need
# --------------------------------------------------------------------------- #
from pymodbus.constants import Endian
from pymodbus.payload import BinaryPayloadDecoder
from pymodbus.payload import BinaryPayloadBuilder
from pymodbus.client.sync import ModbusSerialClient as ModbusClientRtu
from pymodbus.client.sync import ModbusTcpClient as ModbusClientTcp
from pymodbus.server.async import StartTcpServer
from pymodbus.server.async import StartSerialServer
from pymodbus.device import ModbusDeviceIdentification
from pymodbus.datastore import ModbusSequentialDataBlock
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext
from pymodbus.transaction import ModbusRtuFramer, ModbusAsciiFramer

# --------------------------------------------------------------------------- #
# import the mqtt libraries we need
# --------------------------------------------------------------------------- #
import paho.mqtt.client as mqtt

# --------------------------------------------------------------------------- #
# import other libraries
# --------------------------------------------------------------------------- #
from twisted.internet.task import LoopingCall
from twisted.internet import reactor
import argparse
import time
import threading
import signal, os
import traceback

# --------------------------------------------------------------------------- #
# configure the service logging
# --------------------------------------------------------------------------- #
import logging
logging.basicConfig()
log = logging.getLogger()
log.setLevel(logging.DEBUG)
#log.setLevel(logging.CRITICAL)


# --------------------------------------------------------------------------- #
# Constants and global variables
# --------------------------------------------------------------------------- #
MQTT_SERVER = "iot.eclipse.org"
MQTT_TOPIC_BASE = "proesys/modbus-gateway"
MQTT_TOPIC_INPUT_HR = MQTT_TOPIC_BASE  + "/area_in/hr"
MQTT_TOPIC_RT_HR = MQTT_TOPIC_BASE  + "/area_rt/hr"

MODBUS_MODE_RTU = "RTU"
MODBUS_MODE_TCP = "TCP"
MODBUS_TCP_PORT = 5020
MODBUS_RTU_DEVICE_CLIENT = "/dev/ttyUSB0"  # legata a /dev/tnt0 per test senza device
MODBUS_RTU_DEVICE_SERVER = "/dev/ttyUSB1"

MODBUS_CLIENT_ADDRESS = 1
MODBUS_POLLING_TIME = 300
MODBUS_CLIENT_TIMEOUT = 4

MODBUS_RTU_BAUD = 9600

MODBUS_HR_SIZE = 50

mbusContext = None
modbus_server_mode = MODBUS_MODE_TCP

# --------------------------------------------------------------------------- #



# --------------------------------------------------------------------------- #
# define your callback process
# --------------------------------------------------------------------------- #
def updating_writer(a):
    """ A worker process that runs every so often and
    updates live values of the context. It should be noted
    that there is a race condition for the update.

    :param arguments: The input arguments to the call
    """
    # log.debug("updating the context")
    context = a[0]
    register = 4 # 3:hr   / 4:ir
    slave_id = 0x00
    address = 0x0
    # values = context[slave_id].getValues(register, address, count=5)
    # values = [v + 1 for v in values]
    # log.debug("new values: " + str(values))
    # context[slave_id].setValues(register, address, values)



class ModbusGatewayServer:
    """
    A class to  implement the server-side of the Modbus Gateway

    """

    #def __init__(self):


    def initRegistersArea(self,  hr_size):
        # ----------------------------------------------------------------------- #
        # initialize the server information
        # ----------------------------------------------------------------------- #
        # If you don't set this or any fields, they are defaulted to empty strings.
        # ----------------------------------------------------------------------- #
        identity = ModbusDeviceIdentification()
        identity.VendorName = 'ITLabs'
        identity.ProductCode = 'Conta-e-spara'
        identity.VendorUrl = 'https://github.com/fablabromagna-org/Conta-Spara'
        identity.ProductName = 'CS Modbus Gateway'
        identity.ModelName = 'Modbus Gateway'
        identity.MajorMinorRevision = '1.0'
        self.modbusIdentity = identity

        # ----------------------------------------------------------------------- #
        # initialize the data store
        # ----------------------------------------------------------------------- #
        store = ModbusSlaveContext(
            # di=ModbusSequentialDataBlock(0, [991]*100),
            # co=ModbusSequentialDataBlock(0, [992]*100),
            hr=ModbusSequentialDataBlock(0, [0] * hr_size)
        )


        self.serverContext = ModbusServerContext(slaves=store, single=True)
        return  self.serverContext

    def run(self, mode, tcpport=5020, baud=9600, rtuport=None):

        # ----------------------------------------------------------------------- #
        # run the server you want
        # ----------------------------------------------------------------------- #
        # time = 1  # 5 seconds delay
        # loop = LoopingCall(f=updating_writer, a=(context,))
        # loop.start(time, now=False)  # initially delay by time


        if mode == MODBUS_MODE_RTU:
            print ("@@@@@@@ Starting ModBus Server  %s @ %d" % (rtuport, baud))
            StartSerialServer(self.serverContext, framer=ModbusRtuFramer, identity=self.modbusIdentity,
                              port=rtuport, timeout=.05, baudrate=baud)

        elif modbus_server_mode == MODBUS_MODE_TCP:
            StartTcpServer(self.serverContext, identity=self.modbusIdentity, address=("", tcpport))


    def stop (self):
        from twisted.internet import reactor
        print(">>>>>>>> Stopping MODBUS SERVER")
 
        #reactor.stop()
        reactor.callFromThread(reactor.stop)




class ModbusGatewayClient:
    """
    A class to  implement the client-side of the Modbus Gateway

    """
    UNIT = 0x1

    #def __init__(self):


    def initRegistersArea(self, hr_size):
        # ----------------------------------------------------------------------- #
        # initialize the server information
        # ----------------------------------------------------------------------- #
        # If you don't set this or any fields, they are defaulted to empty strings.
        # ----------------------------------------------------------------------- #
        identity = ModbusDeviceIdentification()
        identity.VendorName = 'ITLabs'
        identity.ProductCode = 'Conta-e-spara'
        identity.VendorUrl = 'https://github.com/fablabromagna-org/Conta-Spara'
        identity.ProductName = 'CS Modbus Gateway'
        identity.ModelName = 'Modbus Gateway'
        identity.MajorMinorRevision = '1.0'
        self.modbusIdentity = identity

        # ----------------------------------------------------------------------- #
        # initialize the data store
        # ----------------------------------------------------------------------- #
        store = ModbusSlaveContext(
            # di=ModbusSequentialDataBlock(0, [991]*100),
            # co=ModbusSequentialDataBlock(0, [992]*100),
            hr=ModbusSequentialDataBlock(0, [0] * hr_size)
        )

        self.Size = hr_size

        self.clientContext = ModbusServerContext(slaves=store, single=True)
        return  self.clientContext

    def init_polling(self):
        # self.loop = LoopingCall(self.polling)
        # # self.loop.start(pollingtime, now=False)  # initially delay by time
        #
        # self.loop.start(5)
        #
        # reactor.run()
        while self.clientAlive:
            self.polling()
            time.sleep(self.pollingTime)

    def run(self, mode,  device_ip='', tcp_port=5020, device_id = 1, pollingtime=1, rtubaud=9600, rtuport=None, clienttimeout=1):
        self.deviceId = device_id
        self.pollingTime = pollingtime

        self.\
            HRSize = 10

        self.clientAlive = True
        if mode == MODBUS_MODE_RTU:
            self.client = ModbusClientRtu(method='rtu', port=rtuport, timeout=MODBUS_CLIENT_TIMEOUT, baudrate=rtubaud)

        elif mode == MODBUS_MODE_TCP:
            self.client = ModbusClientTcp(device_ip, port=tcp_port)

        self.client.connect()

        threading.Thread(target=self.init_polling).start()

        # while True:
        #     print("Client is alive")
        #     time.sleep(5)


    def polling(self):

      try:
        #print("Client Polling:")

        # Read HR
        log.debug("Reading HR")

        # Leggo la potenza (W totali) al registro 0x28
        self.hr_power = self.client.read_holding_registers(unit=self.deviceId, address=0x28, count=2)
        if (self.hr_power.registers is not None):
            decoder = BinaryPayloadDecoder.fromRegisters(self.hr_power.registers, byteorder=Endian.Big, wordorder = Endian.Little)
            self.powerTotal = decoder.decode_32bit_int()


        # Leggo la corrente L2  al registro 0x1e
        self.hr_current = self.client.read_holding_registers(unit=self.deviceId, address=0x0e, count=1)
        if (self.hr_current.registers is not None) :
            decoder = BinaryPayloadDecoder.fromRegisters(self.hr_current.registers, byteorder=Endian.Big)
            self.currentL2 = decoder.decode_16bit_uint()

        print ("Power: %4d  - Ampere: %4d" % (self.powerTotal, self.currentL2))
      
      except  Exception as e:
        print ("MODBUS Read error!")
        
    def get_rtdata(self):
        return self.powerTotal, self.currentL2

    def get_rawdata(self):
        return self.hr_power, self.hr_current


    def stop (self):
        from twisted.internet import reactor
        print(">>>>>>>> Stopping MODBUS CLIENT")
 
        #reactor.stop()
        reactor.callFromThread(reactor.stop)




class MqttModbusClient:
    """
    A class to connect to the MQTT server and expose the modbus realtime area

    """
    def __init__(self, modbus_context):
        self.mbusContext = modbus_context

    def connect(self, server):
        client = mqtt.Client("modbus-gateway")
        client.connect(server)
        client.subscribe(MQTT_TOPIC_RT_HR)
        client.on_message = self.on_mqtt_modbus_message
        client.loop_start()
        print("Done")

    # Callback function for mqtt subscription
    def on_mqtt_modbus_message(self, client, userdata, message):
        #Ricevo il topic mqtt
        print("user data=", userdata)
        print("message topic=",message.topic)
        print("message qos=",message.qos)
        print("message retain flag=",message.retain)
        print(message.payload)

        # sulla base del topic stabilisco quale area modbus assegnare
        payloadsize = len(message.payload)
        if (message.topic == MQTT_TOPIC_RT_HR):
            register = 4
            if (payloadsize > MODBUS_HR_SIZE): payloadsize = MODBUS_HR_SIZE
            print("Assegno HREG")
        else :
            register = -1
            print("Errore area modbus")

        if (register > 0):
            context = self.mbusContext
            slave_id = 0x00
            address = 0x0

            print("La lunghezza del messaggio e' ", payloadsize)

            # Leggo l'attuale area di memoria, per il numero di byte da assegnare
            values = context[slave_id].getValues(register, address, count=payloadsize)

            # Compongo la word con ogni 2 bytes ricevuti
            for i in range(len(message.payload)):
                values[i] = int.from_bytes(message.payload[i:i+2], byteorder='big', signed=False)
            context[slave_id].setValues(register, address, values)

    def stop (self):
        from twisted.internet import reactor
        print(">>>>>>>> Stopping MODBUS CLIENT")
 
        #reactor.stop()
        reactor.callFromThread(reactor.stop)




def handler(signum, frame):
    import sys, os
    print ('Signal handler called with signal', signum)


    os.kill(os.getpid(), signal.SIGABRT)

    global modbus_client
    global modbus_server
    #global mqttclient
    modbus_client.clientAlive = False
    modbus_client.stop()
    modbus_server.stop()
    #mqttclient.stop()
    


def updating_writer(a):
    """ A worker process that runs every so often and
    updates live values of the context. It should be noted
    that there is a race condition for the update.

    :param arguments: The input arguments to the call
    """

    log.debug("updating the context")

    context = a[0]
    register = 3
    slave_id = 0x00

    #data_raw_power, data_raw_current = modbus_client.get_rawdata()
    #values = data_raw_power.registers
    #context[slave_id].setValues(register, 0x28, values)

    #values = data_raw_current.registers
    #context[slave_id].setValues(register, 0x0e, values)




if __name__ == "__main__":


    # -------------------------------------
    # Parsing arguments
    # -------------------------------------
    GITHUB_REPO = 'https://github.com/itarozzi'
    description = '''
    A Modbus Gateway with MQTT register interaction.

    Use --advanced_help for useful informations or visit the github repository:
    ''' + GITHUB_REPO

    description_advanced = '''
    *********************************************************
    A Modbus Gateway with MQTT register interaction.
    *********************************************************

     You can define TCP or RTU mode for both Client and Server side of the Gateway


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
      ---------------                           GatewaySwitch
     | MODBUS_CLIENT |  --->   ModbusAreaInput    ---//--->                       ---------------
      ---------------             |                           ModbusAreaRT  ---> | MODBUS_SERVER |
            |                     |                                               ---------------
            |                      -----> MQTT    -------->
            |  RTU or TCP
            |
        real device
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    The Client side load the registers from device to ModbusAreaInput 

    The Server side expose the registers from the ModbusAreaRT

    When the GatewaySwitch is ON, the ModbusAreaInput is copied to Real Time Area; in this case the Gateway act
    as a simple repeater.

    When the GatewaySwitch is OFF, you can modify the Real Time Area using MQTT.

    The ModbusAreaInput is published to mqtt server, in the topic [1]
    The Gateway subscribe the topic [2] and write the ModbusAreaRT when the topic changes
    The GatewaySwitch is assigned by the subscriptin to topic [3]

    [1] {base_topic}/area_in/hr  , {base_topic}/area_in/ir
    [2] {base_topic}/area_rt/hr  , {base_topic}/area_rt/ir
    [3] {base_topic}/gateway_switch   { 0 | 1 }


    For the Modbus interface it use Pymodbus library (pymodbus)
    For the MQTT interface it use Paho MQTT library (paho-mqtt)


    '''
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("--advanced_help", help="show advanced help and usefull informations", action='store_true')


    parser.add_argument("--nomqtt", help="don't use MQTT to read and write registers",  action='store_true')
    parser.add_argument("--mqtt_server", help="The IP of MQTT server (broker),\ndefault: "+MQTT_SERVER , default=MQTT_SERVER )
    args = parser.parse_args()

    if args.advanced_help:
        print(description_advanced)
        exit(0)
    # -------------------------------------

    signal.signal(signal.SIGTERM, handler)
    signal.signal(signal.SIGINT, handler)


    modbus_server_mode = MODBUS_MODE_RTU
    modbus_client_mode = MODBUS_MODE_RTU
    mqtt_server = args.mqtt_server


    # -------------------------------------
    # Init modbus server and registers
    # -------------------------------------

    modbus_server = ModbusGatewayServer()
    modbus_context = modbus_server.initRegistersArea(MODBUS_HR_SIZE)


    # -------------------------------------
    # Start MQTT Connection
    # -------------------------------------
    if not args.nomqtt:
        print("Starting the MQTT client - server: ", mqtt_server)
        mqttclient = MqttModbusClient(modbus_context )
        mqttclient.connect(mqtt_server)
    else:
        print("Starting without MQTT ")

    # -------------------------------------
    # start modbus client loop
    # -------------------------------------
    print("Starting modbus client.")
    modbus_client = ModbusGatewayClient();
    modbus_client.run(mode=modbus_client_mode, pollingtime=MODBUS_POLLING_TIME, device_id=MODBUS_CLIENT_ADDRESS,
                      rtubaud=MODBUS_RTU_BAUD , rtuport = MODBUS_RTU_DEVICE_CLIENT )
    print("Done.")




    #  TODO: if GatewaySwitch is ON copy INPUT to HR
    time_update = 300  # nr seconds delay
    loop = LoopingCall(f=updating_writer, a=(modbus_context,))
    loop.start(time_update, now=False)  # initially delay by time




    # -------------------------------------
    # start modbus server loop
    # -------------------------------------
    if (modbus_server is not  None):
        print("Starting modbus server loop.")
        modbus_server.run(modbus_server_mode,  baud=MODBUS_RTU_BAUD, rtuport=MODBUS_RTU_DEVICE_SERVER )




