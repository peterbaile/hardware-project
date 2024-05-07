for FILE in Simba-GPT/*;
do
    filename=$(basename $FILE)
    ./ns3 run tp-layer -- $FILE > Simba-GPT_1MB/$filename 2>&1
done
