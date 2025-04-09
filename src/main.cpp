#include <WiFiClientSecure.h> // WiFiClientSecure 클래스 포함
#include <WiFi.h>             // Wi-Fi 연결을 위한 클래스
#include <PubSubClient.h>     // MQTT 클라이언트
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <time.h>
#include <esp_wpa2.h>

// Wi-Fi 설정
const char *ssid = "WIFI_NAME";
const char *password = "WIFI_PASSWORD";

// AWS IoT 설정
const char *mqtt_server = "aaaaa.iot.us-east-1.amazonaws.com";
const char *topic = "AWS_IOT_TOPIC";

// 인증서와 키
const char *root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    // 인증서를 추가해주세요
    "-----END CERTIFICATE-----\n";

const char *client_cert =
    "-----BEGIN CERTIFICATE-----\n"
    // 인증서를 추가해주세요
    "-----END CERTIFICATE-----\n";

const char *private_key =
    "-----BEGIN PRIVATE KEY-----\n"
    // 키를 추가해주세요
    "-----END PRIVATE KEY-----\n";

// 클라이언트 초기화
WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

// DHT 설정
#define DHTPIN 15     // DHT 데이터 핀
#define DHTTYPE DHT22 // DHT22 센서
DHT dht(DHTPIN, DHTTYPE);

void reconnectWiFi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Wi-Fi disconnected. Reconnecting...");
        WiFi.disconnect();
        delay(100);
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED)
        {
            Serial.print(".");
            delay(1000);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\nWi-Fi reconnected!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            Serial.println("\nWi-Fi reconnection failed!");
        }
    }
}

void connectToAWSIoT()
{
    wifiClient.setCACert(root_ca);
    wifiClient.setCertificate(client_cert);
    wifiClient.setPrivateKey(private_key);

    client.setServer(mqtt_server, 8883);

    // MQTT 서버 연결 시도
    while (!client.connected())
    {
        Serial.println("Connecting to AWS IoT...");
        if (client.connect("ESP32_Client"))
        {
            Serial.println("Connected to AWS IoT");
        }
        else
        {
            Serial.print("Failed to connect, rc=");
            Serial.println(client.state());
            delay(2000); // 2초 대기 후 재시도
        }
    }
}

// 시간 정보를 포맷화하여 반환하는 함수
String getCurrentFormattedTime()
{
    // 현재 시간 가져오기
    time_t now = time(NULL);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);

    // JST(GMT+9) 시간대로 변환
    struct tm jst_time = timeinfo;
    jst_time.tm_hour = (jst_time.tm_hour + 9) % 24;

    // HH:MM 형식으로 포맷팅
    char currentTime[6];
    snprintf(currentTime, sizeof(currentTime), "%02d:%02d", jst_time.tm_hour, jst_time.tm_min);

    return String(currentTime);
}

// DHT 센서 재초기화 함수
void resetDHT()
{
    Serial.println("Resetting DHT sensor...");

    // 센서 GPIO 핀을 한 번 LOW로 설정해서 리셋
    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(50); // 짧은 지연

    // 센서 재초기화
    dht.begin();

    Serial.println("DHT sensor reset completed");
}

// 센서 데이터 읽기 함수
bool readDHTData(float &temperature, float &humidity)
{
    // 읽기 시도
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    // 데이터 유효성 확인
    if (isnan(temperature) || isnan(humidity))
    {
        char payload[300];

        // 현재 시간 가져오기
        String currentTime = getCurrentFormattedTime();

        snprintf(payload, sizeof(payload), "{\"message\":\"Failed to read from DHT sensor\"}");
        client.publish(topic, payload);

        // 읽기 실패 시 센서 리셋 후 재시도
        resetDHT();

        // 센서 리셋 후 안정화 시간 부여
        delay(2000);

        temperature = dht.readTemperature();
        humidity = dht.readHumidity();

        if (!isnan(temperature) && !isnan(humidity))
        {
            snprintf(payload, sizeof(payload), "{\"message\":\"DHT read successful after reset\"}");
            client.publish(topic, payload);
            return true;
        }

        return false;
    }

    return true;
}

void setup()
{
    Serial.begin(115200);

    // WIFI 연결
    WiFi.mode(WIFI_STA);
    reconnectWiFi();

    // DHT 센서 초기화
    pinMode(DHTPIN, INPUT_PULLUP); // 풀업 저항 활성화
    delay(1000);                   // 센서 안정화 시간
    dht.begin();

    // AWS IoT 연결
    connectToAWSIoT();

    // NTP 서버 설정
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        reconnectWiFi();
    }
    if (!client.connected())
    {
        connectToAWSIoT();
    }
    client.loop();

    // DHT22 온도 및 습도 측정 변수
    float temperature = 0;
    float humidity = 0;
    bool validData = false;

    // 현재 시간 가져오기
    time_t now = time(NULL);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);

    // 20분마다 메시지 발송
    if (timeinfo.tm_min % 20 == 0 && timeinfo.tm_sec == 0)
    {
        // 센서 데이터 읽기
        validData = readDHTData(temperature, humidity);

        // 현재 시간 가져오기
        String currentTime = getCurrentFormattedTime();

        char payload[300];
        if (validData)
        {
            // 센서 데이터가 유효한 경우 - 정상 메시지 전송
            if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && timeinfo.tm_sec == 0)
            {
                snprintf(payload, sizeof(payload),
                         "{\"temperature\":%.2f,\"humidity\":%.2f,\"createThread\":true,\"currentTime\":\"%s\"}",
                         temperature, humidity, currentTime.c_str());
            }
            else
            {
                snprintf(payload, sizeof(payload),
                         "{\"temperature\":%.2f,\"humidity\":%.2f,\"currentTime\":\"%s\"}",
                         temperature, humidity, currentTime.c_str());
            }
            client.publish(topic, payload);
        }
        else
        {
            // 센서 데이터가 유효하지 않은 경우 - 오류 메시지 전송
            if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && timeinfo.tm_sec == 0)
            {
                snprintf(payload, sizeof(payload), "{\"createThread\":true,\"message\":\"Failed to read from DHT sensor\"}");
                client.publish(topic, payload);
            }
        }
    }

    delay(1000);
}