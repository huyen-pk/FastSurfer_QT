# Open Healthcare

## Project Structure

- `core/`: native C++ medical signal processing modules (multimodal), including public headers, implementation sources, tests, and its local CMake build entrypoint.
- `baseline/FastSurfer/`: vendored Python reference implementation used for parity checks and behavioral comparison.
- `specs/`: implementation plans, BDD requirements, and testing summaries for features and refactors.
- `data/`: imaging fixtures, sample datasets, and regression inputs used by native and parity tests.
- `generated/`: derived artifacts produced by validation or reference-generation workflows.
- `web/`, `ml/`, `rendering/`, and `ux/`: additional surfaces for web experiences, machine-learning work, visualization, and design exploration.
- `build/` and other build trees: generated CMake outputs for local compilation and test runs.
