# import pandas as pd
# import numpy as np
# from sklearn.model_selection import train_test_split, RandomizedSearchCV
# from sklearn.metrics import mean_squared_error, mean_absolute_error, r2_score
# from sklearn.ensemble import RandomForestRegressor, ExtraTreesRegressor, GradientBoostingRegressor
# from sklearn.svm import SVR
# import joblib

# # Load your dataset
# df = pd.read_csv('data.csv')

# # Assuming the last column is the target
# X = df.iloc[:, :-1].values  # Features
# y = df.iloc[:, -1].values   # Target

# # Split the dataset into training and testing sets
# X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# # Initialize models
# models = {
#     "Random Forest Regressor": RandomForestRegressor(),
#     "Extra Trees Regressor": ExtraTreesRegressor(),
#     "Gradient Boosting Regressor": GradientBoostingRegressor(),
#     "SVR": SVR()
# }

# # Hyperparameters to tune Random Forest Regressor
# rf_params = {
#     'n_estimators': [100, 200, 300],
#     'max_depth': [None, 10, 20, 30],
#     'min_samples_split': [2, 5, 10],
#     'min_samples_leaf': [1, 2, 4]
# }

# # Train and evaluate models
# for name, model in models.items():
#     if name == "Random Forest Regressor":
#         # Hyperparameter tuning
#         rf_random = RandomizedSearchCV(estimator=model, param_distributions=rf_params, n_iter=100, cv=3, verbose=2, random_state=42, n_jobs=-1)
#         rf_random.fit(X_train, y_train)
#         model = rf_random.best_estimator_
#         print(f"Best parameters for Random Forest Regressor: {rf_random.best_params_}")
#     else:
#         model.fit(X_train, y_train)

#     y_pred = model.predict(X_test)

#     # Evaluation
#     mse = mean_squared_error(y_test, y_pred)
#     mae = mean_absolute_error(y_test, y_pred)
#     r2 = r2_score(y_test, y_pred)

#     print(f"Model: {name}")
#     print(f"MSE: {mse}")
#     print(f"MAE: {mae}")
#     print(f"R^2 Score: {r2}\n")

#     # Display actual vs. predicted for inspection
#     print("Actual vs. Predicted:")
#     for actual, predicted in zip(y_test, y_pred):
#         print(f"Actual: {actual:.2f}, Predicted: {predicted:.2f}, ", "OK" if (actual==round(predicted)) else "NOOOO")
#     print("\n")

#     # Save the model
#     joblib.dump(model, f"{name}.pkl")

#     # Feature Importance for Random Forest
#     if name == "Random Forest Regressor":
#         importances = model.feature_importances_
#         indices = np.argsort(importances)[::-1]
#         print("Feature ranking:")
#         for f in range(X_train.shape[1]):
#             print(f"{f + 1}. feature {indices[f]} ({importances[indices[f]]})")
#         print("\n")

import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_squared_error, mean_absolute_error, r2_score
import joblib

# Load dataset
df = pd.read_csv('data.csv')

# Assuming the last column is the target and the rest are features
X = df.iloc[:, :-1]  # Features
y = df.iloc[:, -1]   # Target (number of drops)

# Feature Engineering
# Adding statistical features to potentially help distinguish between 1 and 2 drops
X['mean'] = X.mean(axis=1)
X['std'] = X.std(axis=1)
X['min'] = X.min(axis=1)
X['max'] = X.max(axis=1)
X['range'] = X['max'] - X['min']

# Split the dataset into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Model Complexity Adjustment with GridSearchCV
param_grid = {
    'n_estimators': [100, 200, 300],
    'max_depth': [None, 10, 20, 30],
    'min_samples_split': [2, 5, 10],
    'min_samples_leaf': [1, 2, 4]
}

rf = RandomForestRegressor(random_state=42)
grid_search = GridSearchCV(estimator=rf, param_grid=param_grid, cv=3, n_jobs=-1, verbose=2, scoring='neg_mean_squared_error')
grid_search.fit(X_train, y_train)

# Best model after grid search
best_rf = grid_search.best_estimator_

# Predictions
y_pred = best_rf.predict(X_test)

# Evaluation
mse = mean_squared_error(y_test, y_pred)
mae = mean_absolute_error(y_test, y_pred)
r2 = r2_score(y_test, y_pred)

print(f"Best Model Parameters: {grid_search.best_params_}")
print(f"MSE: {mse}")
print(f"MAE: {mae}")
print(f"R^2 Score: {r2}")

# Optional: Save the best model
joblib.dump(best_rf, 'best_random_forest_regressor.pkl')