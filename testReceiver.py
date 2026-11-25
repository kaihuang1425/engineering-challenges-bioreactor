import json
import uuid
import paho.mqtt.client as mqtt

BROKER = "engf0001.cs.ucl.ac.uk"
PORT = 1883
TOPIC_TELEMETRY = "group31/bioreactor/telemetry"
TOPIC_SETPOINTS = "group31/bioreactor/setpoints/set"
TOPIC_ALERTS = "group31/bioreactor/alerts"

RANGES = {"temp": (25, 35), "ph": (3, 7), "rpm": (500, 1500)}


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        client.subscribe([(TOPIC_TELEMETRY, 0), (TOPIC_ALERTS, 0)])
        print("Connected to UCL broker")
    else:
        print("MQTT connection failed:", reason_code)


def on_message(client, userdata, message):
    try:
        data = json.loads(message.payload.decode("utf-8"))
        print(f"\n{message.topic}: {json.dumps(data)}")
    except (UnicodeDecodeError, json.JSONDecodeError):
        print("Invalid JSON received on", message.topic)


def valid_setpoints(data):
    if not isinstance(data, dict) or not data:
        return False
    for key, value in data.items():
        if key not in RANGES or isinstance(value, bool) or not isinstance(value, (int, float)):
            return False
        low, high = RANGES[key]
        if not low <= value <= high:
            return False
    return True


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=f"group31_test_{uuid.uuid4().hex[:8]}")
client.on_connect = on_connect
client.on_message = on_message
client.reconnect_delay_set(min_delay=1, max_delay=30)

try:
    client.connect(BROKER, PORT, keepalive=30)
    client.loop_start()
    while True:
        entered = input("Setpoints JSON (or Enter to skip): ").strip()
        if not entered:
            continue
        try:
            data = json.loads(entered)
        except json.JSONDecodeError:
            print('Example: {"temp":31.5} or {"ph":5,"rpm":1200}')
            continue
        if not valid_setpoints(data):
            print("Invalid setpoint values")
            continue
        info = client.publish(TOPIC_SETPOINTS, json.dumps(data), qos=1, retain=False)
        info.wait_for_publish()
        print("Published" if info.rc == mqtt.MQTT_ERR_SUCCESS else "Publish failed")
except (KeyboardInterrupt, EOFError):
    pass
finally:
    client.disconnect()
    client.loop_stop()
