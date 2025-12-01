#!/usr/bin/env python3
import sys
import os

_prev_excepthook = sys.excepthook

def _dep_hint_excepthook(exc_type, exc, tb):
    # Catch missing-module cases and print one install command for all deps
    if exc_type is ModuleNotFoundError or exc_type is ImportError:
        script_path = os.path.abspath(__file__)
        script_dir = os.path.dirname(script_path)
        sys.stderr.write(
            "Required package(s) missing; install with:\n"
            "  python -m pip install matplotlib pandas"
        )
        sys.exit(1)

    # Fallback to the default handler for non-import errors
    _prev_excepthook(exc_type, exc, tb)

sys.excepthook = _dep_hint_excepthook

import re
from enum import Enum
import argparse
import pandas as pd
import tkinter as tk
from tkinter import ttk, messagebox
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
from matplotlib.lines import Line2D

class RunMode(Enum):
    OneShot = "one-shot"
    Full = "full"
    RoundRobin = "round-robin"

class AutocompleteCombobox(ttk.Combobox):
    """A Combobox that filters its dropdown list as you type."""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._all_values = []
        self.bind('<KeyRelease>', self._on_keyrelease)

    def set_completion_list(self, completion_list):
        self._all_values = sorted(completion_list, key=str.lower)
        self['values'] = self._all_values

    def _on_keyrelease(self, event):
        text = self.get()
        if text == '':
            data = self._all_values
        else:
            data = [v for v in self._all_values if text.lower() in v.lower()]
        self['values'] = data

class PolledPlotApp(tk.Tk):
    def __init__(self, folder, name, run_mode, golds_folder=None):
        super().__init__()
        self.title("Polled CSV Plotter")
        self.geometry("900x650")

        # Persist per-series visibility across Y selections
        self.visible = {}  # {series_label: bool}
        # Persist per-series style for legend proxies
        self.styles = {}   # {series_label: dict(color=..., linestyle=..., marker=...)}

        # Load CSVs according to run mode
        self.dfs = self.load_csvs(folder, name, golds_folder, run_mode)
        if not self.dfs:
            messagebox.showerror("Error", "No valid CSV files found for the given inputs.")
            self.destroy()
            return

        # Initialize columns/UI
        cols = list(self.dfs[0][1].columns)
        if len(cols) < 2:
            messagebox.showerror("Error", "CSV files must have at least two columns.")
            self.destroy()
            return

        self.x_col = cols[0]
        self.y_choices = cols[1:]

        ctrl = ttk.Frame(self)
        ctrl.pack(side=tk.TOP, fill=tk.X, padx=8, pady=8)

        ttk.Label(ctrl, text="X-axis:").pack(side=tk.LEFT)
        ttk.Label(ctrl, text=self.x_col, width=15).pack(side=tk.LEFT, padx=(0, 20))
        ttk.Label(ctrl, text="Y-axis:").pack(side=tk.LEFT)

        self.y_combo = AutocompleteCombobox(ctrl, state="normal", width=50)
        self.y_combo.set_completion_list(self.y_choices)
        self.y_combo.set(self.y_choices[0])
        self.y_combo.pack(side=tk.LEFT, padx=(0, 10))
        self.y_combo.bind("<<ComboboxSelected>>", self.on_select)

        ttk.Button(ctrl, text="Exit", command=self.destroy).pack(side=tk.RIGHT)

        # Matplotlib figure/canvas/toolbar
        self.fig = Figure(figsize=(6, 4))
        self.ax = self.fig.add_subplot(111)

        self.canvas = FigureCanvasTkAgg(self.fig, master=self)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=1)

        self.toolbar = NavigationToolbar2Tk(self.canvas, self)
        self.toolbar.update()
        self.toolbar.pack(side=tk.TOP, fill=tk.X)

        # One pick handler for the lifetime of the app
        self.fig.canvas.mpl_connect("pick_event", self.on_pick)

        # Initial draw
        self.draw_plot(self.x_col, self.y_choices[0])

    def load_csvs(self, folder, name, golds_folder, run_mode):
        """
        Returns list of (label, DataFrame) based on run mode.

        RoundRobin:
          - Load: folder/name_robin_#.csv

        OneShot:
          - Must load: folder/name_oneshot.csv (mandatory)
          - Must also load: golds_folder/name_gold.csv

        Full:
          - Load: folder/name_full_#.csv
          - Must also load: golds_folder/name_gold.csv
          - Also try to include: folder/name_oneshot.csv (if present)
        """
        out = []

        def try_load_csv(path, label=None):
            try:
                df = pd.read_csv(path, header=0)
                if df.shape[1] >= 2:
                    lab = label if label is not None else os.path.basename(path)
                    out.append((lab, df))
                    if lab not in self.visible:
                        self.visible[lab] = True
                else:
                    print(f"Skipping {path}: not enough columns")
            except Exception as e:
                print(f"Skipping {path}: {e}")

        try:
            folder_files = sorted(os.listdir(folder))
        except FileNotFoundError:
            print(f"Folder not found: {folder}")
            folder_files = []

        oneshot_fname = f"{name}_oneshot.csv"
        gold_fname = f"{name}_gold.csv"

        if run_mode == RunMode.RoundRobin:
            robin_re = re.compile(rf'^{re.escape(name)}_robin_(\d+)\.csv$')
            for fname in folder_files:
                if robin_re.match(fname):
                    try_load_csv(os.path.join(folder, fname))

        elif run_mode == RunMode.OneShot:
            # Mandatory oneshot
            oneshot_path = os.path.join(folder, oneshot_fname)
            if os.path.isfile(oneshot_path):
                try_load_csv(oneshot_path)
            else:
                print(f"ERROR: Oneshot file is mandatory but not found: {oneshot_path}")
                return []  # cause error popup upstream

            # Gold must be available for the comparison basis
            if not golds_folder:
                print("ERROR: Golds folder is required in one-shot mode.")
                return []
            gold_path = os.path.join(golds_folder, gold_fname)
            if os.path.isfile(gold_path):
                try_load_csv(gold_path)
            else:
                print(f"ERROR: Gold file is mandatory but not found: {gold_path}")
                return []

        elif run_mode == RunMode.Full:
            full_re = re.compile(rf'^{re.escape(name)}_full_(\d+)\.csv$')
            for fname in folder_files:
                if full_re.match(fname):
                    try_load_csv(os.path.join(folder, fname))
            # Always try to include oneshot and gold as context
            oneshot_path = os.path.join(folder, oneshot_fname)
            if os.path.isfile(oneshot_path):
                try_load_csv(oneshot_path)
            else:
                print(f"Oneshot file not found (optional in full): {oneshot_path}")
            if golds_folder:
                gold_path = os.path.join(golds_folder, gold_fname)
                if os.path.isfile(gold_path):
                    try_load_csv(gold_path)
                else:
                    print(f"ERROR: Gold file is mandatory but not found: {gold_path}")
                    return []
            else:
                print("ERROR: Golds folder is required in one-shot mode.")
                return []

        return out

    def _legend_handles(self, legend):
        """
        Compatibility shim:
        - Matplotlib >= 3.8 exposes 'legend_handles'
        - Older versions use 'legendHandles'
        """
        h = getattr(legend, "legend_handles", None)
        if h is None:
            h = getattr(legend, "legendHandles", [])
        return h

    def draw_plot(self, x_col, y_col):
        self.ax.clear()

        plotted = []  # list of (label, line)

        # Plot data and capture styles once for each label
        for label, df in self.dfs:
            line, = self.ax.plot(df[x_col], df[y_col], label=label)
            line.set_visible(self.visible.get(label, True))
            plotted.append((label, line))

            if label not in self.styles:
                self.styles[label] = dict(
                    color=line.get_color(),
                    linestyle=line.get_linestyle(),
                    marker=line.get_marker(),
                )

        self.ax.set_xlabel(x_col)
        self.ax.set_ylabel(y_col)
        self.ax.set_title(f"{y_col} vs {x_col}")

        # Build legend from proxy artists so entries never disappear
        proxies = []
        labels = []
        for label, line in plotted:
            st = self.styles.get(label, {})
            proxy = Line2D(
                [0], [0],
                color=st.get("color", "k"),
                linestyle=st.get("linestyle", "-"),
                marker=st.get("marker", None),
            )
            proxy.set_alpha(1.0 if line.get_visible() else 0.2)
            proxies.append(proxy)
            labels.append(label)

        leg = self.ax.legend(handles=proxies, labels=labels, loc="best",
                             fancybox=True, shadow=True)

        # IMPORTANT: legend clones the handles; attach pick + mapping to the cloned artists
        legend_handles = self._legend_handles(leg)
        for h, (label, line) in zip(legend_handles, plotted):
            h.set_picker(5)       # pickable handle
            h._origline = line    # map legend handle -> real line
            h.set_alpha(1.0 if line.get_visible() else 0.2)

        # Also make legend text clickable and mapped to the real line
        for t, (label, line) in zip(leg.get_texts(), plotted):
            t.set_picker(True)
            t._origline = line

        self.canvas.draw()

    def on_select(self, event):
        # Redraw with same visibility choices
        self.draw_plot(self.x_col, self.y_combo.get())

    def on_pick(self, event):
        # Toggle for both legend handles and legend texts
        artist = event.artist
        origline = getattr(artist, "_origline", None)
        if origline is None:
            return

        new_vis = not origline.get_visible()
        origline.set_visible(new_vis)

        # Persist by label
        label = origline.get_label()
        self.visible[label] = new_vis

        # Refresh legend proxy alphas to reflect visibility
        leg = self.ax.get_legend()
        if leg is not None:
            for h in self._legend_handles(leg):
                l = getattr(h, "_origline", None)
                if l is not None:
                    h.set_alpha(1.0 if l.get_visible() else 0.2)

        self.canvas.draw()

def parse_args():
    parser = argparse.ArgumentParser(
        description="Interactive plot of polled data sequences from test runs."
    )
    parser.add_argument(
        "--folder", required=True,
        help="Folder containing data generated in the current test run."
    )
    parser.add_argument(
        "--golds",
        help="Folder containing gold run data (required for one-shot mode)."
    )
    parser.add_argument(
        "--name", required=True,
        help="Test case name (prefix of CSV files)."
    )
    parser.add_argument(
        "--run-mode", required=True,
        type=RunMode,
        choices=list(RunMode),
        help="Specify a mode of execution"
    )
    return parser.parse_args()

def main():
    args = parse_args()
    app = PolledPlotApp(folder=args.folder, name=args.name, run_mode=args.run_mode, golds_folder=args.golds)
    app.mainloop()

if __name__ == "__main__":
    main()
