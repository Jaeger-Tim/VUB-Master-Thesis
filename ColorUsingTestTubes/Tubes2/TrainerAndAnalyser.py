"""
import joblib
import pandas as pd
import numpy as np

# Assuming the model training part remains the same and you have saved the Random Forest model as 'Random Forest Regressor.pkl'

# Load the trained Random Forest Regressor model
model = joblib.load("Random Forest Regressor.pkl")

def process_and_predict(input_string):
    # Convert the input string to numpy array
    try:
        data = np.fromstring(input_string, dtype=float, sep=',')
    except ValueError as e:
        print(f"Error converting input to array: {e}")
        return

    # Check if the data has 18 features
    if data.shape[0] != 18:
        print("Input data does not have 18 features.")
        return

    # Reshape data for prediction (1, 18) since our model expects 2D array
    data = data.reshape(1, -1)

    # Predict
    prediction = model.predict(data)
    print(f"Predicted class for input data: {prediction[0]}")

# Example usage with UART input strings
uart_inputs = [
    "310.75,109.19,199.41,39.31,119.47,52.16,34.47,37.77,91.08,44.38,51.99,6.89,12.87,10.23,61.62,74.15,41.44,15.44",
    "91.13,37.65,79.22,13.37,42.25,29.05,15.84,14.76,32.21,16.83,32.62,3.99,6.43,6.82,25.53,41.44,15.84,9.08",
    "48.56,19.77,35.51,6.29,19.67,7.92,6.99,8.68,19.99,13.39,19.37,2.18,4.82,5.11,13.20,13.09,9.14,7.27",
    "29.13,10.35,17.30,3.93,9.47,5.28,5.59,6.95,15.55,11.48,17.33,2.18,4.82,4.26,8.80,9.81,6.09,6.36"
]

for input_string in uart_inputs:
    process_and_predict(input_string)
"""

import joblib
import numpy as np
import serial
import time
from termcolor import colored

# Load the trained Random Forest Regressor model
model = joblib.load("Random Forest Regressor.pkl")

def process_and_predict(input_string):
    # Convert the input string to numpy array
    try:
        data = np.fromstring(input_string, dtype=float, sep=',')
    except ValueError as e:
        print(f"Error converting input to array: {e}")
        return

    # Check if the data has 18 features
    if data.shape[0] != 18:
        print("Input data does not have 18 features.")
        return

    # Reshape data for prediction (1, 18) since our model expects 2D array
    data = data.reshape(1, -1)

    # Predict
    prediction = model.predict(data)
    print(f"Predicted class for input data: {prediction[0]}")
    rounded_value = round(prediction[0])
    color_map = {
        1: "orange",
        2: "pink",
        3: "purple",
        4: "grey"
    }
    # Determine the color based on the rounded value
    if rounded_value in color_map:
        color_name = color_map[rounded_value]
        # termcolor does not support orange, pink, or purple directly, so we use closest available colors
        termcolor_name = {'orange': 'light_red', 'pink': 'light_magenta', 'purple': 'magenta', 'grey': 'light_grey'}[color_name]
        color_text = colored(color_name, termcolor_name)
    else:
        color_name = "Unknown"
        color_text = colored(color_name, 'red')
        
    print(f"Predicted class for input data: {prediction[0]} | {rounded_value} | {color_text}")

def read_from_uart():
    # Open serial port
    ser = serial.Serial('COM7', 115200, timeout=1)
    time.sleep(2)  # wait for the serial connection to initialize

    print("Reading from UART on COM7...")
    try:
        while True:
            if ser.in_waiting > 0:
                input_string = ser.readline().decode('utf-8').strip()
                print(f"Received data: {input_string}")
                process_and_predict(input_string)
    except KeyboardInterrupt:
        print("Stopped reading from UART.")
    finally:
        ser.close()

read_from_uart()