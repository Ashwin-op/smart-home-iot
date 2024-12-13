from datetime import datetime

import boto3
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


dynamodb = boto3.resource("dynamodb", region_name="us-east-1")
table = dynamodb.Table("fastapi-table")


def update_dynamodb(item):
    table.put_item(Item=item)


def get_from_dynamodb(key):
    response = table.get_item(Key=key)
    return response.get("Item")


@app.post("/led/{led_id}")
def set_led(led_id: str, state: str):
    if led_id not in ["led1", "led2"]:
        return {"error": "Invalid LED ID"}
    if state not in ["on", "off"]:
        return {"error": "Invalid state"}
    update_dynamodb({"device": led_id, "state": state})
    return {led_id: state}


@app.get("/led/{led_id}")
def get_led(led_id: str):
    if led_id not in ["led1", "led2"]:
        return {"error": "Invalid LED ID"}
    item = get_from_dynamodb({"device": led_id})
    if item:
        return {led_id: item["state"]}
    return {"error": "LED ID not found"}


@app.post("/fan")
def set_fan(state: str):
    if state not in ["on", "off"]:
        return {"error": "Invalid state"}
    update_dynamodb({"device": "fan", "state": state})
    return {"fan": state}


@app.get("/fan")
def get_fan():
    item = get_from_dynamodb({"device": "fan"})
    if item:
        return {"fan": item["state"]}
    return {"error": "Fan state not found"}


@app.post("/alarm")
def set_alarm(time: str):
    try:
        datetime.strptime(time, "%H:%M").time()
        update_dynamodb({"device": "alarm", "time": time})
        return {"alarm": time}
    except ValueError:
        return {"error": "Invalid time format. Use HH:MM"}


@app.get("/alarm")
def get_alarm():
    item = get_from_dynamodb({"device": "alarm"})
    if item:
        return {"alarm": item["time"]}
    return {"alarm": None}


@app.get("/health")
def health():
    return {"status": "ok"}
