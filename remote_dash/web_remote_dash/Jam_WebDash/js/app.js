import { CONFIG } from './config.js';
import { TelemetryStore, formatValue, formatLap, formatElapsed } from './telemetry.js';
import { LapTracker } from './track.js';
import { LiveSource } from './sources.js';
import { DemoSource } from './demo.js';
import { RadialGauge, TrendChart, GGDiagram, drawSparkline } from './widgets.js';
import { TrackMap } from './map.js';
import { AlertCenter, TIERS, loadAlertRules, unlockAudio } from './alerts.js';

const $ = (sel, root = document) => root.querySelector(sel);
const $$ = (sel, root = document) => [...root.querySelectorAll(sel)];
const CH = CONFIG.channels;
const TIER_COLORS = { 1: '#ffcf48', 2: '#fb923c', 3: '#f0484f' };
const SPARK_COLOR = '#e8e8ee';

// localStorage throws in some private/locked-down browser modes; preferences are optional.
const prefs = {
  get(key, fallback) {
    try {
      return localStorage.getItem(`ucbaja.dash.${key}`) ?? fallback;
    } catch {
      return fallback;
    }
  },
  set(key, value) {
    try {
      localStorage.setItem(`ucbaja.dash.${key}`, value);
    } catch {
      /* not persisted */
    }
  },
};

// ---------------------------------------------------------------- state

const store = new TelemetryStore({ historyMs: CONFIG.historyMs });
const laps = new LapTracker(CONFIG.track.outline, { halfWidthM: CONFIG.track.gateHalfWidthM });
const alerts = new AlertCenter(await loadAlertRules('ErrorCodes.json'), { format: formatValue });

const session = { id: null, carId: null, startedAt: 0, endedAt: 0, active: false, endedManually: false };
const link = { kind: null, status: null, carFresh: false };
let source = null;
let canvasesDirty = true;
let trailDirty = false;

// ---------------------------------------------------------------- widgets

const gauges = {
  speed: new RadialGauge($('#gauge-speed'), { ...CH.speed, step: 10, minorPerMajor: 5 }),
  primaryRpm: new RadialGauge($('#gauge-primary'), {
    ...CH.primaryRpm, label: 'Engine', step: 1000, minorPerMajor: 4, tickFormat: (v) => v / 1000,
  }),
  secondaryRpm: new RadialGauge($('#gauge-secondary'), {
    ...CH.secondaryRpm, label: 'Secondary', step: 1000, minorPerMajor: 4, tickFormat: (v) => v / 1000,
  }),
};

const metricTemplate = $('#tpl-metric');
const metrics = CONFIG.metricCards.map(({ key, sub }) => {
  const node = metricTemplate.content.firstElementChild.cloneNode(true);
  $('.metric__unit', node).textContent = CH[key].unit;
  $('#metrics').append(node);
  return {
    key,
    sub,
    el: node,
    label: $('.metric__label', node),
    num: $('.metric__num', node),
    subEl: $('.metric__sub', node),
    spark: $('.metric__spark canvas', node),
  };
});

const trend = new TrendChart(
  $('#trend-chart'),
  $('#trend-legend'),
  CONFIG.trend.map((s) => ({ ...s, label: CH[s.key].label, min: CH[s.key].min, max: CH[s.key].max })),
  { windowMs: CONFIG.historyMs, onToggle: () => (canvasesDirty = true) },
);

const gg = new GGDiagram($('#gg-chart'));

const map = new TrackMap($('#map'), {
  outline: CONFIG.track.outline,
  gateHalfWidthM: CONFIG.track.gateHalfWidthM,
  onFollowChange: (on) => $('#btn-follow').setAttribute('aria-pressed', String(on)),
});

// ---------------------------------------------------------------- data flow

function onRecords(records, { backlog = false, sessionId = null } = {}) {
  const { accepted, fixes } = store.ingest(records, { backlog });
  if (!accepted) return;
  if (sessionId) session.carId = sessionId;

  for (const fix of fixes) laps.update(fix);
  if (fixes.length) {
    const last = fixes.at(-1);
    map.update(last.lat, last.lng);
    trailDirty = true;
  }
  canvasesDirty = true;
  if (backlog) return;

  if (!session.active && !session.endedManually) startSession();
  alerts.evaluate(store);
}

function onStatus(status) {
  const prev = link.status;
  link.status = status;
  if (link.kind !== 'live') return;
  if (!status.ok && (!prev || prev.ok)) {
    alerts.raise({ tier: 2, title: 'Server unreachable', message: status.error, detail: new URL(CONFIG.live.baseUrl).host });
  } else if (status.ok && prev && !prev.ok) {
    alerts.raise({ tier: 0, title: 'Server reconnected', message: `Round trip ${status.latency} ms.` });
  }
}

function setSource(kind) {
  if (kind === link.kind) return;
  source?.stop();
  resetData();
  link.kind = kind;
  link.status = null;
  prefs.set('source', kind);
  document.body.dataset.source = kind;
  $$('[data-source]').forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.source === kind)));

  const handlers = { onRecords, onStatus };
  source = kind === 'demo'
    ? new DemoSource(CONFIG.track, handlers, CONFIG.demo)
    : new LiveSource(CONFIG.live, handlers);
  source.start();

  alerts.raise(kind === 'demo'
    ? { tier: 0, title: 'Demo mode', message: 'Showing simulated telemetry. These values are not from the car.' }
    : { tier: 0, title: 'Live mode', message: `Polling ${new URL(CONFIG.live.baseUrl).host} every ${CONFIG.live.pollMs / 1000}s.` });
}

// ---------------------------------------------------------------- session

function makeSessionId(d = new Date()) {
  const p = (n) => String(n).padStart(2, '0');
  return `UCB-${d.getFullYear()}${p(d.getMonth() + 1)}${p(d.getDate())}-${p(d.getHours())}${p(d.getMinutes())}`;
}

function startSession() {
  Object.assign(session, { id: makeSessionId(), startedAt: Date.now(), endedAt: 0, active: true, endedManually: false });
  alerts.raise({ tier: 0, title: 'Session started', message: `Recording as ${session.id}.` });
  renderSessionControls();
}

function endSession() {
  Object.assign(session, { active: false, endedAt: Date.now(), endedManually: true });
  alerts.raise({ tier: 0, title: 'Session ended', message: `${session.id} ran for ${formatElapsed(session.endedAt - session.startedAt)}.` });
  renderSessionControls();
}

function resetData() {
  store.reset();
  laps.reset();
  map.reset();
  alerts.reset();
  Object.assign(session, { id: null, carId: null, startedAt: 0, endedAt: 0, active: false, endedManually: false });
  link.carFresh = false;
  canvasesDirty = true;
  renderSessionControls();
}

function exportCsv() {
  const blob = new Blob([store.toCSV()], { type: 'text/csv' });
  const a = document.createElement('a');
  a.href = URL.createObjectURL(blob);
  a.download = `${session.id ?? 'ucbaja'}-telemetry.csv`;
  a.click();
  setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}

function renderSessionControls() {
  const btn = $('#btn-session');
  btn.textContent = session.active ? 'End session' : 'Start session';
  btn.classList.toggle('btn--primary', !session.active);
  $('#session-id').textContent = session.id ?? 'Inactive';
}

// ---------------------------------------------------------------- rendering

const setText = (el, text) => {
  if (el.textContent !== text) el.textContent = text;
};

function setChip(id, state, text, title = '') {
  const chip = $(id);
  chip.dataset.state = state;
  chip.title = title;
  setText($('.chip__value', chip), text);
}

function formatAge(ms) {
  if (ms < 1500) return 'just now';
  if (ms < 90_000) return `${Math.round(ms / 1000)}s ago`;
  return `${Math.round(ms / 60_000)} min ago`;
}

function renderStatus(now) {
  const s = link.status;
  if (link.kind === 'demo') setChip('#chip-server', 'demo', `Simulated · ${s?.latency ?? '--'} ms`);
  else if (!s) setChip('#chip-server', 'idle', 'Connecting…');
  else if (s.ok) setChip('#chip-server', 'ok', `Online · ${s.latency} ms`, s.detail);
  else setChip('#chip-server', 'error', 'Offline', s.error);

  const interval = store.avgGapMs ? `Uploads about every ${(store.avgGapMs / 1000).toFixed(1)}s` : '';
  if (!store.lastArrival) setChip('#chip-car', 'idle', store.recordCount ? 'History only' : 'Waiting for data');
  else {
    const age = now - store.lastArrival;
    if (age < store.staleAfterMs) setChip('#chip-car', 'ok', `Live · ${formatAge(age)}`, interval);
    else setChip('#chip-car', 'warn', `No data · ${formatAge(age)}`, interval);
  }
}

function metricSub(m) {
  const { key, sub } = m;
  const fmt = (v) => formatValue(key, v);
  switch (sub) {
    case 'rate': {
      const rate = store.ratePerMin(key);
      const trendText = rate == null ? '' : `${rate >= 0 ? '▲' : '▼'} ${Math.abs(rate).toFixed(1)}°/min · `;
      return `${trendText}max ${fmt(store.peak(key))}`;
    }
    case 'low':
      return `low ${fmt(store.low(key))}`;
    case 'peak':
      return `max ${fmt(store.peak(key))}`;
    case 'range':
      return `${fmt(store.low(key))} – ${fmt(store.peak(key))}`;
    case 'tip': {
      const roll = Math.abs(store.get('rollAngle') ?? 0);
      const pitch = Math.abs(store.get('pitchAngle') ?? 0);
      const angle = roll >= pitch ? `${roll.toFixed(0)}° lean` : `${pitch.toFixed(0)}° pitch`;
      return `${angle} · max ${fmt(store.peak(key))}%`;
    }
    case 'accuracy': {
      const acc = store.get('gpsAccuracy');
      return acc == null ? '' : `±${formatValue('gpsAccuracy', acc)} m`;
    }
    default:
      return '';
  }
}

function renderMetrics(stale) {
  let visible = 0;
  for (const m of metrics) {
    const has = store.has(m.key);
    m.el.hidden = !has;
    if (!has) continue;
    visible++;
    setText(m.label, store.label(m.key));
    setText(m.num, formatValue(m.key, store.get(m.key)));
    setText(m.subEl, metricSub(m));
    const tier = alerts.channelTier(m.key);
    if (m.el.dataset.tier !== String(tier)) m.el.dataset.tier = tier;
    m.el.classList.toggle('is-stale', stale);
  }
  $('#metrics-empty').hidden = visible > 0;
}

function renderText(now) {
  const date = new Date(now);
  setText($('#clock-time'), date.toLocaleTimeString('en-GB', { hour12: false }));
  setText($('#clock-date'), date.toLocaleDateString('en-CA', { weekday: 'short', day: '2-digit', month: 'short', year: 'numeric' }));
  setText($('#session-elapsed'), session.startedAt ? formatElapsed((session.active ? now : session.endedAt) - session.startedAt) : '00:00:00');
  setText($('#car-session'), session.carId ? `Car · ${session.carId}` : '');

  renderStatus(now);
  for (const [key, gauge] of Object.entries(gauges)) gauge.set(store.get(key));
  const hasPrimary = store.has('primaryRpm');
  const hasSecondary = store.has('secondaryRpm');
  $('#gauge-primary').hidden = !hasPrimary;
  $('#gauge-secondary').hidden = !hasSecondary;
  $('#gauge-pair').hidden = !hasPrimary && !hasSecondary;
  $('#gauge-pair').classList.toggle('is-single', hasPrimary !== hasSecondary);
  $('#stat-peak-rpm-wrap').hidden = !hasPrimary;

  const stale = store.lastArrival > 0 && now - store.lastArrival > store.staleAfterMs;
  renderMetrics(stale);

  setText($('#lap-number'), laps.lapStart == null ? '--' : String(laps.laps.length + 1));
  setText($('#lap-current'), laps.lapStart == null ? formatLap(null) : formatLap(store.now - laps.lapStart));
  setText($('#lap-last'), formatLap(laps.lastLap));
  setText($('#lap-best'), formatLap(laps.bestLap));
  $('#lap-last').classList.toggle('is-best', laps.laps.length > 1 && laps.lastLap === laps.bestLap);

  setText($('#stat-top-speed'), formatValue('speed', store.peak('speed')));
  setText($('#stat-avg-speed'), formatValue('speed', store.avgSpeedKmh));
  setText($('#stat-distance'), store.distanceM ? (store.distanceM / 1000).toFixed(2) : '--');
  setText($('#stat-laps'), String(laps.laps.length));
  setText($('#stat-peak-rpm'), formatValue('primaryRpm', store.peak('primaryRpm')));
  setText($('#stat-peak-g'), formatValue('gTotal', store.peak('gTotal')));
  $('#btn-export').disabled = store.log.length === 0;

  setText($('#g-total'), formatValue('gTotal', store.get('gTotal')));
  setText($('#g-long'), formatValue('gx', store.get('gx')));
  setText($('#g-lat'), formatValue('gy', store.get('gy')));
  setText($('#g-vert'), formatValue('gz', store.get('gz')));
  $('#g-yaw-wrap').hidden = !store.has('yawRate');
  setText($('#g-yaw'), formatValue('yawRate', store.get('yawRate'), true));
  $('#gg-empty').hidden = store.has('gx');
  $('#trend-empty').hidden = CONFIG.trend.some((s) => store.has(s.key));
  trend.updateLegend(store, formatValue);
}

function renderCanvases() {
  for (const m of metrics) {
    if (m.el.hidden) continue;
    const tier = alerts.channelTier(m.key);
    drawSparkline(m.spark, store.history(m.key), TIER_COLORS[tier] ?? SPARK_COLOR);
  }
  trend.draw(store);
  gg.draw(store);
  if (trailDirty) {
    map.setTrail(store.track.slice(-400));
    trailDirty = false;
  }
}

const alertTemplate = $('#tpl-alert');

function formatTime(ms) {
  return new Date(ms).toLocaleTimeString('en-GB', { hour12: false });
}

function renderAlerts() {
  const list = $('#alerts');
  const nodes = alerts.items.map((item) => {
    const li = alertTemplate.content.firstElementChild.cloneNode(true);
    li.dataset.id = item.id;
    li.dataset.tier = item.tier;
    li.classList.toggle('is-unacked', !item.acked && !item.cleared);
    li.classList.toggle('is-cleared', item.cleared);
    $('.alert__tier', li).textContent = item.cleared ? `${TIERS[item.tier]} · cleared` : TIERS[item.tier];
    $('.alert__title', li).textContent = item.detail ? `${item.title} · ${item.detail}` : item.title;
    $('.alert__msg', li).textContent = item.message;
    const time = $('.alert__time', li);
    time.dateTime = new Date(item.time).toISOString();
    time.textContent = formatTime(item.time);
    $('[data-action="ack"]', li).hidden = item.acked || item.cleared;
    return li;
  });
  list.replaceChildren(...nodes);
  $('#alerts-empty').hidden = nodes.length > 0;

  for (const count of $$('.alert-counts .count')) {
    const n = alerts.unackedCount(Number(count.dataset.tier));
    count.textContent = String(n);
    count.classList.toggle('has-items', n > 0);
  }
  document.body.classList.toggle('has-emergency', alerts.unackedCount(3) > 0);
  canvasesDirty = true;
}

let lastTextFrame = 0;
function frame(ts) {
  try {
    if (ts - lastTextFrame >= 100) {
      renderText(Date.now());
      lastTextFrame = ts;
    }
    if (canvasesDirty) {
      canvasesDirty = false;
      renderCanvases();
    }
  } catch (err) {
    console.error('Render error:', err);
  }
  requestAnimationFrame(frame);
}

function checkCarLink() {
  const now = Date.now();
  const fresh = store.lastArrival > 0 && now - store.lastArrival < store.staleAfterMs;
  if (link.carFresh && !fresh) {
    alerts.raise({
      tier: 1,
      title: 'Telemetry stalled',
      message: `No data from the car for ${Math.round(store.staleAfterMs / 1000)}s. Check the radio link.`,
    });
  } else if (!link.carFresh && fresh && session.active && now - session.startedAt > 2000) {
    alerts.raise({ tier: 0, title: 'Telemetry resumed', message: 'Receiving data from the car again.' });
  }
  link.carFresh = fresh;
  canvasesDirty = true;
}

// ---------------------------------------------------------------- controls

function toggleFullscreen() {
  const request = document.fullscreenElement
    ? document.exitFullscreen?.()
    : document.documentElement.requestFullscreen?.();
  request?.catch?.(() => {});
}

function setMapLayer(name) {
  map.setLayer(name);
  prefs.set('mapLayer', name);
  $$('[data-layer]').forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.layer === name)));
}

function setMuted(muted) {
  alerts.muted = muted;
  prefs.set('muted', muted ? '1' : '0');
  $('#btn-mute').setAttribute('aria-pressed', String(muted));
}

$$('[data-source]').forEach((b) => b.addEventListener('click', () => setSource(b.dataset.source)));
$$('[data-layer]').forEach((b) => b.addEventListener('click', () => setMapLayer(b.dataset.layer)));
$('#btn-follow').addEventListener('click', () => map.setFollow(!map.follow));
$('#btn-recenter').addEventListener('click', () => map.recenter());
$('#btn-fullscreen').addEventListener('click', toggleFullscreen);
$('#btn-mute').addEventListener('click', () => setMuted(!alerts.muted));
$('#btn-clear-alerts').addEventListener('click', () => alerts.clearHandled());
$('#btn-session').addEventListener('click', () => (session.active ? endSession() : startSession()));
$('#btn-export').addEventListener('click', exportCsv);
$('#btn-reset').addEventListener('click', () => {
  resetData();
  alerts.raise({ tier: 0, title: 'Dashboard reset', message: 'Cleared session data, laps and alerts.' });
});

$('#alerts').addEventListener('click', (e) => {
  const btn = e.target.closest('[data-action]');
  if (!btn) return;
  const id = Number(btn.closest('.alert').dataset.id);
  if (btn.dataset.action === 'ack') alerts.acknowledge(id);
  else alerts.dismiss(id);
});

document.addEventListener('keydown', (e) => {
  if (e.target.closest?.('input, textarea, select') || e.ctrlKey || e.metaKey || e.altKey) return;
  if (e.key === 'f' || e.key === 'F') toggleFullscreen();
});
document.addEventListener('pointerdown', unlockAudio, { once: true });

alerts.addEventListener('change', renderAlerts);
new ResizeObserver(() => (canvasesDirty = true)).observe($('.dash'));

// ---------------------------------------------------------------- boot

$('#event-name').textContent = CONFIG.eventName;
const pick = (value, allowed, fallback) => (allowed.includes(value) ? value : fallback);
setMapLayer(pick(prefs.get('mapLayer'), ['dark', 'satellite'], 'dark'));
setMuted(prefs.get('muted', '0') === '1');
renderAlerts();

const requested = new URLSearchParams(location.search).get('source');
setSource(pick(requested, ['live', 'demo'], pick(prefs.get('source'), ['live', 'demo'], 'live')));

setInterval(checkCarLink, 1000);
requestAnimationFrame(frame);
