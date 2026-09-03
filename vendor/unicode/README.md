# Unicode Grapheme Data

`grapheme_data.inc` contains the Unicode 17.0.0 properties used by `src/util/utf8.cpp` for extended grapheme segmentation. The table combines `Grapheme_Cluster_Break`, `Indic_Conjunct_Break`, and `Extended_Pictographic`; unlisted code points have their default values.

To regenerate it, unpack the [Unicode 17.0.0 UCD](https://www.unicode.org/Public/17.0.0/ucd/) into a directory and run:

```sh
python3 tools/generate_unicode_grapheme_data.py /path/to/ucd
```

`GraphemeBreakTest.txt` is the unmodified conformance fixture from that release's `auxiliary` directory. `tests/util/utf8_test.cpp` verifies every boundary through prefix character counts, plus malformed UTF-8 handling. Builds and tests use the checked-in data without network access.

These Unicode data files are distributed under [LICENSE](LICENSE).
