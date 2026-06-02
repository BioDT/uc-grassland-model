#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd


DESKTOP_DIR = Path.home() / "Desktop"
DEFAULT_INPUT_FILE = DESKTOP_DIR / "bdj-07-e36387-s001"


def _safe_fragment(text: str) -> str:
    safe = "".join(char if char.isalnum() or char in ("-", "_") else "_" for char in text)
    return safe.strip("_") or "column"


def _load_table(input_path: Path) -> pd.DataFrame:
    loaders = [
        ("excel", lambda p: pd.read_excel(p)),
        ("csv-auto", lambda p: pd.read_csv(p, sep=None, engine="python")),
        ("csv-semicolon", lambda p: pd.read_csv(p, sep=";")),
        ("csv-tab", lambda p: pd.read_csv(p, sep="\t")),
    ]

    errors: list[str] = []
    for name, loader in loaders:
        try:
            df = loader(input_path)
            if not df.empty:
                return df
        except Exception as exc:  # pragma: no cover - fallback chain
            errors.append(f"{name}: {exc}")

    error_details = " | ".join(errors) if errors else "unknown format"
    raise ValueError(f"Could not parse '{input_path}'. {error_details}")


def _save_summary(df: pd.DataFrame, output_dir: Path, stem: str) -> Path:
    summary_path = output_dir / f"{stem}_summary_statistics.csv"
    summary = df.describe(include="all").transpose()
    summary["missing_values"] = df.isna().sum()
    summary.to_csv(summary_path, index=True)
    return summary_path


def _plot_histograms(df: pd.DataFrame, output_dir: Path, stem: str) -> Path | None:
    numeric = df.select_dtypes(include="number")
    if numeric.empty:
        return None

    hist_path = output_dir / f"{stem}_histograms.jpeg"
    numeric.hist(figsize=(12, 8), bins=20)
    plt.tight_layout()
    plt.savefig(hist_path, format="jpeg", dpi=200)
    plt.close("all")
    return hist_path


def _plot_boxplot(df: pd.DataFrame, output_dir: Path, stem: str) -> Path | None:
    numeric = df.select_dtypes(include="number")
    if numeric.empty:
        return None

    boxplot_path = output_dir / f"{stem}_boxplot.jpeg"
    plt.figure(figsize=(12, 6))
    numeric.plot(kind="box", rot=45)
    plt.title("Numeric columns boxplot")
    plt.tight_layout()
    plt.savefig(boxplot_path, format="jpeg", dpi=200)
    plt.close("all")
    return boxplot_path


def _plot_correlation(df: pd.DataFrame, output_dir: Path, stem: str) -> Path | None:
    numeric = df.select_dtypes(include="number")
    if numeric.shape[1] < 2:
        return None

    corr = numeric.corr(numeric_only=True)
    corr_path = output_dir / f"{stem}_correlation_heatmap.jpeg"
    plt.figure(figsize=(8, 6))
    plt.imshow(corr, cmap="coolwarm", interpolation="nearest", aspect="auto", vmin=-1, vmax=1)
    plt.colorbar(label="Correlation")
    plt.xticks(range(len(corr.columns)), corr.columns, rotation=45, ha="right")
    plt.yticks(range(len(corr.index)), corr.index)
    plt.title("Correlation heatmap")
    plt.tight_layout()
    plt.savefig(corr_path, format="jpeg", dpi=200)
    plt.close("all")
    return corr_path


def _plot_first_categorical(df: pd.DataFrame, output_dir: Path, stem: str) -> Path | None:
    categorical = df.select_dtypes(exclude="number")
    if categorical.empty:
        return None

    column = categorical.columns[0]
    counts = df[column].astype(str).value_counts(dropna=False).head(15)
    if counts.empty:
        return None

    categorical_path = output_dir / f"{stem}_{_safe_fragment(str(column))}_counts.jpeg"
    plt.figure(figsize=(12, 6))
    counts.plot(kind="bar")
    plt.title(f"Top values for {column}")
    plt.ylabel("Count")
    plt.tight_layout()
    plt.savefig(categorical_path, format="jpeg", dpi=200)
    plt.close("all")
    return categorical_path


def analyze_file(input_file: Path, output_dir: Path) -> list[Path]:
    input_file = input_file.expanduser().resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    df = _load_table(input_file)
    stem = input_file.stem

    created_files: list[Path] = [
        _save_summary(df, output_dir, stem),
    ]

    for plot_path in (
        _plot_histograms(df, output_dir, stem),
        _plot_boxplot(df, output_dir, stem),
        _plot_correlation(df, output_dir, stem),
        _plot_first_categorical(df, output_dir, stem),
    ):
        if plot_path is not None:
            created_files.append(plot_path)

    return created_files


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Read a data file, create summary statistics, and save JPEG plots."
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=DEFAULT_INPUT_FILE,
        help=f"Input file path (default: {DEFAULT_INPUT_FILE})",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DESKTOP_DIR,
        help=f"Directory where output files are written (default: {DESKTOP_DIR})",
    )
    args = parser.parse_args()

    created = analyze_file(args.input, args.output_dir.expanduser())
    print(f"Read: {args.input}")
    print("Created files:")
    for file_path in created:
        print(f" - {file_path}")


if __name__ == "__main__":
    main()
