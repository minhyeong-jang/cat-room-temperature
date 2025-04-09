# 고양이 방 온도 측정 프로젝트 (Cat Room Temperature Monitoring)

## 프로젝트 개요
이 프로젝트는 ESP32와 DHT22 센서를 활용하여 고양이가 있는 방의 온도와 습도를 측정하고, 그 데이터를 AWS IoT Core로 전송하는 시스템입니다. 20분마다 자동으로 데이터를 수집하여 클라우드로 전송합니다.

## 주요 기능
- DHT22 센서를 사용한 온도 및 습도 측정
- 데이터를 AWS IoT Core로 MQTT 프로토콜을 통해 전송
- 20분 간격으로 측정 및 데이터 전송
- 매일 자정에 새로운 스레드 생성 (createThread 플래그 활성화)
- 센서 오류 감지 및 자동 복구 기능
- 오류 발생 시 자동으로 에러 메시지 전송

## 하드웨어 구성요소
- ESP32 개발 보드
- DHT22 온도 및 습도 센서
- 필요한 케이블 및 연결 구성품

## 소프트웨어 의존성
- Arduino 프레임워크
- PubSubClient 라이브러리 (MQTT 클라이언트)
- DHT 센서 라이브러리
- Adafruit Unified Sensor 라이브러리

## 설치 및 사용 방법
1. PlatformIO를 사용하여 프로젝트를 열거나 Arduino IDE로 가져옵니다.
2. `src/main.cpp` 파일에서 다음 값들을 환경에 맞게 수정합니다:
   - WiFi SSID 및 비밀번호
   - AWS IoT 엔드포인트
   - AWS IoT 인증서 및 키
3. DHT22 센서를 ESP32의 지정된 핀(기본값: 15번 핀)에 연결합니다.
4. 코드를 ESP32에 업로드합니다.
5. 시리얼 모니터(115200 baud)를 통해 디버그 메시지를 확인합니다.

## 작동 방식
- 시스템이 시작되면 WiFi에 연결하고 AWS IoT 서비스에 연결됩니다.
- 20분마다 온도 및 습도 데이터를 측정하고 AWS IoT로 전송합니다.
- 데이터는 JSON 형식으로 다음과 같이 구성됩니다:
  ```json
  {
    "temperature": 25.50,
    "humidity": 60.20,
    "currentTime": "14:20"
  }
  ```
- 센서 읽기에 실패할 경우 자동으로 센서를 재설정하고 다시 시도합니다.
- 오류가 발생하면 다음과 같은 형식의 에러 메시지를 AWS IoT로 전송합니다:
  ```json
  {
    "message": "Failed to read from DHT sensor"
  }
  ```
- 센서 재설정 후 읽기에 성공하면 다음 메시지를 전송합니다:
  ```json
  {
    "message": "DHT read successful after reset"
  }
  ```
- 자정에 에러가 발생하면 새로운 스레드 생성 플래그를 함께 포함하여 메시지를 전송합니다:
  ```json
  {
    "createThread": true,
    "message": "Failed to read from DHT sensor"
  }
  ```

## 문제 해결
- 센서 연결 문제: DHT22 센서가 올바르게 연결되었는지 확인하고, 필요한 경우 풀업 저항을 확인하세요.
- WiFi 연결 문제: SSID와 비밀번호가 올바른지 확인하세요.
- AWS IoT 연결 문제: 인증서와 키가 올바르게 설정되었는지 확인하세요.

---

## Project Overview (English)
This project is a temperature and humidity monitoring system designed for a cat's room, using an ESP32 microcontroller with a DHT22 sensor. The system automatically collects data every 20 minutes and transmits it to AWS IoT Core.

## Key Features
- Temperature and humidity measurement using DHT22 sensor
- Data transmission to AWS IoT Core via MQTT protocol
- Measurements and data transmission at 20-minute intervals
- Daily thread creation at midnight (createThread flag activation)
- Sensor error detection and automatic recovery
- Automatic error message transmission when issues occur

## Hardware Components
- ESP32 development board
- DHT22 temperature and humidity sensor
- Necessary cables and connection components

## Software Dependencies
- Arduino framework
- PubSubClient library (MQTT client)
- DHT sensor library
- Adafruit Unified Sensor library

## Installation and Usage
1. Open the project using PlatformIO or import it into Arduino IDE.
2. Modify the following values in the `src/main.cpp` file to match your environment:
   - WiFi SSID and password
   - AWS IoT endpoint
   - AWS IoT certificates and keys
3. Connect the DHT22 sensor to the designated pin on the ESP32 (default: pin 15).
4. Upload the code to the ESP32.
5. Monitor debug messages via the serial monitor (115200 baud).

## Operation
- Upon startup, the system connects to WiFi and the AWS IoT service.
- Every 20 minutes, it measures temperature and humidity data and transmits it to AWS IoT.
- Data is formatted in JSON as follows:
  ```json
  {
    "temperature": 25.50,
    "humidity": 60.20,
    "currentTime": "14:20"
  }
  ```
- If sensor reading fails, it automatically resets the sensor and tries again.
- When an error occurs, it sends an error message to AWS IoT in this format:
  ```json
  {
    "message": "Failed to read from DHT sensor"
  }
  ```
- If sensor reading is successful after reset, it sends this message:
  ```json
  {
    "message": "DHT read successful after reset"
  }
  ```
- If an error occurs at midnight, it includes a thread creation flag in the message:
  ```json
  {
    "createThread": true,
    "message": "Failed to read from DHT sensor"
  }
  ```

## Troubleshooting
- Sensor connection issues: Verify that the DHT22 sensor is properly connected and check the pull-up resistor if necessary.
- WiFi connection issues: Ensure SSID and password are correct.
- AWS IoT connection issues: Verify that certificates and keys are properly set up.
