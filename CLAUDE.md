# TILT-FM: standing rules for every session

## End of every round: publish the preview (ROUND-6.md item 11)

At the end of every round:

1. Publish the new build to the web installer as a **preview**: `web/make_tfm_site.py ... --preview`, onto the
   `gh-pages` branch under `next/` (as rounds 1-5 did).
2. Check that the page loads and offers the new `.fwsc`.
3. End the round's reply to Felix with the link on its own line:

   https://cdmgaming.github.io/TILT-FM/next/webapp/installer/

   plus the package name and the commit it was built from.

The tested release at the site's root is **never** replaced by a preview.
