import paho.mqtt.client as mqtt
import base64
import json
import ssl

import sqlite3
conn = sqlite3.connect('example.db')
c = conn.cursor()

applicationID = "1"
devEUI = [""]

# 當地端程式連線伺服器得到回應時，要做的動作
def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))

    # 將訂閱主題寫在on_connet中
    # 如果我們失去連線或重新連線時 
    # 地端程式將會重新訂閱
    for i in devEUI:
        client.subscribe(f"application/{applicationID}/device/{i}/rx")

# 當接收到從伺服器發送的訊息時要進行的動作
def on_message(client, userdata, msg):
    print(msg.payload.decode())
    status, time = base64.b64decode(json.loads(msg.payload.decode())['data']).decode().split()
    print(msg.topic+" "+ status)
    c.execute(f"UPDATE beds SET status = '{status}',time = '{time}'  WHERE device = '{devEUI}';")
    conn.commit()

# 連線設定
# 初始化地端程式
client = mqtt.Client()

# 設定連線的動作
client.on_connect = on_connect

# 設定接收訊息的動作
client.on_message = on_message

# 設定登入帳號密碼
client.username_pw_set("","")

# 設定 TLS
client.tls_set(tls_version=ssl.PROTOCOL_TLSv1_2)
client.tls_insecure_set(True)

# 設定連線資訊(IP, Port, 連線時間)
client.connect("", 1883, 60)

# 開始連線，執行設定的動作和處理重新連線問題
client.loop_forever()