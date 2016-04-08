#include <BLE_API.h>
#include <Wire.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_FRAM_I2C.h"

#define TXRX_SIZE                         20
#define ITEM_SIZE                         30
#define LED                               13

BLE                                       ble;

Timeout                                   timeout;
Ticker                                    tick;
Adafruit_BMP085_Unified baro_i2c         = Adafruit_BMP085_Unified(10085);
Adafruit_FRAM_I2C fram_i2c               = Adafruit_FRAM_I2C();

union Flip
{
    float input;
    int output;
};

boolean ledOn = false;

// Stores the data
static uint8_t rxBuffer[TXRX_SIZE];
static uint8_t floatBytes[4];
static uint8_t baroValues[ITEM_SIZE][4];
static uint8_t item = 0;

static uint8_t rxBufNum;
static uint8_t rxState = 0;

// The Nordic UART Service
static const uint8_t rxTxServiceUuid[]          = {0x71, 0x3D, 0, 0, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t txCharacteristicUuid[]     = {0x71, 0x3D, 0, 3, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t rxCharacteristicUuid[]     = {0x71, 0x3D, 0, 2, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t uart_base_uuid_rev[]       = {0x1E, 0x94, 0x8D, 0xF1, 0x48, 0x31, 0x94, 0xBA, 0x75, 0x4C, 0x3E, 0x50, 0, 0, 0x3D, 0x71};

uint8_t txValue[TXRX_SIZE] = {0,};
uint8_t rxValue[TXRX_SIZE] = {0,};

/**
*  @brief  Creates a new GattCharacteristic using the specified 16-bit
*          UUID, value length, and properties.
*
*  @note   The UUID value must be unique in the service and is normally >1.
*
*  @param[in]  uuid
*              The UUID to use for this characteristic.
*  @param[in]  valuePtr
*              The memory holding the initial value. The value is copied
*              into the stack when the enclosing service is added, and
*              is thereafter maintained internally by the stack.
*  @param[in]  len
*              The length in bytes of this characteristic's value.
*  @param[in]  maxLen
*              The max length in bytes of this characteristic's value.
*  @param[in]  hasVariableLen
*              Whether the attribute's value length changes over time.
*  @param[in]  props
*              The 8-bit field containing the characteristic's properties.
*  @param[in]  descriptors
*              A pointer to an array of descriptors to be included within
*              this characteristic. The memory for the descriptor array is
*              owned by the caller, and should remain valid at least until
*              the enclosing service is added to the GATT table.
*  @param[in]  numDescriptors
*              The number of descriptors in the previous array.
*
* @NOTE: If valuePtr == NULL, length == 0, and properties == READ
*        for the value attribute of a characteristic, then that particular
*        characteristic may be considered optional and dropped while
*        instantiating the service with the underlying BLE stack.
*/
GattCharacteristic bleRead(
    txCharacteristicUuid,
    txValue,
    1,
    TXRX_SIZE,
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE
);

GattCharacteristic bleWrite(
    rxCharacteristicUuid,
    rxValue,
    1,
    TXRX_SIZE,
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY
);

GattCharacteristic *uartChars[] = {&bleRead, &bleWrite};

/**
*  @brief  Creates a new GattService using the specified 16-bit
*          UUID, value length, and properties.
*
*  @note   The UUID value must be unique and is normally >1.
*
*  @param[in]  uuid
*              The UUID to use for this service.
*  @param[in]  characteristics
*              A pointer to an array of characteristics to be included within this service.
*  @param[in]  numCharacteristics
*              The number of characteristics.
*/
GattService uartService(
    rxTxServiceUuid,
    uartChars,
    sizeof(uartChars) / sizeof(GattCharacteristic *)
);

void disconnectionCallBack(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    Serial.println("Disconnected!");
    Serial.println("Restarting the advertising process");
    ble.startAdvertising();
}

void writtenHandle(const GattWriteCallbackParams *Handler)
{
    uint8_t buff[TXRX_SIZE];
    uint16_t bytesRead, index;
    String command = "";
    char c;

    Serial.println("onDataWritten : ");
    if (Handler->handle == bleRead.getValueAttribute().getHandle()) {
        
        /**
        * Read the value of a characteristic from the local GattServer.
        * @param[in]     attributeHandle
        *                  Attribute handle for the value attribute of the characteristic.
        * @param[out]    buffer
        *                  A buffer to hold the value being read.
        * @param[in/out] lengthP
        *                  Length of the buffer being supplied. If the attribute
        *                  value is longer than the size of the supplied buffer,
        *                  this variable will return the total attribute value length
        *                  (excluding offset). The application may use this
        *                  information to allocate a suitable buffer size.
        *
        * @return BLE_ERROR_NONE if a value was read successfully into the buffer.
        *
        * @note: This API is now *deprecated* and will be dropped in the future.
        * You should use the parallel API from GattServer directly. A former call
        * to ble.readCharacteristicValue() should be replaced with
        * ble.gattServer().read().
        */
        ble.readCharacteristicValue(bleRead.getValueAttribute().getHandle(), buff, &bytesRead);
        Serial.print("(uint16_t)bytesRead: ");
        Serial.println(bytesRead, HEX);
        
        for(byte index = 0; index < bytesRead; index++) {
            Serial.write(buff[index]);
            c = buff[index];
            
            if (index == 0)
            {
                // do nothing
            }
            else
            {
                command += c;
            }
        }
        Serial.println("");

        if (command == "on" && ledOn == false)
        {
            Serial.println("Turning ON the LED");
            digitalWrite(LED, LOW);
            ledOn = true;
        } 
        else if (command == "off" && ledOn == true)
        {
            Serial.println("Turning OFF the LED");
            digitalWrite(LED, HIGH);
            ledOn = false;
        }
        else
        {
            Serial.println("LED state is unchanged.");
        }
        
        // Write values to FRAM
        if (command == "write")
        {
            Flip pressure;
            int entry = 0;
            Serial.println("Writing barometer values to FRAM...");

            while(entry < ITEM_SIZE)
            {
                sensors_event_t event;
                baro_i2c.getEvent(&event);
                
                if (event.pressure)
                {
                    pressure.input = event.pressure;
                    Serial.print("Pressure:     ");
                    Serial.print(pressure.input);
                    Serial.println(" hPa");

                    /**************************************************************************/
                    /*!
                        @brief  Writes a byte at the specific FRAM address
                        
                        @params[in] i2cAddr
                                    The I2C address of the FRAM memory chip 0x50 (1010+A2+A1+A0)
                        @params[in] framAddr
                                    The 16-bit address to write to in FRAM memory
                        @params[in] i2cAddr
                                    The 8-bit value to write at framAddr
                    */
                    /**************************************************************************/
                    fram_i2c.write8(0 + (entry * 4), pressure.output);
                    fram_i2c.write8(1 + (entry * 4), pressure.output >> 8);
                    fram_i2c.write8(2 + (entry * 4), pressure.output >> 16);
                    fram_i2c.write8(3 + (entry * 4), pressure.output >> 24);
                }

                entry++;
                delay(500);
            }
        }
        // Read values from FRAM
        else if (command == "read")
        {
            Flip readPressure;
            readPressure.output = 0;
            uint8_t value;
            
            Serial.println("Reading from the FRAM...");
        
            for (uint8_t i = 0; i < ITEM_SIZE; i++)
            {
                for (uint16_t j = 0; j < 4; j++)
                {
                    /**************************************************************************/
                    /*!
                        @brief  Reads an 8 bit value from the specified FRAM address
                    
                        @params[in] i2cAddr
                                    The I2C address of the FRAM memory chip (1010+A2+A1+A0)
                        @params[in] framAddr
                                    The 16-bit address to read from in FRAM memory
                    
                        @returns    The 8-bit value retrieved at framAddr
                    */
                    /**************************************************************************/
                    value = fram_i2c.read8((i * 4) + (3 - j));
                    baroValues[i][j] = value;
        
                    readPressure.output = (readPressure.output << 8) + value;
                }
                Serial.print(readPressure.input);
                Serial.println(" hPa");
            }

            /*
             * void attach (void(*)(void)  fptr, float  t)   
             * Attach a function to be called by the Ticker, specifiying the interval in seconds.
             * 
             * Parameters:
             * fptr  pointer to the function to be called
             * t     the time between calls in seconds
             * 
             */
            tick.attach(periodicCallback, 1);
        }
        else
        {
            Serial.println("No READ or WRITE command sent.");
            Serial.println("");
        }
    }
}

void periodicCallback()
{

    //if (ble.getGapState().connected && item < ITEM_SIZE)
    if (item < ITEM_SIZE)
    {
        floatBytes[0] = baroValues[item][0];
        floatBytes[1] = baroValues[item][1];
        floatBytes[2] = baroValues[item][2];
        floatBytes[3] = baroValues[item][3];
        
        ble.updateCharacteristicValue(bleWrite.getValueAttribute().getHandle(), floatBytes, 4);
        item++;
    }
    else
    {
        tick.detach();
        item = 0;
    }
    
}

void m_uart_rx_handle()
{
    
    /**
     * Update the value of a characteristic on the local GattServer.
     *
     * @param[in] attributeHandle
     *              Handle for the value attribute of the characteristic.
     * @param[in] value
     *              A pointer to a buffer holding the new value.
     * @param[in] size
     *              Size of the new value (in bytes).
     * @param[in] localOnly
     *              Should this update be kept on the local
     *              GattServer regardless of the state of the
     *              notify/indicate flag in the CCCD for this
     *              characteristic? If set to true, no notification
     *              or indication is generated.
     *
     * @return BLE_ERROR_NONE if we have successfully set the value of the attribute.
     *
     * @note: This API is now *deprecated* and will be dropped in the future.
     * You should use the parallel API from GattServer directly. A former call
     * to ble.updateCharacteristicValue() should be replaced with
     * ble.gattServer().write().
     */
    // Update characteristic data
    ble.updateCharacteristicValue(bleWrite.getValueAttribute().getHandle(), rxBuffer, rxBufNum);
      
    // To clear an array
    memset(rxBuffer, 0x00, 20);
    rxState = 0;
}

void uart_handle(uint32_t id, SerialIrq event)
{   /* Serial rx IRQ */
    if(event == RxIrq)
    {
        if (rxState == 0)
        {
            rxState = 1;
            timeout.attach_us(m_uart_rx_handle, 100000); // 0.1 seconds
            rxBufNum = 0;
        }
        
        while(Serial.available())
        {
            if(rxBufNum < 20)
            {
                rxBuffer[rxBufNum] = Serial.read();
                rxBufNum++;
            }
            else
            {
                Serial.read();
            }
        }
    }
}

void setup() {

    // put your setup code here, to run once
    Serial.begin(9600); 
    Wire.begin();
    pinMode(LED, OUTPUT);
    Serial.attach(uart_handle);
    
    if (!baro_i2c.begin())
    {
        Serial.print("Oops, no BMP085 detected ... check your wiring or I2C address");
    }

    if (!fram_i2c.begin())
    {
        Serial.println("I2C FRAM not identified ... check your connections");
    }

    ble.init();
    ble.onDisconnection(disconnectionCallBack);
    ble.onDataWritten(writtenHandle);

    // setup adv_data and srp_data
    ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                     (const uint8_t *)"BLE Nano", sizeof("BLE Nano") - 1);
    
    ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                     (const uint8_t *)uart_base_uuid_rev, sizeof(uart_base_uuid_rev));

    // set adv_type
    ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    // add service
    ble.addService(uartService);
    // set device name
    ble.setDeviceName((const uint8_t *)"Simple Chat");
    // set tx power,valid values are -40, -20, -16, -12, -8, -4, 0, 4
    ble.setTxPower(4);
    // set adv_interval, 100ms in multiples of 0.625ms.
    ble.setAdvertisingInterval(160);
    // set adv_timeout, in seconds
    ble.setAdvertisingTimeout(0);
    // start advertising
    ble.startAdvertising();

    delay(1000);

    /*
     * Read the first byte
     */
    uint8_t test = fram_i2c.read8(0x00);
    Serial.print("Restarted ");
    Serial.print(test);
    Serial.println(" times");

    /*
     * Test write ++
     */
    fram_i2c.write8(0x00, test + 1);
    
    Serial.println("Advertising Start!");    
}

void loop() {
    ble.waitForEvent();
}
