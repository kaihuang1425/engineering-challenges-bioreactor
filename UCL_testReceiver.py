import json
import paho.mqtt.client as mqtt

BROKER = "engf0001.cs.ucl.ac.uk"
PORT = 1883  # Initial test assumption: confirm UCL's port/security requirements.

# Keep your existing topic direction: ESP32 -> Python / Python -> ESP32.
SUBSCRIBE_TOPIC = "uni-project-23827/python"
PUBLISH_TOPIC = "uni-project-23827/arduino"


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Connected to UCL MQTT broker!")
        result, mid = client.subscribe(SUBSCRIBE_TOPIC)
        print(f"Subscribing to {SUBSCRIBE_TOPIC} (result={result})")
    else:
        print("Connection failed:", reason_code)


def on_message(client, userdata, msg):
    text = msg.payload.decode("utf-8", errors="replace")
    try:
        data = json.loads(text)
        print(f"\nReceived from ESP32 on {msg.topic}: {data}")
        # TODO: Pass real sensor telemetry to the dashboard/fault detector.
    except json.JSONDecodeError:
        print(f"\nReceived from ESP32 on {msg.topic}: {text}")


client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id="uni-python-23827",
)
client.on_connect = on_connect
client.on_message = on_message
client.reconnect_delay_set(min_delay=1, max_delay=30)

try:
    client.connect(BROKER, PORT, keepalive=30)
    client.loop_start()
    while True:
        message = input("Enter message or JSON to ESP32: ")
        info = client.publish(PUBLISH_TOPIC, message, qos=0)
        info.wait_for_publish()
        print("Sent!")
except (KeyboardInterrupt, EOFError):
    pass
except OSError as exc:
    print(f"Cannot connect to {BROKER}:{PORT}: {exc}")
    print("Check UCL network access, DNS, broker port and credentials.")
finally:
    client.disconnect()
    client.loop_stop()
