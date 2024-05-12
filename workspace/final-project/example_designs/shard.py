import ast
import json
from copy import deepcopy
import yaml
import subprocess
import os

THIS_SCRIPT_DIR = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
EXAMPLE_DIR = os.path.join(THIS_SCRIPT_DIR, 'layer_shapes')

BASE_DIRS = {
  'alexnet': f'{EXAMPLE_DIR}/CONV/AlexNet',
  'gpt': f'{EXAMPLE_DIR}/gpt2'
}

LAYERS = {
  'alexnet': [f'AlexNet_layer{i}' for i in range(1, 6)],
  'gpt': [str(i).zfill(3) for i in range(0, 145)]
}


NUM_SHARDS = [1, 2, 4, 8]

def parse_yaml(model, fn):
  base_dir = BASE_DIRS[model]

  if model == 'alexnet':
    with open(f'{base_dir}/{fn}.yaml') as f:
      instance = yaml.safe_load(f)
  else:
    assert model == 'gpt'

    with open(f'{base_dir}/{fn}.yaml') as f:
      instance = f.readlines()
    instance = instance[3]
    
    instance = instance.replace('instance: ', '')
    instance = instance.strip()

    instance = instance.replace('{', '').replace('}', '').split(',')
    instance = [x.split(':') for x in instance]
    instance = {x[0].strip(): int(x[1]) for x in instance}
  return instance

def shard(model, arch, dim, num_shards):
  layers, base_dir = LAYERS[model], BASE_DIRS[model]

  # if not os.path.exists(f'{base_dir}/shard_{num_shards}'):
  #   os.mkdir(f'{base_dir}/shard_{num_shards}')

  for l_idx, fn in enumerate(layers):
    instance = parse_yaml(model, fn)

    save_path = f'./layer_shapes/outputs/{model}/{arch}/shard_{num_shards}/layer{str(l_idx+1).zfill(3)}'
    os.makedirs(save_path)

    if model == 'alexnet':
      sz = instance['problem']['instance'][dim]
    else:
      sz = instance[dim]

    if sz < num_shards:
      sz_0, sz_1 = sz, None
    else:
      sz_0 = sz // num_shards
      sz_1 = sz - (num_shards - 1)*sz_0
      assert sz_0 * (num_shards - 1) + sz_1 == sz
    
    sz_list = [sz_0]

    if sz_1 is not None and sz_1 >= 1 and sz_1 != sz_0:
      sz_list.append(sz_1)

    for g_idx, x in enumerate(sz_list):
      instance_copy = deepcopy(instance)

      if model == 'alexnet':
        instance_copy['problem']['instance'][dim] = x
      else:
        instance_copy[dim] = x
        instance_copy = json.dumps(instance_copy).replace("\"", '')
        instance_copy = ["{{include_text('../../../../../problem_base.yaml')}}\n", "problem:\n", "  <<<: *problem_base\n", f"  instance: {instance_copy}"]

      with open(f'{save_path}/{g_idx+1}.yaml', 'w') as f:
        if model == 'alexnet':
          yaml.safe_dump(instance_copy, f)
        else:
          f.writelines(instance_copy)
      

def run(model, arch, num_shards):
  num_layers = len(LAYERS[model])
  scaled_arch = arch + "_shard_" + str(num_shards) 

  for l in range(num_layers):
    problem_path = f'outputs/{model}/{arch}/shard_{num_shards}/layer{str(l+1).zfill(3)}'
    save_path = f'./layer_shapes/{problem_path}'
    fns = list(os.listdir(save_path))

    for fn in fns:
      problem = f'{problem_path}/{fn}'
      cmd = f'python3 run_example_designs.py --architecture {scaled_arch} --problem {problem}'
      print(cmd)
      subprocess.run(cmd, shell=True, capture_output=True, text=True)
      fn_no_yaml = fn.replace('.yaml', '')
      cmd = f'mv ./example_designs/{scaled_arch}/outputs/{fn_no_yaml}/timeloop-mapper.stats.txt {save_path}/{fn_no_yaml}.txt'
      print(cmd)
      subprocess.run(cmd, shell=True, capture_output=True, text=True)

def pad_dir_names(model, arch, num_shards):
  num_layers = len(LAYERS[model])
  
  for l in range(num_layers):
    problem_path = f'outputs/{model}/{arch}/shard_{num_shards}/layer{l+1}'
    problem_path_new = f'outputs/{model}/{arch}/shard_{num_shards}/layer{str(l+1).zfill(3)}'

    cmd = f'mv ./layer_shapes/{problem_path} ./layer_shapes/{problem_path_new}'
    subprocess.run(cmd, shell=True, capture_output=True, text=True)

if __name__ == '__main__':
  # model, dim = 'alexnet', 'C'
  # model, dim = 'gpt', 'M'

  for num_shards in NUM_SHARDS:
    for arch in ['simba_like', 'eyeriss_like']:
      for (model, dim) in [('alexnet', 'C'), ('gpt', 'M')]:
        # pad_dir_names(model, arch, num_shards)

        # Step 1
        shard(model, arch, dim, num_shards)

        # Step 2
        run(model, arch, num_shards)