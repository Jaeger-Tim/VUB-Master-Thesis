import joblib
import numpy as np
import serial
import time
from termcolor import colored

# Load the trained Random Forest Regressor model
model = joblib.load("best_random_forest_regressor.pkl")

def process_and_predict(input_string):
    # Convert the input string to numpy array
    try:
        data = np.fromstring(input_string, dtype=float, sep=',')
    except ValueError as e:
        print(f"Error converting input to array: {e}")
        return

    # Check if the data has the original number of features before feature engineering
    original_feature_count = 18  # Adjust this number based on your original dataset
    if data.shape[0] != original_feature_count:
        print(f"Input data does not have {original_feature_count} features.")
        return

    # Reshape data for feature engineering (1, original_feature_count)
    data = data.reshape(1, -1)

    # Feature Engineering (replicate the steps from your training script)
    mean = np.mean(data, axis=1)
    std = np.std(data, axis=1)
    min_val = np.min(data, axis=1)
    max_val = np.max(data, axis=1)
    range_val = max_val - min_val

    # Combine original features with engineered features
    engineered_data = np.concatenate((data, mean[:, None], std[:, None], min_val[:, None], max_val[:, None], range_val[:, None]), axis=1)

    # Predict
    prediction = model.predict(engineered_data)
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