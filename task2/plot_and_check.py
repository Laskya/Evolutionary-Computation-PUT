"""Independent re-check of best solutions (recomputes objective from the CSV
coordinates with mathematical rounding) and 2D visualizations.
Usage: python3 plot_and_check.py <results_dir> <TSPA.csv> <TSPB.csv>

Writes <inst>_best.png (the four regret methods), <inst>_weights.png
(weight experiment) and check_report.txt (all 8 methods).
"""
import csv
import math
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

res, files = sys.argv[1], {"TSPA": sys.argv[2], "TSPB": sys.argv[3]}
all_methods = [("random", "Random"), ("nn_end", "NN (end only)"),
               ("nn_any", "NN (any position)"), ("greedy_cycle", "Greedy cycle"),
               ("nn_any_regret", "NN any, 2-regret"),
               ("nn_any_weighted", "NN any, regret + best change"),
               ("greedy_cycle_regret", "Greedy cycle, 2-regret"),
               ("greedy_cycle_weighted", "Greedy cycle, regret + best change")]
plotted = all_methods[4:]


def load(path):
    pts = []
    for line in open(path):
        line = line.strip()
        if line:
            x, y, c = map(int, line.split(";"))
            pts.append((x, y, c))
    return pts


def dist(a, b):
    return int(math.floor(math.hypot(a[0] - b[0], a[1] - b[1]) + 0.5))


def check(pts, m, inst):
    n = len(pts)
    k = (n + 1) // 2
    lines = open(f"{res}/{inst}_{m}.sol").read().split("\n")
    claimed = int(lines[0])
    sol = list(map(int, lines[1].split()))
    ok = len(sol) == k and len(set(sol)) == k and all(0 <= v < n for v in sol)
    length = sum(dist(pts[sol[i]], pts[sol[(i + 1) % len(sol)]]) for i in range(len(sol)))
    costs = sum(pts[v][2] for v in sol)
    return sol, claimed, length, costs, ok, ok and length + costs == claimed


report = []
for inst, path in files.items():
    pts = load(path)
    for m, _ in all_methods:
        sol, claimed, length, costs, ok, match = check(pts, m, inst)
        report.append((inst, m, claimed, length, costs, ok, match))

    # --- best solutions of the four new methods
    fig, axes = plt.subplots(2, 2, figsize=(14, 8.5))
    cmax = max(p[2] for p in pts)
    for ax, (m, title) in zip(axes.flat, plotted):
        sol, claimed, length, costs, ok, match = check(pts, m, inst)
        xs = [pts[v][0] for v in sol] + [pts[sol[0]][0]]
        ys = [pts[v][1] for v in sol] + [pts[sol[0]][1]]
        ax.plot(xs, ys, "-", color="#444", lw=0.8, zorder=1)
        sel = set(sol)
        ux = [p[0] for i, p in enumerate(pts) if i not in sel]
        uy = [p[1] for i, p in enumerate(pts) if i not in sel]
        ax.scatter(ux, uy, s=8, c="#bbbbbb", marker="x", zorder=2)
        sc = ax.scatter([pts[v][0] for v in sol], [pts[v][1] for v in sol],
                        s=[12 + 60 * pts[v][2] / cmax for v in sol],
                        c=[pts[v][2] for v in sol], cmap="viridis_r", vmin=0, vmax=cmax,
                        edgecolors="black", linewidths=0.4, zorder=3)
        ax.set_title(f"{inst} - {title}: {length + costs}", fontsize=11)
        ax.set_aspect("equal")
        ax.set_xticks([]); ax.set_yticks([])
    fig.colorbar(sc, ax=axes.ravel().tolist(), label="node cost (colour and size)", shrink=0.8)
    fig.savefig(f"{res}/{inst}_best.png", dpi=130, bbox_inches="tight")
    plt.close(fig)

    # --- weight experiment
    rows = list(csv.DictReader(open(f"{res}/{inst}_weights.csv")))
    fig, ax = plt.subplots(figsize=(7.5, 4))
    for mname, label, col in [("nn_any_weighted", "NN any position", "#1f77b4"),
                              ("greedy_cycle_weighted", "Greedy cycle", "#d95f02")]:
        r = [x for x in rows if x["method"] == mname and float(x["w_regret"]) <= 0.8]
        ax.plot([float(x["w_regret"]) for x in r], [float(x["avg"]) for x in r],
                "o-", color=col, label=label + " (average)")
        ax.plot([float(x["w_regret"]) for x in r], [float(x["min"]) for x in r],
                "s--", color=col, alpha=0.6, label=label + " (minimum)")
    ax.set_xlabel("weight of the regret (weight of the best change = 1 - w)")
    ax.set_ylabel("objective value")
    ax.set_title(f"{inst}: weighted sum criterion, 200 runs")
    ax.grid(alpha=0.3)
    ax.legend(fontsize=8)
    fig.savefig(f"{res}/{inst}_weights.png", dpi=130, bbox_inches="tight")
    plt.close(fig)

with open(f"{res}/check_report.txt", "w") as f:
    for inst, m, claimed, length, costs, ok, match in report:
        line = (f"{inst:5s} {m:22s} solver={claimed:7d} length={length:7d} costs={costs:6d} "
                f"python={length + costs:7d} feasible={ok} match={match}")
        print(line)
        f.write(line + "\n")
sys.exit(0 if all(r[6] for r in report) else 1)
