#!/usr/bin/env python3
from __future__ import annotations

import csv
from collections import defaultdict
from pathlib import Path
from typing import Callable

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data" / "averaged_results.csv"
OUT_DIR = ROOT / "figures"

ARRAY_LABELS = {
    "random": "случайный массив",
    "reverse_sorted": "обратно отсортированный массив",
    "nearly_sorted": "почти отсортированный массив",
}

ALGORITHM_LABELS = {
    "QuickSort": "Обычный Quick Sort",
    "MergeSort": "Обычный Merge Sort",
    "StringQuickSort": "String Quick Sort",
    "StringMergeSort": "String Merge Sort",
    "MSDRadixSort": "MSD Radix Sort",
    "MSDRadixQuickSort": "MSD Radix + Quick",
}

COLORS = {
    "QuickSort": (31, 119, 180),
    "MergeSort": (255, 127, 14),
    "StringQuickSort": (44, 160, 44),
    "StringMergeSort": (214, 39, 40),
    "MSDRadixSort": (148, 103, 189),
    "MSDRadixQuickSort": (140, 86, 75),
}


def find_font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    candidates = [
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf" if bold else "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial Bold.ttf" if bold else "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/DejaVu Sans Bold.ttf" if bold else "/System/Library/Fonts/Supplemental/DejaVu Sans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ]
    for path in candidates:
        if Path(path).exists():
            return ImageFont.truetype(path, size=size)
    return ImageFont.load_default()


def text_size(draw: ImageDraw.ImageDraw, text: str, font: ImageFont.ImageFont) -> tuple[int, int]:
    box = draw.textbbox((0, 0), text, font=font)
    return box[2] - box[0], box[3] - box[1]


def load_rows() -> list[dict[str, str]]:
    with DATA.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def nice_label(value: float) -> str:
    if value >= 1000:
        return f"{value:,.0f}".replace(",", " ")
    if value >= 100:
        return f"{value:.0f}"
    if value >= 10:
        return f"{value:.1f}"
    return f"{value:.2f}"


def plot_metric(
    rows: list[dict[str, str]],
    array_type: str,
    metric: str,
    title: str,
    y_label: str,
    transform: Callable[[float], float],
    output: Path,
) -> None:
    width, height = 1280, 820
    margin_left, margin_right = 110, 310
    margin_top, margin_bottom = 90, 100
    plot_left, plot_top = margin_left, margin_top
    plot_right, plot_bottom = width - margin_right, height - margin_bottom
    plot_width, plot_height = plot_right - plot_left, plot_bottom - plot_top

    image = Image.new("RGB", (width, height), "white")
    draw = ImageDraw.Draw(image)
    title_font = find_font(30, bold=True)
    label_font = find_font(20)
    tick_font = find_font(16)
    legend_font = find_font(17)

    selected = [row for row in rows if row["array_type"] == array_type]
    by_algorithm: dict[str, list[tuple[int, float]]] = defaultdict(list)
    for row in selected:
        by_algorithm[row["algorithm"]].append((int(row["size"]), transform(float(row[metric]))))

    for values in by_algorithm.values():
        values.sort()

    x_values = sorted({size for values in by_algorithm.values() for size, _ in values})
    y_values = [value for values in by_algorithm.values() for _, value in values]
    x_min, x_max = min(x_values), max(x_values)
    y_min, y_max = 0.0, max(y_values) * 1.08 if y_values else 1.0
    if y_max == 0:
        y_max = 1.0

    def x_to_px(x: int) -> float:
        return plot_left + (x - x_min) / (x_max - x_min) * plot_width

    def y_to_px(y: float) -> float:
        return plot_bottom - (y - y_min) / (y_max - y_min) * plot_height

    draw.text((margin_left, 28), title, fill=(20, 20, 20), font=title_font)
    draw.line((plot_left, plot_top, plot_left, plot_bottom), fill=(30, 30, 30), width=2)
    draw.line((plot_left, plot_bottom, plot_right, plot_bottom), fill=(30, 30, 30), width=2)

    for i in range(6):
        y = y_min + (y_max - y_min) * i / 5
        py = y_to_px(y)
        draw.line((plot_left, py, plot_right, py), fill=(226, 230, 235), width=1)
        label = nice_label(y)
        tw, th = text_size(draw, label, tick_font)
        draw.text((plot_left - tw - 12, py - th / 2), label, fill=(70, 70, 70), font=tick_font)

    for x in [100, 500, 1000, 1500, 2000, 2500, 3000]:
        px = x_to_px(x)
        draw.line((px, plot_bottom, px, plot_bottom + 6), fill=(30, 30, 30), width=2)
        label = str(x)
        tw, _ = text_size(draw, label, tick_font)
        draw.text((px - tw / 2, plot_bottom + 12), label, fill=(70, 70, 70), font=tick_font)

    for algorithm, values in by_algorithm.items():
        points = [(x_to_px(size), y_to_px(value)) for size, value in values]
        color = COLORS[algorithm]
        if len(points) > 1:
            draw.line(points, fill=color, width=4, joint="curve")
        for px, py in points[::3]:
            draw.ellipse((px - 4, py - 4, px + 4, py + 4), fill=color)

    x_axis_label = "Размер массива, строк"
    tw, _ = text_size(draw, x_axis_label, label_font)
    draw.text((plot_left + plot_width / 2 - tw / 2, height - 52), x_axis_label, fill=(35, 35, 35), font=label_font)
    draw.text((22, plot_top - 42), y_label, fill=(35, 35, 35), font=label_font)

    legend_x = plot_right + 34
    legend_y = plot_top + 8
    draw.text((legend_x, legend_y - 34), "Алгоритмы", fill=(35, 35, 35), font=label_font)
    for index, algorithm in enumerate(ALGORITHM_LABELS):
        y = legend_y + index * 42
        color = COLORS[algorithm]
        draw.line((legend_x, y + 12, legend_x + 32, y + 12), fill=color, width=5)
        draw.ellipse((legend_x + 12, y + 4, legend_x + 20, y + 20), fill=color)
        draw.text((legend_x + 44, y), ALGORITHM_LABELS[algorithm], fill=(45, 45, 45), font=legend_font)

    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output)


def main() -> None:
    rows = load_rows()
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    for array_type, label in ARRAY_LABELS.items():
        plot_metric(
            rows,
            array_type,
            "mean_time_us",
            f"Среднее время: {label}",
            "Время, мс",
            lambda value: value / 1000.0,
            OUT_DIR / f"time_{array_type}.png",
        )
        plot_metric(
            rows,
            array_type,
            "mean_char_comparisons",
            f"Символьные сравнения: {label}",
            "Сравнения/обращения, тыс.",
            lambda value: value / 1000.0,
            OUT_DIR / f"comparisons_{array_type}.png",
        )

    print(f"wrote {len(list(OUT_DIR.glob('*.png')))} plots to {OUT_DIR}")


if __name__ == "__main__":
    main()

