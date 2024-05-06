for FILE in alexnet/*;
do
    filename=$(basename $FILE)
    ./ns3 run tp-layer -- $FILE > alexnet_100MB/$filename 2>&1
done
