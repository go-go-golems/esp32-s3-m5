#!/usr/bin/env python3
"""Build a self-contained retro monochrome HTML/JS dashboard for F0 density evidence."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

TICKET = Path(__file__).resolve().parents[1]
EXP = TICKET / "scripts/experiments/EXP-20260715-008-factory-f0-dynamic-density"
STATIC = TICKET / "scripts/output/32-printalyzer-static-calculations.json"


def read_jsonl(path: Path) -> list[dict[str, Any]]:
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def build_payload(
    capture: Path, host_events: Path, analysis: Path, static: Path
) -> dict[str, Any]:
    records = read_jsonl(capture)
    host = read_jsonl(host_events)
    report = json.loads(analysis.read_text())
    static_report = json.loads(static.read_text())
    samples = []
    raw_records = [
        record
        for record in records
        if record.get("parsed", {}).get("kind") == "raw_sensor"
    ]
    start_ns = raw_records[0]["host_monotonic_ns"]
    flash_complete = next(
        record["host_monotonic_ns"]
        for record in host
        if record["event"] == "flash_runner_complete"
    )
    for record in raw_records:
        parsed = record["parsed"]
        derived = parsed["derived"]
        samples.append(
            {
                "seq": record["sequence"],
                "utc": record["host_utc"],
                "t": (record["host_monotonic_ns"] - start_ns) / 1e9,
                "post": (record["host_monotonic_ns"] - flash_complete) / 1e9,
                "ch0": parsed["channel_0"],
                "ch1": parsed["channel_1"],
                "density": derived["density_estimate"],
                "saturated": derived["saturated"],
                "valid": derived["density_estimate_valid"],
            }
        )
    return {
        "meta": {
            "experiment": EXP.name,
            "firmware": "FactoryTest V0.5 exact merged release",
            "firmware_sha256": "d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624",
            "printalyzer": "v1.1.0 / g7101373",
            "settings": "gain 2 · 100 ms · reflection duty 128",
            "capture_sha256": "2a585ee5392b4e71b001a3f82234f45a9bcb1fc47affc57a4bce8ae71e157d50",
            "host_sha256": "cd989eab69b7eb4e24a1b69d36602f71a3a5f656fb33e3bf46ba5b483a23abef",
        },
        "samples": samples,
        "markers": [
            {
                "name": record["event"],
                "t": (record["host_monotonic_ns"] - start_ns) / 1e9,
            }
            for record in host
        ],
        "analysis": report,
        "static": static_report,
    }


HTML = r"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'%3E%3Crect width='16' height='16' fill='%23fffdf5'/%3E%3Crect x='2' y='2' width='12' height='12' fill='none' stroke='%23171717' stroke-width='2'/%3E%3Cpath d='M4 11L7 7l2 2 3-5' fill='none' stroke='%2360999a' stroke-width='2'/%3E%3C/svg%3E">
<title>PaperS3 · F0 Density Instrument Panel</title>
<style>
:root {
  --ink:#171717; --paper:#eee9dc; --white:#fffdf5; --grid:#b8b2a6;
  --teal:#60999a; --coral:#c87868; --mustard:#c8aa58; --mint:#91b69b;
  --blue:#748da8; --plum:#92788f; --gray:#77766f; --danger:#a94f4f;
}
* { box-sizing:border-box; }
html { background:#bdb8ad; }
body {
  margin:0; color:var(--ink); background:
  radial-gradient(circle at 1px 1px, #a7a298 1px, transparent 1px) 0 0/4px 4px,
  var(--paper);
  font-family:"Courier New","Nimbus Mono PS",monospace; font-size:15px; line-height:1.35;
}
main { max-width:1500px; margin:0 auto; padding:26px; }
header { border-bottom:4px double var(--ink); padding:0 0 18px; margin-bottom:20px; display:flex; gap:24px; justify-content:space-between; align-items:end; }
h1 { font-size:clamp(24px,4vw,48px); line-height:.95; letter-spacing:-2px; margin:0; text-transform:uppercase; }
.kicker { font-size:12px; letter-spacing:2px; text-transform:uppercase; color:#3f6f70; margin-bottom:8px; }
.stamp { text-align:right; font-size:12px; max-width:420px; overflow-wrap:anywhere; }
.grid { display:grid; grid-template-columns:repeat(12,1fr); gap:16px; }
.panel { background:var(--white); border:2px solid var(--ink); box-shadow:4px 4px 0 var(--ink); padding:15px; position:relative; }
.panel h2 { font-size:15px; margin:-15px -15px 14px; padding:7px 10px; border-bottom:2px solid var(--ink); background:repeating-linear-gradient(135deg,#fff,#fff 3px,#ddd8ca 3px,#ddd8ca 5px); text-transform:uppercase; letter-spacing:1px; }
.summary { grid-column:span 12; display:grid; grid-template-columns:repeat(6,1fr); gap:10px; }
.metric { border:1px solid var(--ink); padding:10px; min-height:82px; background:#fff; }
.metric b { display:block; font-size:clamp(18px,2.5vw,28px); line-height:1; margin-top:10px; }
.metric small { text-transform:uppercase; letter-spacing:.7px; }
.good b { color:#39766a; } .accent b { color:#9a5e51; }
.chart { grid-column:span 9; } .controls { grid-column:span 3; }
.static { grid-column:span 4; } .histogram { grid-column:span 4; } .markers { grid-column:span 4; }
.table-panel { grid-column:span 12; }
canvas { width:100%; display:block; border:1px solid var(--ink); background:#fffef8; image-rendering:auto; }
#mainChart { height:430px; cursor:crosshair; }
#histogram { height:250px; }
#markerList { font-size:13px; line-height:1.55; }
.control-group { margin-bottom:16px; border-bottom:1px dashed #777; padding-bottom:12px; }
.control-group:last-child { border:0; }
label { display:block; margin:7px 0; }
button { font:inherit; font-weight:bold; color:var(--ink); background:#fff; border:2px solid var(--ink); box-shadow:2px 2px 0 var(--ink); padding:6px 9px; margin:3px 2px 3px 0; cursor:pointer; }
button:active,button.active { transform:translate(2px,2px); box-shadow:none; background:var(--mustard); }
input[type=checkbox] { accent-color:var(--teal); }
.legend { display:flex; flex-wrap:wrap; gap:12px; margin:8px 0 0; font-size:13px; }
.swatch { display:inline-block; width:18px; height:8px; border:1px solid #333; margin-right:4px; }
.tooltip { min-height:94px; border:1px solid var(--ink); background:#f7f3e8; padding:9px; white-space:pre-wrap; font-size:12px; }
.phase { position:relative; height:70px; border:1px solid var(--ink); background:#fff; margin:8px 0; overflow:hidden; }
.phase .line { position:absolute; left:0; right:0; top:34px; border-top:2px solid var(--ink); }
.phase .mark { position:absolute; top:5px; width:1px; height:55px; background:var(--coral); }
.phase .mark span { position:absolute; top:0; left:4px; font-size:10px; writing-mode:vertical-rl; max-height:50px; overflow:hidden; }
.bar-row { display:grid; grid-template-columns:130px 1fr 80px; gap:8px; align-items:center; margin:10px 0; }
.bar-track { height:18px; border:1px solid var(--ink); background:#eee; }
.bar { height:100%; background:var(--teal); border-right:1px solid var(--ink); }
.bar-row:nth-child(2) .bar { background:var(--coral); }
.bar-row:nth-child(3) .bar { background:var(--mustard); }
.bar-row:nth-child(4) .bar { background:var(--blue); }
table { width:100%; border-collapse:collapse; font-size:12px; }
th,td { border:1px solid var(--ink); padding:6px; text-align:right; }
th:first-child,td:first-child { text-align:left; }
th { background:#ddd8ca; position:sticky; top:0; }
tbody tr:nth-child(even) { background:#f2eee3; }
.scroll { max-height:330px; overflow:auto; border:1px solid var(--ink); }
.badge { display:inline-block; border:1px solid var(--ink); padding:2px 6px; background:var(--mint); font-weight:bold; }
.badge.warn { background:var(--mustard); }
.note { border-left:8px solid var(--coral); padding:9px 12px; background:#f7ede5; font-size:13px; margin-top:12px; }
footer { margin-top:24px; border-top:4px double var(--ink); padding-top:12px; font-size:11px; display:flex; justify-content:space-between; gap:20px; }
@media(max-width:1000px) { .chart,.controls,.static,.histogram,.markers { grid-column:span 12; } .summary { grid-template-columns:repeat(3,1fr); } }
@media(max-width:600px) { main{padding:12px}.summary{grid-template-columns:repeat(2,1fr)} header{display:block}.stamp{text-align:left;margin-top:12px} }
@media print { body{background:white} main{max-width:none;padding:0}.panel{box-shadow:none;break-inside:avoid}.controls{display:none} }
</style>
</head>
<body>
<main>
<header>
  <div><div class="kicker">ESP-50 / optical instrumentation</div><h1>PaperS3<br>Density Panel</h1></div>
  <div class="stamp"><b id="experiment"></b><br><span id="firmware"></span><br><span id="settings"></span></div>
</header>
<section class="grid">
  <div class="summary panel">
    <div class="metric good"><small>samples</small><b id="mSamples">—</b></div>
    <div class="metric"><small>coverage</small><b id="mDuration">—</b></div>
    <div class="metric accent"><small>density span</small><b id="mSpan">—</b></div>
    <div class="metric"><small>minimum</small><b id="mMin">—</b></div>
    <div class="metric"><small>maximum</small><b id="mMax">—</b></div>
    <div class="metric good"><small>invalid / saturated</small><b id="mInvalid">—</b></div>
  </div>

  <article class="panel chart">
    <h2>Continuous point density · hover to inspect</h2>
    <canvas id="mainChart"></canvas>
    <div class="legend">
      <span><i class="swatch" style="background:var(--blue)"></i>density estimate</span>
      <span><i class="swatch" style="background:var(--coral)"></i>flash markers</span>
      <span><i class="swatch" style="background:var(--mustard)"></i>candidate 500 ms changes</span>
    </div>
  </article>

  <aside class="panel controls">
    <h2>Scope controls</h2>
    <div class="control-group">
      <button data-view="all" class="active">ALL</button>
      <button data-view="post">POST RESET</button>
      <button data-view="activity">ACTIVITY</button>
    </div>
    <div class="control-group">
      <label><input type="checkbox" id="showMarkers" checked> host markers</label>
      <label><input type="checkbox" id="showCandidates" checked> candidate changes</label>
      <label><input type="checkbox" id="showPoints"> sample points</label>
      <label><input type="checkbox" id="tightScale" checked> tight vertical scale</label>
    </div>
    <div class="control-group">
      <small>cursor sample</small>
      <div class="tooltip" id="tooltip">Move over the trace.</div>
    </div>
    <span class="badge">CAPTURE OK</span>
    <div class="note">Density is a single-aperture host estimate. It is excellent for within-run transitions, not whole-panel uniformity.</div>
  </aside>

  <article class="panel static"><h2>Placement experiments</h2><div id="staticBars"></div></article>
  <article class="panel histogram"><h2>F0 density distribution</h2><canvas id="histogram"></canvas></article>
  <article class="panel markers"><h2>Host event rail</h2><div class="phase"><div class="line"></div><div id="phaseMarks"></div></div><div id="markerList"></div></article>

  <article class="panel table-panel">
    <h2>Deterministic change candidates · Δ bin mean ≥ 0.010 D</h2>
    <div class="scroll"><table><thead><tr><th>time</th><th>mean D</th><th>min D</th><th>max D</th><th>Δ previous</th><th>n</th></tr></thead><tbody id="candidateRows"></tbody></table></div>
    <div class="note">Orange candidates show activity, not semantic labels. Exact title/black/white/grayscale frame boundaries require F2 ring events or video alignment.</div>
  </article>
</section>
<footer><span>Self-contained HTML · no network dependencies</span><span id="hashes"></span></footer>
</main>
<script>
const DATA = __PAYLOAD__;
const palette={ink:'#171717',blue:'#748da8',coral:'#c87868',mustard:'#c8aa58',teal:'#60999a',paper:'#fffef8',grid:'#d8d2c6'};
const samples=DATA.samples, analysis=DATA.analysis, markers=DATA.markers;
let view='all', hoverIndex=-1;
const state={markers:true,candidates:true,points:false,tight:true};
const $=s=>document.querySelector(s);
function fmt(v,n=3){return Number(v).toFixed(n)}
$('#experiment').textContent=DATA.meta.experiment;
$('#firmware').textContent=DATA.meta.firmware;
$('#settings').textContent=DATA.meta.settings;
$('#mSamples').textContent=samples.length;
$('#mDuration').textContent=fmt(analysis.duration_seconds,2)+' s';
$('#mSpan').textContent=fmt(analysis.density.range,3)+' D';
$('#mMin').textContent=fmt(analysis.density.minimum,3)+' D';
$('#mMax').textContent=fmt(analysis.density.maximum,3)+' D';
$('#mInvalid').textContent=analysis.invalid_samples+' / '+analysis.saturated_samples;
$('#hashes').textContent='raw '+DATA.meta.capture_sha256.slice(0,12)+'… · host '+DATA.meta.host_sha256.slice(0,12)+'…';

const flashComplete=markers.find(m=>m.name==='flash_runner_complete').t;
function bounds(){
  if(view==='post') return [flashComplete, samples.at(-1).t];
  if(view==='activity') return [Math.max(0,flashComplete-.5),Math.min(samples.at(-1).t,flashComplete+16)];
  return [samples[0].t,samples.at(-1).t];
}
function visible(){const [a,b]=bounds();return samples.filter(s=>s.t>=a&&s.t<=b)}
function setup(canvas,height){
  const dpr=window.devicePixelRatio||1, rect=canvas.getBoundingClientRect();
  canvas.width=Math.round(rect.width*dpr);canvas.height=Math.round(height*dpr);
  const ctx=canvas.getContext('2d');ctx.setTransform(dpr,0,0,dpr,0,0);return [ctx,rect.width,height];
}
function drawMain(){
  const c=$('#mainChart'), [ctx,w,h]=setup(c,430), pad={l:58,r:18,t:18,b:42};
  const data=visible(), [x0,x1]=bounds(); let lo=Math.min(...data.map(s=>s.density)),hi=Math.max(...data.map(s=>s.density));
  if(!state.tight){lo=0;hi=1.0}else{const p=Math.max(.015,(hi-lo)*.12);lo-=p;hi+=p}
  const X=t=>pad.l+(t-x0)/(x1-x0)*(w-pad.l-pad.r), Y=v=>pad.t+(hi-v)/(hi-lo)*(h-pad.t-pad.b);
  ctx.fillStyle=palette.paper;ctx.fillRect(0,0,w,h);ctx.font='12px Courier New';ctx.fillStyle=palette.ink;ctx.strokeStyle=palette.grid;ctx.lineWidth=1;
  for(let i=0;i<=5;i++){let y=pad.t+i*(h-pad.t-pad.b)/5,val=hi-i*(hi-lo)/5;ctx.beginPath();ctx.moveTo(pad.l,y);ctx.lineTo(w-pad.r,y);ctx.stroke();ctx.fillText(val.toFixed(2),5,y+4)}
  for(let i=0;i<=9;i++){let x=pad.l+i*(w-pad.l-pad.r)/9,t=x0+i*(x1-x0)/9;ctx.beginPath();ctx.moveTo(x,pad.t);ctx.lineTo(x,h-pad.b);ctx.stroke();ctx.fillText(t.toFixed(1),x-12,h-15)}
  if(state.candidates){ctx.strokeStyle=palette.mustard;ctx.globalAlpha=.55;for(const q of analysis.candidate_change_bins){if(q.start_seconds<x0||q.start_seconds>x1)continue;ctx.beginPath();ctx.moveTo(X(q.start_seconds),pad.t);ctx.lineTo(X(q.start_seconds),h-pad.b);ctx.stroke()}ctx.globalAlpha=1}
  if(state.markers){ctx.strokeStyle=palette.coral;ctx.lineWidth=2;for(const m of markers){if(m.t<x0||m.t>x1)continue;ctx.beginPath();ctx.moveTo(X(m.t),pad.t);ctx.lineTo(X(m.t),h-pad.b);ctx.stroke();ctx.save();ctx.translate(X(m.t)+4,pad.t+4);ctx.rotate(Math.PI/2);ctx.fillStyle='#8a4f46';ctx.fillText(m.name,0,0);ctx.restore()}}
  ctx.strokeStyle=palette.blue;ctx.lineWidth=2.4;ctx.beginPath();data.forEach((s,i)=>{const x=X(s.t),y=Y(s.density);i?ctx.lineTo(x,y):ctx.moveTo(x,y)});ctx.stroke();
  if(state.points){ctx.fillStyle=palette.teal;for(const s of data){ctx.beginPath();ctx.arc(X(s.t),Y(s.density),2,0,Math.PI*2);ctx.fill()}}
  if(hoverIndex>=0){const s=samples[hoverIndex];if(s.t>=x0&&s.t<=x1){ctx.strokeStyle=palette.ink;ctx.lineWidth=1;ctx.setLineDash([3,3]);ctx.beginPath();ctx.moveTo(X(s.t),pad.t);ctx.lineTo(X(s.t),h-pad.b);ctx.stroke();ctx.setLineDash([]);ctx.fillStyle=palette.coral;ctx.beginPath();ctx.arc(X(s.t),Y(s.density),4,0,Math.PI*2);ctx.fill()}}
  c._map={x0,x1,pad,w};
}
function drawHistogram(){
  const c=$('#histogram'),[ctx,w,h]=setup(c,250),pad=28,vals=samples.map(s=>s.density),lo=Math.min(...vals),hi=Math.max(...vals),n=24,bins=Array(n).fill(0);
  vals.forEach(v=>bins[Math.min(n-1,Math.floor((v-lo)/(hi-lo)*n))]++);const max=Math.max(...bins),bw=(w-pad*2)/n;
  ctx.fillStyle=palette.paper;ctx.fillRect(0,0,w,h);bins.forEach((v,i)=>{ctx.fillStyle=i%2?palette.teal:palette.mint;ctx.fillRect(pad+i*bw,h-pad-v/max*(h-pad*2),Math.max(1,bw-1),v/max*(h-pad*2))});ctx.strokeStyle=palette.ink;ctx.strokeRect(pad,pad,w-pad*2,h-pad*2);ctx.fillStyle=palette.ink;ctx.font='12px Courier New';ctx.fillText(lo.toFixed(3)+' D',pad,h-8);ctx.fillText(hi.toFixed(3)+' D',w-pad-48,h-8)
}
function fillStatic(){
 const list=DATA.static.captures,max=Math.max(...list.map(x=>x.density.mean));$('#staticBars').innerHTML=list.map(x=>`<div class="bar-row"><span>${x.label}</span><div class="bar-track"><div class="bar" style="width:${x.density.mean/max*100}%"></div></div><b>${x.density.mean.toFixed(3)} D</b></div>`).join('')+`<div class="note">Manual re-seating changed the sampled point. These bars are diagnostic context, not cross-run panel specifications.</div>`
}
function fillMarkers(){const end=samples.at(-1).t;$('#phaseMarks').innerHTML=markers.map(m=>`<i class="mark" style="left:${m.t/end*100}%"><span>${m.name}</span></i>`).join('');$('#markerList').innerHTML=markers.map(m=>`<div><b>${m.t.toFixed(3)}s</b> ${m.name}</div>`).join('')}
function fillCandidates(){$('#candidateRows').innerHTML=analysis.candidate_change_bins.map(q=>`<tr><td>${q.start_seconds.toFixed(1)}s</td><td>${q.mean_density.toFixed(6)}</td><td>${q.minimum_density.toFixed(6)}</td><td>${q.maximum_density.toFixed(6)}</td><td style="color:${q.delta_from_previous_mean>0?'#9a5e51':'#39766a'}">${q.delta_from_previous_mean.toFixed(6)}</td><td>${q.count}</td></tr>`).join('')}
$('#mainChart').addEventListener('mousemove',e=>{const map=e.currentTarget._map;if(!map)return;const r=e.currentTarget.getBoundingClientRect(),px=e.clientX-r.left,t=map.x0+(px-map.pad.l)/(map.w-map.pad.l-map.pad.r)*(map.x1-map.x0);let best=0,dist=Infinity;samples.forEach((s,i)=>{const d=Math.abs(s.t-t);if(d<dist){dist=d;best=i}});hoverIndex=best;const s=samples[best];$('#tooltip').textContent=`t      ${s.t.toFixed(3)} s\nreset  ${s.post.toFixed(3)} s\ndens   ${s.density.toFixed(6)} D\nCH0/1  ${s.ch0} / ${s.ch1}\nUTC    ${s.utc}`;drawMain()});
document.querySelectorAll('button[data-view]').forEach(b=>b.onclick=()=>{document.querySelectorAll('button[data-view]').forEach(x=>x.classList.remove('active'));b.classList.add('active');view=b.dataset.view;hoverIndex=-1;drawMain()});
for(const [id,key] of [['showMarkers','markers'],['showCandidates','candidates'],['showPoints','points'],['tightScale','tight']]){$('#'+id).onchange=e=>{state[key]=e.target.checked;drawMain()}}
window.addEventListener('resize',()=>{drawMain();drawHistogram()});fillStatic();fillMarkers();fillCandidates();drawMain();drawHistogram();
</script>
</body></html>
"""


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path, default=EXP / "raw-dynamic-f0.jsonl")
    parser.add_argument("--host-events", type=Path, default=EXP / "host-events.jsonl")
    parser.add_argument("--analysis", type=Path, default=EXP / "density-analysis.json")
    parser.add_argument("--static-analysis", type=Path, default=STATIC)
    parser.add_argument("--output", type=Path, default=EXP / "dashboard.html")
    args = parser.parse_args()
    payload = build_payload(
        args.capture, args.host_events, args.analysis, args.static_analysis
    )
    embedded = json.dumps(payload, separators=(",", ":")).replace("</", "<\\/")
    rendered = HTML.replace("__PAYLOAD__", embedded)
    args.output.write_text(rendered)
    print(f"dashboard={args.output}")
    print(f"samples={len(payload['samples'])}")
    print(f"bytes={args.output.stat().st_size}")


if __name__ == "__main__":
    main()
