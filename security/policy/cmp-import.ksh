:
for ln in da de en es fr it ja ko nl pt sv
do
 diff /sexport/c40${ln}px/META-INF/manifest.mf ${ln}-export/META-INF/manifest.mf
 diff /sexport/c40${ln}px/META-INF/zigbert.sf  ${ln}-export/META-INF/zigbert.sf
 diff /sexport/c40${ln}px/META-INF/zigbert.rsa ${ln}-export/META-INF/zigbert.rsa
done

for ln in en fr 
do
 diff /sexport/c40${ln}us/META-INF/manifest.mf ${ln}-us/META-INF/manifest.mf
 diff /sexport/c40${ln}us/META-INF/zigbert.sf  ${ln}-us/META-INF/zigbert.sf
 diff /sexport/c40${ln}us/META-INF/zigbert.rsa ${ln}-us/META-INF/zigbert.rsa
done

for ln in en fr 
do
 diff /sexport/c40${ln}fr/META-INF/manifest.mf ${ln}-france/META-INF/manifest.mf
 diff /sexport/c40${ln}fr/META-INF/zigbert.sf  ${ln}-france/META-INF/zigbert.sf
 diff /sexport/c40${ln}fr/META-INF/zigbert.rsa ${ln}-france/META-INF/zigbert.rsa
done


