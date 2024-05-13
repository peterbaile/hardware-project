# Tensor Parallel Sharding for Distributed Inference on the Edge
Authors: Peter Baile Chen, Charlie Liu, Anton Zabreyko

The repository is released under MIT License.

# Run the Project
Simply do "./run.sh", this should run the project with the current settings. 

To see more usage of the data extraction script, please run "python3 ./scripts/extract_data.py --help"

If you encounter permission problem on your machine, you can try "chmod 777 -R final-project" outside of this directory.

# Add Your Own Implementation
To explore tensor parallelism with different architecture, different sharding techniques or different models, please visit example_designs/example_designs directory to add your own hardware implementation, visit example_designs/layer_shapes to add your own model, and visit example_designs/shard.py to modify the sharding techniques. The rest of the code should be able to self-adapt to your own implementation. 

# Overview
Welcome to `timeloop-accelergy-exercises`! This repository contains tutorials,
exercises, and examples to get you started with Timeloop and Accelergy. In this
directory, you'll find the following.

#### `tutorial_exercises`
This directory contains exercises and tutorials to teach you how to use
Timeloop. Note that exercises 1 and 2 are prerequisites for the rest of the
exercises, and should be completed first.

#### `cheatsheets`
This directory contains YAML syntax examples and explanations for YAML syntax
and parsing, as well as some of the Timeloop input files. They're meant to be
used as a quick reference when writing your own input files.

#### `example_designs`
This directory contains example Timeloop accelerator designs that can be used
and/or adapted to run your own experiments.

#### `timeloop_spec.yaml`
This file contains a full specification of the Timeloop input file format,
including all allowed fields and their types. Use this as a reference when
writing your own Timeloop input files.
