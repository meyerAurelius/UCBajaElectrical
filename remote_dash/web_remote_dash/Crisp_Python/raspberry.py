from quart import Quart, request
import json
from quart_cors import cors
import Data_Falsifier
app = Quart(__name__)
app = cors(app, allow_origin="*")

@app.route('/')
async def send__data():
    Data_Falsifier.falsedata()
    with open(r"C:\Users\prudh\Desktop\Baja\UCBajaElectrical\remote_dash\web_remote_dash\Jam_WebDash\TheoreticalDataIn.json", "r") as file:
        data = json.load(file)
        return data

