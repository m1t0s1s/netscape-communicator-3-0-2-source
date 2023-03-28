:
for ln in da de en es fr it ja ko nl pt sv
do
    sed -e "s/xx/$ln/" < xx-export.meta > ${ln}-export.meta 
done

for ln in en fr 
do
    sed -e "s/xx/$ln/" < xx-france.meta > ${ln}-france.meta 
    sed -e "s/xx/$ln/" < xx-us.meta     > ${ln}-us.meta 
done

