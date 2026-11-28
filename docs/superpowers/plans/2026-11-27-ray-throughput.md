# Ray, palette and occupancy throughput pass

Baseline 3c1e574b1fc44fec90c808f2c9bc1648d1f0293f. Preserve 20x12 ANT EYE tile sweep, camera latching, all palettes, geometry and terrain effects, real ant silhouettes, save bytes and ROM3 bank switching.
- Replace height-major perspective storage with depth-major padded rows. Walk a row pointer per ray step, apply eye-height bias once per column. Check every original depth/height combination and source wiring.
- Expand base palette colors into exact 2bpp byte pairs at compile time. Stop palette classification when highest-priority ant/food class is encountered. All existing sky, terrain grain, limb and antenna effects remain unchanged; extend original-packer differential test to 35,360 exact columns and 4,753 span bounds.
- Query occupancy in reverse overlay priority (queen 1, queen 0, highest worker slot first), with pointer traversal and early return. Add deliberate queen/worker overlap tests; retain 8,000-action full-atlas comparison.
- Keep native suite, bank layout check, real GBC ROM build, and complete PyBoy test suite mandatory. No claim of measured hardware FPS.
