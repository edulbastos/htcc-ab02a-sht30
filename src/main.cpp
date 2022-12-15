//#include <Wire.h>
#include <SHT85.h>
#include "LoRaWan_APP.h"

// Endereço I2C SHT85 (SHT30)
#define SHT85_ADDRESS   0x44

// Rotinas para shift de bits para envio LoRa
#define lowByte(w)  ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

/* OTAA parameters*/
uint8_t devEui[] = { 0x22, 0x32, 0x33, 0x00, 0x00, 0x88, 0x88, 0x02 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x66, 0x01 };

/* ABP parameters*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007E6AE1;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = LORAWAN_CLASS;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;
// uint32_t appTxDutyCycle = 60000;   // 

/*OTAA or ABP*/
bool overTheAirActivation = LORAWAN_NETMODE;

/*ADR enable*/
bool loraWanAdr = LORAWAN_ADR;

/* set LORAWAN_Net_Reserve ON, the node could save the network info to flash, when node reset not need to join again */
bool keepNet = LORAWAN_NET_RESERVE;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = LORAWAN_UPLINKMODE;

/* Application port */
uint8_t appPort = 2;

/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 1;

// SHT85 instance
SHT85 sht;

// Delay time for the next transmission 
unsigned long delayTime;

/* Prepares the payload of the frame */
static void prepareTxFrame( uint8_t port )
{

    float temperature, humidity = 0;

    // Envia 4 bytes para servidor LoRa (temp e umidade)
    appDataSize = 4;

    // Lê temperatura e umidade em modo raw do SHT30
    sht.read();

    // Computa a temperatura baseada na leitura raw anterior
    Serial.print("Temperatura = ");
    temperature = sht.getTemperature();
    Serial.print(temperature, 1);
    Serial.println(" *C");

    // Codifica a temperatura para envio LoRaWAN
    appData[0] = lowByte ((int16_t) (temperature * 100.0));
    appData[1] = highByte((int16_t) (temperature * 100.0));

    // Computa a umidade baseada na leitura raw anterior
    Serial.print("Umidade = ");
    humidity = sht.getHumidity();
    Serial.print(humidity, 1);
    Serial.println(" %");

    // Codifica a umidade para envio LoRaWAN
    appData[2] = lowByte ((int16_t) (humidity * 100.0));
    appData[3] = highByte((int16_t) (humidity * 100.0));

    Serial.println();
}

void setup() {

    Serial.begin(115200);
	  pinMode(GPIO13, OUTPUT);

    // Inicializa sensor
    //Wire.begin();
    sht.begin(SHT85_ADDRESS);
    //Wire.setClock(100000);

    // Imprime status
    uint16_t stat = sht.readStatus();
    Serial.print(stat, HEX);
    Serial.println();
    
    deviceState = DEVICE_STATE_INIT;
	  LoRaWAN.ifskipjoin();
}

void loop()
{
	switch( deviceState )
	{
		case DEVICE_STATE_INIT:
		{
			printDevParam();
			LoRaWAN.init(loraWanClass,loraWanRegion);
			deviceState = DEVICE_STATE_JOIN;
			break;
		}
		case DEVICE_STATE_JOIN:
		{
			LoRaWAN.join();
			break;
		}
		case DEVICE_STATE_SEND:
		{
			prepareTxFrame( appPort );
      digitalWrite(GPIO13, HIGH);
			LoRaWAN.send();
      digitalWrite(GPIO13, LOW);
			deviceState = DEVICE_STATE_CYCLE;
			break;
		}
		case DEVICE_STATE_CYCLE:
		{
			// Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle;
			LoRaWAN.cycle(txDutyCycleTime);
			deviceState = DEVICE_STATE_SLEEP;
			break;
		}
		case DEVICE_STATE_SLEEP:
		{
			LoRaWAN.sleep();
			break;
		}
		default:
		{
			deviceState = DEVICE_STATE_INIT;
			break;
		}
	}
}