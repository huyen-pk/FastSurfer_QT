# MindBoggler101 Dataset Structure

## Scope

This document describes the archived subject tree under:

`data/Colin27-1/Users/arno.klein/Data/Mindboggle101/subjects/Colin27-1`

It is not a raw Mindboggle download folder in a simplified layout. It is a nearly complete FreeSurfer subject directory, preserved under its original macOS path, with Mindboggle-specific manual labeling artifacts added on top.

Other subject folders follow the same structure.

## Provenance

| Field | Value | Evidence |
| --- | --- | --- |
| Archived subject folder | `Colin27-1` | subject root directory |
| Original FreeSurfer subject name | `colin27_t1_tal_lin` | `scripts/recon-all.cmd`, `scripts/recon-all.env` |
| Original dataset namespace | `Mindboggle101/subjects` | archived path |
| Processing stack | FreeSurfer on macOS | `scripts/recon-all.env` |
| Reconstruction period | April 2012 | `scripts/recon-all.cmd`, `touch/*.touch` timestamps |
| Manual finalization | yes | `FINALIZED2` |

## Root Layout

| Path | Kind | Status | Role | Representative contents |
| --- | --- | --- | --- | --- |
| `Colin27-1/mri` | directory | populated | Voxel-space MRI volumes, segmentations, masks, transforms | `orig.mgz`, `brainmask.mgz`, `aseg.mgz`, `aparc+aseg.mgz` |
| `Colin27-1/surf` | directory | populated | Surface meshes and per-vertex morphometry | `lh.white`, `rh.pial`, `lh.thickness`, `rh.sphere.reg` |
| `Colin27-1/label` | directory | populated | Cortical annotations, region labels, manual Mindboggle labels | `lh.aparc.annot`, `rh.labels.DKT31.manual.annot` |
| `Colin27-1/stats` | directory | populated | Tabular morphometric summaries | `aseg.stats`, `lh.aparc.stats`, `wmparc.stats` |
| `Colin27-1/scripts` | directory | populated | Processing provenance and execution logs | `recon-all.cmd`, `recon-all.log`, `recon-all.env` |
| `Colin27-1/touch` | directory | populated | Stage-completion markers for pipeline steps | `conform.touch`, `segstats.touch`, `wmaparc.touch` |
| `Colin27-1/bem` | directory | empty | Reserved FreeSurfer subdirectory, unused here | none |
| `Colin27-1/src` | directory | empty | Reserved or archival directory, unused here | none |
| `Colin27-1/trash` | directory | empty | Reserved trash/archive directory, unused here | none |
| `Colin27-1/tmp` | directory | near-empty | Temporary workspace; only CVS metadata remains | `CVS/` |
| `Colin27-1/CVS` | directory | populated | Legacy version-control metadata | `Entries`, `Repository`, `Root` |
| `Colin27-1/.cvsignore` | file | populated | CVS ignore rules | text file |
| `Colin27-1/surfer.log` | file | populated | FreeSurfer logging artifact | log text |
| `Colin27-1/FINALIZED2` | file | populated | Manual review/finalization record | text file |
| `Colin27-1/.xdebug_tksurfer` | file | populated | GUI/editor debug artifact | text file |

## Directory Tree Summary

```text
Colin27-1/
  bem/                empty
  label/              annotations, labels, color tables, manual DKT files
  mri/                MGZ/NIfTI volumes, masks, segmentations, transforms
    orig/             original imported volume(s)
    transforms/       Talairach and related transforms
  scripts/            recon-all commands, logs, environment, completion markers
  src/                empty
  stats/              regional summary tables
  surf/               cortical meshes and scalar surface maps
  tmp/                temporary folder, effectively empty except CVS metadata
  touch/              one file per completed FreeSurfer stage
  trash/              empty
```

## File Family Schema

| Family | Location / pattern | Format | Semantic role | Category |
| --- | --- | --- | --- | --- |
| Raw imported MRI | `mri/orig/001.mgz` | MGZ | Original imported subject volume | core data |
| Conformed MRI | `mri/orig.mgz`, `mri/rawavg.mgz`, `mri/T1.mgz` | MGZ | Canonicalized structural MRI volumes used by FreeSurfer | core data |
| Bias-corrected MRI | `mri/nu.mgz`, `mri/nu_noneck.mgz`, `mri/norm.mgz` | MGZ | Intensity-corrected and normalized structural volumes | core data |
| Brain masks | `mri/brainmask.auto.mgz`, `mri/brainmask.mgz`, `mri/brain.mgz`, `mri/brain.finalsurfs.mgz` | MGZ | Skull stripping and downstream surface-prep masks | core data |
| Subcortical segmentations | `mri/aseg.auto_noCCseg.mgz`, `mri/aseg.auto.mgz`, `mri/aseg.mgz` | MGZ | Subcortical and tissue label maps | core data |
| Cortical-plus-subcortical volumes | `mri/aparc+aseg.mgz`, `mri/aparc.a2009s+aseg.mgz`, `mri/aparcNMMjt+aseg.mgz` | MGZ | Volumetric fusion of cortical parcellation with aseg | core data |
| White-matter derivatives | `mri/wm.seg.mgz`, `mri/wm.asegedit.mgz`, `mri/wm.mgz`, `mri/wmparc.mgz`, `mri/filled.mgz` | MGZ | White-matter processing and surface-generation intermediates | core data |
| Surface ribbons | `mri/lh.ribbon.mgz`, `mri/rh.ribbon.mgz`, `mri/ribbon.mgz` | MGZ | Voxelized cortical ribbons derived from surfaces | core data |
| NIfTI exports | `mri/*.nii.gz` | NIfTI | External-tool-friendly exports of MRI and segmentation volumes | derived export |
| Standard-space affine metadata | `mri/*MNI152*.txt`, `mri/*.lta`, `mri/*.reg` | text / transform | Registration metadata for exported NIfTI files | derived export |
| Surface meshes | `surf/lh.white`, `surf/rh.white`, `surf/lh.pial`, `surf/rh.pial`, `surf/lh.orig`, `surf/rh.orig` | FreeSurfer surface binary | Triangle meshes for each hemisphere | core data |
| Inflated and spherical surfaces | `surf/lh.inflated*`, `surf/rh.inflated*`, `surf/lh.sphere*`, `surf/rh.sphere*` | FreeSurfer surface binary | Registration and visualization surfaces | core data |
| Surface scalar maps | `surf/lh.thickness`, `surf/rh.curv`, `surf/lh.sulc`, `surf/rh.area`, `surf/*jacobian_white` | FreeSurfer scalar binary | Per-vertex morphometric measurements | core data |
| Standard annotation sets | `label/*.annot` | FreeSurfer annotation | Hemisphere-wide cortical region assignments | core data |
| Regional label masks | `label/*.label` | FreeSurfer label | Vertex lists for specific regions or cortex mask | core data |
| Color tables | `label/*.ctab` | text | Label-to-color and ID lookup tables | metadata |
| Manual surface labels | `label/*DKT25.manual*`, `label/*DKT31.manual*` | annot / VTK | Mindboggle manual cortical labeling products | benchmark truth |
| Regional statistics | `stats/*.stats` | text table | Aggregated ROI measurements | analytics |
| Talairach transforms | `mri/transforms/talairach.*`, `mri/transforms/cc_up.lta` | LTA / XFM / M3Z / MGZ | Atlas and coordinate transforms | core metadata |
| Recon command log | `scripts/recon-all.cmd` | text | Exact stage-by-stage command history | provenance |
| Recon environment | `scripts/recon-all.env` | text | Runtime environment capture | provenance |
| Recon status and logs | `scripts/recon-all.log`, `scripts/recon-all-status.log`, `scripts/recon-all.done` | text | Pipeline execution records | provenance |
| Stage markers | `touch/*.touch` | text | One file per completed pipeline stage, often containing the executed command | provenance |
| CVS metadata | `**/CVS/*`, `.cvsignore` | text | Legacy version-control residue | archival noise |
| AppleDouble files | `**/._*` | AppleDouble sidecar | macOS resource-fork archive noise | archival noise |

## MRI Subschema

| Subpath | Role | Representative files | Notes |
| --- | --- | --- | --- |
| `mri/` | Main volumetric namespace | `orig.mgz`, `brainmask.mgz`, `aseg.mgz`, `aparc+aseg.mgz` | Primary folder for voxel-space processing outputs |
| `mri/orig/` | Imported source volume(s) | `001.mgz` | Usually sparse; here only one source image is present |
| `mri/transforms/` | Registration artifacts | `talairach.lta`, `talairach.m3z`, `talairach.xfm`, `cc_up.lta` | Includes both affine and nonlinear transforms |

Representative MRI file groups:

| Group | Files |
| --- | --- |
| Structural intensity volumes | `orig.mgz`, `rawavg.mgz`, `T1.mgz`, `nu.mgz`, `norm.mgz` |
| Brain-extraction products | `brainmask.auto.mgz`, `brainmask.mgz`, `brain.mgz`, `brain.finalsurfs.mgz` |
| Segmentation products | `aseg.auto_noCCseg.mgz`, `aseg.auto.mgz`, `aseg.mgz`, `aparc+aseg.mgz`, `wmparc.mgz` |
| White-matter prep volumes | `wm.seg.mgz`, `wm.asegedit.mgz`, `wm.mgz`, `filled.mgz` |
| External NIfTI exports | `t1weighted.nii.gz`, `t1weighted_brain.nii.gz`, `aparc+aseg.nii.gz` |

## Surface Subschema

| Pattern | Meaning |
| --- | --- |
| `lh.*`, `rh.*` | Left and right hemisphere namespace |
| `*.white` | White-matter boundary surface |
| `*.pial` | Pial boundary surface |
| `*.inflated` | Inflated visualization surface |
| `*.sphere`, `*.sphere.reg` | Spherical registration surfaces |
| `*.orig`, `*.orig.nofix` | Original tessellated surfaces, before and before-topology-fix variants |
| `*.smoothwm`, `*.smoothwm.nofix` | Smoothed white-matter surfaces |
| `*.thickness`, `*.curv`, `*.sulc`, `*.area`, `*.volume` | Per-vertex scalar measurements |
| `*.jacobian_white` | Surface deformation metric |

## Label Subschema

| Group | Representative files | Purpose |
| --- | --- | --- |
| Default aparc annotation | `lh.aparc.annot`, `rh.aparc.annot` | Standard cortical annotation set |
| Destrieux-style annotation | `lh.aparc.a2009s.annot`, `rh.aparc.a2009s.annot` | Alternate cortical atlas |
| NMM / JT atlas variants | `lh.aparcNMM.annot`, `rh.aparcNMMjt.annot`, `lh.JTatlas40.annot` | Additional atlas families |
| Brodmann labels | `lh.BA.annot`, `rh.BA.annot`, `lh.BA44.label`, `rh.BA45.label` | BA-based regional labeling |
| Visual and temporal labels | `lh.V1.label`, `rh.V2.label`, `lh.MT.label` | Region-specific label masks |
| Entorhinal ex vivo labels | `lh.entorhinal_exvivo.label`, `rh.entorhinal_exvivo.label` | High-detail entorhinal labels |
| Mindboggle manual DKT25 | `lh.labels.DKT25.manual.annot`, `rh.labels.DKT25.manual.vtk` | Manual benchmark labels |
| Mindboggle manual DKT31 | `lh.labels.DKT31.manual.annot`, `rh.labels.DKT31.manual.vtk` | Manual benchmark labels |
| Archived older manual versions | `*.OLD.vtk` | Earlier manual-label revisions retained for history |

## Mindboggle-Specific Additions

These files distinguish this subject from a plain FreeSurfer reconstruction:

| File family | Why it matters |
| --- | --- |
| `label/lh.labels.DKT25.manual.*`, `label/rh.labels.DKT25.manual.*` | Manual DKT25 cortical labels for benchmark evaluation |
| `label/lh.labels.DKT31.manual.*`, `label/rh.labels.DKT31.manual.*` | Manual DKT31 cortical labels for benchmark evaluation |
| `mri/labels.DKT31.manual.nii.gz` | Volumetric export of manual cortical labeling |
| `mri/labels.DKT31.manual+aseg.nii.gz` | Fused manual cortical labels with aseg in voxel space |
| `*MNI152*` exports | Standard-space exports for cross-tool and cross-subject comparison |
| `FINALIZED2` | Explicit sign-off that editing/final review was completed |

## Statistics Layer

| File | Level | Typical content |
| --- | --- | --- |
| `stats/aseg.stats` | volume / tissue | Subcortical volumes and tissue summaries |
| `stats/wmparc.stats` | white matter | White-matter parcellation summaries |
| `stats/lh.aparc.stats`, `stats/rh.aparc.stats` | cortical atlas | Region-wise cortical surface metrics |
| `stats/lh.aparc.a2009s.stats`, `stats/rh.aparc.a2009s.stats` | alternate atlas | Region-wise metrics for alternate annotation set |
| `stats/lh.BA.stats`, `stats/rh.BA.stats` | Brodmann atlas | BA regional summaries |
| `stats/lh.curv.stats`, `stats/rh.curv.stats` | surface shape | Curvature summaries |

## Provenance Layer

| File or pattern | Role |
| --- | --- |
| `scripts/recon-all.cmd` | Full command history of the original reconstruction |
| `scripts/recon-all.env` | Environment capture including `FREESURFER_HOME` and original `SUBJECTS_DIR` |
| `scripts/recon-all.log` | Main execution log |
| `scripts/recon-all-status.log` | Status-oriented execution log |
| `scripts/recon-all.done` | Completion marker |
| `touch/*.touch` | Stage markers that often duplicate the executed command for a single stage |

## Noise And Cleanup Rules

| Pattern | Meaning | Safe to ignore for analysis |
| --- | --- | --- |
| `**/._*` | AppleDouble sidecar files from macOS archiving | yes |
| `**/CVS/**` | Legacy CVS metadata | yes |
| `**/.cvsignore` | CVS ignore configuration | yes |
| `.xdebug_tksurfer` | Debug or GUI helper artifact | yes |
| `bem/`, `src/`, `trash/` when empty | Reserved directories with no imaging payload here | yes |

## Recommended Interpretation Order

If you want to use this subject for modeling, evaluation, or format conversion, interpret the tree in this order:

1. `mri/` for voxel-space inputs and outputs.
2. `mri/transforms/` for spatial alignment metadata.
3. `surf/` for cortical geometry and morphometry.
4. `label/` for annotation truth and atlas mappings.
5. `stats/` for analysis-ready region summaries.
6. `scripts/` and `touch/` for provenance and reconstruction replay.
7. Ignore `._*`, `CVS/`, `.cvsignore`, and empty admin directories unless you are auditing the archive itself.

## Minimal Core Set

If you need a compact working subset for downstream testing, the highest-value files are:

| Purpose | Files |
| --- | --- |
| Structural MRI | `mri/orig.mgz`, `mri/T1.mgz`, `mri/norm.mgz` |
| Brain mask | `mri/brainmask.mgz` |
| Subcortical segmentation | `mri/aseg.mgz` |
| Full cortical-plus-subcortical segmentation | `mri/aparc+aseg.mgz` |
| White-matter parcellation | `mri/wmparc.mgz` |
| Surface meshes | `surf/lh.white`, `surf/rh.white`, `surf/lh.pial`, `surf/rh.pial` |
| Manual benchmark labels | `label/lh.labels.DKT31.manual.annot`, `label/rh.labels.DKT31.manual.annot` |
| Volumetric manual benchmark export | `mri/labels.DKT31.manual+aseg.nii.gz` |
| Provenance | `scripts/recon-all.cmd`, `scripts/recon-all.env` |


---

## Subsets

The MindBoggle-101 dataset is a meta-dataset aggregating scans from various research institutions, each subjected to a consistent, manually refined labeling protocol.

### 1. OASIS-TRT-20 (Open Access Series of Imaging Studies)
*   **Classification:** Test-Retest (TRT).
*   **Composition:** 20 subjects, two scans per subject.
*   **Utility:** Primary target for **Intraclass Correlation Coefficient (ICC)** metrics. Differences in C++ output between the two scans of the same subject indicate algorithmic noise rather than biological variation.

### 2. NKI-TRT-20 (Nathan Kline Institute)
*   **Classification:** Test-Retest (TRT).
*   **Composition:** 20 subjects.
*   **Utility:** Validation of robustness across scanner parameters. Discrepancies between OASIS-TRT and NKI-TRT performance may indicate sensitivity to specific noise profiles or signal-to-noise ratios (SNR).

### 3. NKI-RS-22 (NKI Rockland Sample)
*   **Classification:** Community-based sample.
*   **Composition:** 22 subjects.
*   **Utility:** Validation of anatomical generalization. This group provides a broader age range and higher natural anatomical variation compared to screened TRT cohorts.

### 4. MMRR-21 (Multi-Modal MRI Reproducibility)
*   **Classification:** High-fidelity reproducibility set (Kirby 21).
*   **Composition:** 21 healthy volunteers (Johns Hopkins).
*   **Utility:** Benchmark for high-fidelity reproducibility. This dataset is ideal for Python-to-C++ exact voxel parity checks due to the high image quality.

### 5. HLN-12 (Hardy-Levy-Noyes)
*   **Classification:** Diverse anatomical set.
*   **Composition:** 12 subjects.
*   **Utility:** Evaluation of legacy data handling. These older scans provide additional anatomical diversity to ensure the pipeline handles varying image acquisition standards.

---

## Evaluation Strategy

Rigorous validation requires stratifying results by group to isolate specific implementation risks:

| Group | Evaluation Objective | Rationale |
| :--- | :--- | :--- |
| **OASIS-TRT** | **Reliability** | Calculation of Test-Retest ICC to measure measurement stability. |
| **MMRR-21** | **Parity** | Python-to-C++ exact voxel matching using high-SNR inputs. |
| **NKI-RS** | **Generalization** | Verification of label splitting and midline logic across diverse morphologies. |

### Technical Considerations for C++ Implementation
These subsets were acquired using different hardware (Siemens, Philips, GE), resulting in subtle variations in **NIfTI/MGZ header** formatting.

*   **Implementation Risk:** The `conform` stage (LIA transform) must correctly parse the affine matrix across all variations.
*   **Diagnostic Indicator:** A high Dice score on OASIS-TRT paired with a significantly lower score on HLN-12 indicates a failure to correctly interpret affine headers in older or non-standard file formats.
