"""
plot_results.py — BatchBench CSV → README용 그래프 PNG

사용법:
  python Tools/plot_results.py Saved/Bench/bench_results.csv Docs/captures \
      --tags sweep_20261027_232920 sweep_20261028_191950

--tags 를 주면 해당 sweep 회차만 평균에 사용한다. 생략하면 build=EditorGame(Standalone) 전체.
같은 (mode, count, moving) 조합의 행을 평균 내어 그린다.
"""
import argparse
import csv
import statistics as st
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
plt.rcParams["font.family"] = ["Noto Sans CJK KR", "Malgun Gothic", "DejaVu Sans"]
plt.rcParams["axes.unicode_minus"] = False
from matplotlib.ticker import FuncFormatter

MODES = ["Actor", "ISM-DirtyEach", "ISM-DirtyOnce", "ISM-ShaderMove"]
COLORS = {  # 고정 순서 범주색 (validated palette)
    "Actor": "#2a78d6",
    "ISM-DirtyEach": "#eb6834",
    "ISM-DirtyOnce": "#1baf7a",
    "ISM-ShaderMove": "#eda100",
}
TEXT = "#2b2b2b"
MUTED = "#7a7a76"
GRID = "#e6e5e1"


def load(path, tags):
    rows = list(csv.DictReader(open(path, encoding="utf-8")))
    if tags:
        rows = [r for r in rows if r["tag"] in tags]
    else:
        rows = [r for r in rows if r["build"] == "EditorGame"]
    g = defaultdict(list)
    for r in rows:
        g[(r["mode"], int(r["count"]), r["moving"] == "1")].append(r)
    agg = {}
    for k, v in g.items():
        agg[k] = {c: st.mean(float(r[c]) for r in v)
                  for c in ("game_ms", "render_ms", "rhi_ms", "gpu_ms", "frame_ms")}
        agg[k]["n"] = len(v)
    meta = rows[0] if rows else {}
    return agg, meta


def style(ax, ylabel):
    ax.set_facecolor("white")
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(GRID)
    ax.grid(axis="y", color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)
    ax.tick_params(colors=MUTED, labelsize=9)
    ax.set_ylabel(ylabel, color=MUTED, fontsize=9)
    ax.xaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{int(v):,}"))


def line_chart(agg, counts, metric, moving, title, subtitle, out, modes=MODES, ref=None):
    fig, ax = plt.subplots(figsize=(8, 4.6), dpi=150)
    ends = []
    for m in modes:
        ys = [agg[(m, c, moving)][metric] for c in counts if (m, c, moving) in agg]
        xs = [c for c in counts if (m, c, moving) in agg]
        if not xs:
            continue
        ax.plot(xs, ys, color=COLORS[m], linewidth=2, marker="o", markersize=5,
                markeredgecolor="white", markeredgewidth=1.5, label=m)
        ends.append((xs[-1], ys[-1]))
    # 끝값 라벨: 가까운 값끼리 겹치지 않게 위아래로 비킴
    span = max(e[1] for e in ends) - 0
    placed = []
    for x, y in sorted(ends, key=lambda e: e[1]):
        dy = 0
        if placed and (y - placed[-1]) < span * 0.04:
            dy = 9
            placed[-1] = placed[-1]  # 아래 것은 그대로
        ax.annotate(f"{y:.1f}", (x, y), xytext=(6, dy), textcoords="offset points",
                    fontsize=8.5, color=TEXT, va="center")
        placed.append(y + (span * 0.04 if dy else 0))
    if ref:
        ax.axhline(ref, color=MUTED, linewidth=1, linestyle=(0, (4, 3)))
        ax.text(counts[0], ref, f" {ref:.1f} ms (60 fps)", color=MUTED, fontsize=8, va="bottom")
    ax.set_xscale("log")
    ax.set_xticks(counts)
    style(ax, metric.replace("_ms", "").replace("game", "GameThread").replace("frame", "Frame") + " ms")
    ax.set_title(title, loc="left", fontsize=12, color=TEXT, fontweight="bold", pad=18)
    ax.text(0, 1.02, subtitle, transform=ax.transAxes, fontsize=9, color=MUTED)
    ax.legend(frameon=False, fontsize=9, loc="upper left")
    fig.tight_layout()
    fig.savefig(out)
    plt.close(fig)


def thread_breakdown(agg, count, out):
    """5만 개 이동 시 스레드별 비용 — 어디가 병목인지."""
    fig, ax = plt.subplots(figsize=(8, 4.4), dpi=150)
    metrics = [("game_ms", "GameThread"), ("render_ms", "RenderThread"), ("gpu_ms", "GPU")]
    w = 0.2
    xs = range(len(metrics))
    for i, m in enumerate(MODES):
        ys = [agg[(m, count, True)][k] for k, _ in metrics]
        bars = ax.bar([x + (i - 1.5) * w for x in xs], ys, width=w - 0.02, color=COLORS[m], label=m)
        for b, y in zip(bars, ys):
            ax.text(b.get_x() + b.get_width() / 2, y + 2.5, f"{y:.0f}", ha="center", fontsize=8, color=TEXT, bbox=dict(facecolor="white", edgecolor="none", pad=0.5))
    ax.set_xticks(list(xs))
    ax.set_xticklabels([n for _, n in metrics])
    style(ax, "ms")
    ax.xaxis.set_major_formatter(FuncFormatter(lambda v, _: metrics[int(v)][1] if 0 <= int(v) < len(metrics) else ""))
    ax.axhline(16.7, color=MUTED, linewidth=1, linestyle=(0, (4, 3)))
    ax.text(2.45, 16.7, "16.7 ms", color=MUTED, fontsize=8, va="bottom", ha="right")
    ax.set_title(f"{count:,}개 이동 시 스레드별 비용", loc="left", fontsize=12, color=TEXT, fontweight="bold", pad=18)
    ax.text(0, 1.02, "Actor는 게임·렌더 동시 붕괴 · ISM은 게임 스레드만 남음 · ShaderMove는 GPU 한계(≈17ms)만 남음",
            transform=ax.transAxes, fontsize=9, color=MUTED)
    ax.legend(frameon=False, fontsize=9, loc="upper right")
    fig.tight_layout()
    fig.savefig(out)
    plt.close(fig)


def bar_pair(agg, counts, out):
    """DirtyEach vs DirtyOnce, game thread ms, moving — 핵심 비교."""
    fig, ax = plt.subplots(figsize=(8, 4.4), dpi=150)
    w = 0.38
    xs = range(len(counts))
    for i, m in enumerate(["ISM-DirtyEach", "ISM-DirtyOnce"]):
        ys = [agg[(m, c, True)]["game_ms"] for c in counts]
        bars = ax.bar([x + (i - 0.5) * w for x in xs], ys, width=w - 0.03,
                      color=COLORS[m], label=m)
        for b, y in zip(bars, ys):
            ax.text(b.get_x() + b.get_width() / 2, y + 0.3, f"{y:.1f}",
                    ha="center", fontsize=8.5, color=TEXT)
    ax.set_xticks(list(xs))
    ax.set_xticklabels([f"{c:,}" for c in counts])
    style(ax, "GameThread ms")
    ax.xaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{counts[int(v)]:,}" if 0 <= int(v) < len(counts) else ""))
    ax.set_title("더티 마킹 횟수는 비용을 바꾸지 않는다", loc="left", fontsize=12,
                 color=TEXT, fontweight="bold", pad=18)
    ax.text(0, 1.02, "ISM 이동 · 더티 마킹 N회(DirtyEach) vs 1회(DirtyOnce) · GameThread 평균 · 차이 5% 안쪽",
            transform=ax.transAxes, fontsize=9, color=MUTED)
    ax.legend(frameon=False, fontsize=9, loc="upper left")
    fig.tight_layout()
    fig.savefig(out)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv")
    ap.add_argument("outdir")
    ap.add_argument("--tags", nargs="*", default=None)
    a = ap.parse_args()
    out = Path(a.outdir)
    out.mkdir(parents=True, exist_ok=True)
    agg, meta = load(a.csv, a.tags)
    counts = sorted({k[1] for k in agg if k[1] <= 50000})
    eng = meta.get("engine", "").split("-")[0]
    gpu = meta.get("gpu_name", "").replace("NVIDIA GeForce ", "")
    env = f"{gpu} · UE {eng} · {meta.get('res_x','')}x{meta.get('res_y','')} · Standalone"

    line_chart(agg, counts, "game_ms", True,
               "이동 시 게임 스레드 비용",
               f"매 프레임 전 객체 트랜스폼 갱신 · {env}",
               out / "chart_game_moving.png", ref=16.7)
    line_chart(agg, counts, "game_ms", True,
               "ISM 갱신 방식별 게임 스레드 비용",
               f"Actor 제외 · 호출 수에 선형, ShaderMove는 CPU 비용 없음 · {env}",
               out / "chart_game_moving_ism.png",
               modes=["ISM-DirtyEach", "ISM-DirtyOnce", "ISM-ShaderMove"], ref=16.7)
    thread_breakdown(agg, 50000, out / "chart_threads_50k.png")
    bar_pair(agg, counts, out / "chart_dirty_each_vs_once.png")

    # 표 (README용 markdown)
    with open(out / "summary_table.md", "w", encoding="utf-8") as f:
        f.write("| 모드 | 개수 | 이동 | Game ms | Render ms | GPU ms | Frame ms |\n|---|---:|:---:|---:|---:|---:|---:|\n")
        for mv in (False, True):
            for m in MODES:
                for c in counts:
                    k = (m, c, mv)
                    if k not in agg:
                        continue
                    r = agg[k]
                    f.write(f"| {m} | {c:,} | {'O' if mv else '-'} | {r['game_ms']:.1f} | {r['render_ms']:.1f} | {r['gpu_ms']:.1f} | {r['frame_ms']:.1f} |\n")
    print("written to", out)


if __name__ == "__main__":
    main()
