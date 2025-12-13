# Example using Flask-Sock
from flask import Flask
from flask_sock import Sock

app = Flask(__name__)
sock = Sock(app)


@app.route('/')
def index():
    return "Hello, Flask!"


@sock.route('/echo')
def echo(ws):
    while True:
        data = ws.receive()
        ws.send(data)