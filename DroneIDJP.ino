/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <Arduino.h>
#include <bluefruit.h>

// Define UUID
#define USER_SERVICE_UUID "f9ed6165-faa8-4f2d-8b82-dc67d3444b0f"
#define RIDAUTH_CHARACTERISTIC_UUID "2d67083e-5291-4dfa-a357-8ae4317413f5"
#define COMMAND_CHARACTERISTIC_UUID "0663577e-1837-4e14-853b-a3478d2c7778"
#define NOTIFY_CHARACTERISTIC_UUID "7d46750b-443f-4de5-95be-7e86311acc1b"

//Service & Characteristic
BLEService        droneService;
BLECharacteristic droneNotifyCharacteristic;
BLECharacteristic RIDAuthCharacteristic;
BLECharacteristic RIDCommandCharacteristic;

uint8_t  num = 0;

void setup() 
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println("Bluefruit52 drone");
  Serial.println("--------------------------\n");

  // Initialise the Bluefruit module
  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

  // Set the advertised device name (keep it short!)
  Bluefruit.setName("MyDroneIDJP");

  // Set the connect/disconnect callback handlers
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Setup the Heart Rate Monitor service using
  // BLEService and BLECharacteristic classes
  Serial.println("Configuring the Heart Rate Monitor Service");
  setupTestService();

  // Set up and start advertising
  startAdv();

}

void startAdv(void)
{  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();

  // Include HRM Service UUID
  Bluefruit.Advertising.addService(droneService);

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  

  // Create loop2() using Scheduler to run in 'parallel' with loop()
  //Scheduler.startLoop(loop2);
}

void setupTestService(void)
{
  uint8_t userServiceUUID[16];
  uint8_t notifyCharacteristicUUID[16];
  uint8_t RIDAuthCharacteristicUUID[16];
  uint8_t RIDCommandCharacteristicUUID[16];

  strUUID2Bytes(USER_SERVICE_UUID, userServiceUUID);
  strUUID2Bytes(NOTIFY_CHARACTERISTIC_UUID, notifyCharacteristicUUID);
  strUUID2Bytes(RIDAUTH_CHARACTERISTIC_UUID, RIDAuthCharacteristicUUID);
  strUUID2Bytes(COMMAND_CHARACTERISTIC_UUID, RIDCommandCharacteristicUUID);

  droneService = BLEService(userServiceUUID);
  droneService.begin();

  RIDAuthCharacteristic = BLECharacteristic(RIDAuthCharacteristicUUID);
  RIDAuthCharacteristic.setProperties(CHR_PROPS_WRITE);
  RIDAuthCharacteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  RIDAuthCharacteristic.setFixedLen(1);
  RIDAuthCharacteristic.setUserDescriptor("RID Auth");
  RIDAuthCharacteristic.setWriteCallback(write_callback);
  RIDAuthCharacteristic.begin();

  RIDCommandCharacteristic = BLECharacteristic(RIDCommandCharacteristicUUID);
  RIDCommandCharacteristic.setProperties(CHR_PROPS_WRITE);
  RIDCommandCharacteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  RIDCommandCharacteristic.setFixedLen(1);
  RIDCommandCharacteristic.setUserDescriptor("RID Command");
  RIDCommandCharacteristic.setWriteCallback(write_callback);
  RIDCommandCharacteristic.begin();

  droneNotifyCharacteristic = BLECharacteristic(notifyCharacteristicUUID);
  droneNotifyCharacteristic.setProperties(CHR_PROPS_NOTIFY);
  droneNotifyCharacteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  droneNotifyCharacteristic.setMaxLen(5);
  droneNotifyCharacteristic.setUserDescriptor("RID Response");
  droneNotifyCharacteristic.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  droneNotifyCharacteristic.begin();
}

void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
  Serial.println("Advertising!");
}

void cccd_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint16_t cccd_value)
{
    // Display the raw request packet
    Serial.print("CCCD Updated: ");
    //Serial.printBuffer(request->data, request->len);
    Serial.print(cccd_value);
    Serial.println("");

    // Check the characteristic this CCCD update is associated with in case
    // this handler is used for multiple CCCD records.
    if (chr->uuid == droneNotifyCharacteristic.uuid) {
        if (chr->notifyEnabled(conn_hdl)) {
            Serial.println("'Notify' enabled");
        } else {
            Serial.println("'Notify' disabled");
        }
    }
}

void write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len)
{
  Serial.println(data[0]);
  digitalToggle(LED_RED); // Toggle LED   
}

void loop() 
{
  uint8_t dronedata[5] = { 0x01, 0x02, 0x03, 0x04, 0x05 }; 

  if ( Bluefruit.connected() ) {      

    // Note: We use .notify instead of .write!
    // If it is connected but CCCD is not enabled
    // The characteristic's value is still updated although notification is not sent
    if ( droneNotifyCharacteristic.notify(dronedata, num%5+1) ){
      ++num;
      Serial.print("SUCCESS"); 
    }else{
      Serial.println("ERROR: Notify not set in the CCCD or not connected!");
    }
  }

  vTaskDelay(1000);              // wait for a half second 
}

void loop2(){ }

// UUID Converter
void strUUID2Bytes(String strUUID, uint8_t binUUID[]) {
  String hexString = String(strUUID);
  hexString.replace("-", "");

  for (int i = 16; i != 0 ; i--) {
    binUUID[i - 1] = hex2c(hexString[(16 - i) * 2], hexString[((16 - i) * 2) + 1]);
  }
}

char hex2c(char c1, char c2) {
  return (nibble2c(c1) << 4) + nibble2c(c2);
}

char nibble2c(char c) {
  if ((c >= '0') && (c <= '9'))
    return c - '0';
  if ((c >= 'A') && (c <= 'F'))
    return c + 10 - 'A';
  if ((c >= 'a') && (c <= 'f'))
    return c + 10 - 'a';
  return 0;
}
