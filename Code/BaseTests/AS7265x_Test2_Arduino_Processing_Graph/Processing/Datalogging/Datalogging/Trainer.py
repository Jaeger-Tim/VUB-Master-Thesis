import pandas as pd
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.preprocessing import StandardScaler
from sklearn.pipeline import Pipeline
from sklearn.metrics import accuracy_score
import joblib
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
from sklearn.svm import SVC

# Load the dataset without assuming a header
df = pd.read_csv('combined.csv', header=None)

# Split the dataset into features and target variable
X = df.iloc[:, 1:]  # Assuming the first column is the label and the rest are features
y = df.iloc[:, 0]

# Split the data into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Define a list to store the best models
best_models = []

# Define a list of models and their parameter grids for GridSearchCV
models_params = [
    {
        'model': LogisticRegression(max_iter=10000),
        'params': {
            'clf__C': [0.1, 1, 10, 100]
        }
    },
    {
        'model': RandomForestClassifier(),
        'params': {
            'clf__n_estimators': [100, 200, 300],  # More options
            'clf__max_depth': [None, 10, 20, 30, 40],  # More options
            'clf__min_samples_split': [2, 5, 10],  # New parameter
            'clf__max_features': ['sqrt'],  # New parameter
        }
    },
    {
        'model': SVC(),
        'params': {
            'clf__C': [0.1, 1, 10, 100],
            'clf__kernel': ['linear', 'rbf']
        }
    }
]

# Iterate over the models and their parameter grids
for model_param in models_params:
    # Create a pipeline with a scaler and the current model
    pipeline = Pipeline([
        ('scaler', StandardScaler()),
        ('clf', model_param['model'])
    ])

    # Use GridSearchCV to find the best parameters for the current model
    grid_search = GridSearchCV(pipeline, model_param['params'], cv=5, scoring='accuracy')
    grid_search.fit(X_train, y_train)

    # Predict on the test set
    y_pred = grid_search.predict(X_test)

    # Calculate the accuracy
    accuracy = accuracy_score(y_test, y_pred)
    print(f"Best parameters for {type(model_param['model']).__name__}: {grid_search.best_params_}")
    print(f"Accuracy: {accuracy}")

    # Save the best model
    best_model_path = f"best_{type(model_param['model']).__name__}.joblib"
    joblib.dump(grid_search.best_estimator_, best_model_path)
    best_models.append(grid_search.best_estimator_)

print("Training complete. Best models saved.")