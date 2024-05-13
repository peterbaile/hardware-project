cd example_designs
python3 shard.py
cd ..

# eyeriss + alexnet
python3 scripts/extract_data.py --shard_dir ./example_designs/layer_shapes/outputs/alexnet/eyeriss_like --dataspace Outputs --datawidth 1 --model conv --csv_network shard_eyeriss_alexnet_network.csv --csv_stats shard_eyeriss_alexnet_stats.csv
mv *_eyeriss_alexnet_network.csv ./csv/network_stats/eyeriss/alexnet/
mv *_eyeriss_alexnet_stats.csv ./csv/local_stats/eyeriss/alexnet/

# simba + alexnet
python3 scripts/extract_data.py --shard_dir ./example_designs/layer_shapes/outputs/alexnet/simba_like --dataspace Outputs --datawidth 1 --model conv --csv_network shard_simba_alexnet_network.csv --csv_stats shard_simba_alexnet_stats.csv
mv *_simba_alexnet_network.csv ./csv/network_stats/simba/alexnet/
mv *_simba_alexnet_stats.csv ./csv/local_stats/simba/alexnet/

# eyeriss + gpt
python3 scripts/extract_data.py --shard_dir ./example_designs/layer_shapes/outputs/gpt/eyeriss_like --datawidth 1 --model gpt --csv_network shard_eyeriss_gpt_network.csv --csv_stats shard_eyeriss_gpt_stats.csv
mv *_eyeriss_gpt_network.csv ./csv/network_stats/eyeriss/gpt/
mv *_eyeriss_gpt_stats.csv ./csv/local_stats/eyeriss/gpt/

# simba + gpt
python3 scripts/extract_data.py --shard_dir ./example_designs/layer_shapes/outputs/gpt/simba_like --datawidth 1 --model gpt --csv_network shard_simba_gpt_network.csv --csv_stats shard_simba_gpt_stats.csv
mv *_simba_gpt_network.csv ./csv/network_stats/simba/gpt/
mv *_simba_gpt_stats.csv ./csv/local_stats/simba/gpt/