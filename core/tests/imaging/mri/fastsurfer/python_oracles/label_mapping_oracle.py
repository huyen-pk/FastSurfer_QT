from __future__ import annotations

import json
import sys
from pathlib import Path

import nibabel as nib
import numpy as np


def save_volume(reference_image: nib.spatialimages.SpatialImage, data: np.ndarray, output_path: Path) -> None:
    if output_path.suffix == ".mgz":
        image = nib.MGHImage(data, reference_image.affine, reference_image.header.copy())
    else:
        image = nib.Nifti1Image(data, reference_image.affine, reference_image.header.copy())
    image.set_data_dtype(np.int16)
    nib.save(image, str(output_path))


def main() -> int:
    repo_root = Path(sys.argv[1])
    input_path = Path(sys.argv[2])
    output_path = Path(sys.argv[3])
    report_path = Path(sys.argv[4])
    no_cc_path = Path(sys.argv[5]) if len(sys.argv) > 5 else None

    sys.path.insert(0, str(repo_root / "baseline" / "FastSurfer"))
    from FastSurferCNN.data_loader import data_utils as du

    image = nib.load(str(input_path))
    data = np.asarray(image.dataobj).astype(np.int32)

    if no_cc_path is not None:
        no_cc = np.asarray(nib.load(str(no_cc_path)).dataobj).astype(np.int32)
        cc_mask = (data >= 251) & (data <= 255)
        data[cc_mask] = no_cc[cc_mask]

    data = du.clean_cortex_labels(data.copy())
    if np.any(data == 1000):
        data = du.fill_unknown_labels_per_hemi(data, 1000, 2000)
    if np.any(data == 2000):
        data = du.fill_unknown_labels_per_hemi(data, 2000, 3000)

    save_volume(image, data.astype(np.int16), output_path)

    unknown_left_count = int(np.count_nonzero(data == 1000))
    unknown_right_count = int(np.count_nonzero(data == 2000))
    unique_labels, counts = np.unique(data, return_counts=True)
    report = {
        "remaining_unknown_left_count": unknown_left_count,
        "remaining_unknown_right_count": unknown_right_count,
        "unique_label_count": int(unique_labels.size),
        "label_frequencies": {str(int(label)): int(count) for label, count in zip(unique_labels, counts)},
    }

    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())