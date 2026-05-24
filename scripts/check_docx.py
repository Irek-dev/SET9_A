#!/usr/bin/env python3
from __future__ import annotations

import sys
import zipfile
from pathlib import Path

from docx import Document


def main() -> None:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("report/report_A1.docx")
    if not path.exists():
        raise SystemExit(f"not found: {path}")

    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        required = {"word/document.xml", "[Content_Types].xml", "word/_rels/document.xml.rels"}
        missing = sorted(required - names)
        if missing:
            raise SystemExit(f"missing docx parts: {missing}")
        images = [name for name in names if name.startswith("word/media/")]

    doc = Document(path)
    paragraphs = [p.text for p in doc.paragraphs if p.text.strip()]
    if not any("Отчёт A1" in text for text in paragraphs):
        raise SystemExit("report title not found")
    if len(images) < 6:
        raise SystemExit(f"expected at least 6 embedded images, found {len(images)}")

    print(f"docx ok: {path} ({len(paragraphs)} non-empty paragraphs, {len(images)} images)")


if __name__ == "__main__":
    main()

