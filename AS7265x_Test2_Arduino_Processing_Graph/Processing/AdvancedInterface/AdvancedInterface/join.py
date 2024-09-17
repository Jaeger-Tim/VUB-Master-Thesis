import pandas as pd
import os
import glob

# Define the column names (wavelengths) for the CSV files
column_names = ['410nm', '435nm', '460nm', '485nm', '510nm', '535nm', '560nm', '585nm', '610nm', '645nm', '680nm', '705nm', '730nm', '760nm', '810nm', '860nm', '900nm', '940nm']

def find_shortest_csv(files):
    min_length = float('inf')
    for file in files:
        # Adjusted to correctly handle CSVs without headers
        df = pd.read_csv(file, header=None)
        if len(df.columns) == len(column_names):  # Ensure the dataframe has the correct number of columns
            df.columns = column_names
        elif len(df.columns) == len(column_names) + 1:  # Handle potential extra column
            df = df.iloc[:, :-1]  # Drop the first column if there's an extra
            df.columns = column_names
        if len(df) < min_length:
            min_length = len(df)
    return min_length

def combine_csv_files(files, min_samples):
    combined_file_path = 'combinedData.csv'
    # Check if combinedData.csv exists; if so, remove it to start fresh
    if os.path.exists(combined_file_path):
        os.remove(combined_file_path)

    for file in files:
        df = pd.read_csv(file, header=None)
        if len(df.columns) == len(column_names):  # Ensure the dataframe has the correct number of columns
            df.columns = column_names
        elif len(df.columns) == len(column_names) + 1:  # Handle potential extra column
            print(df.columns);
            df = df.iloc[:, :-1]  # Drop the first column if there's an extra
            df.columns = column_names
        # Sample "min" amount of random samples from the file
        sampled_df = df.sample(n=min_samples)
        # Extract the "x" from "x.csv" and add it as a new column
        data_tag = os.path.splitext(os.path.basename(file))[0]  # This gets the file name without extension
        sampled_df['DataTag'] = data_tag
        # Append to combinedData.csv
        header = column_names + ['DataTag'] if not os.path.exists(combined_file_path) else False
        sampled_df.to_csv(combined_file_path, mode='a', index=False, header=header)

def main():
    csv_files = glob.glob('*.csv')
    # Filter out the combinedData.csv if it exists in the directory to avoid including it in the processing
    csv_files = [file for file in csv_files if file != 'combinedData.csv']

    min_samples = find_shortest_csv(csv_files)
    combine_csv_files(csv_files, min_samples)

if __name__ == "__main__":
    main()