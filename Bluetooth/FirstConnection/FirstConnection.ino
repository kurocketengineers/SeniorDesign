#include <BLE_API.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include "Adafruit_FRAM_I2C.h"
#define TXRX_BUF_LEN                      20

//Sensors
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

Adafruit_FRAM_I2C fram     = Adafruit_FRAM_I2C();
uint16_t          framAddr = 0;
uint16_t          a = 0;
boolean           startProgram = 0;

//Bluetooth
BLE                                 	    ble;
Timeout                                   timeout;

static uint8_t rx_buf[TXRX_BUF_LEN];
static uint8_t rx_buf_num;
static uint8_t rx_state=0;

// The Nordic UART Service
static const uint8_t service1_uuid[]                = {0x71, 0x3D, 0, 0, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_tx_uuid[]             = {0x71, 0x3D, 0, 3, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_rx_uuid[]             = {0x71, 0x3D, 0, 2, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t uart_base_uuid_rev[]           = {0x1E, 0x94, 0x8D, 0xF1, 0x48, 0x31, 0x94, 0xBA, 0x75, 0x4C, 0x3E, 0x50, 0, 0, 0x3D, 0x71};

uint8_t tx_value[TXRX_BUF_LEN] = {0,};

//uint8_t rx_value[TXRX_BUF_LEN] = {0,};
float   rx_value = 0;

//Creates a characteristic responsible for writing over BLE
GattCharacteristic  characteristic1(service1_tx_uuid, tx_value, 1, TXRX_BUF_LEN, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE );

//Creates a characteristic responsible for reading over BLE
GattCharacteristic  characteristic2(service1_rx_uuid, rx_value, 1, TXRX_BUF_LEN, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

//Creates an arrary of the previous two characteristics
GattCharacteristic *uartChars[] = {&characteristic1, &characteristic2};

//Creates the Gatt service with the two characteristics above.
GattService         uartService(service1_uuid, uartChars, sizeof(uartChars) / sizeof(GattCharacteristic *));


void disconnectionCallBack(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    Serial.println("Disconnected!");
    Serial.println("Restarting the advertising process");
    ble.startAdvertising();
}

void writtenHandle(const GattWriteCallbackParams *Handler)
{
    uint8_t buf[TXRX_BUF_LEN];
    uint16_t bytesRead, index;

    Serial.println("onDataWritten : ");
    if (Handler->handle == characteristic1.getValueAttribute().getHandle()) {
        ble.readCharacteristicValue(characteristic1.getValueAttribute().getHandle(), buf, &bytesRead);
        Serial.print("bytesRead: ");
        Serial.println(bytesRead, HEX);
        for(byte index=0; index<bytesRead; index++) {
            Serial.write(buf[index]);
        }
        Serial.println("");
    }
}

void m_uart_rx_handle()
{   //update characteristic data
    ble.updateCharacteristicValue(characteristic2.getValueAttribute().getHandle(), rx_buf, rx_buf_num);
    //20, TXRX_BUF_LEN?
    memset(rx_buf, 0x00, 20);
    rx_state = 0;
}

void uart_handle(uint32_t id, SerialIrq event)
{   /* Serial rx IRQ */
    if(event == RxIrq) {
        //stop receiving for 100 ms?
        if (rx_state == 0) {
            rx_state = 1;
            timeout.attach_us(m_uart_rx_handle, 100000);
            rx_buf_num=0;
        }
        while(Serial.available()) {
            //20, TXRX_BUF_LEN?
            if(rx_buf_num < 20) {
                rx_buf[rx_buf_num] = Serial.read();
                rx_buf_num++;
            }
            else {
                Serial.read();
            }
        }
    }
}

void setup() {

    // put your setup code here, to run once
    Serial.begin(9600);
    //Serial.attach(uart_handle);
    Wire.begin();

    //BLE setup
    ble.init();
    ble.onDisconnection(disconnectionCallBack);
    ble.onDataWritten(writtenHandle);

    // setup adv_data and srp_data
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                     (const uint8_t *)"NANO", sizeof("NANO") - 1);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                     (const uint8_t *)uart_base_uuid_rev, sizeof(uart_base_uuid_rev));

    // set adv_type
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    // add service
    ble.gattServer().addService(uartService);
    // set device name
    ble.gap().setDeviceName((const uint8_t *)"Simple Chat");
    // set tx power,valid values are -40, -20, -16, -12, -8, -4, 0, 4
    ble.gap().setTxPower(4);
    // set adv_interval, 100ms in multiples of 0.625ms.
    ble.gap().setAdvertisingInterval(160);
    // set adv_timeout, in seconds
    ble.gap().setAdvertisingTimeout(0);
    // start advertising
    ble.gap().startAdvertising();

    Serial.println("Advertising Start!");
}

void loop() {
    //Have the app request the pressure
    ble.waitForEvent();

  
    /* Get a new sensor event */ 
    sensors_event_t event;
    bmp.getEvent(&event);
   
    /* Display the results (barometric pressure is measure in hPa) */
    if (event.pressure)
    {
      /* Display atmospheric pressue in hPa */
      //Serial.print("Pressure:    ");
      //Serial.print(event.pressure);
      //Serial.println(" hPa");
      tx_value = event.pressure;
      
      
      /* Calculating altitude with reasonable accuracy requires pressure    *
       * sea level pressure for your position at the moment the data is     *
       * converted, as well as the ambient temperature in degress           *
       * celcius.  If you don't have these values, a 'generic' value of     *
       * 1013.25 hPa can be used (defined as SENSORS_PRESSURE_SEALEVELHPA   *
       * in sensors.h), but this isn't ideal and will give variable         *
       * results from one day to the next.                                  *
       *                                                                    *
       * You can usually find the current SLP value by looking at weather   *
       * websites or from environmental information centers near any major  *
       * airport.                                                           *
       *                                                                    *
       * For example, for Paris, France you can check the current mean      *
       * pressure and sea level at: http://bit.ly/16Au8ol                   */
       
      /* First we get the current temperature from the BMP085 */
      //float temperature;
      //bmp.getTemperature(&temperature);
      //Serial.print("Temperature: ");
      //Serial.print(temperature);
      //Serial.println(" C");
   
      /* Then convert the atmospheric pressure, SLP and temp to altitude    */
      /* Update this next line with the current SLP for better results      */
      //float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
      //Serial.print("Altitude:    "); 
      //Serial.print(bmp.pressureToAltitude(seaLevelPressure,
        //                                  event.pressure,
          //                                temperature)); 
      //Serial.println(" m");
      //Serial.println("");
    }
    else
    {
      //Serial.println("Sensor error");
    }
  

}
