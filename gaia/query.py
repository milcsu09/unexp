#!/usr/bin/python3

from astroquery.gaia import Gaia

OUTPUT_PATH = "gaia/data.csv"

query = """
SELECT TOP 250000
  gs.source_id,
  gs.ra,
  gs.dec,
  gs.parallax,
  ap.lum_flame

FROM
  gaiadr3.gaia_source AS gs

JOIN
  gaiadr3.astrophysical_parameters AS ap
  ON gs.source_id = ap.source_id

WHERE
  gs.parallax > 0
  AND ap.lum_flame IS NOT NULL

ORDER BY
  gs.parallax DESC
"""

job = Gaia.launch_job_async(query)
results = job.get_results()

if results:
    results.write(OUTPUT_PATH, format="csv", overwrite=True)

    print(f"{OUTPUT_PATH}: {len(results)} column(s) written")

