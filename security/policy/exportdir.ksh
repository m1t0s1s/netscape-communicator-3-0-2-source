:
rm -rf /export || true

for c40dir in c40enfr c40enex c40enus c40frfr c40frex c40frus \
    c40daex c40deex c40esex c40itex c40jaex c40koex c40nlex c40ptex c40svex
do
    mkdir -p /export/${c40dir}
done

cp da-export.meta	/export/c40daex/policy.meta
cp de-export.meta	/export/c40deex/policy.meta
cp en-france.meta	/export/c40enfr/policy.meta
cp en-export.meta	/export/c40enex/policy.meta
cp en-us.meta		/export/c40enus/policy.meta
cp es-export.meta	/export/c40esex/policy.meta
cp fr-france.meta	/export/c40frfr/policy.meta
cp fr-export.meta	/export/c40frex/policy.meta
cp fr-us.meta		/export/c40frus/policy.meta 
cp it-export.meta	/export/c40itex/policy.meta
cp ja-export.meta	/export/c40jaex/policy.meta
cp ko-export.meta	/export/c40koex/policy.meta
cp nl-export.meta	/export/c40nlex/policy.meta
cp pt-export.meta	/export/c40ptex/policy.meta
cp sv-export.meta	/export/c40svex/policy.meta


