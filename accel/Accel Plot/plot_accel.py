import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import MaxNLocator, FormatStrFormatter

# --- Get user input for file name ---
filename = input("Enter the name of the .txt file (with extension): ").strip()

# --- Load CSV file (no header, add column names) ---
df = pd.read_csv(filename, header=None, names=["timestamp", "x", "y", "z"])

# --- Convert timestamp strings to seconds ---
df["timestamp"] = pd.to_datetime(df["timestamp"], format="%H:%M:%S.%f", errors="coerce")
time = (df["timestamp"] - df["timestamp"].iloc[0]).dt.total_seconds().values

# --- Compute dt array for variable sampling intervals ---
dt = np.diff(time, prepend=time[0])

# --- Subtract gravity (assuming Z-axis points up) ---
df["z"] = df["z"] - 9.81

# --- Integrate acceleration to get velocity (trapezoidal rule with actual dt) ---
df["vx"] = np.cumsum((df["x"].shift(fill_value=0) + df["x"]) / 2 * dt)
df["vy"] = np.cumsum((df["y"].shift(fill_value=0) + df["y"]) / 2 * dt)
df["vz"] = np.cumsum((df["z"].shift(fill_value=0) + df["z"]) / 2 * dt)

# --- Function to plot data with auto-scaled axes, numeric ticks, and direction labels ---
def plot_xyz(data, cols, title_prefix, ylabel, time):
    fig, axs = plt.subplots(3, 1, figsize=(10, 8), sharex=True)
    colors = ["r", "g", "b"]
    directions = [
        ("Left", "Right"),    # X-axis
        ("Back", "Forward"),  # Y-axis
        ("Down", "Up")        # Z-axis
    ]
    
    for i, col in enumerate(cols):
        axs[i].plot(time, data[col], color=colors[i])
        axs[i].set_ylabel(ylabel, fontsize=10)
        
        # Auto-scale Y-axis with 10% padding
        y_min, y_max = data[col].min(), data[col].max()
        padding = (y_max - y_min) * 0.1 if y_max != y_min else 1
        axs[i].set_ylim(y_min - padding, y_max + padding)
        
        # Auto-scaled numeric ticks
        axs[i].yaxis.set_major_locator(MaxNLocator(nbins=7, prune='both'))
        axs[i].yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
        axs[i].tick_params(axis='y', labelsize=8)
        
        axs[i].set_title(f"{title_prefix} {col.upper()}-axis", fontsize=11)
        
        # Direction labels
        axs[i].text(0.99, 0.02, directions[i][0], transform=axs[i].transAxes,
                    fontsize=9, color=colors[i], ha='right', va='bottom')
        axs[i].text(0.99, 0.95, directions[i][1], transform=axs[i].transAxes,
                    fontsize=9, color=colors[i], ha='right', va='top')
    
    # X-axis labels and ticks
    axs[2].set_xlabel("Time (seconds)", fontsize=10)
    axs[2].xaxis.set_major_locator(MaxNLocator(nbins=10, integer=False))
    axs[2].tick_params(axis='x', labelsize=8)
    
    plt.tight_layout()
    plt.show()

# --- Plot acceleration ---
plot_xyz(df, ["x", "y", "z"], "Acceleration", "Accel (m/s²)", time)

# --- Plot velocity (integrated) ---
plot_xyz(df, ["vx", "vy", "vz"], "Velocity", "Velocity (m/s)", time)
