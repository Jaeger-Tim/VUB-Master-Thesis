import serial
import joblib
import numpy as np
import pandas as pd

# Function to load models
def load_models():
    models = {}
    #model_names = ['LogisticRegression', 'RandomForestClassifier', 'SVC']
    model_names = ['RandomForestClassifier']
    for name in model_names:
        try:
            models[name] = joblib.load(f'best_{name}.joblib')
            print(f"Model {name} loaded successfully.")
        except FileNotFoundError:
            print(f"Model file for {name} not found.")
    return models

# Load all models
models = load_models()

# Function to process and predict the label of the input data with all models
def predict_label(data):
    try:
        # Convert the comma-separated string to a numpy array of floats
        data_array = np.array(data.split(','), dtype=float).reshape(1, -1)
        # Create a DataFrame without specifying column names, matching training
        data_df = pd.DataFrame(data_array)
        # Predict the label using each loaded model
        for name, model in models.items():
            prediction = model.predict(data_df)
            print(f"Predicted Label by {name}: {prediction[0]}")
    except ValueError as e:
        print(f"Invalid data. Error: {e}")

# Setup serial connection
try:
    ser = serial.Serial('COM7', 115200, timeout=1)
    print("Serial port COM7 opened successfully.")
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    exit()

# Read and process data from serial port
try:
    while True:
        line = ser.readline().decode('utf-8').strip()
        if line:  # If line is not empty
            print(f"Received: {line}")
            if len(line.split(',')) == 18:  # Check if the data format is correct
                predict_label(line)
            else:
                print("Invalid data format. Expected 18 comma-separated values.")
except KeyboardInterrupt:
    print("Program terminated by user.")
finally:
    ser.close()
    print("Serial port closed.")