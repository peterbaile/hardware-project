for FILE in $1/*;
do
    filename=$(basename $FILE)
    ./ns3 run gpt-layer -- $FILE > $1_1MB/$filename 1Mbps 10ms 2>&1
    ./ns3 run gpt-layer -- $FILE > $1_100MB/$filename 100Mbps 10ms 2>&1
    ./ns3 run gpt-layer -- $FILE > $1_5GB/$filename 5Gbps 10us 2>&1
done
