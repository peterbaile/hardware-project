for FILE in alexnet_data/*;
do
    filename=$(basename $FILE)
    ./ns3 run tp-layer -- $FILE > alexnet_1MB/$filename 2>&1
done
