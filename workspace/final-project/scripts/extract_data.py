import argparse
import yaml
import math
import re

# Define the pattern you're looking for
energy_pattern = r""

# Open and read the file
with open("your_file.txt", "r") as file:
    lines = file.readlines()

# Search for the pattern in each line and print the matching lines
for line in lines:
    if re.search(pattern, line):
        print(line.strip())


def tensor_size(dimensions, data_width):
    tensor_elements = math.prod(dimensions)
    tensor_size_in_bytes = tensor_elements * data_width
    return tensor_size_in_bytes


def parse_yaml_file(file_path):
    with open(file_path, "r") as file:
        return yaml.safe_load(file)


def extract_dimensions(yaml_data, data_space):
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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Calculate tensor sizes from a YAML file.")
    parser.add_argument("--yaml", type=str, help="Path to the YAML file")
    parser.add_argument("--dataspace", type=str, help="Name of the data space to analyze")
    parser.add_argument("--datawidth", type=int, help="Data width of each element in bits")

    args = parser.parse_args()
    yaml_data = parse_yaml_file(args.yaml)
    dimensions = extract_dimensions(yaml_data, args.dataspace)
    size = tensor_size(dimensions, args.datawidth)
    print(f"Tensor size for data space '{args.dataspace}': {size} bits")
