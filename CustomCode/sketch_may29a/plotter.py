import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Set up the serial connection
port = 'COM9'  # Replace with your serial port
baud_rate = 115200
ser = serial.Serial(port, baud_rate)

# Initialize the data lists
x_data = []
y_data = []

# Set up the plot
fig, ax = plt.subplots()
line, = ax.plot([], [])
ax.set_xlabel('Index')
# ax.set_xlim(0, 1000)
ax.set_ylim(-500, 500)
ax.set_ylabel('Gyro Value')
ax.set_title('Gyro Data')

# Main function to read data


def read_data():
    serial_line = ser.readline().decode('utf-8').strip()
    if len(serial_line.split(',')) < 7:
        return None
    y, _, a, b, c, d, e = map(float, serial_line.split(','))
    return y

# Animation update function


def update(frame):
    global x_data, y_data, line, ax, fig, i  # Add global keyword
    y = read_data()
    if y is not None:
        y_data.append(y)
        x_data.append(frame)
        # Truncate the data lists to keep only the last 1000 values
        line.set_data(x_data, y_data)
        ax.relim()
        ax.autoscale_view()

        if frame == 1000:
            i = 0  # Clear figure after 1000 frames
            fig.clear()
            ax = fig.add_subplot(1, 1, 1)
            line, = ax.plot([0], [0])
            ax.set_xlabel('Index')
            ax.set_ylabel('Gyro Value')
            ax.set_title('Gyro Data')
            x_data = []
            y_data = []


# Set the maximum number of frames (iterations)
xmax = 1000

# Create the animation
ani = animation.FuncAnimation(fig, update, frames=xmax, interval=1)

# Show the plot
plt.show()
