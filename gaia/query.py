#!/usr/bin/python3

from astroquery.gaia import Gaia

query = (
    "SELECT TOP 100000 source_id, ra, dec, parallax "
    "FROM gaiadr3.gaia_source "
    "WHERE parallax > 0 "
    "ORDER BY parallax DESC"
)

job = Gaia.launch_job(query)
results = job.get_results()

if results:
    results.write("gaia/data.csv", format="csv", overwrite=True)

