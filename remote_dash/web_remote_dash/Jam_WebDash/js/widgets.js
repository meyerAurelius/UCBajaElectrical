const START_DEG = 135;
const SWEEP_DEG = 270;
const FONT = 'Nutmeg, Arial, Helvetica, sans-serif';

const clamp01 = (x) => Math.min(1, Math.max(0, x));

export function withAlpha(hex, alpha) {
  const n = parseInt(hex.slice(1), 16);
  return `rgba(${(n >> 16) & 255}, ${(n >> 8) & 255}, ${n & 255}, ${alpha})`;
}

/** Sizes a canvas for the current devicePixelRatio. Returns null while it has no layout box. */
export function fitCanvas(canvas) {
  const w = canvas.clientWidth;
  const h = canvas.clientHeight;
  if (!w || !h) return null;
  const dpr = Math.min(window.devicePixelRatio || 1, 2);
  const pw = Math.round(w * dpr);
  const ph = Math.round(h * dpr);
  if (canvas.width !== pw || canvas.height !== ph) {
    canvas.width = pw;
    canvas.height = ph;
  }
  const ctx = canvas.getContext('2d');
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, w, h);
  return { ctx, w, h };
}

function polar(r, deg) {
  const a = (deg * Math.PI) / 180;
  return [100 + r * Math.cos(a), 100 + r * Math.sin(a)];
}

function arc(r, from, to) {
  const [x0, y0] = polar(r, from);
  const [x1, y1] = polar(r, to);
  return `M ${x0.toFixed(2)} ${y0.toFixed(2)} A ${r} ${r} 0 ${to - from > 180 ? 1 : 0} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
}

let gaugeSeq = 0;

export class RadialGauge {
  constructor(el, { label, unit, min = 0, max = 100, step = 10, minorPerMajor = 4, redline = null, decimals = 0, tickFormat = String }) {
    this.el = el;
    this.min = min;
    this.max = max;
    this.redline = redline;
    this.decimals = decimals;

    const gradId = `gauge-grad-${++gaugeSeq}`;
    const angle = (v) => START_DEG + SWEEP_DEG * clamp01((v - min) / (max - min));
    const majors = Math.round((max - min) / step);
    const ticks = [];
    for (let i = 0; i <= majors * minorPerMajor; i++) {
      const v = min + (i * step) / minorPerMajor;
      const deg = angle(v);
      const major = i % minorPerMajor === 0;
      const [x0, y0] = polar(major ? 66 : 70, deg);
      const [x1, y1] = polar(75, deg);
      ticks.push(`<line class="gauge__tick${major ? ' gauge__tick--major' : ''}" x1="${x0.toFixed(2)}" y1="${y0.toFixed(2)}" x2="${x1.toFixed(2)}" y2="${y1.toFixed(2)}"/>`);
      if (major) {
        const [lx, ly] = polar(55, deg);
        ticks.push(`<text class="gauge__tick-label" x="${lx.toFixed(2)}" y="${ly.toFixed(2)}">${tickFormat(v)}</text>`);
      }
    }

    el.classList.add('gauge', 'is-empty');
    el.innerHTML = `
      <svg class="gauge__svg" viewBox="0 0 200 200" aria-hidden="true">
        <defs>
          <linearGradient id="${gradId}" x1="0" y1="1" x2="1" y2="0">
            <stop offset="0" stop-color="#ffcf48"/>
            <stop offset="0.55" stop-color="#f0484f"/>
            <stop offset="1" stop-color="#d71925"/>
          </linearGradient>
        </defs>
        <path class="gauge__track" d="${arc(84, START_DEG, START_DEG + SWEEP_DEG)}"/>
        ${redline != null ? `<path class="gauge__redline" d="${arc(84, angle(redline), START_DEG + SWEEP_DEG)}"/>` : ''}
        <g>${ticks.join('')}</g>
        <path class="gauge__value" d="${arc(84, START_DEG, START_DEG + SWEEP_DEG)}" pathLength="100" stroke="url(#${gradId})"/>
      </svg>
      <div class="gauge__readout">
        <span class="gauge__num">--</span>
        <span class="gauge__unit"></span>
      </div>
      <span class="gauge__label"></span>`;
    el.querySelector('.gauge__unit').textContent = unit;
    el.querySelector('.gauge__label').textContent = label;
    this.valueEl = el.querySelector('.gauge__value');
    this.numEl = el.querySelector('.gauge__num');
  }

  set(value) {
    const empty = value == null || !Number.isFinite(value);
    const pct = empty ? 0 : clamp01((value - this.min) / (this.max - this.min));
    this.valueEl.style.strokeDashoffset = String(100 - pct * 100);
    this.numEl.textContent = empty ? '--' : value.toFixed(this.decimals);
    this.el.classList.toggle('is-empty', empty);
    this.el.classList.toggle('is-redline', !empty && this.redline != null && value >= this.redline);
  }
}

export function drawSparkline(canvas, data, color, windowMs = 30_000) {
  const fit = fitCanvas(canvas);
  if (!fit || data.length < 2) return;
  const { ctx, w, h } = fit;

  const tEnd = data.at(-1).t;
  let start = data.length - 1;
  while (start > 0 && tEnd - data[start - 1].t <= windowMs) start--;
  let lo = Infinity;
  let hi = -Infinity;
  for (let i = start; i < data.length; i++) {
    lo = Math.min(lo, data[i].v);
    hi = Math.max(hi, data[i].v);
  }
  if (hi - lo < 1e-6) {
    lo -= 1;
    hi += 1;
  }
  const pad = (hi - lo) * 0.15;
  const x = (t) => w * (1 - (tEnd - t) / windowMs);
  const y = (v) => h - 2 - (h - 4) * ((v - lo + pad) / (hi - lo + 2 * pad));

  ctx.beginPath();
  for (let i = start; i < data.length; i++) {
    const px = x(data[i].t);
    const py = y(data[i].v);
    i === start ? ctx.moveTo(px, py) : ctx.lineTo(px, py);
  }
  ctx.strokeStyle = color;
  ctx.lineWidth = 1.5;
  ctx.lineJoin = 'round';
  ctx.stroke();

  ctx.lineTo(x(tEnd), h);
  ctx.lineTo(x(data[start].t), h);
  ctx.closePath();
  const fill = ctx.createLinearGradient(0, 0, 0, h);
  fill.addColorStop(0, withAlpha(color, 0.28));
  fill.addColorStop(1, withAlpha(color, 0));
  ctx.fillStyle = fill;
  ctx.fill();
}

/** Overlaid, range-normalised traces for the last `windowMs` of data. */
export class TrendChart {
  constructor(canvas, legendEl, series, { windowMs, onToggle }) {
    this.canvas = canvas;
    this.windowMs = windowMs;
    this.series = series.map((s) => ({ ...s, visible: true }));

    legendEl.replaceChildren();
    for (const s of this.series) {
      const btn = document.createElement('button');
      btn.type = 'button';
      btn.className = 'legend__item';
      btn.style.setProperty('--swatch', s.color);
      btn.setAttribute('aria-pressed', 'true');
      btn.innerHTML = '<span class="legend__swatch"></span><span class="legend__name"></span><span class="legend__value">--</span>';
      btn.querySelector('.legend__name').textContent = s.label;
      btn.addEventListener('click', () => {
        s.visible = !s.visible;
        btn.setAttribute('aria-pressed', String(s.visible));
        onToggle?.();
      });
      s.valueEl = btn.querySelector('.legend__value');
      legendEl.append(btn);
    }
  }

  updateLegend(store, format) {
    for (const s of this.series) {
      s.valueEl.closest('.legend__item').hidden = !store.has(s.key);
      s.valueEl.textContent = format(s.key, store.get(s.key));
    }
  }

  draw(store) {
    const fit = fitCanvas(this.canvas);
    if (!fit) return;
    const { ctx, w, h } = fit;
    const pad = { l: 6, r: 6, t: 6, b: 22 };
    const pw = w - pad.l - pad.r;
    const ph = h - pad.t - pad.b;
    const secs = this.windowMs / 1000;

    ctx.strokeStyle = 'rgba(255, 255, 255, 0.07)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let i = 0; i <= 4; i++) {
      const y = Math.round(pad.t + (ph * i) / 4) + 0.5;
      ctx.moveTo(pad.l, y);
      ctx.lineTo(pad.l + pw, y);
    }
    for (let s = 0; s <= secs; s += 10) {
      const x = Math.round(pad.l + pw * (1 - s / secs)) + 0.5;
      ctx.moveTo(x, pad.t);
      ctx.lineTo(x, pad.t + ph);
    }
    ctx.stroke();

    ctx.fillStyle = 'rgba(255, 255, 255, 0.4)';
    ctx.font = `700 10px ${FONT}`;
    ctx.textBaseline = 'top';
    for (let s = 0; s <= secs; s += 10) {
      ctx.textAlign = s === 0 ? 'right' : s === secs ? 'left' : 'center';
      ctx.fillText(s === 0 ? 'NOW' : `-${s}s`, pad.l + pw * (1 - s / secs), pad.t + ph + 7);
    }

    const now = store.now;
    if (!now) return;
    ctx.lineWidth = 2;
    ctx.lineJoin = 'round';
    for (const s of this.series) {
      if (!s.visible) continue;
      const data = store.history(s.key);
      if (data.length < 2) continue;
      ctx.beginPath();
      let started = false;
      for (const p of data) {
        const age = now - p.t;
        if (age > this.windowMs) continue;
        const x = pad.l + pw * (1 - age / this.windowMs);
        const y = pad.t + ph * (1 - clamp01((p.v - s.min) / (s.max - s.min)));
        if (started) ctx.lineTo(x, y);
        else {
          ctx.moveTo(x, y);
          started = true;
        }
      }
      ctx.strokeStyle = s.color;
      ctx.shadowColor = withAlpha(s.color, 0.6);
      ctx.shadowBlur = 8;
      ctx.stroke();
      ctx.shadowBlur = 0;
    }
  }
}

/** Friction circle: longitudinal G up/down, lateral G left/right, with a fading trail. */
export class GGDiagram {
  constructor(canvas, { rangeG = 1.5, trailMs = 4000 } = {}) {
    this.canvas = canvas;
    this.rangeG = rangeG;
    this.trailMs = trailMs;
  }

  draw(store) {
    const fit = fitCanvas(this.canvas);
    if (!fit) return;
    const { ctx, w, h } = fit;
    const cx = w / 2;
    const cy = h / 2;
    const R = Math.max(10, Math.min(w, h) / 2 - 20);
    const scale = R / this.rangeG;

    ctx.lineWidth = 1;
    for (let g = 0.5; g <= this.rangeG + 1e-9; g += 0.5) {
      ctx.strokeStyle = g === 1 ? 'rgba(255, 255, 255, 0.22)' : 'rgba(255, 255, 255, 0.09)';
      ctx.beginPath();
      ctx.arc(cx, cy, g * scale, 0, Math.PI * 2);
      ctx.stroke();
    }
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.09)';
    ctx.beginPath();
    ctx.moveTo(cx - R, cy);
    ctx.lineTo(cx + R, cy);
    ctx.moveTo(cx, cy - R);
    ctx.lineTo(cx, cy + R);
    ctx.stroke();

    ctx.fillStyle = 'rgba(255, 255, 255, 0.38)';
    ctx.font = `700 9px ${FONT}`;
    ctx.textAlign = 'left';
    ctx.textBaseline = 'bottom';
    for (let g = 0.5; g <= this.rangeG + 1e-9; g += 0.5) {
      ctx.fillText(`${g.toFixed(1)}g`, cx + g * scale * 0.707 + 3, cy - g * scale * 0.707 - 1);
    }
    ctx.fillStyle = 'rgba(255, 207, 72, 0.8)';
    ctx.font = `900 9px ${FONT}`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('ACCEL', cx, cy - R - 10);
    ctx.fillText('BRAKE', cx, cy + R + 10);
    ctx.fillText('L', cx - R - 10, cy);
    ctx.fillText('R', cx + R + 10, cy);

    const gx = store.history('gx');
    const gy = store.history('gy');
    const n = Math.min(gx.length, gy.length);
    if (!n) return;
    const tEnd = gx[n - 1].t;
    const px = (i) => cx + Math.max(-1, Math.min(1, gy[i].v / this.rangeG)) * R;
    const py = (i) => cy - Math.max(-1, Math.min(1, gx[i].v / this.rangeG)) * R;

    for (let i = 0; i < n - 1; i++) {
      const age = tEnd - gx[i].t;
      if (age > this.trailMs) continue;
      ctx.fillStyle = `rgba(240, 72, 79, ${(1 - age / this.trailMs) * 0.7})`;
      ctx.beginPath();
      ctx.arc(px(i), py(i), 2.2, 0, Math.PI * 2);
      ctx.fill();
    }

    ctx.shadowColor = 'rgba(215, 25, 37, 0.9)';
    ctx.shadowBlur = 14;
    ctx.fillStyle = '#ffffff';
    ctx.beginPath();
    ctx.arc(px(n - 1), py(n - 1), 6, 0, Math.PI * 2);
    ctx.fill();
    ctx.shadowBlur = 0;
  }
}
