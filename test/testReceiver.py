import paho.mqtt.client as mqtt
import threading

BROKER = "broker.hivemq.com"

SUBSCRIBE_TOPIC = "uni-project-23827/python"
PUBLISH_TOPIC = "uni-project-23827/arduino"

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Connected to MQTT!")
        client.subscribe(SUBSCRIBE_TOPIC)
    else:
        print("Connection failed:", reason_code)

def on_message(client, userdata, msg):
    print("\nReceived from Arduino:", msg.payload.decode())

client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id="uni-python-23827"
)

client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, 1883)
client.loop_start()

try:
    while True:
        message = input("Enter message to Arduino: ")
        info = client.publish(PUBLISH_TOPIC, message)
        info.wait_for_publish()
        print("Sent!")
except (KeyboardInterrupt, EOFError):
    pass
finally:
    client.disconnect()
    client.loop_stop()
