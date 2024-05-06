import argparse
import yaml
import math
import re
import os
import numpy as np
import csv

energy_pattern = r"Energy:"
cycles_pattern = r"Cycles:"
area_pattern = r"um\^2"
clock_speed = 1000000 # clock speed in Hz

def extract_stats(stats_file):
    with open(stats_file, "r") as file:
        lines = file.readlines()
    
    area = 0

    for line in lines:
        if re.search(energy_pattern, line):
            energy = float(line.strip().split()[1])
        if re.search(cycles_pattern, line):
            cycles = float(line.strip().split()[1])
        if re.search(area_pattern, line):
            # print(line)
            area += float(line.strip().split()[-2])
    return area, energy, cycles
    
def tensor_size(dimensions, data_width):
    tensor_elements = math.prod(dimensions)
    tensor_size_in_bytes = tensor_elements * data_width
    return tensor_size_in_bytes


def parse_yaml_conv(file_path):
    with open(file_path, "r") as file:
        return yaml.safe_load(file)


def extract_dimensions_conv(yaml_data, data_space):
    instance = yaml_data["problem"]["instance"]
    projections = next(
        ds["projection"] for ds in yaml_data["problem"]["shape"]["data_spaces"]
        if ds["name"] == data_space
    )

    dimensions = []
    for projection in projections:
        if isinstance(projection[0], list):
            # Handle the case where projection contains lists of lists
            dimension = 1
            for sub_projection in projection:
                sub_dim_name = sub_projection[0]
                multiplier = 1
                for sub_dimension in sub_projection[1:]:
                    if isinstance(sub_dimension, list):
                        sub_multiplier_name = sub_dimension[-1]
                        multiplier *= instance[sub_multiplier_name]
                    else:
                        multiplier *= instance[sub_dimension]
                dimension *= instance[sub_dim_name] * multiplier
            dimensions.append(dimension)
        else:
            # Handle the case where projection contains a single list
            dim_name = projection[0]
            multiplier = 1
            for sub_dimension in projection[1:]:
                if isinstance(sub_dimension, list):
                    sub_dim_name, sub_multiplier_name = sub_dimension
                    multiplier *= instance[sub_dim_name] * instance[sub_multiplier_name]
                else:
                    multiplier *= instance[sub_dimension]
            dimensions.append(instance[dim_name] * multiplier)

    return dimensions

def extract_dimensions_gpt(yaml_file):
    
    dimensions = []

    # Open the file and read it line by line
    with open(yaml_file, 'r') as file:
        # Variables to store M, C, P
        M, C, P = None, None, None
        for line in file:
            # Strip whitespace from the beginning and end of the line
            line = line.strip()
            # Check if the line contains the keyword 'instance:'
            if 'instance:' in line:
                # Extract the part after 'instance:'
                instance_part = line.split('instance:', 1)[1].strip()
                # Remove braces and split by comma
                entries = instance_part.strip('{}').split(',')
                # print(entries)
                for entry in entries:
                    key, value = entry.split(':')
                    key = key.strip()
                    value = value.strip()
                    if key == 'M':
                        M = int(value)
                    elif key == 'C':
                        C = int(value)
                    elif key == 'P':
                        P = int(value)
    
    dimensions.append(M)
    dimensions.append(P)
    
    return dimensions


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Calculate tensor sizes from a YAML file.")
    parser.add_argument("--shard_dir", type=str, help="Path to the directory containing shard_n folders")
    parser.add_argument("--model", type=str, help="Name of the model to analyze")
    parser.add_argument("--dataspace", type=str, help="Name of the data space to analyze")
    parser.add_argument("--datawidth", type=int, help="Data width of each element in bytes")
    parser.add_argument("--csv_network", type=str, help="Path to the output csv for network simulation")
    parser.add_argument("--csv_stats", type=str, help="Path to the output csv for stats (energy, area, etc.)")

    args = parser.parse_args()
    
    list_shards = os.listdir(args.shard_dir)
    
    for shard in list_shards:
        num_shard = shard.split("_")[1]
     
        with open((str(num_shard) + "_" + args.csv_network), 'w', newline='') as file, open((str(num_shard) + "_" + args.csv_stats), 'w', newline='') as stats_file:
            writer = csv.writer(file)
            stats_writer = csv.writer(stats_file)

            writer.writerow(list(num_shard))
            print(f"number of shards: {num_shard}")
            layer_dir = os.path.join(args.shard_dir, shard)
            list_layers = os.listdir(layer_dir)
            
            for layer in list_layers:
                sharded_layer_dir = os.path.join(layer_dir, layer)

                files = os.listdir(sharded_layer_dir)

                yaml_files = [f for f in files if f.endswith(".yaml")]
                stats_files = [f for f in files if f.endswith(".txt")]

                # Calculate the maximum memory transfer
                memory_usage = list()

                for yaml_file in yaml_files:
                    if (args.model == "conv"):
                        yaml_data = parse_yaml_conv(os.path.join(sharded_layer_dir, yaml_file))
                        dimensions = extract_dimensions_conv(yaml_data, args.dataspace)

                    elif (args.model == "gpt"):
                        dimensions = extract_dimensions_gpt(os.path.join(sharded_layer_dir, yaml_file))
                        
                    size = tensor_size(dimensions, args.datawidth)
                    
                    memory_usage.append(size)
                
                max_mem_transfer = max(memory_usage)

                # Calculate the maximum latency and extract energy usage from the stats file
                energy_list = list()
                latency_list = list()
                area_list = list()

                for stat_file in stats_files:
                    area, energy, cycles = extract_stats(os.path.join(sharded_layer_dir, stat_file))
                    latency = cycles / float(clock_speed)
                    area_list.append(area)
                    energy_list.append(energy)
                    latency_list.append(latency)
                    
                max_area = max(area_list)
                max_latency = max(latency_list)
                max_energy = max(energy_list)
                
                csv_layer_line = [str(max_latency), str(max_mem_transfer)]
                # print(csv_layer_line)
                writer.writerow(csv_layer_line)
                csv_stats_line = [str(max_area), str(max_energy)]
                print(csv_stats_line)
                stats_writer.writerow(csv_stats_line)
            
    # size = tensor_size(dimensions, args.datawidth)
    # energy = extract_energy(args.stats)

    # print(f"Tensor size for data space '{args.dataspace}': {size} bytes")
    # print(f"Energy used: {energy} uj")