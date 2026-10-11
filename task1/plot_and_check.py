"""Independent re-check of best solutions (recomputes objective from the CSV
coordinates with mathematical rounding) and 2D visualizations.
Usage: python3 plot_and_check.py <results_dir> <TSPA.csv> <TSPB.csv>
"""
import math
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

res, files = sys.argv[1], {"TSPA": sys.argv[2], "TSPB": sys.argv[3]}
methods = [("random", "Random"), ("nn_end", "NN (end only)"),
           ("nn_any", "NN (any position)"), ("greedy_cycle", "Greedy cycle")]


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


report = []
for inst, path in files.items():
    pts = load(path)
    n = len(pts)
    k = (n + 1) // 2
    fig, axes = plt.subplots(2, 2, figsize=(14, 8.5))
    cmax = max(p[2] for p in pts)
    for ax, (m, title) in zip(axes.flat, methods):
        lines = open(f"{res}/{inst}_{m}.sol").read().split("\n")
        claimed = int(lines[0])
        sol = list(map(int, lines[1].split()))
        ok = len(sol) == k and len(set(sol)) == k and all(0 <= v < n for v in sol)
        total = sum(dist(pts[sol[i]], pts[sol[(i + 1) % k]]) + pts[sol[i]][2] for i in range(k))
        match = ok and total == claimed
        report.append((inst, m, claimed, total, ok, match))
        xs = [pts[v][0] for v in sol] + [pts[sol[0]][0]]
        ys = [pts[v][1] for v in sol] + [pts[sol[0]][1]]
        ax.plot(xs, ys, "-", color="#444", lw=0.8, zorder=1)
        sel = set(sol)
        ux = [p[0] for i, p in enumerate(pts) if i not in sel]
        uy = [p[1] for i, p in enumerate(pts) if i not in sel]
        ax.scatter(ux, uy, s=8, c="#bbbbbb", marker="x", zorder=2, label="not selected")
        sc = ax.scatter([pts[v][0] for v in sol], [pts[v][1] for v in sol],
                        s=[12 + 60 * pts[v][2] / cmax for v in sol],
                        c=[pts[v][2] for v in sol], cmap="viridis_r", vmin=0, vmax=cmax,
                        edgecolors="black", linewidths=0.4, zorder=3)
        ax.set_title(f"{inst} - {title}: {total}", fontsize=11)
        ax.set_aspect("equal")
        ax.set_xticks([]); ax.set_yticks([])
    fig.colorbar(sc, ax=axes.ravel().tolist(), label="node cost (colour and size)", shrink=0.8)
    fig.savefig(f"{res}/{inst}_best.png", dpi=130, bbox_inches="tight")
    plt.close(fig)

with open(f"{res}/check_report.txt", "w") as f:
    for inst, m, claimed, total, ok, match in report:
        line = f"{inst:5s} {m:13s} cpp={claimed:7d} python={total:7d} feasible={ok} match={match}"
        print(line)
        f.write(line + "\n")
sys.exit(0 if all(r[5] for r in report) else 1)
