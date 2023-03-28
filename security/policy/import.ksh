:
CP=cp
for ln in da de en es fr it ja ko nl pt sv
do
    echo cp -r -v /sexport/c40${ln}ex/META-INF/*.* ${ln}-export/META-INF
    ${CP}   -r -v /sexport/c40${ln}ex/META-INF/*.* ${ln}-export/META-INF
done

for ln in en fr 
do
    echo cp -r -v /sexport/c40${ln}us/META-INF/*.* ${ln}-us/META-INF
    ${CP}   -r -v /sexport/c40${ln}us/META-INF/*.* ${ln}-us/META-INF
    echo cp -r -v /sexport/c40${ln}fr/META-INF/*.* ${ln}-france/META-INF
    ${CP}   -r -v /sexport/c40${ln}fr/META-INF/*.* ${ln}-france/META-INF
done

