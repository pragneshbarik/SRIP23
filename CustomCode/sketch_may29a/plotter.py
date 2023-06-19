import serial
import matplotlib.pyplot as plt
import datetime
import time
# import ba

# Set up the serial connection
port = 'COM10'  # Replace with your serial port
baud_rate = 115200
ser = serial.Serial(port, baud_rate)

# Initialize the data lists
x_data = []
y_data = []

# Set up the plot
fig, ax = plt.subplots()
line, = ax.plot(x_data, y_data)

# Set the plot limits to show the last 1000 points
ax.set_xlim(0, 100)
# ax.set_ylim(-180, 180)
ax.set_xlabel('Index')
ax.set_ylabel('Gyro Value')
ax.set_title('Gyro Data')

# Main loop to read and plot data
index = 0
while True:
    # Read a line from the serial monitor
    serial_line = ser.readline().decode('utf-8').strip()
    if (len(serial_line.split(',')) < 6):
        continue
    # Split the line into x and y values
    # y = 0
    # print(line.split(','))
    y, a, b, c, d, e = map(float, serial_line.split(','))
    print(y, a, b, c, d, e)

    # Append the values to the data lists
    x_data.append(index)
    y_data.append(y)

    # Truncate the data lists to keep only the last 1000 values
    x_data = x_data[-1000:]
    y_data = y_data[-1000:]

    # Update the plot
    line.set_xdata(x_data)
    line.set_ydata(y_data)

    # Adjust the plot limits
    ax.relim()
    ax.autoscale_view()

    # Refresh the plot
    plt.draw()
    plt.pause(0.0001)

    index += 1
