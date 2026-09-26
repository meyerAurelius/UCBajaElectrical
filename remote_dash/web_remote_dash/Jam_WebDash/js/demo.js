// Scripted demo car. Loops a race stint on the practice track so every panel, card and alert gets used:
//   grid (idle, rev-up) -> warm-up laps -> push laps -> in-lap -> pit stop (cool-down, battery swap) -> repeat
// Push laps vary in pace and each carries one event: engine over-rev with CVT slip, a near-tip corner,
// weak GPS with a short loss of fix, and a radio dropout that catches up in one batch. Every push lap has a
// jump on the fastest straight, the transmission heat-soaks into Warning then Emergency, and the battery
// drains into a low-voltage alert.
// Emits only the firmware reading formats (gps, imu, temp, pressure, rpm, voltage) and legacy TransTemp/TransRPM.
import { buildCircuit, VEHICLE } from './track.js';

const G = 9.80665;
const TIRE_CIRCUMFERENCE_M = 1.835; // 23" tire
const GEARBOX_RATIO = 8;
const CVT_LOW_RATIO = 3.9;
const IDLE_RPM = 1750;
const LAUNCH_RPM = 2600;
const GOVERNED_RPM = 3550;
const OVERREV_RPM = 3990;
const COAST_RPM = 2500;
const TRACK_ALTITUDE_M = 1085;

const PUSH_PACE = [1.0, 0.98, 1.02, 0.97, 0.99, 1.0];
const WARMUP_PACE = 0.75;
const INLAP_PACE = 0.6;
// Push lap (0-based) each one-off event happens on.
const EVENT_LAP = { overRev: 1, nearTip: 2, weakGps: 3, dropout: 4 };
const GRID_S = 6;
const PIT_HOLD_S = 15;
const BATTERY_SWAP_AT_S = 6;
const DROPOUT_S = 8;
const NO_FIX_S = 3;
const STOP_BEFORE_LINE_M = 8;

const noise = (amp) => (Math.random() * 2 - 1) * amp;
const clamp = (x, lo, hi) => Math.min(hi, Math.max(lo, x));
const argmax = (arr, score) => arr.reduce((best, v, i) => (score(v) > score(arr[best]) ? i : best), 0);

export class DemoSource {
  #timer = 0;

  constructor(track, handlers, { hz = 10 } = {}) {
    this.circuit = buildCircuit(track.outline);
    this.handlers = handlers;
    this.hz = hz;
    this.dt = 1 / hz;
    const c = this.circuit;
    this.fastS = argmax(c.speedProfile, (v) => v) * c.ds;
    this.apexS = argmax(c.curvature, Math.abs) * c.ds;
    this.stopS = c.length - STOP_BEFORE_LINE_M;
  }

  start() {
    this.sim = {
      time: 0,
      tick: 0,
      s: this.stopS,
      v: 0,
      primary: IDLE_RPM,
      engineTemp: 46,
      transTemp: 38,
      swapAt: 0,
      swapped: false,
      phase: 'grid',
      phaseTime: 0,
      lap: 0,
      stint: 0,
      until: {},
      done: new Set(),
      buffer: null,
      bootMs: performance.now(),
    };
    this.#timer = setInterval(() => this.#safeStep(), 1000 * this.dt);
    this.handlers.onStatus({ ok: true, latency: 42, detail: 'Simulator' });
  }

  stop() {
    clearInterval(this.#timer);
  }

  #safeStep() {
    try {
      this.#step();
    } catch (err) {
      console.error('Demo step failed:', err);
    }
  }

  #enter(phase) {
    Object.assign(this.sim, { phase, phaseTime: 0, lap: 0 });
    if (phase === 'pit') this.sim.swapped = false;
  }

  #active(name) {
    return (this.sim.until[name] ?? 0) > this.sim.time;
  }

  #trigger(name, seconds) {
    this.sim.until[name] = this.sim.time + seconds;
  }

  /** One-off event for the current stint, fired when the car passes `atS` on push lap `lap`. */
  #once(name, lap, atS, prevS, seconds) {
    const sim = this.sim;
    if (sim.phase !== 'push' || sim.lap !== lap || sim.done.has(name)) return false;
    if (!this.#passed(atS, prevS)) return false;
    sim.done.add(name);
    this.#trigger(name, seconds);
    return true;
  }

  #passed(atS, prevS) {
    const L = this.circuit.length;
    const x = ((atS % L) + L) % L;
    return prevS <= this.sim.s ? prevS < x && this.sim.s >= x : prevS < x || this.sim.s >= x;
  }

  #onLineCrossed() {
    const sim = this.sim;
    sim.lap += 1;
    const warmupWraps = sim.stint === 0 ? 3 : 2; // the first crossing is from the grid box, not a lap
    if (sim.phase === 'warmup' && sim.lap >= warmupWraps) this.#enter('push');
    else if (sim.phase === 'push' && sim.lap >= PUSH_PACE.length) this.#enter('inlap');
  }

  #step() {
    const c = this.circuit;
    const sim = this.sim;
    const dt = this.dt;
    const { topSpeed, brakeLimit, accelLimit } = VEHICLE;
    sim.time += dt;
    sim.phaseTime += dt;
    sim.tick += 1;

    // ------------------------------------------------------------ phase and pace
    let pace = 0;
    let stopAt = null;
    switch (sim.phase) {
      case 'grid':
        if (sim.phaseTime >= GRID_S) this.#enter('warmup');
        break;
      case 'warmup':
        pace = WARMUP_PACE;
        break;
      case 'push':
        pace = PUSH_PACE[sim.lap] ?? 1;
        break;
      case 'inlap':
        pace = INLAP_PACE;
        if (sim.s > c.length / 2) stopAt = this.stopS;
        break;
      case 'pit':
        if (!sim.swapped && sim.phaseTime >= BATTERY_SWAP_AT_S) {
          sim.swapAt = sim.time;
          sim.swapped = true;
        }
        if (sim.phaseTime >= PIT_HOLD_S) {
          sim.stint += 1;
          sim.done.clear();
          this.#enter('warmup');
          pace = WARMUP_PACE;
        }
        break;
    }

    // ------------------------------------------------------------ motion
    const i = Math.floor(sim.s / c.ds) % c.count;
    let target = c.speedProfile[i] * pace * (1 + 0.02 * Math.sin(sim.time * 0.37));
    if (stopAt != null) target = Math.min(target, Math.sqrt(2 * 2.5 * Math.max(0, stopAt - sim.s)));
    const dv = clamp(target - sim.v, -brakeLimit * dt, accelLimit(sim.v) * dt);
    sim.v = Math.max(0, sim.v + dv);
    if (stopAt != null && stopAt - sim.s < 0.5 && sim.v < 0.5) {
      sim.v = 0;
      this.#enter('pit');
    }

    const prevS = sim.s;
    sim.s += sim.v * dt;
    if (sim.s >= c.length) {
      sim.s -= c.length;
      this.#onLineCrossed();
    }
    const frac = sim.s / c.length;

    // ------------------------------------------------------------ scripted events
    this.#once('overRev', EVENT_LAP.overRev, this.fastS - 25, prevS, 1.6);
    this.#once('nearTip', EVENT_LAP.nearTip, this.apexS - 3, prevS, 0.7);
    this.#once('noFix', EVENT_LAP.weakGps, c.length * 0.45, prevS, NO_FIX_S);
    if (this.#once('dropout', EVENT_LAP.dropout, c.length * 0.3, prevS, DROPOUT_S)) sim.buffer = [];
    if (sim.phase === 'push' && this.#passed(this.fastS, prevS)) {
      this.#trigger('air', 0.35);
      this.#trigger('land', 0.55);
    }
    const weakGps = sim.phase === 'push' && sim.lap === EVENT_LAP.weakGps;

    // ------------------------------------------------------------ IMU
    const accel = dv / dt;
    const onThrottle = sim.v > 0.3 && accel > -0.3;
    const curv = c.curvature[i];
    let ax = accel / G + noise(0.02);
    let ay = (-sim.v * sim.v * curv) / G + noise(0.03);
    let az = 1 + noise(0.03 + 0.05 * (sim.v / topSpeed));
    if (this.#active('nearTip')) {
      ay += Math.sign(ay || 1) * 0.2;
      az -= 0.04;
    }
    if (this.#active('air')) {
      ax = noise(0.03);
      ay = noise(0.03);
      az = 0.08 + noise(0.04);
    } else if (this.#active('land')) {
      az = 2.3 + noise(0.2);
      ax -= 0.25;
    }
    const yawDeg = (sim.v * curv * 180) / Math.PI;

    // ------------------------------------------------------------ drivetrain
    const secondary = (sim.v / TIRE_CIRCUMFERENCE_M) * 60 * GEARBOX_RATIO;
    let primaryTarget = sim.v < 0.3
      ? IDLE_RPM
      : clamp(secondary * CVT_LOW_RATIO, IDLE_RPM, onThrottle ? GOVERNED_RPM : COAST_RPM);
    if (sim.phase === 'grid' && sim.phaseTime > GRID_S - 1.5) primaryTarget = LAUNCH_RPM;
    if (this.#active('overRev')) primaryTarget = OVERREV_RPM;
    sim.primary += (primaryTarget - sim.primary) * Math.min(1, dt * 5);

    // ------------------------------------------------------------ temperatures and battery
    const load = onThrottle ? sim.v / topSpeed : 0.1;
    let transTarget = 62 + 10 * load;
    let transTau = 20;
    let engineTarget = 88 + 10 * load;
    if (sim.phase === 'push') {
      // Heat soak across the stint: Warning around push lap 4, Emergency at the end of lap 6.
      const p = (sim.lap + frac) / PUSH_PACE.length;
      transTarget = 72 + 50 * p ** 1.5;
      transTau = 4;
    } else if (sim.phase === 'inlap') {
      transTarget = 100;
      transTau = 8;
    } else if (sim.phase === 'pit') {
      transTarget = 70;
      transTau = 7;
      engineTarget = 78;
    }
    sim.transTemp += (transTarget - sim.transTemp) * (dt / transTau);
    sim.engineTemp += (engineTarget - sim.engineTemp) * (dt / 45);
    const voltage = Math.max(11.3, 12.85 - 0.008 * (sim.time - sim.swapAt) - 0.05 * (onThrottle ? load : 0)) + noise(0.01);
    const oilKpa = 95 + 0.085 * sim.primary - 0.6 * (sim.engineTemp - 80) + noise(3);

    // ------------------------------------------------------------ GPS
    // GPS error wanders slowly rather than jumping every fix; a weak fix wanders further.
    const wanderM = weakGps ? 2.5 : 0.3;
    sim.gpsError = (sim.gpsError ?? [0, 0]).map((e) => e * 0.95 + noise(wanderM * 0.3));
    const k = Math.floor(sim.s / c.ds) % c.count;
    const f = sim.s / c.ds - Math.floor(sim.s / c.ds);
    const a = c.pts[k], b = c.pts[(k + 1) % c.count];
    const [lat, lng] = c.toLatLng([
      a[0] + (b[0] - a[0]) * f + sim.gpsError[0],
      a[1] + (b[1] - a[1]) * f + sim.gpsError[1],
    ]);
    const gps = this.#active('noFix')
      ? [0, 0, 0, 0, 0, null]
      : [
        +lat.toFixed(7),
        +lng.toFixed(7),
        +(TRACK_ALTITUDE_M + 1.2 * Math.sin(frac * Math.PI * 2) + noise(weakGps ? 1.5 : 0.3)).toFixed(1),
        +Math.max(0, sim.v * 3.6 + noise(0.3)).toFixed(1),
        weakGps ? 5 + Math.round(Math.random()) : 9 + Math.round(Math.sin(sim.time / 30) + 1),
        +(weakGps ? 5 + Math.random() * 3 : 1.5 + Math.random() * 0.6).toFixed(1),
      ];

    // ------------------------------------------------------------ records
    const timestamp = new Date().toISOString();
    const tickstamp = Math.round(performance.now() - sim.bootMs);
    const rec = (type, deviceId, data) => ({ timestamp, tickstamp, type, device_id: deviceId, data });
    const round = (x, d = 3) => +x.toFixed(d);
    const records = [
      rec('gps', 'gps_1', gps),
      rec('imu', 'imu_accel_1', [round(ax), round(ay), round(az), round(noise(1.5), 1), round(noise(1.5), 1), round(yawDeg + noise(1.5), 1)]),
      rec('rpm', 'eng_rpm_1', [Math.round(sim.primary + noise(25))]),
      rec('TransRPM', '1', [Math.round(secondary > 50 ? secondary + noise(15) : secondary)]),
    ];
    if (sim.tick % 5 === 0) {
      records.push(
        rec('temp', 'eng_temp_1', [round(sim.engineTemp + noise(0.3), 1)]),
        rec('TransTemp', '1', [round(sim.transTemp + noise(0.3), 1)]),
        rec('voltage', 'batt_volt_1', [round(voltage, 2)]),
        rec('pressure', 'oil_pressure_1', [Math.round(oilKpa), round(sim.engineTemp - 4, 1), Math.round(oilKpa * 6.2)]),
      );
    }

    // Radio dropout: readings pile up on the car and arrive in one batch when the link returns.
    if (this.#active('dropout')) {
      (sim.buffer ??= []).push(...records);
    } else {
      const batch = sim.buffer?.length ? [...sim.buffer, ...records] : records;
      sim.buffer = null;
      this.handlers.onRecords(batch, { sessionId: 'demo_session' });
    }

    if (sim.tick % this.hz === 0) {
      this.handlers.onStatus({ ok: true, latency: Math.round(38 + Math.random() * 30), detail: 'Simulator' });
    }
  }
}
