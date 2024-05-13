import json
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os
import math

from shard import parse_yaml, LAYERS

def get_num_shards(model, num_shard, l_idx):
  instance = parse_yaml(model, LAYERS[model][l_idx])

  dim = 'C' if model == 'alexnet' else 'M'

  if model == 'alexnet':
    sz = instance['problem']['instance'][dim]
  else:
    sz = instance[dim]
  
  actual_num_shard = 1 if sz < num_shard else num_shard

  return actual_num_shard

def get_energy(df: pd.DataFrame, model: str, num_shard: int):
  assert type(num_shard) is int
  energy = 0
  for l_idx, row in df.iterrows():
    energy += get_num_shards(model, num_shard, l_idx) * row[1]
  return energy

MODEL_PLOT_NAMES = {
  'gpt': 'GPT-2',
  'alexnet': 'AlexNet'
}

def plot(model, metric):
  NUM_SHARDS = [1, 2, 4, 8]
  ARCH = ['eyeriss', 'simba']

  if metric != 'weight':
    data = {
      'eyeriss': [],
      'simba': [],
    }
  
    for num_shard in NUM_SHARDS:
      for arch in ARCH:
        df = pd.read_csv(f'../Hardware-Stats/{num_shard}_shard_{arch}_{model}_hardware_stats.csv', header=None)

        if metric == 'area':
          num = df[0][0]*num_shard
          # num = math.log(num)
          data[arch].append(num)
        elif metric == 'energy':
          energy = get_energy(df, model, num_shard)
          # energy = math.log(energy)
          data[arch].append(energy)
  else:
    data = {'alexnet': [], 'gpt': []}

    for num_shard in NUM_SHARDS:
      for model in ['alexnet', 'gpt']:
        df = pd.read_csv(f'../Hardware-Stats/Weights/{num_shard}_shard_weight-{"conv" if model == "alexnet" else "gpt"}-eyeriss.csv', skiprows=1)
        data[model].append(df.iloc[:, 0].sum())
  
  for x in data:
    data[x] = np.array(data[x])
    data[x] = np.round(data[x] / data[x][0], 2)

  x = np.arange(len(NUM_SHARDS))  # the label locations
  width = 0.25  # the width of the bars
  multiplier = 0

  fig, ax = plt.subplots(layout='constrained')

  for attribute, measurement in data.items():
    # offset = width * multiplier
    if attribute == 'eyeriss' or attribute == 'alexnet':
      rects = ax.bar(x - width/2, measurement, width, label=attribute)
    else:
      rects = ax.bar(x + width/2, measurement, width, label=attribute)
    # rects = ax.bar(x + offset, measurement, width, label=attribute)
    ax.bar_label(rects, padding=3)
    multiplier += 1

  # Add some text for labels, title and custom x-axis tick labels, etc.
  if metric == 'area':
    ax.set_ylabel('Area (um^2, normalized)')
    t = 'Area'
  elif metric == 'energy':
    ax.set_ylabel('Energy (uJ, normalized)')
    t = 'Energy'
  elif metric == 'weight':
    ax.set_ylabel('Number of weights (normalized)')
    t = 'Number of weights'
  ax.set_xlabel('Number of shards')

  ax.set_title(f'{t} with {MODEL_PLOT_NAMES[model]} and different architectures and number of shards', fontsize=15)
  ax.set_xticks(x, NUM_SHARDS)
  ax.legend(loc='upper left', ncols=3)

  y_up = 1.2

  # y_up = 17

  # if model == 'alexnet' and metric == 'energy':
  #   y_up = 12
  
  # if metric == 'weight':
  #   y_up = 22
  # 17

  ax.set_ylim(0, y_up)

  # plt.show()
  plt.savefig(f'./plots/{model}_{metric}.png', dpi=300, bbox_inches='tight')

def plot_latency(model, arch):
  NUM_SHARDS = [1, 2, 4, 8]
  ARCH = ['eyeriss', 'simba']

  data = {
    '1MB': [],
    '100MB': [],
    '5GB': []
  }

  SPEEDS = ['1MB', '100MB', '5GB']
  
  for num_shard in NUM_SHARDS:
    for speed in SPEEDS:
      df_total = pd.read_csv(f'../{model}_{speed}/{num_shard}_shard_{model}_{arch}.csv')
      df_compute = pd.read_csv(f'../{model}_data/{num_shard}_shard_{model}_{arch}.csv', header=None, skiprows=1)

      compute_time = df_compute.iloc[:, 0].sum()
      network_time = df_total['Time'].sum() - compute_time

      data[speed].append((compute_time, network_time))
  
  print(np.array(data['1MB']).shape)
  
  dfs = [pd.DataFrame(np.array(data[speed]),
                    index=['1', '2', '4', '8'],
                    columns=['Computation time', 'Network time']) for speed in SPEEDS]
  
  title=f'Latency with {MODEL_PLOT_NAMES[model]} and {arch} under different network speeds and number of shards'
  fn = f'latency_{model}_{arch}'

  # df1MB, df100MB, df5GB
  plot_clustered_stacked(dfs, ['1MB', '100MB', '5GB'], title, fn)

def plot_clustered_stacked(dfall, labels, title, fn, H=['', '//', '\\\\'], **kwargs):
    """Given a list of dataframes, with identical columns and index, create a clustered stacked bar plot. 
labels is a list of the names of the dataframe, used for the legend
title is a string for the title of the plot
H is the hatch used for identification of the different dataframe"""

    n_df = len(dfall)
    n_col = len(dfall[0].columns) 
    n_ind = len(dfall[0].index)
    axe = plt.subplot(111)

    for df in dfall : # for each data frame
        axe = df.plot(kind="bar",
                      linewidth=0,
                      stacked=True,
                      ax=axe,
                      legend=False,
                      grid=False,
                      **kwargs)  # make bar plots

    h,l = axe.get_legend_handles_labels() # get the handles we want to modify
    for i in range(0, n_df * n_col, n_col): # len(h) = n_col * n_df
        for j, pa in enumerate(h[i:i+n_col]):
            for rect in pa.patches: # for each index
                rect.set_x(rect.get_x() + 1 / float(n_df + 1) * i / float(n_col))
                rect.set_hatch(H[int(i / n_col)]) #edited part     
                rect.set_width(1 / float(n_df + 1))

    axe.set_xticks((np.arange(0, 2 * n_ind, 2) + 1 / float(n_df + 1)) / 2.)
    axe.set_xticklabels(df.index, rotation = 0)
    axe.set_title(title)
    axe.set_xlabel('Number of shards')
    axe.set_ylabel('Latency (sec)')

    # Add invisible data to add another legend
    n=[]        
    for i in range(n_df):
      n.append(axe.bar(0, 0, color="gray", hatch=H[i]))

    l1 = axe.legend(h[:n_col], l[:n_col], loc=[0.01, 0.85])
    if labels is not None:
      l2 = plt.legend(n, labels, loc=[0.01, 0.6]) 
    axe.add_artist(l1)
    # plt.show()
    plt.savefig(f'./plots/{fn}.png', dpi=300, bbox_inches='tight')
    return axe

if __name__ == '__main__':
  MODEL = ['alexnet', 'gpt']
  ARCH = ['eyeriss', 'simba']

  # plot('gpt', 'area')

  # for model in MODEL:
  #   for arch in ARCH:
  #     plot_latency(model, arch)
  

  # for model in MODEL:
  #   for metric in ['area', 'energy']:
  #     plot(model, metric)
  plot('alexnet', 'weight')
  # plot_latency('gpt', 'eyeriss')