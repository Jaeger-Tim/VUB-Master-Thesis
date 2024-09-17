"""
Here a result indicates one of four colors:
1=Orange
2=Pink
3=Purple
4=Gray

This script works great with a Random Forest Regressor, which had an error of 1/100 in tests and
MSE: 0.02837684210526315
MAE: 0.10189473684210526
R^2 Score: 0.9801102050326188

"""

import joblib
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error, mean_absolute_error, r2_score
from sklearn.linear_model import LinearRegression
from sklearn.ensemble import RandomForestRegressor
from sklearn.svm import SVR

# Load your dataset
df = pd.read_csv('data.csv')

# Assuming the last column is the target
X = df.iloc[:, :-1].values  # Features
y = df.iloc[:, -1].values   # Target

# Split the dataset into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Initialize models
models = {
    "Linear Regression": LinearRegression(),
    "Random Forest Regressor": RandomForestRegressor(n_estimators=100),
    "SVR": SVR()
}

# Train and evaluate models
for name, model in models.items():
    model.fit(X_train, y_train)
    y_pred = model.predict(X_test)

    # Evaluation
    mse = mean_squared_error(y_test, y_pred)
    mae = mean_absolute_error(y_test, y_pred)
    r2 = r2_score(y_test, y_pred)

    print(f"Model: {name}")
    print(f"MSE: {mse}")
    print(f"MAE: {mae}")
    print(f"R^2 Score: {r2}")

    # Display actual vs. predicted for inspection
    print("Actual vs. Predicted:")
    for actual, predicted in zip(y_test, y_pred):
        print(f"Actual: {actual:.2f}, Predicted: {predicted:.2f}, ", "OK" if (actual==round(predicted)) else "NOOOO")
    print("\n")
    
    joblib.dump(model, f    "{name}.pkl")
    
