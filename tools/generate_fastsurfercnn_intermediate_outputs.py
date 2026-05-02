#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import re
import sys
from pathlib import Path
from typing import Any

import numpy as np
import torch


REPO_ROOT = Path(__file__).resolve().parents[1]
FASTSURFER_ROOT = REPO_ROOT / "baseline" / "FastSurfer"
if str(FASTSURFER_ROOT) not in sys.path:
    sys.path.insert(0, str(FASTSURFER_ROOT))

from FastSurferCNN.data_loader import data_utils as du
from FastSurferCNN.data_loader.conform import Reorientation, conform, is_conform
from FastSurferCNN.quick_qc import check_volume
from FastSurferCNN.reduce_to_aseg import create_mask, flip_wm_islands, reduce_to_aseg
from FastSurferCNN.run_prediction import RunModelOnData
from FastSurferCNN.utils.checkpoint import get_checkpoints, get_config_file, load_checkpoint_config_defaults
from FastSurferCNN.utils.logging import setup_logging
from FastSurferCNN.utils.parser_defaults import FASTSURFER_ROOT as BASELINE_FASTSURFER_ROOT


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate per-step FastSurferCNN intermediate outputs for one input volume.",
    )
    parser.add_argument(
        "--input",
        type=Path,
        required=True,
        help="Input T1 volume to process.",
    )
    parser.add_argument(
        "--mapping-csv",
        type=Path,
        default=REPO_ROOT / "fastsurfer_freesurfer_segmentation_step_mapping.csv",
        help="CSV describing FastSurferCNN steps.",
    )
    parser.add_argument(
        "--output-root",
        type=Path,
        default=REPO_ROOT / "generated" / "FastSurferCNN",
        help="Root directory for generated artifacts.",
    )
    parser.add_argument(
        "--device",
        default="auto",
        help="FastSurfer model device.",
    )
    parser.add_argument(
        "--viewagg-device",
        default="cpu",
        help="Device used for view aggregation.",
    )
    parser.add_argument(
        "--threads",
        type=int,
        default=1,
        help="Torch thread count.",
    )
    parser.add_argument(
        "--batch-size",
        type=int,
        default=1,
        help="Inference batch size.",
    )
    parser.add_argument(
        "--vox-size",
        default="min",
        help="FastSurfer conformation vox_size option.",
    )
    parser.add_argument(
        "--orientation",
        default="lia",
        help="FastSurfer conformation orientation option.",
    )
    parser.add_argument(
        "--image-size",
        type=int,
        default=256,
        help="FastSurfer conformation image size option.",
    )
    parser.add_argument(
        "--no-image-size",
        action="store_const",
        const=None,
        dest="image_size",
        help="Disable FastSurfer image_size behavior.",
    )
    parser.add_argument(
        "--conform-to-1mm-threshold",
        type=float,
        default=0.95,
        help="FastSurfer conform threshold.",
    )
    parser.add_argument(
        "--save-full-logits",
        action="store_true",
        help="Persist the full 4D aggregated logit tensor after each plane and final aggregation.",
    )
    parser.add_argument(
        "--log-file",
        type=Path,
        default=None,
        help="Optional log file path.",
    )
    return parser.parse_args()


def slugify(name: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_")


def load_step_rows(mapping_csv: Path) -> list[dict[str, str]]:
    with mapping_csv.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    rows.sort(key=lambda row: int(row["fastsurfer_step_id"]))
    return rows


def create_step_directories(subject_root: Path, step_rows: list[dict[str, str]]) -> dict[int, Path]:
    step_dirs: dict[int, Path] = {}
    for row in step_rows:
        step_id = int(row["fastsurfer_step_id"])
        step_dir = subject_root / f"step_{step_id:02d}_{slugify(row['fastsurfer_step'])}"
        step_dir.mkdir(parents=True, exist_ok=True)
        step_dirs[step_id] = step_dir
        write_json(step_dir / "step_metadata.json", row)
    return step_dirs


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def format_repo_path(path: Path) -> str:
    resolved = (BASELINE_FASTSURFER_ROOT / path).resolve() if not path.is_absolute() else path.resolve()
    return str(resolved.relative_to(REPO_ROOT) if resolved.is_relative_to(REPO_ROOT) else resolved)


def save_volume(path: Path, data: np.ndarray | torch.Tensor, reference_img, dtype: Any | None = None) -> None:
    np_data = data if isinstance(data, np.ndarray) else data.detach().cpu().numpy()
    header = reference_img.header.copy()
    if dtype is not None:
        header.set_data_dtype(dtype)
    du.save_image(header, reference_img.affine, np_data, path, dtype=dtype)


def save_array(path: Path, data: np.ndarray | torch.Tensor) -> None:
    np_data = data if isinstance(data, np.ndarray) else data.detach().cpu().numpy()
    path.parent.mkdir(parents=True, exist_ok=True)
    np.save(path, np_data)


def save_plane_snapshot(step_dir: Path, plane: str, pred_prob: torch.Tensor, save_full_logits: bool) -> None:
    argmax_path = step_dir / f"argmax_lia_after_{plane}.npy"
    maxlogit_path = step_dir / f"max_logit_lia_after_{plane}.npy"
    argmax_lia = torch.argmax(pred_prob, dim=3).to(torch.int16).cpu().numpy()
    max_logit_lia = torch.max(pred_prob, dim=3).values.cpu().numpy().astype(np.float16)
    np.save(argmax_path, argmax_lia)
    np.save(maxlogit_path, max_logit_lia)

    summary = {
        "plane": plane,
        "pred_prob_shape": list(pred_prob.shape),
        "pred_prob_dtype": str(pred_prob.dtype),
        "argmax_shape": list(argmax_lia.shape),
        "max_logit_min": float(max_logit_lia.min()),
        "max_logit_max": float(max_logit_lia.max()),
        "max_logit_mean": float(max_logit_lia.mean()),
    }
    write_json(step_dir / f"summary_after_{plane}.json", summary)

    if save_full_logits:
        torch.save(pred_prob.detach().cpu(), step_dir / f"pred_prob_after_{plane}.pt")


def split_cortex_labels_with_fallback(pred_fs_labels: torch.Tensor) -> tuple[np.ndarray, dict[str, Any]]:
    pred_fs_labels_np = pred_fs_labels.cpu().numpy()
    has_lh_wm = bool(np.any(pred_fs_labels_np == 2))
    has_rh_wm = bool(np.any(pred_fs_labels_np == 41))

    if has_lh_wm and has_rh_wm:
        return du.split_cortex_labels(pred_fs_labels_np), {
            "split_mode": "upstream",
            "has_lh_white_matter": has_lh_wm,
            "has_rh_white_matter": has_rh_wm,
        }

    return pred_fs_labels_np.copy(), {
        "split_mode": "fallback_passthrough",
        "has_lh_white_matter": has_lh_wm,
        "has_rh_white_matter": has_rh_wm,
        "warning": "Skipped split_cortex_labels because one or both hemisphere white matter seed labels were absent.",
    }


def resolve_defaults() -> dict[str, Any]:
    config_file = get_config_file("FastSurferCNN")
    checkpoints = {
        key: (BASELINE_FASTSURFER_ROOT / value).resolve()
        for key, value in load_checkpoint_config_defaults("checkpoint", config_file).items()
    }
    configs = {
        key: (BASELINE_FASTSURFER_ROOT / value).resolve()
        for key, value in load_checkpoint_config_defaults("config", config_file).items()
    }
    return {
        "config_file": config_file,
        "urls": load_checkpoint_config_defaults("url", config_file),
        "checkpoints": checkpoints,
        "configs": configs,
        "lut": BASELINE_FASTSURFER_ROOT / "FastSurferCNN" / "config" / "FastSurfer_ColorLUT.tsv",
    }


def main() -> int:
    args = parse_args()
    setup_logging(args.log_file)

    input_path = args.input.resolve()
    if not input_path.exists():
        raise FileNotFoundError(f"Input file not found: {input_path}")

    step_rows = load_step_rows(args.mapping_csv)
    subject_name = input_path.stem
    subject_root = args.output_root / subject_name
    subject_root.mkdir(parents=True, exist_ok=True)
    step_dirs = create_step_directories(subject_root, step_rows)

    defaults = resolve_defaults()
    checkpoints = defaults["checkpoints"]
    configs = defaults["configs"]
    get_checkpoints(
        checkpoints["axial"],
        checkpoints["coronal"],
        checkpoints["sagittal"],
        urls=defaults["urls"],
    )

    step1_dir = step_dirs[1]
    write_json(
        step1_dir / "resolved_resources.json",
        {
            "input": str(input_path.relative_to(REPO_ROOT) if input_path.is_relative_to(REPO_ROOT) else input_path),
            "lut": format_repo_path(defaults["lut"]),
            "config_file": format_repo_path(defaults["config_file"]),
            "configs": {k: format_repo_path(v) for k, v in configs.items()},
            "checkpoints": {k: format_repo_path(v) for k, v in checkpoints.items()},
            "download_urls": defaults["urls"],
        },
    )

    runner = RunModelOnData(
        lut=defaults["lut"],
        ckpt_ax=checkpoints["axial"],
        ckpt_sag=checkpoints["sagittal"],
        ckpt_cor=checkpoints["coronal"],
        cfg_ax=configs["axial"],
        cfg_sag=configs["sagittal"],
        cfg_cor=configs["coronal"],
        device=args.device,
        viewagg_device=args.viewagg_device,
        threads=args.threads,
        batch_size=args.batch_size,
        vox_size=args.vox_size,
        orientation=args.orientation,
        image_size=args.image_size,
        async_io=False,
        conform_to_1mm_threshold=args.conform_to_1mm_threshold,
    )

    raw_img, raw_data = du.load_image(input_path, "orig image")

    step2_dir = step_dirs[2]
    save_volume(step2_dir / "input_copy.mgz", raw_data, raw_img, dtype=raw_data.dtype)
    conformed_img = raw_img
    conformed_data = raw_data
    conform_kwargs = {
        "threshold_1mm": runner.conform_to_1mm_threshold,
        "vox_size": runner.vox_size,
        "orientation": runner.orientation,
        "img_size": runner.image_size,
    }
    if not is_conform(conformed_img, **conform_kwargs, verbose=True):
        conformed_img = conform(conformed_img, **conform_kwargs)
        conformed_data = np.asanyarray(conformed_img.dataobj)
    save_volume(step2_dir / "conformed_orig.mgz", conformed_data, conformed_img, dtype=np.uint8)
    write_json(
        step2_dir / "summary.json",
        {
            "raw_shape": list(raw_data.shape),
            "conformed_shape": list(conformed_data.shape),
            "raw_zooms": [float(value) for value in raw_img.header.get_zooms()[:3]],
            "conformed_zooms": [float(value) for value in conformed_img.header.get_zooms()[:3]],
        },
    )

    step3_dir = step_dirs[3]
    native_zoom = np.asarray(conformed_img.header.get_zooms()[:3], dtype=float)
    native_to_lia = Reorientation.from_target_orientation(
        conformed_img.affine,
        "soft LIA",
        conformed_data.shape,
        native_zoom,
    )
    orig_in_lia = native_to_lia(conformed_data, order=1)
    zoom_in_lia = native_to_lia.reorder_axes(native_zoom)
    save_array(step3_dir / "orig_in_lia.npy", orig_in_lia)
    write_json(
        step3_dir / "summary.json",
        {
            "orig_in_lia_shape": list(orig_in_lia.shape),
            "native_zoom": [float(value) for value in native_zoom.tolist()],
            "zoom_in_lia": [float(value) for value in np.asarray(zoom_in_lia).tolist()],
        },
    )

    step4_dir = step_dirs[4]
    pred_prob = torch.zeros(
        orig_in_lia.shape + (runner.get_num_classes(),),
        device=runner.viewagg_device,
        dtype=torch.float16,
        requires_grad=False,
    )
    for plane, model in runner.models.items():
        runner.set_model(plane)
        pred_prob = model.run(pred_prob, input_path.name, orig_in_lia, zoom_in_lia, out=pred_prob)
        save_plane_snapshot(step4_dir, plane, pred_prob, args.save_full_logits)
    write_json(
        step4_dir / "summary.json",
        {
            "pred_prob_shape": list(pred_prob.shape),
            "pred_prob_dtype": str(pred_prob.dtype),
            "save_full_logits": args.save_full_logits,
        },
    )
    if args.save_full_logits:
        torch.save(pred_prob.detach().cpu(), step4_dir / "pred_prob_final.pt")

    step5_dir = step_dirs[5]
    pred_classes_lia = torch.argmax(pred_prob, dim=3)
    save_array(step5_dir / "pred_classes_lia.npy", pred_classes_lia.to(torch.int16))
    pred_classes_native = native_to_lia.inverse(pred_classes_lia, order=0)
    save_volume(step5_dir / "pred_classes_native_indices.mgz", pred_classes_native, conformed_img, dtype=np.int16)
    write_json(
        step5_dir / "summary.json",
        {
            "pred_classes_native_shape": list(pred_classes_native.shape),
            "pred_classes_native_dtype": str(pred_classes_native.dtype),
        },
    )

    step6_dir = step_dirs[6]
    pred_fs_labels = du.map_label2aparc_aseg(pred_classes_native, runner.labels)
    save_volume(step6_dir / "pred_classes_freesurfer_ids.mgz", pred_fs_labels, conformed_img, dtype=np.int16)
    write_json(
        step6_dir / "summary.json",
        {
            "num_unique_labels": int(torch.unique(pred_fs_labels).numel()),
        },
    )

    step7_dir = step_dirs[7]
    pred_split, split_summary = split_cortex_labels_with_fallback(pred_fs_labels)
    save_volume(step7_dir / "pred_classes_split_cortex.mgz", pred_split, conformed_img, dtype=np.int16)
    split_summary["num_unique_labels"] = int(np.unique(pred_split).size)
    write_json(step7_dir / "summary.json", split_summary)

    step8_dir = step_dirs[8]
    final_seg_path = step8_dir / "aparc.DKTatlas+aseg.deep.mgz"
    save_volume(final_seg_path, pred_split, conformed_img, dtype=np.int16)

    step9_dir = step_dirs[9]
    brainmask = create_mask(pred_split, 5, 4)
    brainmask_path = step9_dir / "brainmask.mgz"
    save_volume(brainmask_path, brainmask, conformed_img, dtype=np.uint8)

    step10_dir = step_dirs[10]
    aseg_reduced = reduce_to_aseg(pred_split)
    save_volume(step10_dir / "aseg_reduced_unmasked.mgz", aseg_reduced, conformed_img, dtype=np.uint8)
    aseg_masked = aseg_reduced.copy()
    aseg_masked[brainmask == 0] = 0
    save_volume(step10_dir / "aseg_reduced_masked.mgz", aseg_masked, conformed_img, dtype=np.uint8)

    step11_dir = step_dirs[11]
    aseg_flipped = flip_wm_islands(aseg_masked)
    aseg_flipped_path = step11_dir / "aseg.auto_noCCseg.mgz"
    save_volume(aseg_flipped_path, aseg_flipped, conformed_img, dtype=np.uint8)

    step12_dir = step_dirs[12]
    derivative_root = subject_root / "mri"
    derivative_root.mkdir(parents=True, exist_ok=True)
    final_derivative_seg = derivative_root / "aparc.DKTatlas+aseg.deep.mgz"
    final_derivative_brainmask = derivative_root / "brainmask.mgz"
    final_derivative_aseg = derivative_root / "aseg.auto_noCCseg.mgz"
    save_volume(final_derivative_seg, pred_split, conformed_img, dtype=np.int16)
    save_volume(final_derivative_brainmask, brainmask, conformed_img, dtype=np.uint8)
    save_volume(final_derivative_aseg, aseg_flipped, conformed_img, dtype=np.uint8)
    write_json(
        step12_dir / "saved_derivatives.json",
        {
            "aparc_dktatlas_aseg_deep": str(final_derivative_seg.relative_to(subject_root)),
            "brainmask": str(final_derivative_brainmask.relative_to(subject_root)),
            "aseg_auto_noCCseg": str(final_derivative_aseg.relative_to(subject_root)),
        },
    )

    step13_dir = step_dirs[13]
    voxel_volume = float(np.prod(conformed_img.header.get_zooms()[:3]))
    qc_passed = bool(check_volume(pred_split, voxel_volume))
    write_json(
        step13_dir / "qc_report.json",
        {
            "qc_passed": qc_passed,
            "voxel_volume_mm3": voxel_volume,
            "segmentation_voxel_count": int(np.count_nonzero(pred_split)),
        },
    )

    root_summary = {
        "subject_name": subject_name,
        "input": str(input_path.relative_to(REPO_ROOT) if input_path.is_relative_to(REPO_ROOT) else input_path),
        "output_root": str(subject_root.relative_to(REPO_ROOT) if subject_root.is_relative_to(REPO_ROOT) else subject_root),
        "final_outputs": {
            "segmentation": str(final_derivative_seg.relative_to(subject_root)),
            "brainmask": str(final_derivative_brainmask.relative_to(subject_root)),
            "aseg": str(final_derivative_aseg.relative_to(subject_root)),
        },
        "qc_passed": qc_passed,
    }
    write_json(subject_root / "run_summary.json", root_summary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())