#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Ticker.h> // タイマー割り込み用
#include <Arduino.h>
#include <wifiboypro.h>
#include "wb-sprite.h"


#define DEVICE_NAME "ESP32" // デバイス名
#define SPI_SPEED 115200	// SPI通信速度


#define SIGNAL_ERROR 'E' // (Error:異常発生)


#define SERVICE_UUID "28b0883b-7ec3-4b46-8f64-8559ae036e4e"			  // サービスのUUID
#define CHARACTERISTIC_UUID_TX "2049779d-88a9-403a-9c59-c7df79e1dd7c" // 送信用のUUID


BLECharacteristic *pCharacteristicTX; // 送信用キャラクタリスティック
bool deviceConnected = false;		  // デバイスの接続状態
bool bAbnormal = false;				  // デバイス異常判定


struct Data
{ // 計測データ
	int state;
};

const int m_forward = 36;
const int m_back = 39;
const int m_right = 32;
const int m_left = 33;
uint8_t state = 0;

struct Data data;
int incomeByte[7];
int sensorData;
int z = 0;
int sum;
struct tmpSignal
{ // シグナル
	char hdr1;
	char signalCode;
};
struct tmpSignal signaldata = {0xff, 0x00};


// const int ledPin = 16; // 接続ピン
// int ledState = LOW;	// 状態


// const int buttonPin = 32; // 接続ピン


Ticker ticker;
bool bReadyTicker = false;
const float iIntervalTime = 0.2; // 計測間隔（10秒）

// 接続・切断時コールバック
class funcServerCallbacks : public BLEServerCallbacks
{
	void onConnect(BLEServer *pServer)
	{
		deviceConnected = true;
	}
	void onDisconnect(BLEServer *pServer)
	{
		deviceConnected = false;
	}
};

//  タイマー割り込み関数  //
static void kickRoutine()
{
	bReadyTicker = true;
}

void setup()
{
	wbpro_init();
	wbpro_initBuf8();
	Serial.begin(SPI_SPEED);
	// 初期化処理を行ってBLEデバイスを初期化する
	doInitialize();
	BLEDevice::init(DEVICE_NAME);

	// Serverオブジェクトを作成してコールバックを設定する
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new funcServerCallbacks());

	// Serviceオブジェクトを作成して準備処理を実行する
	BLEService *pService = pServer->createService(SERVICE_UUID);
	doPrepare(pService);

	// サービスを開始して、SERVICE_UUIDでアドバタイジングを開始する
	pService->start();
	BLEAdvertising *pAdvertising = pServer->getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->start();
	// タイマー割り込みを開始する
	ticker.attach(iIntervalTime, kickRoutine);
	// Serial.println("Waiting to connect ...");
}

void loop()
{
	wbpro_clearBuf8();
	if (digitalRead(m_forward) == LOW && digitalRead(m_left) == HIGH && digitalRead(m_right) == HIGH)
	{
		state = 1;
	}
	if (digitalRead(m_forward) == HIGH && digitalRead(m_left) == HIGH && digitalRead(m_right) == HIGH && digitalRead(m_back) == HIGH)
	{
		state = 0;
	}
	if (digitalRead(m_back) == LOW && digitalRead(m_left) == HIGH && digitalRead(m_right) == HIGH)
	{
		state = 2;
	}
	if (digitalRead(m_right) == LOW)
	{
		state = 3;
	}
	if (digitalRead(m_left) == LOW)
	{
		state = 4;
	}
	Movement();
	// 接続が確立されていて異常でなければ
	if (deviceConnected && !bAbnormal)
	{
		// タイマー割り込みによって主処理を実行する
		if (bReadyTicker)
		{
			doMainProcess();
			bReadyTicker = false;
		}
	}

	wbpro_blit8();
}


void doInitialize()
{
	pinMode(m_forward, INPUT);
	pinMode(m_back, INPUT);
	pinMode(m_left, INPUT);
	pinMode(m_right, INPUT);
	for (int i = 0; i < 256; i++) // 定義 256色（唯一色庫）
		wbpro_setPal8(i, wbpro_color565(standardColour[i][0], standardColour[i][1], standardColour[i][2]));
}

void doPrepare(BLEService *pService)
{
	// Notify用のキャラクタリスティックを作成する
	pCharacteristicTX = pService->createCharacteristic(
		CHARACTERISTIC_UUID_TX,
		BLECharacteristic::PROPERTY_NOTIFY);
	pCharacteristicTX->addDescriptor(new BLE2902());
}


void doMainProcess()
{
	
	pCharacteristicTX->setValue((uint8_t *)&data, sizeof(Data));
	pCharacteristicTX->notify();
	// シリアルモニターに表示する

	// Serial.print("Send data: ");
	// Serial.println(data.pmData);
}

void Movement()
{
	switch (state)
	{
	case 0:
		data.state = state;
		Serial.println(data.state);
		wbpro_blitBuf8(0, 0, 240, 0, 100, 240, 240, (uint8_t *)stop);
		break;

	case 1: //forward
		data.state = state;
		Serial.println(data.state);
		wbpro_blitBuf8(0, 0, 240, 0, 100, 240, 240, (uint8_t *)forward);
		break;

	case 2: //back
		data.state = state;
		Serial.println(data.state);
		wbpro_blitBuf8(0, 0, 240, 0, 100, 240, 240, (uint8_t *)backward);
		break;

	case 3: //right
		if (digitalRead(m_forward) == HIGH && digitalRead(m_back) == HIGH)
		{
			data.state = state;
			Serial.println(data.state);
		}

		if (digitalRead(m_forward) == LOW)
		{
			data.state = 5;
			Serial.println(data.state);
		}

		if (digitalRead(m_back) == LOW)
		{
			data.state = 6;
			Serial.println(data.state);
		}

		wbpro_blitBuf8(0, 0, 240, 0, 100, 240, 240, (uint8_t *)right);
		break;

	case 4: //left
		if (digitalRead(m_forward) == HIGH && digitalRead(m_back) == HIGH)
		{
			data.state = state;
			Serial.println(data.state);
		}
		if (digitalRead(m_forward) == LOW)
		{
			data.state = 7;
			Serial.println(data.state);
		}
		if (digitalRead(m_back) == LOW)
		{
			data.state = 8;
			Serial.println(data.state);
		}
		wbpro_blitBuf8(0, 0, 240, 0, 100, 240, 240, (uint8_t *)left);
		break;
	}
}
