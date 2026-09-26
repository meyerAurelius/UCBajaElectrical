import { CONFIG } from './config.js';

/*
 * Accepted record shapes (all inside Logging_Data):
 *   Firmware (BAJA/digital_dash/gps_module):
 *     { timestamp, tickstamp, type, device_id, data }  with type one of
 *     gps [lat, lon, alt, speed_kmh, satellites, accuracy_m] | imu [x, y, z, gyro_x, gyro_y, gyro_z]
 *     temp [°C] | rpm [rpm] | voltage [V] | pressure [pressure, temperature, raw]
 *   Legacy (Crisp_Python/Data_Falsifier.py):
 *     EnigneTemp | EngineRPM | TransTemp | TransRPM | Voltage   with data [value]
 * Values may be null when a sensor has no reading.
 */
const TYPE_ALIASES = {
  gps: 'gps',
  imu: 'imu', accel: 'imu', accelerometer: 'imu',
  temp: 'temp', temperature: 'temp',
  rpm: 'rpm',
  voltage: 'voltage', batteryvoltage: 'voltage',
  pressure: 'pressure',
  speed: 'speed',
  enginetemp: 'engineTemp', enignetemp: 'engineTemp',
  transtemp: 'transTemp', transmissiontemp: 'transTemp', cvttemp: 'transTemp',
  enginerpm: 'primaryRpm', primaryrpm: 'primaryRpm',
  transrpm: 'secondaryRpm', secondaryrpm: 'secondaryRpm',
};

const EMPTY = Object.freeze([]);
const EARLIEST_VALID = Date.UTC(2020, 0, 1);
const MAX_CLOCK_AHEAD_MS = 5 * 60_000;
// Below clutch engagement the CVT slips, so primary/secondary isn't a drive ratio.
const CLUTCH_ENGAGED_RPM = 2000;
const MIN_SECONDARY_RPM = 150;

const toNumber = (v) => (v === null || v === undefined || v === '' ? NaN : Number(v));

function routeDevice(kind, deviceId) {
  const id = String(deviceId ?? '').toLowerCase();
  const mapped = CONFIG.devices[id];
  if (mapped) return mapped;
  if (kind === 'temp') {
    if (/eng/.test(id)) return 'engineTemp';
    return 'transTemp'; // the original dash showed bare `temp` as transmission temperature
  }
  if (kind === 'rpm') return /sec|trans|cvt|driven/.test(id) ? 'secondaryRpm' : 'primaryRpm';
  return kind;
}

function axisValue(data, spec) {
  const neg = spec.startsWith('-');
  const v = data['xyz'.indexOf(spec.replace('-', ''))];
  return Number.isFinite(v) ? (neg ? -v : v) : NaN;
}

function pressureLabel(deviceId) {
  const id = String(deviceId ?? '').toLowerCase();
  if (id.includes('oil')) return 'Oil Pressure';
  if (id.includes('brake')) return 'Brake Pressure';
  if (id.includes('fuel')) return 'Fuel Pressure';
  return 'Pressure';
}

export function parseTimestamp(ts) {
  if (typeof ts === 'number') return ts > 1e12 ? ts : ts > 1e9 ? ts * 1000 : NaN;
  if (typeof ts !== 'string') return NaN;
  const s = ts.trim();
  // Only real calendar timestamps; the firmware may also send seconds-since-boot like "123.456".
  if (!/^\d{4}-\d{2}-\d{2}/.test(s)) return NaN;
  return Date.parse(s.replace(' ', 'T').replace(/(\.\d{3})\d+/, '$1'));
}

/** Converts one raw record into { t, tick, values, position, device, kind } or null. */
export function normalizeRecord(raw) {
  if (!raw || typeof raw !== 'object') return null;
  const alias = TYPE_ALIASES[String(raw.type ?? '').toLowerCase().replace(/[^a-z0-9]/g, '')];
  if (!alias) return null;

  const kind = routeDevice(alias, raw.device_id);
  const data = (Array.isArray(raw.data) ? raw.data : [raw.data]).map(toNumber);
  const ok = (i) => Number.isFinite(data[i]);
  const values = {};
  let position = null;

  switch (kind) {
    case 'gps': {
      const [lat, lng, alt, speed, sats, accuracy] = data;
      const noFix = ok(4) && sats === 0;
      if (ok(4)) values.gpsSats = sats;
      if (ok(5)) values.gpsAccuracy = accuracy;
      if (!noFix && ok(0) && ok(1) && Math.abs(lat) <= 90 && Math.abs(lng) <= 180 && !(lat === 0 && lng === 0)) {
        position = { lat, lng };
        if (ok(2)) values.altitude = alt;
        if (ok(3) && speed >= 0) values.speed = speed;
      }
      break;
    }
    case 'imu': {
      const { axes } = CONFIG.imu;
      const lon = axisValue(data, axes.longitudinal);
      const lat = axisValue(data, axes.lateral);
      const vert = axisValue(data, axes.vertical);
      if (Number.isFinite(lon) && Number.isFinite(lat)) {
        values.gx = lon;
        values.gy = lat;
        values.gTotal = Math.hypot(lon, lat);
      }
      if (Number.isFinite(vert)) values.gz = vert;
      const yaw = axisValue(data.slice(3), axes.vertical);
      if (Number.isFinite(yaw)) values.yawRate = yaw;
      break;
    }
    case 'pressure':
      if (ok(0)) values.pressure = data[0];
      break;
    default:
      if (ok(0)) values[kind] = data[0];
  }

  if (!position && !Object.keys(values).length) return null;
  return {
    t: parseTimestamp(raw.timestamp),
    tick: toNumber(raw.tickstamp),
    values,
    position,
    kind,
    device: String(raw.device_id ?? ''),
  };
}

export function haversine(a, b) {
  const R = 6_371_000;
  const rad = Math.PI / 180;
  const dLat = (b.lat - a.lat) * rad;
  const dLng = (b.lng - a.lng) * rad;
  const h = Math.sin(dLat / 2) ** 2 + Math.cos(a.lat * rad) * Math.cos(b.lat * rad) * Math.sin(dLng / 2) ** 2;
  return 2 * R * Math.asin(Math.sqrt(h));
}

export function bearing(a, b) {
  const rad = Math.PI / 180;
  const y = Math.sin((b.lng - a.lng) * rad) * Math.cos(b.lat * rad);
  const x = Math.cos(a.lat * rad) * Math.sin(b.lat * rad)
    - Math.sin(a.lat * rad) * Math.cos(b.lat * rad) * Math.cos((b.lng - a.lng) * rad);
  return (Math.atan2(y, x) / rad + 360) % 360;
}

export class TelemetryStore {
  constructor({ historyMs, trackLimit = 3000, logLimit = 300_000 }) {
    this.historyMs = historyMs;
    this.trackLimit = trackLimit;
    this.logLimit = logLimit;
    this.reset();
  }

  reset() {
    this.latest = new Map();
    this.series = new Map();
    this.peaks = new Map();
    this.lows = new Map();
    this.labels = new Map();
    this.track = [];
    this.log = [];
    this.distanceM = 0;
    this.movingMs = 0;
    this.now = 0;
    this.lastArrival = 0;
    this.avgGapMs = 0;
    this.recordCount = 0;
    this.imuDevice = CONFIG.imu.device;
    this.lastExplicitSpeedAt = -Infinity;
    this.tipWindow = [];
  }

  /**
   * Ingests raw records. Returns { accepted, fixes } where fixes are new GPS points in time order.
   * Backlog records (history replayed on connect) don't count as the car being live.
   */
  ingest(rawRecords, { backlog = false } = {}) {
    const arrival = Date.now();
    const recs = [];
    let maxTick = -Infinity;
    for (const raw of rawRecords) {
      const rec = normalizeRecord(raw);
      if (!rec) continue;
      if (rec.kind === 'imu') {
        this.imuDevice ??= rec.device;
        if (rec.device !== this.imuDevice) continue;
      }
      if (!(rec.t >= EARLIEST_VALID && rec.t <= arrival + MAX_CLOCK_AHEAD_MS)) rec.t = NaN;
      if (Number.isFinite(rec.tick)) maxTick = Math.max(maxTick, rec.tick);
      recs.push(rec);
    }

    // Without a usable clock, place readings relative to each other using the ms-since-boot tickstamp.
    for (const rec of recs) {
      if (Number.isFinite(rec.t)) continue;
      rec.t = Number.isFinite(rec.tick) && maxTick - rec.tick < 600_000 ? arrival - (maxTick - rec.tick) : arrival;
    }
    recs.sort((a, b) => a.t - b.t);

    const fixes = [];
    for (const rec of recs) {
      const { t, values } = rec;
      if ('speed' in values) this.lastExplicitSpeedAt = t;
      if ('pressure' in values) this.labels.set('pressure', pressureLabel(rec.device));
      for (const [key, v] of Object.entries(values)) this.set(key, v, t);
      if ('primaryRpm' in values || 'secondaryRpm' in values) this.#deriveRatio(t);
      if (rec.kind === 'imu') this.#deriveTip(values, t);
      if (rec.position) fixes.push(this.#addFix(rec.position, t));
    }

    if (recs.length) {
      this.recordCount += recs.length;
      if (!backlog) {
        if (this.lastArrival) {
          const gap = arrival - this.lastArrival;
          this.avgGapMs = this.avgGapMs ? this.avgGapMs * 0.7 + gap * 0.3 : gap;
        }
        this.lastArrival = arrival;
      }
    }
    return { accepted: recs.length, fixes };
  }

  set(key, v, t, { log = true } = {}) {
    this.latest.set(key, { v, t });
    if (t > this.now) this.now = t;

    let list = this.series.get(key);
    if (!list) this.series.set(key, (list = []));
    list.push({ t, v });
    const cutoff = this.now - this.historyMs;
    let drop = 0;
    while (drop < list.length && list[drop].t < cutoff) drop++;
    if (drop) list.splice(0, drop);

    if (v > (this.peaks.get(key) ?? -Infinity)) this.peaks.set(key, v);
    if (v < (this.lows.get(key) ?? Infinity)) this.lows.set(key, v);
    if (log && this.log.length < this.logLimit) this.log.push([t, key, v]);
  }

  has(key) {
    return this.latest.has(key);
  }

  get(key) {
    return this.latest.get(key)?.v ?? null;
  }

  history(key) {
    return this.series.get(key) ?? EMPTY;
  }

  peak(key) {
    return this.peaks.get(key) ?? null;
  }

  /** Highest value recorded after time `t`, or null if nothing new arrived. */
  maxSince(key, t) {
    const data = this.history(key);
    let max = null;
    for (let i = data.length - 1; i >= 0 && data[i].t > t; i--) {
      if (max == null || data[i].v > max) max = data[i].v;
    }
    return max;
  }

  low(key) {
    return this.lows.get(key) ?? null;
  }

  label(key) {
    return this.labels.get(key) ?? CONFIG.channels[key]?.label ?? key;
  }

  /** How long without data before the car counts as stale. Adapts to the car's upload interval. */
  get staleAfterMs() {
    return Math.max(CONFIG.staleMs, this.avgGapMs * 2.5);
  }

  /** Least-squares slope in units per minute over the last `windowMs`, or null if too little data. */
  ratePerMin(key, windowMs = 60_000) {
    const data = this.history(key);
    if (data.length < 4) return null;
    const tEnd = data.at(-1).t;
    let n = 0, sx = 0, sy = 0, sxx = 0, sxy = 0, tStart = tEnd;
    for (let i = data.length - 1; i >= 0 && tEnd - data[i].t <= windowMs; i--) {
      const x = (data[i].t - tEnd) / 60_000;
      const y = data[i].v;
      n++; sx += x; sy += y; sxx += x * x; sxy += x * y;
      tStart = data[i].t;
    }
    if (n < 4 || tEnd - tStart < 10_000) return null;
    const denom = n * sxx - sx * sx;
    return denom ? (n * sxy - sx * sy) / denom : null;
  }

  get avgSpeedKmh() {
    return this.movingMs > 5000 ? (this.distanceM / (this.movingMs / 1000)) * 3.6 : null;
  }

  toCSV() {
    const lines = ['timestamp,channel,value'];
    for (const [t, key, v] of this.log) lines.push(`${new Date(t).toISOString()},${key},${v}`);
    return lines.join('\n');
  }

  #deriveRatio(t) {
    const primary = this.get('primaryRpm');
    const secondary = this.get('secondaryRpm');
    if (primary >= CLUTCH_ENGAGED_RPM && secondary > MIN_SECONDARY_RPM) this.set('cvtRatio', primary / secondary, t, { log: false });
  }

  #deriveTip({ gx, gy, gz }, t) {
    if (![gx, gy, gz].every(Number.isFinite)) return;
    // Airborne: no ground contact, so no tipping moment and the angles are meaningless.
    if (Math.hypot(gx, gy, gz) < 0.5) return;

    const s = CONFIG.stability;
    const rollLimit = s.trackWidthM / 2 / s.cgHeightM;
    const limitLong = gx >= 0 ? s.cgToRearAxleM : s.wheelbaseM - s.cgToRearAxleM;
    const vert = Math.max(gz, 0.05);
    const raw = 100 * Math.max(Math.abs(gy) / vert / rollLimit, Math.abs(gx) / vert / (limitLong / s.cgHeightM));

    const win = this.tipWindow;
    win.push({ t, v: Math.min(raw, 200) });
    while (win.length > 1 && win[0].t < t - s.sustainMs) win.shift();
    const deg = 180 / Math.PI;
    this.set('tipRisk', Math.min(...win.map((w) => w.v)), t, { log: false });
    this.set('rollAngle', Math.atan2(gy, gz) * deg, t, { log: false });
    this.set('pitchAngle', Math.atan2(gx, gz) * deg, t, { log: false });
  }

  #addFix(pos, t) {
    const fix = { lat: pos.lat, lng: pos.lng, t };
    const prev = this.track.at(-1);
    if (prev && t > prev.t) {
      const d = haversine(prev, fix);
      const dt = (t - prev.t) / 1000;
      const plausible = d / dt < 40; // reject GPS jumps faster than ~144 km/h
      if (plausible) {
        this.distanceM += d;
        if (d / dt > 0.5 && dt < 30) this.movingMs += dt * 1000;
        if (d > 2) this.set('heading', bearing(prev, fix), t, { log: false });
      }
      if (plausible && t - this.lastExplicitSpeedAt > 2000 && dt > 0.05 && dt < 30) {
        const raw = (d / dt) * 3.6;
        const prevSpeed = this.get('speed');
        this.set('speed', prevSpeed == null ? raw : prevSpeed * 0.5 + raw * 0.5, t, { log: false });
      }
    }
    this.track.push(fix);
    if (this.track.length > this.trackLimit) this.track.splice(0, this.track.length - this.trackLimit);
    if (this.log.length < this.logLimit - 1) this.log.push([t, 'lat', fix.lat], [t, 'lng', fix.lng]);
    return fix;
  }
}

export function formatValue(key, v, withUnit = false) {
  if (v == null || !Number.isFinite(v)) return '--';
  const meta = CONFIG.channels[key];
  const text = v.toFixed(meta?.decimals ?? 1);
  if (!withUnit || !meta?.unit) return text;
  return meta.unit.startsWith(':') ? `${text}${meta.unit}` : `${text} ${meta.unit}`;
}

export function formatLap(ms) {
  if (ms == null || !Number.isFinite(ms) || ms < 0) return '--:--.-';
  const totalTenths = Math.floor(ms / 100);
  const minutes = Math.floor(totalTenths / 600);
  const seconds = Math.floor((totalTenths % 600) / 10);
  return `${minutes}:${String(seconds).padStart(2, '0')}.${totalTenths % 10}`;
}

export function formatElapsed(ms) {
  const s = Math.max(0, Math.floor(ms / 1000));
  const pad = (n) => String(n).padStart(2, '0');
  return `${pad(Math.floor(s / 3600))}:${pad(Math.floor((s % 3600) / 60))}:${pad(s % 60)}`;
}
