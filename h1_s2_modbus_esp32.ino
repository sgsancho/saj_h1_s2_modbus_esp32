//#include <WiFi.h>
#include <WiFiManager.h>
#include "BLEDevice.h"

unsigned long previousMillis = 0;
unsigned long previousMillistry = 0;

static BLEUUID serviceUUID("0000ffff-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUIDw("0000ff01-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID("0000ff02-0000-1000-8000-00805f9b34fb");

static boolean doConnect_ble = false;
static boolean connected_ble = false;
static boolean doScan_ble = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristicw;
static BLEAdvertisedDevice* myDevice;

BLEClient*  pClient  = BLEDevice::createClient();

bool errorr = false;
bool waitingmessage = false;

int soft = 0;
byte modbus_frame_response[8];
int total_registros;

#define UInt16 uint16_t

int ModbusTCP_port = 502;
WiFiServer MBServer(ModbusTCP_port);

//////// Required for Modbus TCP / IP /// Requerido para Modbus TCP/IP /////////

#define MB_FC_NONE 0
#define MB_FC_READ_REGISTERS 3 //implemented
#define MB_FC_WRITE_REGISTER 6 //implemented


WiFiClient client;
byte mosbus_request[260];

int errlen = 0;

void(* resetFunc) (void) = 0; //declare reset function @ address 0

int c = 0;
int total_llamadas = 0;
int total_errores = 0;

unsigned long previousMillis_wifi = 0;

void print_array(byte data[], int longitud, String titulo){
    Serial.println("");
    String sdata =  titulo + ": [";
    for (int j = 0; j < longitud; j++) {
      sdata = sdata + (String)(int8_t) data[j]+",";
    }
    sdata = sdata + "]";
    Serial.print(sdata);
    Serial.println("");
}
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    waitingmessage = false;
    
    if(modbus_frame_response[7] == 6) { //function code
       Serial.println("¿Actualización realizada?");
       client.flush();
    }
    else {
      if(((int)length-7)!=total_registros) { //Se esperan recibir el numero de registros solicitados. En la respuesta hay 7 bytes más de los solicitados
        errlen = errlen +1;
        String detalle_error = (String)"Registros solicitados: " + total_registros + (String)". Recibidos: " + (String)(length-7);
        Serial.println(detalle_error);
        print_array(pData, length, "ERROR DE RESPUESTAS <25 BYTES");
        if(errlen > 5) {
          /*
          pClient->disconnect();
          errorr = true;
          Serial.println("sending disconnect signal len="+String(length)+"  - ");
          */
          Serial.println("Reset 1. Múltiples mensajes recibidos erróneos (5)");
          resetFunc();
        }
        else {
          previousMillistry = millis();
        }
      }
      else {
        Serial.println("elseeeeeeeeeeeeeeee");
        errlen=0;
        previousMillistry = millis();
        int longitud = abs(pData[7]); //20
        byte slice[longitud+1];
        memcpy(slice, pData + 7, longitud+1);
        slice[0] = longitud;
        
        byte combined[longitud+9];
        memcpy(combined, modbus_frame_response, 8);
        memcpy(&combined[8], slice, longitud+1);
    
        combined[1] = modbus_frame_response[1];
  
        print_array(combined, length+8, "Response final");
        
        client.write(combined, longitud+9);
        client.flush();
        
  
      }
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("Reset 2. Cliente Bluetooth desconectado");
    resetFunc();
  }
};

bool connectToServer_ble() {
    BLEDevice::getScan()->stop();
    connected_ble = true;
    
    Serial.println("Forming a connection to ");
    
    previousMillistry = millis();
    Serial.println(myDevice->getAddress().toString().c_str());
    
    
    Serial.println(" - Created client");
    pClient->setClientCallbacks(new MyClientCallback());
    
    previousMillistry = millis();
    pClient->connect(myDevice);  
    BLEDevice::setMTU(512); ///mtu size equals packet limit-3 250
    Serial.println(" - Connected to server");
    
    previousMillistry = millis();
    
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.println("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    previousMillistry = millis();
    
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    previousMillistry = millis();
    pRemoteCharacteristicw = pRemoteService->getCharacteristic(charUUIDw);
    if (pRemoteCharacteristic == nullptr) {
      Serial.println("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    previousMillistry = millis();
    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    soft = 0;
    doConnect_ble = false;
    return true;
}
/* Scan for BLE servers and find the first one that advertises the service we are looking for.*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /* Called for each advertising BLE server.*/
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    //Serial.println("BLE Advertised Device found: ");
    //Serial.println(advertisedDevice.toString().c_str());
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect_ble = true;
      doScan_ble = true;
    } 
  } 
}; 


void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to WIFI successfully!");
  MBServer.begin();
  setup_ble();
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(String(ModbusTCP_port));
  Serial.println("Modbus TCP/IP Online");
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
}



void setup_wifi_manager() {
  WiFi.disconnect(true);

  delay(1000);

  WiFi.onEvent(WiFiStationConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  
  WiFiManager wm;

  //reset settings - wipe stored credentials for testing
  //wm.resetSettings();

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("ESP32_SAJ_MODBUS"); // password protected ap

  if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("connected...yeey :)");
  }
}

void setup_ble() {
  /////////////////////BLUFI///////////////////////


  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5000, false);
  ////////////////////////////////////////////////
}

void setup() {

  Serial.begin(115200);
  setup_wifi_manager();
}

void loop() {

  unsigned long currentMillis_wifi = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis_wifi - previousMillis_wifi >=30000)) {
    Serial.println("Reset 3. Sin conectividad WIFI surante mas de 30 segundos");
    resetFunc();
  }
  
  if (doConnect_ble == true) {
    if (connectToServer_ble()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
  }
  
  unsigned long currentMillis = millis();
/*
  if((currentMillis - previousMillistry) >= 9000  ){
    Serial.println("Reset 4");
    resetFunc();
    Serial.println("disconnected no messages - retrying");
    delay(2000);
    soft = 0;
    previousMillistry = currentMillis;
    previousMillis = currentMillis + 12000;
    doConnect_ble = true;
    connected_ble = false;
  }*/

  if (connected_ble) {
      client = MBServer.available();
      if (!client) {
        //Serial.println(client);
        return;
      }

      byte byteFN = MB_FC_NONE;
  
      // Modbus TCP/IP
      while (client.connected()) {
      if(client.available())
      {
        if(c>255){
          c=0;
        }
        Serial.println("client.available");
        int i = 0;
        while(client.available())
        {
          mosbus_request[i] = client.read();
          i++;
        }
        client.flush();
        Serial.println("Petición TCP recibida");

        
        previousMillis = currentMillis;
        waitingmessage = true;
        byteFN = mosbus_request[7];
         
        //Ejemplo desde NodeRed de HA: 0, 1, 0, 0, 0, 6, 1, 3, 64, -94, 0, 1
        byte modbus_transaction_identifier[2] = {(byte) mosbus_request[0], (byte) mosbus_request[1]};
        byte modbus_protocol_identifier[2] = {(byte) mosbus_request[2], (byte) mosbus_request[3]};
        byte modbus_length[2] = {(byte) mosbus_request[4], (byte) mosbus_request[5]};
        byte modbus_unit_id = (byte) mosbus_request[6];
        byte modbus_function_code = (byte) mosbus_request[7];
        byte modbus_address[2] = {(byte) mosbus_request[8], (byte) mosbus_request[9]};
        byte modbus_number_registers[2] = {(byte) mosbus_request[10], (byte) mosbus_request[11]};

        Serial.println("");
        Serial.print("c: ");
        Serial.print(c);

        Serial.println("");
        Serial.print("total_llamadas: ");
        Serial.print(total_llamadas);
        
        Serial.println("");

        Serial.print("modbus_transaction_identifier: " );
        Serial.print(modbus_transaction_identifier[0]);
        Serial.print(" " );
        Serial.print(modbus_transaction_identifier[1]);
        Serial.println("");

        Serial.print("modbus_protocol_identifier: " );
        Serial.print(modbus_protocol_identifier[0]);
        Serial.print(" " );
        Serial.print(modbus_protocol_identifier[1]);
        Serial.println("");

        Serial.print("modbus_length: " );
        Serial.print(modbus_length[0]);
        Serial.print(" " );
        Serial.print(modbus_length[1]);
        Serial.println("");

        Serial.print("modbus_unit_id: " );
        Serial.print(modbus_unit_id);
        Serial.println("");

        Serial.print("modbus_function_code: " );
        Serial.print(modbus_function_code);
        Serial.println("");   

        Serial.print("modbus_address: " );
        Serial.print(modbus_address[0]);
        Serial.print(" " );
        Serial.print(modbus_address[1]);
        Serial.println("");

        Serial.print("modbus_number_registers: " );
        Serial.print(modbus_number_registers[0]);
        Serial.print(" " );
        Serial.print(modbus_number_registers[1]);
        Serial.println("");        
             
        //byte paylo[13] = {(byte) 77, (byte) 0, (byte) c, (byte) 9, (byte) 50, (byte) 1, (byte) 3, (byte) 64, (byte) 105, (byte) 0, (byte) 14, (byte) 1, (byte) 210};
      
        byte modbus_message_final[6] = {
          (byte) modbus_unit_id, 
          (byte) modbus_function_code, 
          (byte) modbus_address[0], 
          (byte) modbus_address[1], 
          (byte) modbus_number_registers[0], 
          (byte) modbus_number_registers[1]
        };

        UInt16 crc = (ModRTU_CRC((char*) modbus_message_final,6));

        unsigned char high_byte = crc >> 8;
        unsigned char low_byte = crc & 0xFF; 


        int transaction_identifier = ((modbus_transaction_identifier[0]<<8) +(modbus_transaction_identifier[1]))-1;
        byte transaction_identifier_hex[2];
        transaction_identifier_hex[0]=(transaction_identifier & 0x0000ff00) >> 8;
        transaction_identifier_hex[1]=transaction_identifier & 0x000000ff;
        
        byte mosbus_request_final[13] = {
        (byte) 77,
          0,
          c,
          (byte) 9,
          (byte) 50,
          (byte) modbus_unit_id, //modbus_unit_id
          (byte) modbus_function_code, //modbus_function_code
          (byte) modbus_address[0], //modbus_address
          (byte) modbus_address[1], //modbus_address
          (byte) modbus_number_registers[0], //modbus_number_registers
          (byte) modbus_number_registers[1], //modbus_number_registers
          (byte) low_byte, //crc
          (byte) high_byte //crc
        };

        total_registros = ((modbus_number_registers[0]<<8) +(modbus_number_registers[1]) * 2) + 3;
        byte total_registros_hex[2];
        total_registros_hex[0]=(total_registros & 0x0000ff00) >> 8;
        total_registros_hex[1]=total_registros & 0x000000ff;
        
        modbus_frame_response[0] = (byte) mosbus_request[0];
        modbus_frame_response[1] = (byte) mosbus_request[1];
        modbus_frame_response[2] = (byte) mosbus_request[2];
        modbus_frame_response[3] = (byte) mosbus_request[3];
        modbus_frame_response[4] = (byte) total_registros_hex[0];
        modbus_frame_response[5] = (byte) total_registros_hex[1];
        modbus_frame_response[6] = (byte) mosbus_request[6];
        modbus_frame_response[7] = (byte) mosbus_request[7];


        Serial.print("Enviando mensaje de lectura al BT: ");
        Serial.println(byteFN);
        print_array(mosbus_request_final, sizeof(mosbus_request_final), "mosbus_request_final");
        
        switch(byteFN) {
          case MB_FC_NONE:
            break;

          case MB_FC_READ_REGISTERS: // 03 Read Holding Registers

            // 50:   32 on hexadecimal view on the app
            // 1:    01 The unit identifier
            // 3:    03 The function code
            // 64:   40 address
            // 105:  69 address
            // 0:    00 length data
            // 14:   0E length data
            // 1:    01 crc
            // 210:  D2 crc
           

            pRemoteCharacteristicw->writeValue(mosbus_request_final, 13,true);
  
            byteFN = MB_FC_NONE;
  
            break;
          
          case MB_FC_WRITE_REGISTER: // 06 Write Holding Register
            pRemoteCharacteristicw->writeValue(mosbus_request_final, 13,true);
            byteFN = MB_FC_NONE;
            break;
          
          }
          c=c+1;
          total_llamadas = total_llamadas +1;
        }
      }
  }
  else if(doScan_ble){ 
    Serial.println("scanning again");
    //BLEDevice::getScan()->start(0);
    doConnect_ble = true;
    connected_ble = false;
  }
} 


UInt16 ModRTU_CRC(char * buf, int len)
{
  UInt16 crc = 0xFFFF;
 
  for (int pos = 0; pos < len; pos++) {
    crc ^= (UInt16)buf[pos];          // XOR byte into least sig. byte of crc
 
    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  //Serial.println(crc);
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;  
}
