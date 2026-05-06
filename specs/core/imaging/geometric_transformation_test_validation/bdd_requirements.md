# BDD Requirements: Oblique Geometry Conform Validation

## Scenario 1: Native orientation preserves oblique geometry

Given the 3D oblique NIfTI fixture `data/parrec_oblique/NIFTI/3D_T1W_trans_35_25_15_SENSE_26_1.nii`
When `ConformStepService::run()` is invoked with `orientation="native"`
Then the copy-orig MGZ must match the direct NIfTI-to-MGZ conversion payload and affine
And the conformed MGZ must remain single-frame `uint8`
And the conformed MGZ must preserve the source oblique direction cosines
And the conformed MGZ affine-equivalent geometry must match an independent ITK-backed target-geometry oracle
And the source and output world-space centers must match within numeric tolerance
And deterministic ITK probe sampling must satisfy hard max and 95th-percentile interpolation-error thresholds

## Scenario 2: LIA orientation applies the target orientation policy

Given the 3D oblique NIfTI fixture `data/parrec_oblique/NIFTI/3D_T1W_trans_35_25_15_SENSE_26_1.nii`
When `ConformStepService::run()` is invoked with `orientation="lia"`
Then the copy-orig MGZ must match the direct NIfTI-to-MGZ conversion payload and affine
And the conformed MGZ must use LIA direction cosines with isotropic target spacing
And the conformed MGZ affine-equivalent geometry must match an independent ITK-backed target-geometry oracle
And the conformed direction matrix must be orthonormal (columns are both orthogonal (mutually perpendicular) and normalized (length = 1)), and with the expected handedness sign
And deterministic ITK probe sampling must satisfy hard max and 95th-percentile interpolation-error thresholds

## Scenario 3: Converter regression remains ingress-scoped

Given the existing NIfTI converter regression suite
When the oblique conform verification is strengthened
Then the NIfTI converter regression must continue to pass unchanged as an ingress and MGZ-serialization check
And the primary oblique conform verification must not depend on Python parity helpers