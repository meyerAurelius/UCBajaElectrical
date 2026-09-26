const G = 9.80665;
const STEP_M = 0.5;

export const VEHICLE = {
  topSpeed: 13.5,          // m/s, ~49 km/h
  lateralLimit: 0.7 * G,
  brakeLimit: 0.75 * G,
  accelLimit: (v) => 0.4 + 3.2 * Math.max(0, 1 - v / 15),
};

/** Equirectangular projection around an origin; accurate to centimetres over a track-sized area. */
export function localProjection([lat0, lng0]) {
  const mLat = 111_132;
  const mLng = 111_320 * Math.cos((lat0 * Math.PI) / 180);
  return {
    toXY: ([lat, lng]) => [(lng - lng0) * mLng, (lat - lat0) * mLat],
    toLatLng: ([x, y]) => [lat0 + y / mLat, lng0 + x / mLng],
  };
}

function catmullRom(p0, p1, p2, p3, t) {
  const t2 = t * t;
  const t3 = t2 * t;
  return [0, 1].map((i) => 0.5 * (
    2 * p1[i]
    + (-p0[i] + p2[i]) * t
    + (2 * p0[i] - 5 * p1[i] + 4 * p2[i] - p3[i]) * t2
    + (-p0[i] + 3 * p1[i] - 3 * p2[i] + p3[i]) * t3
  ));
}

/**
 * Turns the corner points in CONFIG.track.outline into a smooth closed loop sampled every 0.5 m,
 * with signed curvature and a lap-sim speed profile.
 */
export function buildCircuit(outline) {
  const { toXY, toLatLng } = localProjection(outline[0]);
  const ctrl = outline.map(toXY);
  const n = ctrl.length;
  const dense = [];
  for (let i = 0; i < n; i++) {
    const p0 = ctrl[(i - 1 + n) % n], p1 = ctrl[i], p2 = ctrl[(i + 1) % n], p3 = ctrl[(i + 2) % n];
    for (let j = 0; j < 80; j++) dense.push(catmullRom(p0, p1, p2, p3, j / 80));
  }

  const cum = [0];
  for (let i = 1; i <= dense.length; i++) {
    const a = dense[i - 1], b = dense[i % dense.length];
    cum.push(cum[i - 1] + Math.hypot(b[0] - a[0], b[1] - a[1]));
  }
  const length = cum.at(-1);
  const count = Math.max(8, Math.round(length / STEP_M));
  const ds = length / count;

  const pts = [];
  let seg = 0;
  for (let k = 0; k < count; k++) {
    const s = k * ds;
    while (cum[seg + 1] < s) seg++;
    const a = dense[seg], b = dense[(seg + 1) % dense.length];
    const f = (s - cum[seg]) / (cum[seg + 1] - cum[seg] || 1);
    pts.push([a[0] + (b[0] - a[0]) * f, a[1] + (b[1] - a[1]) * f]);
  }

  // Signed curvature (positive = left turn), lightly smoothed.
  const rawK = pts.map((b, i) => {
    const a = pts[(i - 1 + count) % count], c = pts[(i + 1) % count];
    const cross = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0]);
    const d = Math.hypot(b[0] - a[0], b[1] - a[1]) * Math.hypot(c[0] - b[0], c[1] - b[1]) * Math.hypot(c[0] - a[0], c[1] - a[1]);
    return d ? (2 * cross) / d : 0;
  });
  const W = 6;
  const curvature = rawK.map((_, i) => {
    let sum = 0;
    for (let j = -W; j <= W; j++) sum += rawK[(i + j + count) % count];
    return sum / (2 * W + 1);
  });

  // Corner-limited speed, then braking (backward) and traction (forward) passes around the loop.
  const { topSpeed, lateralLimit, brakeLimit, accelLimit } = VEHICLE;
  const v = curvature.map((k) => Math.min(topSpeed, Math.sqrt(lateralLimit / Math.max(Math.abs(k), 1e-4))));
  for (let pass = 0; pass < 2; pass++) {
    for (let i = count - 1; i >= 0; i--) {
      const next = v[(i + 1) % count];
      v[i] = Math.min(v[i], Math.sqrt(next * next + 2 * brakeLimit * ds));
    }
  }
  for (let pass = 0; pass < 2; pass++) {
    for (let i = 0; i < count; i++) {
      const prev = v[(i - 1 + count) % count];
      v[i] = Math.min(v[i], Math.sqrt(prev * prev + 2 * accelLimit(prev) * ds));
    }
  }

  return {
    length,
    ds,
    count,
    pts,
    curvature,
    speedProfile: v,
    toLatLng,
    latlngs: pts.map(toLatLng),
  };
}

/**
 * Start/finish gate: a line across the track at the first outline point, perpendicular to the
 * direction of travel. Returns endpoints in lat/lng plus data used for crossing tests.
 */
export function buildGate(outline, halfWidthM) {
  const { toXY, toLatLng } = localProjection(outline[0]);
  const circuit = buildCircuit(outline);
  const [ax, ay] = circuit.pts[circuit.count - 2];
  const [bx, by] = circuit.pts[2];
  const len = Math.hypot(bx - ax, by - ay) || 1;
  const dir = [(bx - ax) / len, (by - ay) / len];
  const normal = [-dir[1], dir[0]];
  const a = [normal[0] * halfWidthM, normal[1] * halfWidthM];
  const b = [-normal[0] * halfWidthM, -normal[1] * halfWidthM];
  return { toXY, dir, a, b, latlngs: [toLatLng(a), toLatLng(b)] };
}

/** Returns u in [0,1] along p→q where it crosses segment a→b, or null. */
function crossing(p, q, a, b) {
  const r = [q[0] - p[0], q[1] - p[1]];
  const s = [b[0] - a[0], b[1] - a[1]];
  const denom = r[0] * s[1] - r[1] * s[0];
  if (Math.abs(denom) < 1e-9) return null;
  const u = ((a[0] - p[0]) * s[1] - (a[1] - p[1]) * s[0]) / denom;
  const v = ((a[0] - p[0]) * r[1] - (a[1] - p[1]) * r[0]) / denom;
  return u >= 0 && u <= 1 && v >= 0 && v <= 1 ? u : null;
}

/** Counts laps when consecutive GPS fixes cross the start/finish gate in the direction of travel. */
export class LapTracker {
  constructor(outline, { halfWidthM = 15, minLapMs = 8000 } = {}) {
    this.gate = buildGate(outline, halfWidthM);
    this.minLapMs = minLapMs;
    this.reset();
  }

  reset() {
    this.laps = [];
    this.lapStart = null;
    this.prev = null;
  }

  update(fix) {
    const p = this.gate.toXY([fix.lat, fix.lng]);
    const prev = this.prev;
    this.prev = { p, t: fix.t };
    if (!prev || fix.t <= prev.t) return;

    const move = [p[0] - prev.p[0], p[1] - prev.p[1]];
    if (move[0] * this.gate.dir[0] + move[1] * this.gate.dir[1] <= 0) return;
    const u = crossing(prev.p, p, this.gate.a, this.gate.b);
    if (u == null) return;

    const t = prev.t + (fix.t - prev.t) * u;
    if (this.lapStart != null) {
      if (t - this.lapStart < this.minLapMs) return;
      this.laps.push(t - this.lapStart);
    }
    this.lapStart = t;
  }

  get lastLap() {
    return this.laps.at(-1) ?? null;
  }

  get bestLap() {
    return this.laps.length ? Math.min(...this.laps) : null;
  }
}
