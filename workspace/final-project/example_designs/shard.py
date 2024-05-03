import ast
import json
from copy import deepcopy
import yaml
import subprocess
import os

THIS_SCRIPT_DIR = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
EXAMPLE_DIR = os.path.join(THIS_SCRIPT_DIR, "layer_shapes")

base_dir = f'{EXAMPLE_DIR}/CONV/AlexNet'

NUM_SHARDS = [2, 3]

def parse_yaml(fn):
  with open(f'{base_dir}/AlexNet_{fn}.yaml') as f:
    instance = yaml.safe_load(f)
  # instance = instance.replace('instance: ', '')
  # instance = instance.strip()

  # print(instance)
  # instance = instance.replace('{', '').replace('}', '').split(',')
  # instance = [x.split(':') for x in instance]
  # instance = {x[0].strip(): int(x[1]) for x in instance}
  return instance

def shard(dim, num_shards):
  # groups = [[], []]

  layers = ['layer1', 'layer2', 'layer3', 'layer4', 'layer5']

  if not os.path.exists(f'{base_dir}/shard_{num_shards}'):
    os.mkdir(f'{base_dir}/shard_{num_shards}')

  for l_idx, fn in enumerate(layers):
    instance = parse_yaml(fn)

    print(instance)
    sz = instance['problem']['instance'][dim]
    interval = sz // num_shards
    diff = sz - (num_shards - 1)*interval

    for g_idx, x in enumerate([interval, diff] if interval != diff else [interval]):
      instance_copy = deepcopy(instance)
      instance_copy['problem']['instance'][dim] = x
      # instance_copy = json.dumps(instance_copy).replace("\"", '')
      # lines = ["{{include_text('./problem_base.yaml')}}\n", "problem:\n", "  <<<: *problem_base\n", f"  instance: {instance_copy}"]
      # groups[g_idx].append(instance_copy)

      with open(f'{base_dir}/shard_{num_shards}/AlexNet_layer{l_idx+1}_{g_idx}.yaml', 'w') as f:
        # f.writelines(lines)
        yaml.safe_dump(instance_copy, f)
      

def run(num_shards):
  fns = list(os.listdir(f'{base_dir}/shard_{num_shards}'))

  print(fns)

  if not os.path.exists(f'./example_designs/eyeriss_like/outputs/shard_{num_shards}'):
    os.mkdir(f'./example_designs/eyeriss_like/outputs/shard_{num_shards}')

  for fn in fns:
    cmd = f'python3 run_example_designs.py --architecture eyeriss_like --problem CONV/AlexNet/shard_{num_shards}/{fn}'
    subprocess.run(cmd, shell=True, capture_output=True, text=True)
    fn_no_yaml = fn.replace('.yaml', '')
    cmd = f'mv ./example_designs/eyeriss_like/outputs/{fn_no_yaml} ./example_designs/eyeriss_like/outputs/shard_{num_shards}/{fn_no_yaml}'
    subprocess.run(cmd, shell=True, capture_output=True, text=True)

# def round_robin(num_shards):
  # lo, hi two directories
  # for aggregation

if __name__ == '__main__':
  # instance = parse_yaml()
  # shard('C', 2)
  run(2)