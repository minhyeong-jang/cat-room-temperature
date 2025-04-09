import json
import requests
import boto3
from datetime import datetime

# DynamoDB와 Slack 설정
dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('TemperatureSlackMessages')

SLACK_BOT_TOKEN = "xoxb-00000000000000000000000000000000"
SLACK_CHANNEL = "#단추-케어방"

def lambda_handler(event, context):
    # IoT Rule에서 전달된 메시지 처리
    temperature = event.get("temperature")
    humidity = event.get("humidity")
    createThread = event.get("createThread")
    currentTime = event.get("currentTime")
    errorMessage = event.get("message")

    try:
        if createThread:
            # 새 Slack 스레드 생성
            title = ":thermometer: 오늘의 단추방 :thermometer:"
            response = requests.post(
                "https://slack.com/api/chat.postMessage",
                headers={"Authorization": f"Bearer {SLACK_BOT_TOKEN}"},
                data={"channel": SLACK_CHANNEL, "text": title},
            )
            slack_response = response.json()

            if not slack_response.get("ok"):
                raise Exception(f"Slack API Error: {slack_response.get('error')}")

            # 스레드 ID 저장
            thread_ts = slack_response["ts"]
            table.put_item(
                Item={
                    "MessageID": thread_ts,
                    "Timestamp": datetime.utcnow().isoformat()
                }
            )
        else:
            # DynamoDB에서 스레드 ID 가져오기
            response = table.scan()
            if "Items" not in response or not response["Items"]:
                print("No items found in DynamoDB")
                return {
                    "statusCode": 400,
                    "body": json.dumps("No items found in DynamoDB")
                }

            sorted_items = sorted(response["Items"], key=lambda x: x['Timestamp'], reverse=True)
            
            # 가장 최신 항목 가져오기
            thread_ts = sorted_items[0]["MessageID"]

        if temperature is None or humidity is None or errorMessage:
            requests.post(
                "https://slack.com/api/chat.postMessage",
                headers={"Authorization": f"Bearer {SLACK_BOT_TOKEN}"},
                data={
                    "channel": SLACK_CHANNEL,
                    "text": errorMessage if errorMessage else "Invalid payload: Missing temperature or humidity",
                    "thread_ts": thread_ts
                },
            )
            return {
                "statusCode": 400,
                "body": json.dumps("Invalid payload: Missing temperature or humidity")
            }

        # 메시지 생성 및 Slack 스레드에 추가
        temperature = float(temperature)
        temperatureAlert = temperature >= 27
        if temperatureAlert:
            message = f"단추방이 핫해지고 있다옹 :crazycat: 온도를 주시해달라옹 ( {temperature}°C )"
        else:
            message = f"{currentTime} / 온도 - {temperature}°C, 습도 - {humidity}%"

        response = requests.post(
            "https://slack.com/api/chat.postMessage",
            headers={"Authorization": f"Bearer {SLACK_BOT_TOKEN}"},
            data={
                "channel": SLACK_CHANNEL,
                "text": message,
                "thread_ts": thread_ts,
                "reply_broadcast": temperatureAlert
            },
        )
        slack_response = response.json()

        if not slack_response.get("ok"):
            raise Exception(f"Slack API Error: {slack_response.get('error')}")

    except Exception as e:
        print(f"Error: {str(e)}")
        return {
            "statusCode": 500,
            "body": json.dumps(f"Internal server error: {str(e)}")
        }

    return {
        "statusCode": 200,
        "body": json.dumps("Success")
    }
