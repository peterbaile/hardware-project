for FILE in csv/network_stats/eyeriss/alexnet/*;
do
    filename=$(basename $FILE)
    echo $FILE
    ./ns3/ns3 run tp-layer -- ../$FILE 1Mbps 10ms > csv/runtime/eyeriss/alexnet/1_MB/$filename 2>&1
    ./ns3/ns3 run tp-layer -- ../$FILE 100Mbps 10ms > csv/runtime/eyeriss/alexnet/100_MB/$filename 2>&1
    ./ns3/ns3 run tp-layer -- ../$FILE 5Gbps 10us > csv/runtime/eyeriss/alexnet/5_GB/$filename 2>&1
done

for FILE in csv/network_stats/eyeriss/gpt/*;
do
    filename=$(basename $FILE)
    echo $FILE
    ./ns3/ns3 run tp-layer -- ../$FILE 1Mbps 10ms > csv/runtime/eyeriss/gpt/1_MB/$filename 2>&1
    ./ns3/ns3 run tp-layer -- ../$FILE 100Mbps 10ms > csv/runtime/eyeriss/gpt/100_MB/$filename 2>&1
    ./ns3/ns3 run tp-layer -- ../$FILE 5Gbps 10us > csv/runtime/eyeriss/gpt/5_GB/$filename 2>&1
done

for FILE in csv/network_stats/simba/alexnet/*;
do
    filename=$(basename $FILE)
    echo $FILE
    ./ns3/ns3 run tp-layer -- ../$FILE 1Mbps 10ms > csv/runtime/simba/alexnet/1_MB/$filename 2>&1
    ./ns3/ns3 run tp-layer -- ../$FILE 100Mbps 10ms > csv/runtime/simba/alexnet/100_MB/$filename 2>&1
    ./ns3/ns3 run tp-layer -- ../$FILE 5Gbps 10us > csv/runtime/simba/alexnet/5_GB/$filename 2>&1
done


for FILE in csv/network_stats/simba/gpt/*;
do
    filename=$(basename $FILE)
    echo $FILE
    ./ns3/ns3 run tp-layer -- ../$FILE 1Mbps 10ms > csv/runtime/simba/gpt/1_MB/$filename 2>&1
    ./ns3/ns3 run tp-layer -- ../$FILE 100Mbps 10ms > csv/runtime/simba/gpt/100_MB/$filename 2>&1
    ./ns3/ns3 run tp-layer -- ../$FILE 5Gbps 10us > csv/runtime/simba/gpt/5_GB/$filename 2>&1
done


