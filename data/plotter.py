import sys
import matplotlib.pyplot as plt
import csv

# Check for command-line argument
if len(sys.argv) < 2:
    print("Usage: python plotter.py <datafile.csv>")
    sys.exit(1)

filename = sys.argv[1]

# Initialize lists
time = []
data7in = []
data20in = []
data40in = []
data70in = []
data100in = []
dataLightAndCam7in = []
dataNoLight = []
dataJustLight = []
dataBox = []

try:
    with open(filename, 'r') as file:
        reader = csv.reader(file)
        header = next(reader, None)  # skip header

        for row in reader:
            # Only process rows with at least 9 columns
            if len(row) < 10:
                continue
            try:
                # Convert all 9 values to float in one go
                values = [float(row[i]) for i in range(10)]
                time.append(values[0])
                data7in.append(values[1])
                data20in.append(values[2])
                data40in.append(values[3])
                data70in.append(values[4])
                data100in.append(values[5])
                dataLightAndCam7in.append(values[6])
                dataNoLight.append(values[7])
                dataJustLight.append(values[8])
                dataBox.append(values[9])
            except ValueError:
                # Skip rows with invalid numeric data
                continue

except FileNotFoundError:
    print(f"Error: File '{filename}' not found.")
    sys.exit(1)
except Exception as e:
    print(f"Error reading file: {e}")
    sys.exit(1)

# Check if data exists
if not time:
    print("No valid data to plot.")
    sys.exit(1)

# Create figure
plt.figure(figsize=(12, 6))

# Plot all datasets
plt.plot(time, data7in, label="7 in")
plt.plot(time, data20in, label="20 in")
plt.plot(time, data40in, label="40 in")
plt.plot(time, data70in, label="70 in")
plt.plot(time, data100in, label="100 in")
plt.plot(time, dataLightAndCam7in, label="Light + Cam 7 in") # control value
plt.plot(time, dataNoLight, label="No Light") # control value
plt.plot(time, dataJustLight, label="Just Light") # control value
plt.plot(time, dataBox, label="Just Box") # control value

# Labels and title
plt.xlabel("Time")
plt.ylabel("Analog Reading")
plt.title(f"Plot from {filename}")

# Add padding
x_padding = (max(time) - min(time)) * 0.05
all_data = (
    data7in + data20in + data40in + data70in +
    data100in + dataLightAndCam7in + dataNoLight + dataJustLight
)
y_padding = (max(all_data) - 100) * 0.05

plt.xlim(min(time) - x_padding, max(time) + x_padding)
plt.ylim(100, max(all_data) + y_padding)

# Grid and legend
plt.grid(True)
plt.legend()

# Show plot
plt.show()
