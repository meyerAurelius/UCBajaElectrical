import datetime
import json
import random
from time import sleep

file_path = r"C:\Users\prudh\Desktop\Baja\UCBajaElectrical\remote_dash\web_remote_dash\Jam_WebDash\TheoreticalDataIn.json"


def falsedata():
    Data_Stream = """{\n"Logging_Event":1,\n"Logging_Data":\n["""
    for i in range(0, 10):
      randomData = random.randint(1, 8)

      timestamp = datetime.datetime.now()

      match randomData:
          case 1:
              Data_Stream += """{"timestamp": \"""" + str(timestamp) + """\", "type": "Accel", "device_id": "1", "data": [""" + str(random.randint(0, 1000)) + """]}"""
          case 2:
              Data_Stream += """{"timestamp": \"""" + str(timestamp) + """\", "type": "Accel", "device_id": "2", "data": [""" + str(random.randint(0, 1000)) + """]}"""
          case 3:
              Data_Stream += """{"timestamp": \"""" + str(timestamp) + """\", "type": "Accel", "device_id": "3", "data":[""" + str(random.randint(0, 1000)) + """]}"""
          case 4: # Engine Temp
              Data_Stream += """{"timestamp": \"""" + str(timestamp) + """\", "type": "EnigneTemp", "device_id": "1", "data":[""" + str(random.randint(0, 1000)) + """]}"""
          case 5:
              Data_Stream += """{"timestamp": \"""" + str(timestamp) + """\", "type": "EngineRPM", "device_id": "1", "data":[""" + str(random.randint(0, 1000)) + """]}"""
          case 6:
              Data_Stream += """{"timestamp": \"""" + str(timestamp) + """\", "type": "TransTemp", "device_id": "1", "data":[""" + str(random.randint(0, 1000)) + """]}"""
          case 7:
              Data_Stream += """{"timestamp": \"""" + str(timestamp) + """\", "type": "TransRPM", "device_id": "1", "data":[""" + str(random.randint(0, 1000)) + """]}"""
          case 8:
              Data_Stream += """{"timestamp": \"""" + str(timestamp) + """\", "type": "Voltage", "device_id": "1", "data":[""" + str(random.randint(0, 16)) + """]}"""
      if i != 9:
          Data_Stream += ","
      Data_Stream += "\n"
      sleep(0.1)
    Data_Stream += """]\n}"""

    with open(file_path, "w") as file:
       file.write(Data_Stream)
    file.close()


