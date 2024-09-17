import serial
import joblib
import numpy as np
import pandas as pd

def load_model_and_scaler(model_path='model.joblib', scaler_path='scaler.joblib'):
    """Loads and returns the machine learning model and scaler."""
    try:
        model = joblib.load(model_path)
        print(f"Model loaded successfully from {model_path}.")
    except FileNotFoundError:
        print(f"Model file {model_path} not found.")
        model = None

    try:
        scaler = joblib.load(scaler_path)
        print(f"Scaler loaded successfully from {scaler_path}.")
    except FileNotFoundError:
        print(f"Scaler file {scaler_path} not found.")
        scaler = None

    return model, scaler

def predict_label(data, model, scaler):
    """Preprocesses the data, predicts the label using the loaded model, and prints the prediction."""
    try:
        data_array = np.fromstring(data, sep=',').reshape(1, -1)
        if scaler:
            data_array = scaler.transform(data_array)
        if model:
            prediction = model.predict(data_array)
            print(f"Predicted Label: {prediction[0]}")
        else:
            print("Model not loaded, cannot predict.")
    except ValueError as e:
        print(f"Invalid data. Error: {e}")

def main():
    model, scaler = load_model_and_scaler()

    try:
        with serial.Serial('COM7', 115200, timeout=1) as ser:
            print("Serial port COM7 opened successfully.")
            while True:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    print(f"Received: {line}")
                    if len(line.split(',')) == 18:
                        predict_label(line, model, scaler)
                    else:
                        print("Invalid data format. Expected 18 comma-separated values.")
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
    except KeyboardInterrupt:
        print("Program terminated by user.")
    finally:
        print("Serial port closed.")

if __name__ == "__main__":
    main()