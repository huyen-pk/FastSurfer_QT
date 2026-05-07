# Open Healthcare

This project focuses on research graded end-to-end signal processing pipelines.
It aims to:
- Provide the complete foundation of signal processing to build end-user medical applications.
- Provide the essential infrastructure for medical grade data governance.

The project is intended to be modular but more coarse than framework-oriented libraries such as MONAI.

C++ library will serve as baseline for implementation in other programming languages.

## Project Structure

- `core/`: native C++ medical signal processing modules (multimodal), including public headers, implementation sources, tests, and its local CMake build entrypoint.
- `specs/`: implementation plans, BDD requirements, and testing summaries for features and refactors.
- `evals/`: blueprints for evaluation and benchmark for core modules. This includes **methodologies and metrics** to evaluate against reference implementations and/or research papers. **Please do check this folder for scientific evidence of the implementation**.
- `baseline/`: vendored reference implementations or datasets used for parity checks and behavioral comparison.
- `data/`: imaging fixtures, sample datasets, and regression inputs used by native and parity tests.
- `generated/`: derived artifacts produced by validation or reference-generation workflows.
- `build/` and other build trees: generated CMake outputs for local compilation and test runs.

## namespace
- use `OpenHC` as root namespace. For example: 

    `namespace OpenHC::imaging::mri`

- use `ohc` as alias for long qualifiers. For example:

    `namespace ohc = OpenHC::imaging::mri;`

## Work in progress

Current plan is porting FastSurfer to C++ for MRI processing.

[Evaluation and benchmark](./evals/fastsurfer_cpp/EVALUATION.md)
