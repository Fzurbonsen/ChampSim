# 
# plot.py
# Python script to plot the data from the benchmark
# Author: Frederic zur Bonsen
# E-Mail: fzurbonsen@ethz.ch
# 

import argparse, os
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from scipy.stats import gmean
import matplotlib.patches as mpatches


# function to read csv file
def read_csv(file):
  
  # check if the file oxists
  if not os.path.exists(file):
    print("Error: data file does not exist!")
    print(file)
    exit(1)

  df = pd.read_csv(file)
  df = df.sort_values(by="trace")

  # clean trace names
  df["trace"] = df["trace"].str.replace(".trace.gz", "", regex=False)
  df["trace"] = df["trace"].str.replace(".champsim.gz", "", regex=False)
  df["trace"] = df["trace"].str.replace("GAP/", "", regex=False)
  df["trace"] = df["trace"].str.replace("charlie/", "", regex=False)
  df["trace"] = df["trace"].str.replace(r"_0+(\d+)", r"_\1", regex=True)
  return df


# function to create ipc bar plot
def ipc_bar_plot(df, file):
    df = df.set_index("trace")

    plt.figure(figsize=(10, 5))

    n = len(df)
    gradient = np.linspace(0.3, 1.0, n)
    colors = plt.cm.Blues(gradient)

    ax = df["IPC"].plot(
        kind="bar",
        color=colors,
    )

    ax.set_xticklabels(ax.get_xticklabels(), rotation=45, ha="right")

    ax.set_ylabel("IPC")
    ax.set_xlabel("")
    ax.grid(axis="y", linestyle="--", linewidth=0.5, alpha=0.7)

    for p in ax.patches:
        ax.annotate(
            f"{p.get_height():.2f}",
            (p.get_x() + p.get_width() / 2, p.get_height()),
            ha="center",
            va="bottom",
            fontsize=10
        )

    plt.tight_layout()
    plt.savefig(f"{file}_ipc.png", dpi=300)
    plt.savefig(f"{file}_ipc.eps")
    plt.show()


# function to create speedup bar plot
def speedup_bar_plot(df_data, df_base, file=None):
    df_data = df_data.set_index("trace")
    df_base = df_base.set_index("trace")

    # Calculate speedup per trace
    speedup = df_data["IPC"] / df_base["IPC"]

    # Calculate geometric mean
    geo_mean = gmean(speedup)

    # Append geometric mean using pd.concat
    geo_series = pd.Series([geo_mean], index=["GeoMean"])
    speedup_with_mean = pd.concat([speedup, geo_series])

    plt.figure(figsize=(10, 5))
    n = len(speedup_with_mean)

    # Gradient for normal bars
    gradient = np.linspace(0.3, 1.0, n-1)  # leave last bar for GeoMean
    colors = plt.cm.Oranges(gradient)
    colors = np.vstack([colors, np.array([[0.2, 0.2, 0.2, 1]])])  # dark gray for GeoMean

    ax = speedup_with_mean.plot(
        kind="bar",
        color=colors,
    )

    ax.set_xticklabels(ax.get_xticklabels(), rotation=45, ha="right")
    ax.set_ylabel("Speedup")
    ax.set_xlabel("")
    ax.grid(axis="y", linestyle="--", linewidth=0.5, alpha=0.7)

    # Annotate bars
    for p in ax.patches:
        ax.annotate(
            f"{p.get_height():.2f}",
            (p.get_x() + p.get_width() / 2, p.get_height()),
            ha="center",
            va="bottom",
            fontsize=10
        )

    plt.tight_layout()
    plt.savefig(f"{file}_speedup.png", dpi=300)
    plt.savefig(f"{file}_speedup.eps")
    plt.show()


def speedup2_bar_plot(df_data1, df_data2, df_base, file=None):
    # Set trace as index
    df_data1 = df_data1.set_index("trace")
    df_data2 = df_data2.set_index("trace")
    df_base = df_base.set_index("trace")

    # Compute speedups
    speedup1 = df_data1["IPC"] / df_base["IPC"]
    speedup2 = df_data2["IPC"] / df_base["IPC"]

    # Compute geometric mean
    geo_mean1 = gmean(speedup1)
    geo_mean2 = gmean(speedup2)

    # Append geometric mean
    speedup1 = pd.concat([speedup1, pd.Series([geo_mean1], index=["GeoMean"])])
    speedup2 = pd.concat([speedup2, pd.Series([geo_mean2], index=["GeoMean"])])

    all_traces = list(speedup1.index)

    # Bar positions
    x = np.arange(len(all_traces))
    width = 0.35

    plt.figure(figsize=(12, 6))

    # Colors: gradient for normal bars, distinct gray for GeoMean
    gradient1 = np.linspace(0.5, 1.0, len(speedup1)-1)
    colors1 = np.vstack([plt.cm.Purples(gradient1), np.array([[0.5,0.5,0.5,1]])])  # GeoMean gray

    gradient2 = np.linspace(0.5, 1.0, len(speedup2)-1)
    colors2 = np.vstack([plt.cm.Oranges(gradient2), np.array([[0.3,0.3,0.3,1]])])  # GeoMean gray

    # Plot bars
    bars1 = plt.bar(x - width/2, speedup1.values, width, color=colors1)
    bars2 = plt.bar(x + width/2, speedup2.values, width, color=colors2)

    # Annotate bars
    for bars in [bars1, bars2]:
        for p in bars:
            plt.annotate(
                f"{p.get_height():.2f}",
                (p.get_x() + p.get_width()/2, p.get_height()),
                ha="center", va="bottom", fontsize=7
            )

    # Labels, grid, ticks
    plt.xticks(x, all_traces, rotation=45, ha="right")
    plt.ylabel("Speedup")
    plt.xlabel("")
    plt.grid(axis="y", linestyle="--", linewidth=0.5, alpha=0.7)

    # Legend using middle color of each gradient
    legend_color1 = colors1[len(colors1)//2]
    legend_color2 = colors2[len(colors2)//2]
    handle1 = mpatches.Patch(color=legend_color1, label="Delta Correlation")
    handle2 = mpatches.Patch(color=legend_color2, label="Stride")
    plt.legend(handles=[handle1, handle2])

    plt.tight_layout()

    # Save figure
    if file:
        plt.savefig(f"{file}_speedup.png", dpi=300, bbox_inches="tight")
        plt.savefig(f"{file}_speedup.eps", bbox_inches="tight")

    plt.show()



# main function
def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("--data1", type=str, required=True,
                      help="Data input file [.csv]")
  parser.add_argument("--data2", type=str, required=True,
                      help="Data input file [.csv]")
  parser.add_argument("--base", type=str, required=True,
                      help="Baseline data input file [.csv]")
  parser.add_argument("--img", type=str, required=True,
                      help="Imga output file [no suffix (will be stored as .png and .esp)]")

  args = parser.parse_args()

  df1 = read_csv(args.data1)
  df2 = read_csv(args.data2)
  df_base = read_csv(args.base)

  img_file = args.img

  # bar plot of the IPC values
  # ipc_bar_plot(df1, img_file)

  # bar plot of the relative speedup
  # speedup_bar_plot(df1, df_base, img_file)
  speedup2_bar_plot(df1, df2, df_base, img_file)
  return



if __name__ == "__main__":
  main()