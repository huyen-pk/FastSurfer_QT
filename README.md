> Disclaimer:
> Human guided  & human reviewed AI generation. Please DO double check the maths and report if you have any doubts.

> Reviewer: Huyen Pham

- [FastSurfer original paper](https://doi.org/10.1016/j.neuroimage.2020.117012)
- [FastSurfer github](https://github.com/Deep-MI/FastSurfer)

# FastSurfer C++ Rewrite — Evaluation Plan

This document captures the evaluation plan, datasets, metrics, and CI guidance for the project objective: rewrite the FastSurferCNN pipeline in C++ while preserving numerical equivalence and improving performance and robustness.

**This is done for each granular step in the pipeline (preprocessing, inference, postprocessing).**

## Goals
- Verify numerical and functional equivalence between the original Python FastSurferCNN and the new C++ implementation.
- Validate robustness across vendors, field strengths, and pathologies.
- Automate regression tests in CI with a small cached dataset.

## High-level Plan
1. Curate a set of input for test and validation from different datasets. Separate healthy and pathological sets.
2. Run Python baseline inference and store outputs (probability maps + `.mgz` outputs).
3. Run C++ implementation on same inputs and compare outputs with equivalence tests.
4. Track performance (runtime, peak RAM/VRAM) and resource usage.
5. Run robustness tests with out-of-distribution pathological datasets (e.g., brain tumors).

## Datasets (recommended)
Use a mixture of healthy, multi-vendor, high-resolution, and pathological datasets. Below are recommended public datasets and links.

- **IXI (multi-site vendor variety — lightweight, good for CI seeds)**
  - https://brain-development.org/ixi-dataset/
- **Calgary-Campinas (CC-359) (multi-vendor, multi-field-strength)**
  - https://sites.google.com/view/calgary-campinas-dataset/home
- Human Connectome Project (HCP) — Young Adult (high-resolution 0.7mm)
  - https://www.humanconnectome.org/study/hcp-young-adult
- **OASIS-1 (cross-sectional healthy / lifespan)**
  - https://www.oasis-brains.org/
- OASIS-3 (longitudinal, includes Alzheimer’s pathology)
  - https://www.oasis-brains.org/
- ADNI (Alzheimer's Disease Neuroimaging Initiative) — (application required)
  - https://adni.loni.usc.edu/data-samples/access-data/
- **Mindboggle-101 (manually labeled parcellations — gold standard)**
  - https://osf.io/nhtur/
- **Medical Segmentation Decathlon (MSD) — Task 01 Brain Tumour (use for robustness/OOD)**
  - http://medicaldecathlon.com/ (Task 01: Brain Tumour)

Notes:
- For CI, prefer IXI or a small OASIS subset to avoid heavy downloads. Keep HCP and ADNI for more thorough nightly/scheduled validation.
- The concrete benchmark implementation defined below currently assumes a **MindBoggle-based cohort** with multiple subjects, scan-rescan pairs, and generated FastSurfer artifacts; the broader dataset list remains optional for future extensions.
(In bold are downloadable for non research institution)

## Using MSD Brain Tumour Data (Guidance)
- Purpose: robustness and OOD testing only. Do not use to claim improved anatomical segmentation accuracy.
- Workflow:
  - Select 2–5 T1w(/T1gd) volumes from MSD Task 01.
  - Generate Python baseline outputs and store them.
  - Run C++ pipeline and assert exact equivalence (100% DSC between Python and C++ outputs) on these inputs.
  - Verify no crashes or memory corruption; differences should be debugged to ensure math/inference parity.

## Acceptance Criteria

Here is a comprehensive evaluation plan for rewriting the FastSurferCNN pipeline in C++, synthesized through the requested expert lenses.

### 1. Software Architecture Design Evaluation
**Objective:** Ensure the C++ rewrite is scalable, maintainable, and highly performant.
*   **Decoupling & Modularity:** Evaluate the separation between core data structures (e.g., `mgh_image.cpp`), ML inference execution, and the Qt UI/CLI. The pipeline should use a layered architecture.
*   **Dependency Management:** Evaluate the integration of native ML runtimes (e.g., ONNX Runtime or LibTorch) to replace Python PyTorch dependencies. Minimize bloated dependencies to facilitate edge/desktop deployment.
*   **Memory Management Strategy:** 3D MRI volumes are memory-intensive. Evaluate zero-copy data pathways between model inputs/outputs and C++ memory buffers (e.g., `std::vector<float>` mapped directly to ONNX tensors).
*   **Cross-platform CI/CD:** Establish build evaluations across Linux, macOS, and Windows utilizing CMake.

### 2. Senior Computer Vision Evaluation
**Objective:** Validate inference integrity and optimize visual computing performance.
*   **Multi-View Pipeline Integrity:** FastSurfer VINN leverages coronal, axial, and sagittal networks. The evaluation must verify that 2D slice extraction, 3D aggregation, and view-aggregation algorithms match the Python baseline exactly.
*   **Hardware Acceleration:** Evaluate inference latency and throughput on targeted hardware using ONNX Runtime execution providers (e.g., CUDA, TensorRT, CoreML, OpenVINO). 
*   **Numerical Precision:** Floating-point operations differ between Python/PyTorch and C++/ONNX. Establish acceptable delta margins for voxel predictions before the argmax/softmax aggregation layers.

### 3. Statistician Evaluation
**Objective:** Rigorously quantify equivalence between the Python and C++ implementations.
*   **Equivalence Testing:** For the current implementation, treat exact or tolerance-bounded agreement on exported tensors and discrete label volumes as the executable parity checks. Defer Two One-Sided Tests (TOST) on raw probability maps until both Python and C++ pipelines export aligned pre-argmax probability or logit artifacts for the same subjects.
*   **Segmentation Concordance Metrics:** 
  *   **Dice Similarity Coefficient (DSC):** Use exact voxel equality as the primary parity goal for discrete outputs; track DSC as a secondary diagnostic and alert if $DSC < 0.9999$ where exact equality is not feasible.
  *   **Hausdorff Family Metrics:** Use Hausdorff-style boundary metrics such as HD or HD95 to monitor boundary alignment on final volumetric outputs and manual benchmark masks.
*   **ROI Volumetric Agreement:** For derived ROI volumes from `aseg`-style and `aparc+aseg`-style outputs, calculate absolute-agreement ICC(A,1) and Bland-Altman summaries across multiple MindBoggle subjects.
*   **Repeated-Scan Reliability:** For MindBoggle subjects with both scan and rescan sessions, compute repeated-scan ROI reliability separately from Python-vs-C++ implementation agreement.
*   **Future Statistical Extensions:** TOST on probability maps, ROI group-separability analysis, and cortical-thickness-based metrics remain future work until logits, phenotype/covariate tables, and surface-pipeline outputs are available.
*   **Detailed Metrics Plan:** See the [Metrics](#metrics) section for the paper-aligned metrics review, implementation plan, benchmark script specification, and native-library mapping.

### 4. Medical Research Evaluation
**Objective:** Ensure the rewrite maintains clinical reliability and FreeSurfer compatibility.
*   **Clinical Robustness:** Evaluate the C++ pipeline against a diverse clinical dataset covering different MRI scanners (Siemens, GE, Philips), field strengths (1.5T, 3T, 7T), and head motion artifacts.
*   **Pathologic Resilience:** Test the pipeline on scans with neurodegeneration (e.g., Alzheimer's, MS lesions, enlarged ventricles) to ensure the C++ port hasn't altered the network's resilience to anomalies.
*   **Data IO Compliance:** Ensure absolute fidelity in reading and writing headers (e.g., the `fastsurfer::core::MghImage` header parsing, RAS orientation matrices, and affine transforms) so downstream medical tools (like FreeSurfer) parse the output flawlessly.

### 5. QA-Expert Evaluation
**Objective:** Institute continuous quality control and prevent regressions.
*   **Baseline Locking:** Generate a locked dataset of 10-20 diverse MRIs processed by the legacy FastSurferCNN Python pipeline. These serve as the ground-truth regression targets.
*   **Unit Testing:** Validate standard computational primitives (e.g., matrix inversions and endianness conversions from `mgh_image.cpp`) using C++ test frameworks like GoogleTest or Catch2.
*   **E2E Integration Testing:** Automate a pipeline that injects an `.mgz` or `.nii.gz` file, runs the C++ inference, pulls the output, and automatically triggers the Statistical Evaluation metrics against the Python baseline.
*   **Performance Profiling:** Track memory footprint (RAM/VRAM peaks) and execution speed in CI tracking to ensure the C++ version meets the primary goal of being faster and more resource-efficient than the Python setup.

## Concrete Oracle Files For Parity And Benchmarking

Use three distinct oracle sets. They answer different questions and should not be merged into one pass/fail gate:

1. **Python-to-C++ parity:** Does the C++ rewrite reproduce the legacy FastSurferCNN implementation?
2. **FreeSurfer compatibility benchmark:** Are the C++ outputs structurally compatible with standard FreeSurfer volumetric artifacts?
3. **Manual anatomical benchmark:** Do the final labels agree with curated human annotations better than, or at least comparably to, the existing baselines?

### A. Python FastSurfer Parity Oracles

These files are the primary release blockers for the reimplementation. The C++ code should be judged first against Python FastSurfer outputs generated from the **same input subject**.

For each test subject, persist the Python baseline into:

- `generated/FastSurferCNN/<subject>/step_02_conform_input_image_and_save_conformed_orig/conformed_orig.mgz`
- `generated/FastSurferCNN/<subject>/step_03_reorient_native_data_to_soft_lia_for_inference/orig_in_lia.npy`
- `generated/FastSurferCNN/<subject>/step_04_run_three_plane_cnn_inference_with_logit_space_view_aggregation/argmax_lia_after_coronal.npy`
- `generated/FastSurferCNN/<subject>/step_04_run_three_plane_cnn_inference_with_logit_space_view_aggregation/argmax_lia_after_sagittal.npy`
- `generated/FastSurferCNN/<subject>/step_04_run_three_plane_cnn_inference_with_logit_space_view_aggregation/argmax_lia_after_axial.npy`
- `generated/FastSurferCNN/<subject>/step_05_take_argmax_and_map_prediction_back_to_native_orientation/pred_classes_native_indices.mgz`
- `generated/FastSurferCNN/<subject>/step_06_map_internal_class_indices_to_freesurfer_label_ids/pred_classes_freesurfer_ids.mgz`
- `generated/FastSurferCNN/<subject>/step_07_split_cortical_labels_into_left_and_right_dkt_cortical_labels/pred_classes_split_cortex.mgz`
- `generated/FastSurferCNN/<subject>/mri/aparc.DKTatlas+aseg.deep.mgz`
- `generated/FastSurferCNN/<subject>/mri/aseg.auto_noCCseg.mgz`
- `generated/FastSurferCNN/<subject>/mri/brainmask.mgz`

If full logits can be saved in the future, add them as stronger pre-argmax parity artifacts.

| Objective | Python oracle file | C++ target artifact | Metric | Tolerance / gate | Release use |
| --- | --- | --- | --- | --- | --- |
| Conform parity | `generated/FastSurferCNN/<subject>/step_02_conform_input_image_and_save_conformed_orig/conformed_orig.mgz` | C++ conformed orig | header equality + voxel equality | exact header match; exact voxel match after integer conform | blocker |
| Orientation parity | `generated/FastSurferCNN/<subject>/step_03_reorient_native_data_to_soft_lia_for_inference/orig_in_lia.npy` | C++ soft-LIA tensor | tensor equality | exact or `max_abs_err <= 1e-6` for float serialization differences | blocker |
| Per-plane aggregation parity | `generated/FastSurferCNN/<subject>/step_04_.../argmax_lia_after_{coronal,sagittal,axial}.npy` | C++ per-plane aggregated class volume | voxel equality | exact class equality | blocker |
| Native label-space parity | `generated/FastSurferCNN/<subject>/step_05_take_argmax_and_map_prediction_back_to_native_orientation/pred_classes_native_indices.mgz` | C++ native class-index volume | voxel equality | exact integer match | blocker |
| LUT remap parity | `generated/FastSurferCNN/<subject>/step_06_map_internal_class_indices_to_freesurfer_label_ids/pred_classes_freesurfer_ids.mgz` | C++ FS-ID volume | voxel equality + label vocabulary equality | exact integer match | blocker |
| Cortical split parity | `generated/FastSurferCNN/<subject>/step_07_split_cortical_labels_into_left_and_right_dkt_cortical_labels/pred_classes_split_cortex.mgz` | C++ split-cortex volume | voxel equality | exact integer match | blocker |
| Final segmentation parity | `generated/FastSurferCNN/<subject>/mri/aparc.DKTatlas+aseg.deep.mgz` | C++ final segmentation | DSC + voxel equality | target exact match; alert if `DSC < 0.9999` | blocker |
| Reduced aseg parity | `generated/FastSurferCNN/<subject>/mri/aseg.auto_noCCseg.mgz` | C++ reduced aseg | voxel equality | exact integer match | blocker |
| Brainmask parity | `generated/FastSurferCNN/<subject>/mri/brainmask.mgz` | C++ brainmask | voxel equality | exact binary mask match | blocker |

### B. FreeSurfer Compatibility And Behavioral Benchmarks

These files are **not** the primary parity oracle. They should be used to measure cross-pipeline compatibility and anatomical reasonableness relative to FreeSurfer-style outputs.

Use the archived Colin27 FreeSurfer subject for benchmark artifacts:

- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/orig.mgz`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/brainmask.mgz`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/aseg.auto_noCCseg.mgz`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/aseg.auto.mgz`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/aseg.mgz`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/aparc+aseg.mgz`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/wmparc.mgz`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/stats/aseg.stats`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/stats/wmparc.stats`

| Objective | FreeSurfer benchmark file | C++ target artifact | Metric | Tolerance / gate | Release use |
| --- | --- | --- | --- | --- | --- |
| Input geometry compatibility | `.../mri/orig.mgz` | C++ input loading and conform path | header compatibility | no orientation/header corruption | blocker |
| Brainmask compatibility | `.../mri/brainmask.mgz` | C++ brainmask | DSC + Hausdorff | track only; not exact gate | benchmark |
| Aseg compatibility | `.../mri/aseg.auto_noCCseg.mgz` | C++ reduced aseg | DSC + ROI volume comparison | alert if major deep-structure deviations | benchmark |
| Post-CC aseg comparison | `.../mri/aseg.auto.mgz` or `.../mri/aseg.mgz` | optional downstream derivative | DSC + ROI volume comparison | informational only for FastSurferCNN inference scope | benchmark |
| Full aparc+aseg compatibility | `.../mri/aparc+aseg.mgz` | C++ final segmentation | DSC + label-range comparison | benchmark; not parity blocker | benchmark |
| White-matter parcellation comparison | `.../mri/wmparc.mgz` | optional derived output | ROI overlap + summary comparison | informational unless wmparc is implemented | benchmark |
| Summary-stat compatibility | `.../stats/aseg.stats`, `.../stats/wmparc.stats` | stats computed from C++ outputs | ICC + Bland-Altman on ROI volumes | target `ICC > 0.999` for parity-derived stats | benchmark |

### C. Manual Anatomical Benchmark Oracles

These files are the strongest available benchmark truth in this repository for anatomical labeling quality. Use them to judge segmentation accuracy, not implementation equivalence.

Preferred manual-truth files:

- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/labels.DKT31.manual+aseg.nii.gz`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/labels.DKT31.manual.nii.gz`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/label/lh.labels.DKT31.manual.annot`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/label/rh.labels.DKT31.manual.annot`

Secondary manual-truth files:

- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/label/lh.labels.DKT25.manual.annot`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/label/rh.labels.DKT25.manual.annot`

| Objective | Manual-truth file | C++ target artifact | Metric | Tolerance / gate | Release use |
| --- | --- | --- | --- | --- | --- |
| Voxel-space anatomical accuracy | `.../mri/labels.DKT31.manual+aseg.nii.gz` | C++ final segmentation | DSC per label + macro DSC + Hausdorff | benchmark target, not exact equality | benchmark |
| Cortical-only anatomical accuracy | `.../mri/labels.DKT31.manual.nii.gz` | C++ cortical parcels | cortical DSC per region | benchmark target | benchmark |
| Surface/annotation consistency | `.../label/lh.labels.DKT31.manual.annot`, `.../label/rh.labels.DKT31.manual.annot` | optional projected surface labels | per-hemisphere parcel agreement | use only if surface projection is part of evaluation | optional |

### Files To Avoid As Primary Truth

Do **not** use these as primary pass/fail truth for the FastSurferCNN inference rewrite:

- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/transforms/talairach.lta`
- `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/mri/transforms/talairach.m3z`
- any files under `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/surf/` for the current inference-only rewrite
- any files under `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/touch/`
- any files under `data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1/CVS/`
- any `**/._*` AppleDouble sidecar files

These are provenance, registration, surface-reconstruction, or archive artifacts. They are useful for audit and debugging, but not as primary inference parity oracles.

### Recommended Release Gates

Use the following release logic:

1. **Must pass:** all Python FastSurfer parity oracles for the locked baseline set.
2. **Must pass:** no header, dtype, affine, or label-vocabulary regressions in saved `.mgz` outputs.
3. **Must track:** FreeSurfer compatibility metrics on `brainmask.mgz`, `aseg.auto_noCCseg.mgz`, and `aparc+aseg.mgz`.
4. **Must track:** manual-label accuracy against `labels.DKT31.manual+aseg.nii.gz` for benchmark subjects.
5. **Must not block:** surface files, Talairach transforms, `touch/`, or CVS artifacts for the current FastSurferCNN inference reimplementation scope.

## Metrics

### Paper-Aligned Metrics Review

The original FastSurfer paper uses two metric families beyond voxel overlap:

1. **Reliability / agreement analysis** using ICC on test-retest data.
2. **Group separability analysis** using diagnosis effects in linear models.

These metrics fit the overall evaluation strategy, but only part of them fit the current **FastSurferCNN inference rewrite** scope.

### A. Metrics That Fit The Current Inference Rewrite

These should be added to the plan as secondary validation layers after Python parity has passed:

- **ROI-level ICC for subcortical volumes** derived from `aseg`-style outputs.
- **ROI-level ICC for volumetric cortical labels** derived from `aparc+aseg`-style outputs, with the explicit caveat that these are voxelized cortical estimates rather than surface-native cortical thickness.
- **Repeated-scan ROI reliability** on MindBoggle scan-rescan pairs, computed separately for Python outputs and C++ outputs.

Recommended implementation details, aligned with the paper:

- Use **absolute-agreement ICC** as described by McGraw and Wong (1996), not only correlation.
- Report **point estimate plus lower and upper confidence bounds** at $\alpha = 0.05$.
- For repeated-scan reliability in the current implementation, use the available **MindBoggle scan-rescan subset**.
- If a dedicated external test-retest cohort is added later, treat it as an extension rather than a prerequisite for the current benchmark harness.

### B. Metrics Deferred For Future Implementation

These metrics are intentionally deferred because the current benchmark data and generated artifacts do not yet support them end-to-end:

- **TOST on raw probability maps**, which requires aligned exported probability or logit volumes from both Python and C++ implementations.
- **ROI group separability for clinical groups**, which requires phenotype and covariate tables such as group, age, sex, site, and eTIV.

- **Cortical thickness ICC** based on white and pial surfaces.
- **Vertex-wise thickness ICC on fsaverage** after mapping to common space and smoothing at 15 mm FWHM.
- **Vertex-wise group analysis of cortical thickness maps** on fsaverage surfaces.

The thickness-related items should only become release gates if the project scope expands from FastSurferCNN inference to the **full FastSurfer pipeline including recon-surf / surface reconstruction**.

### C. Scope Decision For This Rewrite

For the current rewrite, use a two-layer metric policy:

1. **Primary release gates:** Python-to-C++ parity on intermediate and final FastSurferCNN inference artifacts.
2. **Secondary scientific validation:** ROI-level agreement and repeated-scan reliability on volumetric outputs.

Do **not** block the inference rewrite on cortical-thickness ICC or fsaverage vertex-wise analyses unless the implementation also reproduces the surface pipeline that generates `lh.thickness`, `rh.thickness`, `lh.white`, `rh.white`, `lh.pial`, and `rh.pial`.

### D. Recommended Additions To The Plan

Add the following explicit checks:

| Analysis layer | Metric | Input artifacts | Scope fit | Release use |
| --- | --- | --- | --- | --- |
| Python parity | exact voxel equality, DSC, Hausdorff family metrics | FastSurfer Python baseline vs C++ outputs | direct | blocker |
| ROI agreement | ICC with confidence bounds and Bland-Altman | `aseg` and `aparc+aseg`-derived ROI volumes | direct | benchmark |
| Repeated-scan ROI reliability | ICC with confidence bounds and within-subject repeatability summaries | MindBoggle scan-rescan pairs | direct | benchmark |
| Probability-map equivalence | TOST on probability or logit outputs | exported Python/C++ pre-argmax tensors | future | future |
| ROI group separability | matched GLM p-values / t-statistics | regional volumes on labeled cohorts with covariates | future | future |
| Surface thickness reliability | ICC on cortical thickness ROIs | `lh.thickness`, `rh.thickness` and fsaverage mapping | only if surface pipeline exists | future |
| Vertex-wise thickness group analysis | fsaverage vertex-wise GLM | thickness maps after 15 mm FWHM smoothing | only if surface pipeline exists | future |

### E. Practical Interpretation Against This Repository

This repository already supports the overlap-style metrics well for the inference rewrite. The existing FastSurfer utility metrics include Dice, IoU, precision/recall, and Hausdorff-style distance calculations in `baseline/FastSurfer/FastSurferCNN/utils/metrics.py`.

However, that file does **not** provide the paper's repeated-scan ICC workflow, probability-map TOST workflow, or diagnosis-controlled GLM workflow. Those should be treated as higher-level evaluation layers implemented in the benchmark harness, not as low-level segmentation metrics.

The currently generated FastSurfer artifacts in this repository expose final derivative volumes such as `aparc.DKTatlas+aseg.deep.mgz`, `aseg.auto_noCCseg.mgz`, and `brainmask.mgz`; they do **not** yet guarantee exported raw probability maps suitable for TOST.

### Concrete Paper-Metrics Implementation Plan

This section turns the paper-aligned metric review into an executable benchmark plan for the current rewrite.

### A. Dataset Roles And Scope

Use datasets for distinct purposes rather than forcing one cohort to answer every question.

| Dataset | Role in this plan | Minimum subject count | Required files | Notes |
| --- | --- | --- | --- | --- |
| **MindBoggle101 benchmark subset** | primary ROI agreement benchmark and anatomical benchmark | `>= 10` benchmark subjects| input `mri/orig.mgz`; optional manual truth `mri/labels.DKT31.manual+aseg.nii.gz`; FreeSurfer-style outputs such as `mri/aparc+aseg.mgz` | current primary benchmark cohort |
| **MindBoggle101 repeated-scan subset** | current repeated-scan reliability benchmark | all available scan-rescan pairs | T1w inputs plus `subject_id` and `session_id` linking scan and rescan | current source for repeated-scan ROI reliability |
| **Future labeled cohort** | ROI group-separability benchmark | `>= 25` per group after QC | T1w inputs plus metadata table with `group`, `age`, `sex`, `site`, and `eTIV` | not part of the current implementation scope |

Practical scope rule:

- Use **MindBoggle101** as the concrete benchmark cohort already represented in this repository.
- Use **Colin27-1** as the benchmark harness smoke test and schema reference.
- Use the **MindBoggle scan-rescan subset** for current repeated-scan reliability.
- Treat **group-separability cohorts** as a future extension.

### B. Exact Derived Outputs To Compute

For every subject $s$, ROI $r$, and implementation $m \in \{\text{python}, \text{cpp}\}$, compute the following from the final volumetric outputs that are currently available:

- `aparc.DKTatlas+aseg.deep.mgz`
- `aseg.auto_noCCseg.mgz`
- optional benchmark reference volumes from FreeSurfer `aparc+aseg.mgz` and manual labels `labels.DKT31.manual+aseg.nii.gz`

Define:

$$
N_{s,r,m} = \sum_v \mathbf{1}[\hat{y}_{s,m}(v) = r]
$$

$$
V_{s,r,m} = N_{s,r,m} \cdot (\Delta x_s \Delta y_s \Delta z_s)
$$

where $N_{s,r,m}$ is the voxel count for ROI $r$ and $\Delta x_s \Delta y_s \Delta z_s$ is the subject voxel volume in mm$^3$ from the conformed or final output header.

Required subject-level tabular outputs:

- `generated/benchmarks/paper_metrics/<cohort>/subject_level/roi_volumes_python.csv`
- `generated/benchmarks/paper_metrics/<cohort>/subject_level/roi_volumes_cpp.csv`
- `generated/benchmarks/paper_metrics/<cohort>/subject_level/roi_volumes_reference.csv` when FreeSurfer or manual labels are available
- `generated/benchmarks/paper_metrics/<cohort>/subject_level/roi_volumes_long.csv`

Required columns in the long table:

- `subject_id`
- `session_id`
- `cohort`
- `implementation`
- `source_volume`
- `roi_name`
- `label_id`
- `voxel_count`
- `voxel_volume_mm3`
- `roi_volume_mm3`
- `group`
- `age`
- `sex`
- `site`
- `etiv_mm3`

**Probability-map outputs are intentionally excluded from the current implementation because the benchmark artifacts do not yet guarantee aligned exported logits or probability maps for both implementations.**

### C. ROI ICC And Bland-Altman Formulas

For agreement between Python and C++ ROI volumes, use **absolute-agreement ICC(A,1)** with $k = 2$ methods and $n$ subjects:

$$
ICC(A,1) = \frac{MS_R - MS_E}{MS_R + (k - 1)MS_E + \frac{k(MS_C - MS_E)}{n}}
$$

where:

- $MS_R$ is the mean square for subjects
- $MS_C$ is the mean square for methods
- $MS_E$ is the residual mean square

For each ROI, also compute Bland-Altman summaries:

$$
\mu_{s,r} = \frac{V_{s,r,\text{python}} + V_{s,r,\text{cpp}}}{2}
$$

$$
d_{s,r} = V_{s,r,\text{cpp}} - V_{s,r,\text{python}}
$$

$$
	ext{bias}_r = \overline{d_{s,r}}, \quad
	ext{LoA}_{r}^{\pm} = \text{bias}_r \pm 1.96 \cdot SD(d_{s,r})
$$

Required reliability outputs:

- `generated/benchmarks/paper_metrics/<cohort>/derived/roi_icc_summary.csv`
- `generated/benchmarks/paper_metrics/<cohort>/derived/roi_bland_altman.csv`
- `generated/benchmarks/paper_metrics/<cohort>/derived/roi_reliability_summary.json`

Required columns in `roi_icc_summary.csv`:

- `roi_name`
- `label_id`
- `n_subjects`
- `icc_a1`
- `ci_low_95`
- `ci_high_95`
- `mean_signed_diff_mm3`
- `mean_abs_diff_mm3`
- `mean_abs_pct_diff`
- `pass_threshold`

### D. Repeated-Scan ROI Reliability

For subjects with both scan and rescan sessions, compute repeatability on ROI volumes separately for each implementation.

For subject $u$, ROI $r$, session pair $(a, b)$, and implementation $m$:

$$
\Delta^{\mathrm{retest}}_{u,r,m} = V_{u,r,a,m} - V_{u,r,b,m}
$$

Track at minimum:

- repeated-scan ICC(A,1) per ROI
- mean signed retest difference in mm$^3$
- mean absolute retest difference in mm$^3$
- mean absolute retest percent difference

Required repeated-scan outputs:

- `generated/benchmarks/paper_metrics/<cohort>/derived/roi_retest_summary_python.csv`
- `generated/benchmarks/paper_metrics/<cohort>/derived/roi_retest_summary_cpp.csv`
- `generated/benchmarks/paper_metrics/<cohort>/derived/roi_retest_delta.csv`

### E. ROI Group-Separability Model (Future Implementation)

This stage is intentionally deferred until a labeled cohort with phenotype and covariate data is available.

For each ROI, fit identical linear models independently on Python-derived and C++-derived ROI tables:

$$
V_{s,r,m} = \beta_{0,r,m} + \beta_{1,r,m} \cdot \mathrm{Group}_s + \beta_{2,r,m} \cdot \mathrm{Age}_s + \beta_{3,r,m} \cdot \mathrm{Sex}_s + \beta_{4,r,m} \cdot \mathrm{eTIV}_s + \sum_j \gamma_{j,r,m} \cdot \mathrm{Site}_{s,j} + \epsilon_{s,r,m}
$$

Minimum comparison outputs per ROI:

- estimated group coefficient $\hat{\beta}_{1,r,m}$
- standard error $SE(\hat{\beta}_{1,r,m})$
- $t$ statistic
- raw $p$ value
- FDR-corrected $q$ value
- coefficient sign

Required GLM outputs:

- `generated/benchmarks/paper_metrics/<cohort>/derived/roi_group_glm_python.csv`
- `generated/benchmarks/paper_metrics/<cohort>/derived/roi_group_glm_cpp.csv`
- `generated/benchmarks/paper_metrics/<cohort>/derived/roi_group_glm_delta.csv`
- `generated/benchmarks/paper_metrics/<cohort>/derived/roi_group_glm_summary.json`

Required columns in `roi_group_glm_delta.csv`:

- `roi_name`
- `label_id`
- `beta_python`
- `beta_cpp`
- `abs_beta_diff`
- `t_python`
- `t_cpp`
- `abs_t_diff`
- `p_python`
- `p_cpp`
- `q_python`
- `q_cpp`
- `sign_match`
- `pass_threshold`

Interpretation rules:

- If the cohort has fewer than two valid groups after QC, skip GLM and mark the dataset as **not eligible** for group-separability analysis.
- If MindBoggle101 is the only available cohort, run the ROI extraction, ICC, Bland-Altman, and repeated-scan reliability stages, but do **not** claim paper-style diagnosis separability.

### F. Benchmark Script Specification

Implement the benchmark harness as a dedicated analysis script specification:

- `tools/metrics.py`
Implement metric computations.

- `tools/benchmark.py`
Ingest a manifest of benchmark subjects, extract data and compute metrics.

- Draft manifest: `_app/specs/evaluation_data_manifest.csv`
Use this as the canonical starter manifest for the smoke test, MindBoggle benchmark subjects, MindBoggle repeated-scan subjects, Python-generated FastSurfer outputs under `generated/FastSurferCNN`, and future C++ outputs under `generated/CPP`.

- Repeated-scan pairing convention:
Rows sharing the same `subject_id` but different `session_id` values represent scan-rescan pairs for repeated-scan reliability analysis.


Required manifest columns:

- `subject_id`
- `session_id`
- `input_path`
- `python_aparc_path`
- `python_aseg_path`
- `cpp_aparc_path`
- `cpp_aseg_path`
- `reference_aparc_path`
- `reference_manual_path`

Optional metadata columns:

- `group`
- `age`
- `sex`
- `site`
- `etiv_mm3`

Execution stages:

1. Validate file existence, header readability, and label-map compatibility.
2. Extract voxel counts and ROI volumes from Python, C++, and optional reference volumes.
3. Write subject-level CSV outputs and a unified long table.
4. Compute ROI-level ICC(A,1) and Bland-Altman summaries.
5. If repeated-scan pairs are available, compute repeated-scan ROI reliability summaries separately for Python and C++.
6. Emit CSV, JSON, and optional plots into the benchmark output directory.

Required failure behavior:

- hard fail on missing Python or C++ segmentation files
- hard fail on malformed voxel spacing or unreadable headers
- soft skip repeated-scan summaries when paired sessions are unavailable
- soft skip GLM stage when metadata are incomplete or group cardinality is insufficient
- soft warn when an ROI is absent in all subjects and exclude it from ICC/GLM summaries

Smoke-test mode using the repository-local subject:

- Manifest containing only `data/Colin27-1/.../mri/orig.mgz` should succeed through extraction and table-generation stages.
- The same single-subject manifest should skip ICC confidence intervals, repeated-scan summaries, and GLM with explicit reason codes in the summary JSON.

### G. `metrics.py` Review And Native C++ Library Mapping

The current Python metric module is useful as a semantic reference, but it should not be ported line-for-line into C++.

| Python function | Current dependency pattern | Recommended native implementation | Recommendation |
| --- | --- | --- | --- |
| `iou_score` | torch boolean masks and per-class counts | plain C++ loops over label volumes with integer counters | no extra library needed |
| `precision_recall` | torch boolean masks and reductions | plain C++ loops over label volumes with TP/FP/FN counters | no extra library needed |
| `DiceScore` | dense confusion-style accumulator in torch | plain C++ accumulator backed by `std::vector<uint64_t>` or similar | no extra library needed |
| `dice_score` | scipy or numpy boolean arithmetic | plain C++ binary-mask arithmetic, or ITK overlap filter for binary ROIs | use plain C++ for simplicity |
| `volume_similarity` | numpy reductions | ITK overlap metrics for binary ROIs or plain C++ counts | use ITK when already materializing ROI masks |
| `hd`, `hd95` | scipy morphology + Euclidean distance transform + percentile | ITK contour extraction plus ITK distance map, then percentile reduction in C++ | use ITK |
| `__surface_distances` | `binary_erosion`, `generate_binary_structure`, `distance_transform_edt` | ITK binary contour / erosion plus `SignedMaurerDistanceMapImageFilter` | use ITK |

Concrete library guidance for the C++ rewrite:

- Prefer **ITK** for 3D image morphology, contour extraction, spacing-aware distance maps, and overlap-style image metrics because it is already linked in `_app/core/CMakeLists.txt`.
- Prefer **plain C++ containers and loops** for per-class confusion counts, IoU, precision, recall, and Dice accumulation. These operations are simpler and clearer than wrapping them in a larger numerical dependency.
- If ICC confidence intervals or GLM p-values are later moved into native C++, add **Boost.Math** for $t$ and $F$ distribution functions rather than hand-deriving special-function code.
- Do **not** prefer OpenCV for these metrics. Its strengths are 2D computer vision and it is a poor fit for spacing-aware 3D neuroimaging morphology.

Important implementation note from `metrics.py`:

- `volume_similarity(pred, gt)` currently uses `np.save(gt)` where a volume reduction is expected. Treat this as a probable bug in the Python reference and do not mirror it in the C++ implementation.

