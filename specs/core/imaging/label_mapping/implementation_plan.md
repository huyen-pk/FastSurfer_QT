# Label Mapping Implementation Plan

## Scope

Design a native C++ label-mapping layer for:

- FastSurfer <-> MindBoggle101 Colin27 manual DKT31 labels
- FastSurfer <-> MindBoggle101 Colin27 `manual+aseg` labels
- FastSurfer <-> MSD Brain Tumor (`BRATS_001_labels.nii.gz`) labels

This plan covers analysis and architecture only. No implementation is included in this step.

## Baseline Findings

### 1. FastSurfer label handling in the Python reference

FastSurfer does not treat the brainmask as an independent semantic label space. It derives the brainmask from the segmentation by:

1. Thresholding all non-zero labels.
2. Excluding a small frontal special case.
3. Applying dilation then erosion.
4. Keeping the largest connected component.

Reference:

- `create_mask()` in `baseline/FastSurfer/FastSurferCNN/reduce_to_aseg.py`
- `run_prediction.py` creates `mask.mgz` from the segmentation and then creates reduced `aseg` from the full output.

The relevant reductions are split across two kinds of logic:

- Pure value remaps, for example `29 -> 0`, `30 -> 2`, `61 -> 0`, `62 -> 41`, `72 -> 24`, `80 -> 77`, and `85 -> 0`.
- Spatially dependent transforms, for example:
  - filling unknown cortex labels `1000` and `2000` from nearby parcels,
  - splitting reduced cortical labels back into right-hemisphere labels after inference.

One important distinction for the native design is that the Python reference mixes two different concerns:

- `clean_cortex_labels()` and `fuse_cortex_labels()` operate in FreeSurfer-style anatomical ID space.
- `map_aparc_aseg2label()` is a training-target generator that converts anatomical IDs into sequential class-index volumes for coronal/axial and sagittal supervision.

That means `map_aparc_aseg2label()` is evidence for reference preprocessing behavior, but it is not itself the source of truth for the final segmentation-stage output ontology.

Reference code paths:

- `clean_cortex_labels()`
- `fuse_cortex_labels()`
- `split_cortex_labels()`
- `map_aparc_aseg2label()`
- `map_prediction_sagittal2full()`

### 2. FastSurfer ontology source of truth and stage boundaries

FastSurfer does not use one single ontology file as the source of truth for every stage. The correct source of truth depends on which output is being discussed.

#### A. Source of truth for direct FastSurferCNN inference

For direct `FastSurferCNN/run_prediction.py` inference, the initial label-ID source of truth is the LUT passed through `--lut`.

Observed behavior:

- `run_prediction.py` loads `self.labels = lut["ID"]`.
- If the caller does not override `--lut`, the default is `FastSurfer_ColorLUT.tsv`.
- `map_label2aparc_aseg()` maps network class indices to the IDs listed in that LUT.

Conclusion:

- `FastSurfer_ColorLUT.tsv` is the authoritative ID list for the **pre-split inference label space**.
- It contains `79` labels including background.

#### B. Source of truth for the final segmentation-stage deep output

`FastSurfer_ColorLUT.tsv` is **not** by itself the exhaustive label-ID list for the final `aparc.DKTatlas+aseg.deep.mgz` output.

Observed behavior:

- After LUT mapping, `run_prediction.py` calls `split_cortex_labels()`.
- `split_cortex_labels()` can generate additional right-hemisphere cortical IDs by re-lateralizing selected cortical parcels after inference.
- These additional IDs include labels such as `2003`, `2006`, `2007`, `2008`, `2009`, `2011`, `2015`, `2018`, `2019`, `2020`, `2026`, `2027`, `2029`, `2030`, `2031`, `2034`, and `2035`, which are not present in `FastSurfer_ColorLUT.tsv`.

Verified on the generated Colin27 artifact:

- `generated/FastSurferCNN/Colin27-1/mri/aparc.DKTatlas+aseg.deep.mgz` contains `96` unique labels.
- All `79` IDs from `FastSurfer_ColorLUT.tsv` are present in the generated Colin27 output.
- The final generated output contains `17` additional right-hemisphere cortical IDs introduced by `split_cortex_labels()`.

Exhaustive segmentation-stage comparison baseline for `aparc.DKTatlas+aseg.deep.mgz`:

```text
0
2, 4, 5, 7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 24, 26, 28, 31
41, 43, 44, 46, 47, 49, 50, 51, 52, 53, 54, 58, 60, 63, 77
1002, 1003, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019, 1020, 1021, 1022, 1023, 1024, 1025, 1026, 1027, 1028, 1029, 1030, 1031, 1034, 1035
2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026, 2027, 2028, 2029, 2030, 2031, 2034, 2035
```

Conclusion:

- The correct exhaustive comparison baseline for the segmentation-stage deep output is the **96-ID post-split output ontology**, not the raw `FastSurfer_ColorLUT.tsv` file alone.
- The practical source of truth for this baseline is the code path `FastSurfer_ColorLUT.tsv -> map_label2aparc_aseg() -> split_cortex_labels()`.

#### C. Source of truth for stats, reporting, and downstream modules

FastSurfer also uses `FreeSurferColorLUT.txt`, but for a different purpose.

Observed behavior:

- `run_fastsurfer.sh` uses `FreeSurferColorLUT.txt` for `segstats.py` and other downstream reporting tasks.
- The ids passed to stats generation are explicitly enumerated in `run_fastsurfer.sh`.
- Surface and corpus callosum stages produce richer outputs such as `aparc.DKTatlas+aseg.deep.withCC.mgz`, `aparc.DKTatlas+aseg.mapped.mgz`, and `aseg.mgz` after downstream refinement.

Conclusion:

- `FreeSurferColorLUT.txt` is the appropriate reference for **stats/reporting and broader downstream FreeSurfer-compatible products**.
- It is not the direct source of truth used by `run_prediction.py` to produce the segmentation-stage deep output.

### 3. Hemisphere handling in FastSurfer

FastSurfer handles the hemispheres in multiple stages rather than keeping one fixed bilateral ontology throughout the pipeline.

Observed behavior:

1. Coronal and axial prediction operate in the full inference label space.
2. Sagittal prediction uses a reduced hemisphere-collapsed class space.
3. Sagittal predictions are expanded back into the full class space before view aggregation completes.
4. The final deep segmentation is post-processed to re-lateralize selected cortical parcels into right-hemisphere IDs.

Reference mechanics:

- `get_labels_from_lut()` constructs both the full label list and the sagittal-reduced label list.
- The default sagittal mask removes labels whose names start with `Left-` and `ctx-rh`, which collapses hemisphere-specific distinctions for the sagittal model.
- `unify_lateralized_labels()` pairs left and right subcortical labels using their name prefixes.
- `map_aparc_aseg2label()` de-lateralizes cortical labels for sagittal preparation and maps lateralized labels into the sagittal label set.
- `map_prediction_sagittal2full()` expands sagittal predictions back into the full label space during inference.
- `split_cortex_labels()` then uses the centroids of left and right white-matter components plus Gaussian-smoothed hemisphere support maps to decide whether selected cortical parcels should become right-hemisphere labels in the final output.

Conclusion:

- FastSurfer does not treat hemispheres as a single static ontology decision.
- Hemisphere handling is a staged process: collapse for sagittal learning efficiency, expand back to the full class space, then perform spatial re-lateralization in the final output.
- Any native parity implementation must preserve this staged behavior rather than treating the final IDs as a simple static LUT.

### 4. Verified Colin27 label-space compatibility

The following comparisons were verified from the actual dataset files and the generated FastSurfer output for Colin27.

#### A0. Cross-check against Colin27 `label/` surface annotations

Directory:

- `data/MindBoggle101_20260501/Colin27-1/.../label/`

Verified manual surface annotation files:

- `lh.labels.DKT31.manual.annot`
- `rh.labels.DKT31.manual.annot`

Observed parcel counts from the surface annotations:

- `34` named cortical parcels on the left hemisphere
- `34` named cortical parcels on the right hemisphere

The surface manual DKT31 annotations include these parcels that do **not** appear in the volumetric `labels.DKT31.manual.nii.gz` export and do **not** appear in the generated FastSurfer output:

- `ctx-lh-bankssts`
- `ctx-lh-frontalpole`
- `ctx-lh-temporalpole`
- `ctx-rh-bankssts`
- `ctx-rh-frontalpole`
- `ctx-rh-temporalpole`

All cortical labels present in the volumetric manual export are represented in the surface manual annotations, but the surface annotations are a slightly richer label space.

Conclusion:

- The earlier `63 / 63` compatibility result applies to the **volumetric** manual DKT31 export, not to the richer surface `*.annot` files.
- The Colin27 `label/` directory confirms that MindBoggle surface DKT31 is not an exact identity match to final FastSurfer output.
- The mismatch is narrowly scoped to six parcels: `bankssts`, `frontalpole`, and `temporalpole` in both hemispheres.

#### A. MindBoggle manual cortical labels

File:

- `data/MindBoggle101_20260501/Colin27-1/.../mri/labels.DKT31.manual.nii.gz`

Observed unique labels:

- 63 total unique values
- `0` plus DKT31 cortical IDs in the `1000` and `2000` ranges

Compatibility with the 96-ID segmentation-stage FastSurfer baseline:

- Exact label-space coverage: `63 / 63`
- Missing labels from FastSurfer output: none

Conclusion:

- The pure DKT31 manual cortical volume is already in the final FastSurfer output label space.
- For this file, the preferred C++ mapping is effectively identity.

#### B. MindBoggle manual fused with aseg

File:

- `data/MindBoggle101_20260501/Colin27-1/.../mri/labels.DKT31.manual+aseg.nii.gz`

Observed unique labels:

- 107 total unique values

Compatibility with the 96-ID segmentation-stage FastSurfer baseline:

- Shared output labels: `96 / 107` unique labels
- Missing labels in FastSurfer output:
  - `30`, `62` (`Left-vessel`, `Right-vessel`)
  - `80` (`non-WM-hypointensities`)
  - `85` (`Optic-Chiasm`)
  - `251`-`255` (corpus callosum sublabels)
  - `1000`, `2000` (unknown cortex)

Conclusion:

- Compatibility is high once FastSurfer-style canonicalization is applied.
- A pure lookup-table implementation is insufficient because `1000` and `2000` require spatial fill logic if parity with FastSurfer behavior is desired.

#### C. Colin27 FreeSurfer `aparc+aseg`

File:

- `data/MindBoggle101_20260501/Colin27-1/.../mri/aparc+aseg.nii.gz`

Observed unique labels:

- 113 total unique values

Labels missing from the 96-ID segmentation-stage FastSurfer baseline:

- The same 11 labels missing from `manual+aseg`, plus:
  - `1001`, `2001` (`bankssts`)
  - `1032`, `2032` (`frontalpole`)
  - `1033`, `2033` (`temporalpole`)

Conclusion:

- FastSurfer does not emit the full raw FreeSurfer `aparc+aseg` ontology.
- It emits a reduced DKT-style ontology with spatial relateralization for selected cortical parcels.

### 5. Verified BRATS label-space compatibility

File:

- `data/MSD/BrainTumor/BRATS_001_labels.nii.gz`

Observed unique labels:

- `0, 1, 2, 3`

Observed header facts:

- `uint8`
- shape `240 x 240 x 155`
- isotropic `1.0 mm`

Compatibility with FastSurfer label IDs:

- Only numeric overlap is accidental (`0` and `2`)
- There is no anatomical semantic compatibility with FastSurfer labels

Conclusion:

- BRATS labels must be treated as a separate ontology.
- Do not coerce BRATS tumor-region labels into FastSurfer anatomical IDs.
- If needed, support only higher-level derived bridges such as `foreground tumor mask`, `tumor within brainmask`, or overlay visualization.

## Architecture Recommendation

### 1. Separate label ontology from mapping policy

Implement three layers:

1. **Typed label IDs**
2. **Static label remap tables**
3. **Spatial relabel transforms** for the cases that are not pure table lookups

This avoids overloading a single remapper with behaviors that belong to different abstraction levels.

### 2. Strongly typed enums

Add explicit enums for each source family instead of raw integers in the public API.

Suggested types:

```cpp
enum class FastSurferLabels : std::uint16_t;
enum class FastSurferAuxiliaryLabel : std::uint16_t;
enum class MindBoggleDkt31Label : std::uint16_t;
enum class MindBoggleManualAsegLabel : std::uint16_t;
enum class BratsTumorLabel : std::uint8_t;
```

Recommended enum contents:

- `FastSurferLabels`: the exhaustive 96-ID post-split label set for `aparc.DKTatlas+aseg.deep.mgz`; this should be the single source of truth for the final segmentation-stage output ontology used by native comparison code
- `FastSurferAuxiliaryLabel`: compatibility-only labels that participate in reduction or spatial repair but are not members of the final 96-ID output ontology, such as `30`, `62`, `72`, `80`, `85`, `251`-`255`, `1000`, `1001`, `1032`, `1033`, `2000`, `2001`, `2032`, and `2033`
- `MindBoggleDkt31Label`: the DKT31 cortical IDs used in `labels.DKT31.manual.nii.gz`
- `MindBoggleManualAsegLabel`: the `manual+aseg` label space, including the auxiliary labels that require reduction
- `BratsTumorLabel`: the dataset-native values `Background = 0`, `Class1 = 1`, `Class2 = 2`, `Class3 = 3`

For BRATS, keep the class names neutral until the source semantics are explicitly documented in-repo. If later confirmed, add named aliases or comments without changing the underlying enum values.

Engineering note:

- Do not derive `FastSurferLabels` directly from `FastSurfer_ColorLUT.tsv` alone.
- `FastSurferLabels` should be materialized once in native code as the authoritative 96-label enum, and tests should verify that this enum matches the real generated `aparc.DKTatlas+aseg.deep.mgz` baseline.
- The comparison baseline against MindBoggle and other anatomical datasets should use `FastSurferLabels`, because that matches the actual segmentation-stage deep output after hemisphere post-processing.

### 3. Explicit mapping result types

Represent mapping outcomes, not just target IDs.

Suggested types:

```cpp
enum class MappingDisposition : std::uint8_t {
    Exact,
    Reduced,
    Dropped,
    SpatiallyResolved,
    Unsupported,
};

template <typename FromLabel, typename ToLabel>
struct LabelMappingRule {
    FromLabel from;
    ToLabel to;
    MappingDisposition disposition;
};
```

This matters for QA and debugging because `30 -> 2` is not the same kind of transformation as `1000 -> nearest parcel`.

### 4. Distinguish static from spatial transforms

Implement two interfaces:

```cpp
template <typename FromLabel, typename ToLabel>
class IStaticLabelMapper {
public:
    virtual std::optional<ToLabel> map(FromLabel label) const = 0;
    virtual ~IStaticLabelMapper() = default;
};

template <typename TImage>
class ISpatialLabelTransform {
public:
    virtual TImage apply(const TImage& image) const = 0;
    virtual ~ISpatialLabelTransform() = default;
};
```

Use static mappers for:

- `29 -> 0`
- `30 -> 2`
- `61 -> 0`
- `62 -> 41`
- `72 -> 24`
- `80 -> 77`
- `85 -> 0`
- `3 -> 0` and `42 -> 0` when coarse cortical markers appear in an aparc-cleanup context

Use a dedicated corpus-callosum resolver for:

- `251`-`255 -> aseg_nocc substitute` when a no-CC reference volume is provided
- `251`-`255 -> Dropped` only in an explicit fallback mode when no parity reference is available

Use a separate **training-parity adapter** for the `map_aparc_aseg2label(..., processing="aseg")` branch:

- `1000 -> 3`
- `2000 -> 42`

Those two remaps are part of FastSurfer's coarse aseg training-target preparation, not part of the final 96-label segmentation-output ontology represented by `FastSurferLabels`.

Use spatial transforms for:

- `1000` / `2000` FastSurfer-parity nearest-parcel fill
- any future re-lateralization that depends on hemisphere geometry rather than direct value lookup

Design consequence:

- The native final-output comparison mapper should remain separate from any training-label indexer that mirrors `map_aparc_aseg2label()`.
- A single flat static table is not sufficient; static canonicalization must be parameterized by processing mode or target space.

### 5. Canonical target spaces

Define these canonical target spaces:

1. **FastSurfer Segmentation Output Label Space**
  - Matches the exhaustive 96-ID post-split `aparc.DKTatlas+aseg.deep` output ontology
  - Use this as the primary comparison baseline against MindBoggle volumetric datasets

2. **FastSurfer Reduced Aseg NoCc Space**
  - Matches `aseg.auto_noCCseg.mgz`
  - Use this for reduced-space compatibility studies and no-CC subcortical evaluation

3. **FastSurfe WithCc Or Surface Space**
  - Matches downstream outputs such as `aparc.DKTatlas+aseg.deep.withCC.mgz` and surface-refined mapped outputs
  - Use this when the benchmark explicitly targets corpus callosum or downstream surface-refined products

4. **Brats Tumor Label Space**
   - Remains disjoint from anatomical labels
   - Use this for tumor overlays, mask operations, and pathology-specific workflows

Do not introduce a fake single enum that mixes anatomical and tumor labels in one integer namespace.

### 6. Spatial label mapping approach, evidence, and evaluation

Spatial label mapping is required only where FastSurfer behavior depends on neighborhood geometry rather than a direct integer remap. For the verified datasets in this task, that means primarily the unknown-cortex labels `1000` and `2000`, and the hemisphere re-lateralization behavior that determines the final 96-ID segmentation-stage output ontology.

Implementation approach:

1. Keep spatial mapping as a separate transform layer that runs after static canonicalization and before comparison.
2. Restrict candidate replacement labels by hemisphere, matching the reference behavior for `1000` and `2000`.
3. Fill unknown cortex voxels from neighboring valid cortical parcels using a FastSurfer-parity implementation rather than a generic nearest-label shortcut.
4. Treat surface-derived relabeling as a separate concern from volumetric canonicalization; do not mix surface truth refinement into the first volumetric mapper.  Mixing them would hide which differences come from ontology reduction versus which come from surface reconstruction (where cortical labels are refined using surface reconstruction and then projected back into volume).

Planned C++ behavior for `1000` and `2000` parity:

1. Identify the unknown-cortex mask for one hemisphere.
2. Build the shell of neighboring candidate parcels using binary dilation around the unknown region.
3. Keep only cortical candidate labels from the same hemisphere.
4. Form per-label score volumes from those neighboring labels.
5. Smooth the competing score volumes with a Gaussian kernel.
6. Assign each unknown voxel to the label with the highest local smoothed support.

This is the correct implementation target because it matches the reference logic more closely than a pure Euclidean nearest-neighbor fill. A naive nearest-label rule is easier to write, but it is not the algorithm FastSurfer uses and would change parcel boundaries at sulcal or gyral interfaces.

Evidence:

- `baseline/FastSurfer/FastSurferCNN/data_loader/data_utils.py::fill_unknown_labels_per_hemi()` performs hemisphere-constrained filling of `1000` and `2000` using neighboring parcels and Gaussian-smoothed competition, not a plain lookup table.
- `baseline/FastSurfer/FastSurferCNN/data_loader/data_utils.py::clean_cortex_labels()` establishes that FastSurfer canonicalization combines static remaps with later spatial repair.
- `baseline/FastSurfer/FastSurferCNN/data_loader/data_utils.py::fuse_cortex_labels()` and `split_cortex_labels()` show that FastSurfer changes cortical IDs in stages, including geometry-dependent re-lateralization after inference.
- `baseline/FastSurfer/FastSurferCNN/data_loader/data_utils.py::map_aparc_aseg2label()` converts processed anatomical IDs into sequential training label indices for coronal/axial and sagittal supervision, so it should be treated as training-target preparation rather than as the final output ontology definition.
- `baseline/FastSurfer/FastSurferCNN/run_prediction.py` applies `map_label2aparc_aseg()` followed by `split_cortex_labels()`, which confirms that the final segmentation-stage output ontology is a post-processed space rather than the raw LUT ID list.
- `baseline/FastSurfer/doc/overview/OUTPUT_FILES.md` documents that the surface module further refines cortical labeling and projects annotations back to volume, which is evidence that cortical label reconciliation is intentionally spatial and stage-dependent rather than a single static LUT operation.
- `baseline/FastSurfer/recon_surf/recon-surf.sh` calls `mri_surf2volseg`, confirming that the final mapped cortical volume is produced through spatial projection from surface annotations in the surface stage.
- Real Colin27 generated artifacts already present in this repository expose the relevant stage boundary for parity testing: `generated/FastSurferCNN/Colin27-1/step_06_map_internal_class_indices_to_freesurfer_label_ids/summary.json` reports `79` unique labels before cortical splitting, while `generated/FastSurferCNN/Colin27-1/step_07_split_cortical_labels_into_left_and_right_dkt_cortical_labels/summary.json` reports `96` unique labels after the split.

Concrete Colin27 parity fixtures available in-repo:

- `data/MindBoggle101_20260501/Colin27-1/.../mri/labels.DKT31.manual+aseg.nii.gz`
  - primary unknown-cortex parity source because it contains the verified `1000` and `2000` placeholders in a real Colin27 anatomical volume
- `data/MindBoggle101_20260501/Colin27-1/.../mri/aseg.auto_noCCseg.mgz`
  - source-side no-CC companion for reduced-space canonicalization when a parity fixture needs corpus-callosum substitution before unknown-cortex fill
- `generated/FastSurferCNN/Colin27-1/step_06_map_internal_class_indices_to_freesurfer_label_ids/pred_classes_freesurfer_ids.mgz`
  - generated pre-split volumetric artifact for verifying the `79 -> 96` stage boundary
- `generated/FastSurferCNN/Colin27-1/step_07_split_cortical_labels_into_left_and_right_dkt_cortical_labels/pred_classes_split_cortex.mgz`
  - generated post-split volumetric artifact for verifying hemisphere-specific cortical relabeling behavior separately from unknown-cortex fill

Scientific rationale:

- Spatial propagation is justified when the source label is explicitly an "unknown cortex" placeholder rather than a true anatomical class. The task is to infer the most plausible neighboring cortical parcel under hemisphere constraints.
- Gaussian-smoothed local competition is more anatomically defensible than arbitrary tie-breaking because it uses surrounding parcel support and reduces voxel-scale noise sensitivity.
- Restricting candidates by hemisphere preserves a known anatomical constraint already encoded by the reference pipeline.

Evaluation plan:

1. Add parity tests that run the C++ spatial transform and the Python reference transform on the same Colin27-derived input volumes.
2. Require exact voxel agreement on the set of voxels originally labeled `1000` or `2000` for the first parity target.
3. Report per-label Dice and confusion counts on the filled voxels so boundary failures are visible even when total voxel agreement is high.
4. Compare the parity implementation against a naive nearest-neighbor baseline and record the disagreement rate; this guards against accidentally shipping an oversimplified algorithm.
5. Keep volumetric evaluation separate from surface-truth evaluation: volumetric parity is measured against `manual+aseg` or `aparc+aseg`-derived volumes, while richer `*.annot` truth is reserved for future surface-aware comparison.

Concrete implementation for section 6 parity and evaluation:

1. Add a small repo-local Python oracle entrypoint under `core/tests/imaging/mri/fastsurfer/python_oracles/` that:
  - adds `baseline/FastSurfer` to `sys.path`
  - loads a label volume from disk with `nibabel`
  - runs `clean_cortex_labels()` and `fill_unknown_labels_per_hemi()` exactly as the reference does for `1000` and `2000`
  - writes the resulting oracle volume plus a JSON summary of the filled-voxel counts and per-label frequencies
2. Add a native parity test executable dedicated to label-mapping parity, separate from the current synthetic rule test, with at least these cases:
  - `fill_unknown_cortex_should_match_python_reference_on_colin27_manual_aseg_fixture`
  - `split_cortex_stage_should_expose_expected_79_to_96_label_transition_on_generated_colin27_artifacts`
3. For `fill_unknown_cortex_should_match_python_reference_on_colin27_manual_aseg_fixture`:
  - load `labels.DKT31.manual+aseg.nii.gz` with the native NIfTI loader
  - record the original unknown mask `label in {1000, 2000}` before any transform
  - run the native mapper in a mode that performs static reductions and unknown-cortex fill but does not yet drop surface-only labels unrelated to this fixture
  - invoke the Python oracle on the same input buffer
  - assert exact voxel agreement restricted to the original unknown mask
  - assert zero remaining `1000` or `2000` voxels in both outputs
  - assert no left-hemisphere unknown voxel resolves to a right-hemisphere cortical label and vice versa
4. For `split_cortex_stage_should_expose_expected_79_to_96_label_transition_on_generated_colin27_artifacts`:
  - load `pred_classes_freesurfer_ids.mgz` and `pred_classes_split_cortex.mgz`
  - assert the pre-split unique label count is `79`
  - assert the post-split unique label count is `96`
  - assert the introduced labels are the expected right-hemisphere cortical IDs rather than arbitrary new labels
  - treat this as a stage-consistency regression until native split-cortex parity is implemented explicitly
5. Add a nearest-neighbor baseline helper for evaluation only, not production mapping:
  - assign each unknown voxel to the label with the smallest Euclidean distance to any same-hemisphere cortical parcel voxel
  - compare its output with the Python oracle and with the native Gaussian-competition transform
  - record disagreement count and disagreement fraction on the original unknown mask
6. Emit a deterministic evaluation artifact, for example `label_mapping_spatial_parity_colin27.json`, with:
  - `unknown_voxel_count_left`
  - `unknown_voxel_count_right`
  - `exact_match_voxel_count`
  - `exact_match_fraction`
  - `native_vs_python_confusion_on_unknown_mask`
  - `per_label_dice_on_unknown_mask`
  - `native_vs_nearest_neighbor_disagreement_count`
  - `native_vs_nearest_neighbor_disagreement_fraction`
7. Keep the acceptance split explicit:
  - unknown-cortex parity is a deterministic mapper-parity target
  - step-06 to step-07 split-cortex validation is a stage-behavior target
  - neither test should be interpreted as surface-truth evaluation against `*.annot`

Acceptance criteria for the spatial transform:

- No filled voxel remains labeled `1000` or `2000` after canonicalization.
- The C++ output matches the Python reference output on the parity fixtures.
- The transform never assigns a left-hemisphere unknown voxel to a right-hemisphere cortical label, or vice versa.

### 7. Corpus callosum mapping resolution, evidence, and evaluation

Corpus callosum labels require a separate resolution policy because FastSurfer does not treat the segmentation-stage reduced aseg and the later with-CC outputs as the same ontology.

Implementation approach:

1. Represent corpus callosum handling as its own resolver rather than as a silent static lookup rule.
2. Default to FastSurfer-parity replacement from a provided no-CC reference volume.
3. Keep a fallback `Drop` mode for reduced-space benchmarking when no no-CC reference is available.
4. Reserve with-CC comparison for a later output space that explicitly includes corpus callosum labels.

Recommended policy surface:

```cpp
enum class CorpusCallosumResolutionMode : std::uint8_t {
  UseReferenceNoCc,
  Drop,
  RequireWithCcTarget,
};
```

Planned C++ behavior:

1. Detect corpus callosum sublabels `251` through `255` in the source volume.
2. If `UseReferenceNoCc` is active and a no-CC companion volume is available, replace those voxels with the companion volume values on a voxel-by-voxel basis (detect 251-255 in the original richer source then substitute them using corresponding voxel values from aseg.auto_noCCseg-style data).
3. If `Drop` is active, map those voxels out of the reduced comparison space and record the disposition explicitly.
4. If the caller requests a with-CC target space, reject reduced-space replacement and require a with-CC FastSurfer target instead.

This design matches FastSurfer more closely than collapsing `251`-`255` directly to background or white matter. It also keeps the reduced-space benchmark and the with-CC benchmark conceptually separate.

Evidence:

- `baseline/FastSurfer/FastSurferCNN/data_loader/data_utils.py` replaces corpus callosum voxels from `aseg_nocc` when that reference is provided before downstream aparc or aseg processing.
- `baseline/FastSurfer/FastSurferCNN/generate_hdf5.py` documents `gt_nocc` as a segmentation without corpus callosum and explicitly recommends `mri/aseg.auto_noCCseg.mgz` for normal FreeSurfer inputs.
- `baseline/FastSurfer/doc/overview/OUTPUT_FILES.md` documents `aseg.auto_noCCseg.mgz` as the segmentation-module output without corpus callosum labels.
- The same output documentation states that later outputs such as `aparc.DKTatlas+aseg.deep.withCC.mgz` include corpus callosum after downstream processing.
- `baseline/FastSurfer/doc/overview/modules/CC.md` and `baseline/FastSurfer/doc/scripts/fastsurfer_cc.rst` document a dedicated corpus callosum pipeline with midsagittal alignment, segmentation, and morphometry, which is evidence that CC is treated as a distinct anatomical processing problem rather than a trivial remap.

Scientific rationale:

- Corpus callosum is a thin midline structure whose segmentation quality depends on midline geometry and dedicated processing; reducing it away in the coarse aseg and adding it back later is consistent with how the reference pipeline organizes uncertainty.
- Replacing `251`-`255` by using a no-CC reference is scientifically preferable to guessing a direct coarse label because it preserves the reduced-space ontology that FastSurfer actually uses during training preparation.
- Evaluating CC fidelity in a no-CC target is undefined by construction, so CC-sensitive benchmarking must be separated from reduced-space benchmarking.

Evaluation plan:

1. Add parity tests that compare the C++ resolver against the Python `aseg_nocc` replacement behavior on fixtures containing `251`-`255`.
2. Assert that `UseReferenceNoCc` eliminates all `251`-`255` labels while preserving the replacement voxels from the no-CC companion volume exactly.
3. In `Drop` mode, report the dropped corpus-callosum voxel count and voxel fraction explicitly.
4. Add a benchmark split:
   - reduced-space evaluation against `aseg.auto_noCCseg`-compatible targets
   - with-CC evaluation against outputs such as `aparc.DKTatlas+aseg.deep.withCC.mgz`
5. Verify that reduced-space metrics never claim corpus callosum agreement when the target ontology excludes CC by design.

Acceptance criteria for corpus callosum resolution:

- Default reduced-space canonicalization uses `UseReferenceNoCc` when the companion volume is available.
- The mapper never silently invents a with-CC label in a no-CC target space.
- Evaluation output always states whether CC was replaced, dropped, or benchmarked in a with-CC target space.

### 8. Missing-label handling approach, evidence, and evaluation

Missing labels are an ontology problem first and an implementation problem second. The mapper must distinguish between labels that are exactly shared, labels that are safely reducible, labels that require spatial resolution, and labels that are unsupported or non-invertible.

Implementation approach:

1. Classify every mapping outcome with an explicit disposition.
2. Support forward canonicalization from richer spaces into the FastSurfer-compatible comparison space.
3. Support only partial inverse mapping from FastSurfer-style outputs back to MindBoggle or FreeSurfer-style spaces.
4. Refuse to invent fine-grained labels when the FastSurfer-style output does not contain enough information to recover them.

Recommended disposition model:

- `Exact`: one-to-one label preserved across both spaces.
- `Reduced`: richer source label is intentionally merged into a coarser FastSurfer-compatible label.
- `SpatiallyResolved`: label identity is recovered through the spatial transform rather than a static LUT.
- `Dropped`: label is intentionally removed from the comparison space.
- `Unsupported`: no valid target exists in the requested ontology.
- `AmbiguousInverse`: multiple finer target labels are possible and the inverse is not identifiable.

Forward policy: MindBoggle or FreeSurfer-style labels to FastSurfer-style output space

- Map shared labels exactly.
- Apply the documented FastSurfer-style final-output reductions from `clean_cortex_labels()`, namely `29 -> 0`, `30 -> 2`, `61 -> 0`, `62 -> 41`, `72 -> 24`, `80 -> 77`, and `85 -> 0`.
- If coarse cortical markers `3` or `42` appear in an aparc-cleanup context, map them to background as in the Python reference.
- Resolve `1000` and `2000` through the FastSurfer-parity spatial transform described above, not through a simplified nearest-neighbor shortcut.
- Expose corpus-callosum handling as an explicit resolver policy instead of hard-coding a silent rule.
- Mark labels absent from the 96-ID segmentation-stage FastSurfer baseline, such as `bankssts`, `frontalpole`, and `temporalpole` in the Colin27 `aparc+aseg` comparison, as unsupported for volumetric FastSurfer-style comparison rather than coercing them into neighboring parcels.

Separate training-space parity policy: `map_aparc_aseg2label(..., processing="aseg")`

- Preserve the reference's coarse cortical-marker preparation by mapping `1000 -> 3` and `2000 -> 42` before LUT indexing.
- Treat this as a distinct adapter for reproducing FastSurfer training labels, not as the canonical final-output comparison mapper.

Inverse policy: FastSurfer-style output space to MindBoggle or FreeSurfer-style spaces

- Map only exact one-to-one labels directly.
- Return `AmbiguousInverse` when a FastSurfer-compatible coarse label could correspond to several finer target labels.
- Never guess missing fine-grained labels such as `bankssts`, `frontalpole`, `temporalpole`, vessel labels, optic chiasm, or corpus-callosum sublabels if they are not recoverable from the source output.

This partial inverse behavior is required because many-to-one reductions are not mathematically invertible without additional information. Any forced inverse would create synthetic agreement and bias downstream metrics.

Evidence:

- `baseline/FastSurfer/FastSurferCNN/data_loader/data_utils.py::clean_cortex_labels()` explicitly performs the static reductions `29 -> 0`, `30 -> 2`, `61 -> 0`, `62 -> 41`, `72 -> 24`, `80 -> 77`, `85 -> 0`, `3 -> 0`, and `42 -> 0`, which confirms that FastSurfer itself collapses parts of the richer FreeSurfer-style ontology before downstream use.
- `baseline/FastSurfer/FastSurferCNN/data_loader/data_utils.py::map_aparc_aseg2label(..., processing="aseg")` separately applies `1000 -> 3` and `2000 -> 42`, which shows that those two remaps belong to coarse aseg training-target generation rather than to final segmentation-output canonicalization.
- `baseline/FastSurfer/FastSurferCNN/data_loader/data_utils.py::fill_unknown_labels_per_hemi()` confirms that `1000` and `2000` are not preserved as final semantic classes; they are placeholders to be spatially resolved.
- The verified Colin27 comparisons show that `labels.DKT31.manual.nii.gz` is compatible with the FastSurfer-style emitted label space, while `manual+aseg` and `aparc+aseg` contain additional labels that are reduced, dropped, or unsupported.
- The Colin27 `label/*.annot` cross-check shows that MindBoggle surface truth contains six parcels absent from the final FastSurfer-style volume inspected here: left and right `bankssts`, `frontalpole`, and `temporalpole`.
- `baseline/FastSurfer/doc/overview/OUTPUT_FILES.md` distinguishes segmentation-stage outputs from later surface-refined outputs, which is evidence that not every label family should be forced into one common volumetric ontology.

Scientific rationale:

- Comparison is only statistically meaningful in a shared or explicitly reduced label space. If one ontology is finer than another, exact voxel agreement for the missing fine labels is undefined rather than merely low.
- A many-to-one forward reduction does not admit a unique inverse. Reporting `AmbiguousInverse` is scientifically more defensible than arbitrarily selecting a fine label.
- Coverage loss must be reported explicitly because a high Dice score in a reduced space can hide the fact that an entire family of labels was excluded before scoring.

Evaluation plan:

1. Compute three reporting modes for Colin27 comparisons:
  - exact shared-label mode,
  - FastSurfer-compatible reduced mode,
  - coverage report for unsupported or dropped labels.
2. Track unique-label coverage ratio, unsupported unique-label count, and unsupported voxel fraction for each source volume.
3. Report per-label Dice only on the shared or canonicalized label set; do not score unsupported labels as false negatives in the reduced-space metric.
4. Emit a separate missing-label report that lists every source label assigned to `Dropped`, `Unsupported`, or `AmbiguousInverse`.
5. Add regression fixtures that assert the known Colin27 unsupported sets:
  - `manual+aseg`: `30`, `62`, `80`, `85`, `251`-`255`, `1000`, `2000` before canonicalization
  - `aparc+aseg`: the same set plus `1001`, `2001`, `1032`, `2032`, `1033`, `2033`

Concrete implementation for section 8 parity and evaluation:

1. Add a native evaluation helper, for example `LabelCoverageReport`, that consumes:
  - source volume labels
  - canonicalized source labels
  - the `VolumeLabelMappingResult::dispositions`
  - target ontology labels from `FastSurferLabels`
2. The helper should compute two layers of metrics:
  - ontology-only metrics that are independent of segmentation quality
  - optional voxelwise overlap metrics that are only valid when the mapped source and target volume are already aligned in the same Colin27 space
3. Implement three Colin27 regression fixtures using the actual data files already present in-repo:
  - `labels.DKT31.manual.nii.gz`
  - `labels.DKT31.manual+aseg.nii.gz`
  - `aparc+aseg.nii.gz`
4. Add these subject-level tests:
  - `evaluate_colin27_manual_dkt31_should_report_exact_shared_label_coverage`
  - `evaluate_colin27_manual_aseg_should_report_expected_raw_missing_labels_and_zero_post_canonicalization_unsupported_labels`
  - `evaluate_colin27_aparc_aseg_should_report_expected_surface_only_unsupported_labels_after_canonicalization`
5. The exact expectations for those tests should be data-driven and explicit:
  - DKT31 manual:
    - raw unique-label coverage remains `63 / 63`
    - raw missing-from-target set is empty
    - every mapped voxel disposition is `Exact`
  - manual+aseg:
    - raw missing-from-target unique-label set is exactly `30`, `62`, `80`, `85`, `251`, `252`, `253`, `254`, `255`, `1000`, `2000`
    - after canonicalization with a supplied no-CC companion volume, the unsupported unique-label set is empty
    - after canonicalization, no auxiliary label remains in the output volume
  - aparc+aseg:
    - raw missing-from-target unique-label set is exactly the `manual+aseg` set plus `1001`, `2001`, `1032`, `2032`, `1033`, `2033`
    - after canonicalization, the remaining unsupported unique-label set is exactly `1001`, `2001`, `1032`, `2032`, `1033`, `2033`
    - all non-surface-only discrepancies must be explained by `Reduced`, `Dropped`, or `SpatiallyResolved` dispositions rather than silently disappearing
6. Emit a deterministic missing-label report artifact, for example `label_mapping_missing_label_report_colin27.json`, with at least:
  - `source_name`
  - `raw_unique_label_count`
  - `canonicalized_unique_label_count`
  - `raw_missing_from_target_unique_labels`
  - `canonicalized_missing_from_target_unique_labels`
  - `unsupported_unique_labels`
  - `unsupported_unique_label_count`
  - `unsupported_voxel_count`
  - `unsupported_voxel_fraction`
  - `dropped_voxel_count`
  - `spatially_resolved_voxel_count`
  - `reduced_voxel_count`
  - `disposition_histogram`
7. Keep statistical reporting honest by separating ontology coverage from segmentation accuracy:
  - use the missing-label report to quantify label-space compatibility without conflating it with model-quality Dice
  - compute per-label Dice only in a second report when comparing spatially aligned Colin27 source and generated target volumes after canonicalization
  - exclude voxels mapped to `Unsupported` from reduced-space Dice denominators and report that exclusion explicitly
8. Implement one optional subject-level overlap report, for example `label_mapping_overlap_report_colin27.json`, that compares canonicalized Colin27 source volumes against `generated/FastSurferCNN/Colin27-1/mri/aparc.DKTatlas+aseg.deep.mgz` and records:
  - `evaluated_label_set`
  - `excluded_unsupported_labels`
  - `per_label_dice`
  - `macro_dice`
  - `micro_dice`
  - `voxel_confusion_summary`
  - `coverage_loss_fraction`
9. Treat `AmbiguousInverse` as future work in code but current work in reporting design:
  - include the field in report schemas now
  - keep the current Colin27 parity suite forward-only until inverse mapping is implemented

Acceptance criteria for missing-label handling:

- The mapper never silently invents a fine-grained label that is absent from the source ontology.
- Every non-exact mapping is represented by an explicit disposition in logs, test assertions, or evaluation output.
- Reduced-space metrics and coverage-loss metrics are reported together so compatibility claims remain interpretable.

## Proposed Native File Layout

Suggested additions under the current native module:

```text
core/include/imaging/mri/fastsurfer/label_types.h
core/include/imaging/mri/fastsurfer/label_mapping.h
core/include/imaging/mri/fastsurfer/label_mapping_result.h
core/include/imaging/mri/fastsurfer/spatial_label_transforms.h
core/src/imaging/mri/fastsurfer/label_mapping.cpp
core/src/imaging/mri/fastsurfer/spatial_label_transforms.cpp
core/tests/imaging/mri/fastsurfer/test_label_mapping.cpp
```

Recommended namespace:

```cpp
namespace OpenHC::imaging::mri::fastsurfer
```

This matches the current native library structure and avoids introducing a second parallel namespace tree.

## Mapping Plan By Dataset

### A. FastSurfer <-> MindBoggle DKT31 manual

Implementation target:

- Identity mapping from MindBoggle DKT31 manual cortical labels into the 96-ID segmentation-stage FastSurfer baseline.

Rules:

- `0 -> 0`
- Every observed DKT31 cortical label in `labels.DKT31.manual.nii.gz` maps directly to the same FastSurfer output ID.

Engineering consequence:

- This can be implemented as a compile-time checked identity set plus validation for unsupported IDs.

### B. FastSurfer <-> MindBoggle manual+aseg

Implementation target:

- Match the 96-ID segmentation-stage FastSurfer output semantics, not raw FreeSurfer raw-label preservation.

Required static rules:

- `29 -> 0`
- `30 -> 2`
- `61 -> 0`
- `62 -> 41`
- `72 -> 24`
- `80 -> 77`
- `85 -> 0`
- if coarse cortical markers appear in the source preprocessing path, `3 -> 0` and `42 -> 0`
- `251`-`255 -> voxelwise replacement from a provided no-CC reference volume`

Required spatial rules:

- `1000 -> FastSurfer-parity nearest left cortical parcel`
- `2000 -> FastSurfer-parity nearest right cortical parcel`

Optional training-parity adapter rules for reproducing `processing="aseg"` targets:

- `1000 -> 3`
- `2000 -> 42`

These two rules should not be folded into the final-output comparison mapper because `3` and `42` are transient coarse cortical markers for training, not members of `FastSurferLabels`.

Policy decision to expose explicitly:

- `CorpusCallosumResolutionMode::UseReferenceNoCc`
- `CorpusCallosumResolutionMode::Drop`
- `CorpusCallosumResolutionMode::RequireWithCcTarget`

`UseReferenceNoCc` is the default for FastSurfer-parity reduced-space mapping. `Drop` is a fallback for evaluation-only workflows when no no-CC reference volume exists. `RequireWithCcTarget` prevents accidental use of a no-CC benchmark when the user actually wants corpus-callosum fidelity.

### C. FastSurfer <-> BRATS

Implementation target:

- No direct anatomical label translation.
- Keep the v1 bridge intentionally small while preserving room for richer pathology-aware overlays later.

Provide only these supported bridges:

- `BratsTumorLabel -> binary tumor foreground`
- `FastSurferLabels -> binary brain foreground`
- optional overlay utilities that preserve both ontologies independently

Extensibility requirement:

- Keep BRATS-specific logic behind a dedicated interface so later versions can add region-wise tumor overlays, spatial overlap summaries, or pathology-anatomy cross-tabulation without changing the core anatomical label enums.

Explicitly reject these operations:

- `BratsTumorLabel -> FastSurferLabels`
- `FastSurferLabels -> BratsTumorLabel`

This should fail at compile time where possible and at runtime otherwise.

## QA And Validation Plan

### 1. Unit tests

Add table-driven tests for:

- exact identity mappings for DKT31 manual labels
- static `manual+aseg` reductions including `29`, `30`, `61`, `62`, `72`, `80`, and `85`
- aparc-cleanup handling of coarse cortical markers `3` and `42`
- optional training-parity adapter handling of `1000 -> 3` and `2000 -> 42`
- corpus callosum replacement from `aseg_nocc`
- corpus callosum fallback drop behavior
- unsupported-label detection
- BRATS cross-ontology rejection

### 2. Subject-level regression tests

Use Colin27 as the primary reference subject.

Acceptance checks:

- DKT31 manual label-set coverage remains `63 / 63`
- `manual+aseg` reduction yields the expected unsupported set only
- corpus callosum replacement from a no-CC companion volume exactly matches the reference voxels on the Colin27-style fixtures
- final-output canonicalization never emits any `FastSurferAuxiliaryLabel` value
- training-parity checks report separately from final-output checks so sequential-label supervision metrics are not mixed with final-output ontology metrics

### 3. Statistical reporting

Track these metrics in tests or evaluation tooling:

- unique-label coverage ratio
- unsupported unique-label count
- unsupported voxel fraction
- per-label Dice on the shared label set when spatially aligned outputs are compared
- mode-stratified reporting for final-output comparison versus training-parity preprocessing, so the denominator and ontology stay consistent across metrics

These metrics are more informative than a single aggregate score because the mapping includes both exact and reduced transformations.

## Implementation Order

1. Add typed enums and conversion helpers.
2. Add static mapping tables for MindBoggle `manual+aseg` auxiliary labels.
3. Add corpus callosum resolver with `UseReferenceNoCc`, `Drop`, and `RequireWithCcTarget` modes.
4. Add DKT31 identity mapper.
5. Add BRATS disjoint label family and minimal binary-bridge utilities behind an extensible interface.
6. Add optional FastSurfer-parity unknown-label fill transform for `1000` and `2000`.
7. Add Colin27 regression tests.

## What Not To Build Yet

- Do not build a generic many-dataset ontology framework.
- Do not mix tumor and anatomical labels in one enum.
- Do not implement spatial parcel filling for labels that are not required by the verified datasets.
- Do not attempt voxel-wise semantic equivalence between BRATS tumor classes and FastSurfer anatomical regions.
