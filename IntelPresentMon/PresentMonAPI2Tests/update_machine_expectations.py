#!/usr/bin/env python3
# Copyright (C) 2026 Intel Corporation
# SPDX-License-Identifier: MIT

import argparse
import json
import math
import os
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox, ttk


SKIP_EXPECTATION = object()


def load_measurements(path):
    records = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                records.append(json.loads(line))
    return records


def default_expectation_path():
    env = os.environ.get("PRESENTMON_MACHINE_EXPECTATIONS")
    if env:
        return env
    return str(Path(__file__).resolve().parents[1] / "machine_expectations.json")


def metric_bucket(root, measurement):
    section = measurement["section"]
    metric = measurement["metric"]
    if section == "system":
        return root.setdefault("system", {}), metric
    device_id = int(measurement.get("device_id", 0))
    gpus = root.setdefault("gpu", [])
    if isinstance(gpus, dict):
        gpus = [{"device_id": device_id, "metrics": gpus}]
        root["gpu"] = gpus
    for gpu in gpus:
        if int(gpu.get("device_id", -1)) == device_id:
            return gpu.setdefault("metrics", {}), metric
    gpu = {"device_id": device_id, "metrics": {}}
    gpus.append(gpu)
    return gpu["metrics"], metric


def normalize_gpu_expectations(root):
    if "gpus" in root and "gpu" not in root:
        root["gpu"] = root.pop("gpus")
    gpu = root.setdefault("gpu", [])
    if isinstance(gpu, dict):
        root["gpu"] = [{"device_id": 0, "metrics": gpu}]


def simplify_values(values):
    if not values:
        return SKIP_EXPECTATION
    present = [v for v, _ in values if v is not None]
    if not present:
        if all(not introspection_available for _, introspection_available in values):
            return None
        return SKIP_EXPECTATION
    if all(isinstance(v, (int, float)) and not isinstance(v, bool) for v in present):
        numbers = [float(v) for v in present if math.isfinite(float(v))]
        if not numbers:
            return None
        lo = min(numbers)
        hi = max(numbers)
        if lo == hi:
            return lo
        return {"min": lo, "max": hi}
    strings = [str(v) for v in present]
    if len(set(strings)) == 1:
        return strings[0]
    return {"value": strings[-1]}


def update_existing_value(old, new):
    if old is None or new is None:
        return new
    if isinstance(old, dict) and ("min" in old or "max" in old):
        vals = []
        if "min" in old:
            vals.append(float(old["min"]))
        if "max" in old:
            vals.append(float(old["max"]))
        if isinstance(new, dict):
            if "min" in new:
                vals.append(float(new["min"]))
            if "max" in new:
                vals.append(float(new["max"]))
        elif isinstance(new, (int, float)) and not isinstance(new, bool):
            vals.append(float(new))
        else:
            return new
        return {"min": min(vals), "max": max(vals)}
    if isinstance(old, (int, float)) and isinstance(new, (int, float)):
        if float(old) == float(new):
            return old
        return {"min": min(float(old), float(new)), "max": max(float(old), float(new))}
    return new if old != new else old


def build_generated(records, existing=None):
    generated = json.loads(json.dumps(existing or {"wait_multiplier": 1.0, "system": {}, "gpu": []}))
    generated.setdefault("wait_multiplier", 1.0)
    normalize_gpu_expectations(generated)
    grouped = {}
    for record in records:
        for measurement in record.get("measurements", []):
            key = (
                measurement["section"],
                int(measurement.get("device_id", 0)) if measurement["section"] == "gpu" else 0,
                measurement["metric"],
            )
            grouped.setdefault(key, []).append(
                (
                    None if measurement.get("available") is False else measurement.get("value"),
                    bool(measurement.get("introspection_available", True)),
                )
            )

    changes = []
    for (section, device_id, metric), values in grouped.items():
        target, target_metric = metric_bucket(
            generated,
            {"section": section, "device_id": device_id, "metric": metric},
        )
        new_value = simplify_values(values)
        if new_value is SKIP_EXPECTATION:
            continue
        old_value = target.get(target_metric, "<missing>")
        merged = new_value if old_value == "<missing>" else update_existing_value(old_value, new_value)
        if old_value != merged:
            changes.append({
                "section": section,
                "device_id": device_id,
                "metric": metric,
                "before": old_value,
                "after": merged,
            })
        target[target_metric] = merged

    generated["gpu"] = sorted(generated.get("gpu", []), key=lambda g: int(g.get("device_id", 0)))
    return generated, changes


def pretty_json(data):
    return json.dumps(data, indent=2, sort_keys=False)


class ExpectationEditor(tk.Tk):
    def __init__(self, measurements_path, expectation_path):
        super().__init__()
        self.title("PresentMon Machine Expectations")
        self.geometry("1100x760")
        self.measurements_path = measurements_path
        self.expectation_path = expectation_path
        self.records = load_measurements(measurements_path)
        self.existing = self.load_existing()
        self.generated, self.changes = build_generated(self.records, self.existing)
        self.create_widgets()

    def load_existing(self):
        if not os.path.exists(self.expectation_path):
            return None
        with open(self.expectation_path, "r", encoding="utf-8") as f:
            return json.load(f)

    def create_widgets(self):
        top = ttk.Frame(self)
        top.pack(fill=tk.X, padx=10, pady=8)
        ttk.Label(top, text=f"Measurements: {self.measurements_path}").pack(anchor=tk.W)
        ttk.Label(top, text=f"Expectation: {self.expectation_path}").pack(anchor=tk.W)

        paned = ttk.Panedwindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=10, pady=4)

        left = ttk.Frame(paned)
        right = ttk.Frame(paned)
        paned.add(left, weight=1)
        paned.add(right, weight=2)

        ttk.Label(left, text="Changes").pack(anchor=tk.W)
        self.change_text = tk.Text(left, wrap=tk.NONE, height=20)
        self.change_text.pack(fill=tk.BOTH, expand=True)
        self.change_text.insert("1.0", self.format_changes())
        self.change_text.configure(state=tk.DISABLED)

        ttk.Label(right, text="Editable expectation JSON").pack(anchor=tk.W)
        self.json_text = tk.Text(right, wrap=tk.NONE)
        self.json_text.pack(fill=tk.BOTH, expand=True)
        self.json_text.insert("1.0", pretty_json(self.generated))

        buttons = ttk.Frame(self)
        buttons.pack(fill=tk.X, padx=10, pady=8)
        ttk.Button(buttons, text="Write expectation", command=self.write_expectation).pack(side=tk.RIGHT)
        ttk.Button(buttons, text="Choose output...", command=self.choose_output).pack(side=tk.RIGHT, padx=8)

    def format_changes(self):
        if not self.changes:
            return "No changes. Measurements match the current expectation file.\n"
        lines = []
        for change in self.changes:
            prefix = change["section"]
            if change["section"] == "gpu":
                prefix += f"[{change['device_id']}]"
            lines.append(f"{prefix}.{change['metric']}")
            lines.append(f"  before: {json.dumps(change['before'])}")
            lines.append(f"  after:  {json.dumps(change['after'])}")
            lines.append("")
        return "\n".join(lines)

    def choose_output(self):
        path = filedialog.asksaveasfilename(
            defaultextension=".json",
            initialfile=os.path.basename(self.expectation_path),
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
        )
        if path:
            self.expectation_path = path

    def write_expectation(self):
        try:
            data = json.loads(self.json_text.get("1.0", tk.END))
        except json.JSONDecodeError as e:
            messagebox.showerror("Invalid JSON", str(e))
            return
        folder = os.path.dirname(os.path.abspath(self.expectation_path))
        if folder:
            os.makedirs(folder, exist_ok=True)
        with open(self.expectation_path, "w", encoding="utf-8", newline="\r\n") as f:
            f.write(pretty_json(data))
            f.write("\n")
        messagebox.showinfo("Wrote expectation", self.expectation_path)


def main():
    parser = argparse.ArgumentParser(description="Generate or update PresentMon API2 machine expectations.")
    parser.add_argument(
        "--measurements",
        default=os.path.join("TestOutput", "MachineExpectations", "measurements.jsonl"),
        help="Path to the accumulated measurement JSONL file from a test run.",
    )
    parser.add_argument(
        "--expectation",
        default=default_expectation_path(),
        help="Path to the editable machine expectation JSON file.",
    )
    parser.add_argument("--print", action="store_true", help="Print generated JSON and exit.")
    args = parser.parse_args()

    records = load_measurements(args.measurements)
    existing = None
    if os.path.exists(args.expectation):
        with open(args.expectation, "r", encoding="utf-8") as f:
            existing = json.load(f)
    generated, changes = build_generated(records, existing)

    if args.print:
        for change in changes:
            print(f"{change['section']}.{change['metric']}: {change['before']} -> {change['after']}")
        print(pretty_json(generated))
        return

    app = ExpectationEditor(args.measurements, args.expectation)
    app.mainloop()


if __name__ == "__main__":
    main()
