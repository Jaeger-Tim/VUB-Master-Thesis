import os
import csv

# Directory containing the CSV files
path = '.'  # Use '.' for current directory or specify the path

# Name of the combined CSV file
combined_filename = 'combined.csv'

# Collect all CSV filenames in the directory
csv_files = [f for f in os.listdir(path) if f.endswith('.csv') and f != combined_filename]

# Open the combined CSV file for writing
with open(combined_filename, 'w', newline='') as combined_file:
    csv_writer = csv.writer(combined_file)

    # Optional: Write a header row for the combined CSV, if desired
    # csv_writer.writerow(['Wavelength1', 'Wavelength2', ...])

    # Iterate over each CSV file
    for filename in csv_files:

        # Open the current CSV file for reading
        with open(filename, 'r') as f:
            csv_reader = csv.reader(f)

            # Skip the header row of each CSV
            next(csv_reader, None)  # This skips the first row

            # Read each row from the current CSV file
            for row in csv_reader:
                # Write the row to the combined CSV file
                csv_writer.writerow(row)

print("CSV files have been combined into", combined_filename)