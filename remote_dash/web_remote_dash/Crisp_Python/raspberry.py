from flask import Flask, request, jsonify

from flask_cors import CORS

app = Flask(__name__)
import json

CORS(app)

def json_file_to_object(file_path):
    with open(file_path, 'r') as f:
        data = json.load(f)  # Parses the JSON data from the file into a Python object (dict or list)
    return data

@app.route('/<helloworld>', methods=['GET'])
def give_json(helloworld):
    return helloworld


@app.route('/')
def function():
    return "<h1>Hey Ya'll!</h1> <p>You arent supposed to be here. Go to the real dash.</p>"