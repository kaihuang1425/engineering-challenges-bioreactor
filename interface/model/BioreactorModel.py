#stores samples, history, and pending setpoint requests.
import json
from .controller import DashboardController
from .view import DashboardView

class BioreactorModel():
    def __init__(self):
        self.history = []
        self.latest = None

    def append(self, data):
        self.latest = data
        self.history.append(data)

class Packet():
    def __init__(self, data):
        self.timestamp = data["timestamp"]
        self.measured = data["measured"]
        self.actuators = data["actuators"]

    @classmethod
    def load(cls, path):
        with open(path) as file:
            return cls(json.loads(file))

packet = Packet.load("./test/test.json")
print(packet.timestamp)