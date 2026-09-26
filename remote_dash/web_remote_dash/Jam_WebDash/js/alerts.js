export const TIERS = {
  0: 'System',
  1: 'Alert',
  2: 'Warning',
  3: 'Emergency',
};

export async function loadAlertRules(url) {
  try {
    const res = await fetch(url, { cache: 'no-store' });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const spec = await res.json();
    return Array.isArray(spec.rules) ? spec.rules : [];
  } catch (err) {
    console.warn(`Could not load alert rules from ${url}:`, err);
    return [];
  }
}

function inRange(rule, value, active) {
  const [lo, hi] = rule.range ?? [];
  const h = active ? rule.hysteresis ?? 0 : 0;
  return (lo == null || value >= lo - h) && (hi == null || value <= hi + h);
}

let audio = null;

export function unlockAudio() {
  try {
    audio ??= new AudioContext();
    if (audio.state === 'suspended') audio.resume();
  } catch {
    audio = null;
  }
}

function chime(tier) {
  if (!audio || audio.state !== 'running') return;
  const beeps = tier >= 3 ? 3 : 1;
  for (let i = 0; i < beeps; i++) {
    const t = audio.currentTime + i * 0.22;
    const osc = audio.createOscillator();
    const gain = audio.createGain();
    osc.type = 'sine';
    osc.frequency.value = tier >= 3 ? 880 : 660;
    gain.gain.setValueAtTime(0.0001, t);
    gain.gain.exponentialRampToValueAtTime(0.18, t + 0.02);
    gain.gain.exponentialRampToValueAtTime(0.0001, t + 0.18);
    osc.connect(gain).connect(audio.destination);
    osc.start(t);
    osc.stop(t + 0.2);
  }
}

/** Evaluates threshold rules against the store and keeps the alert feed. Fires 'change'. */
export class AlertCenter extends EventTarget {
  #items = [];
  #ruleState = new Map();
  #seq = 0;

  constructor(rules, { format = (key, v) => String(v), cooldownMs = 20_000, limit = 100 } = {}) {
    super();
    this.rules = rules;
    this.format = format;
    this.cooldownMs = cooldownMs;
    this.limit = limit;
    this.muted = false;
  }

  get items() {
    return this.#items;
  }

  evaluate(store, now = Date.now()) {
    for (const rule of this.rules) {
      const state = this.#ruleState.get(rule.id) ?? { active: false, alertId: null, lastRaised: -Infinity, checkedTo: -Infinity };
      // Peak rules catch short events that happen between uploads, not just the newest reading.
      const value = rule.peak ? store.maxSince(rule.channel, state.checkedTo) : store.get(rule.channel);
      if (rule.peak) state.checkedTo = store.now;
      this.#ruleState.set(rule.id, state);
      if (value == null) continue;
      const tripped = inRange(rule, value, state.active);

      if (tripped && !state.active) {
        state.active = true;
        if (now - state.lastRaised >= this.cooldownMs) {
          state.alertId = this.raise({
            tier: rule.tier,
            title: rule.title,
            message: rule.message,
            detail: this.format(rule.channel, value, true),
          }).id;
          state.lastRaised = now;
        }
      } else if (!tripped && state.active) {
        state.active = false;
        this.#resolve(state.alertId);
        state.alertId = null;
      }
      this.#ruleState.set(rule.id, state);
    }
  }

  /** Highest tier currently tripped on a channel, or -1. */
  channelTier(channel) {
    let tier = -1;
    for (const rule of this.rules) {
      if (rule.channel === channel && this.#ruleState.get(rule.id)?.active) tier = Math.max(tier, rule.tier);
    }
    return tier;
  }

  raise({ tier = 0, title, message = '', detail = '' }) {
    const item = { id: ++this.#seq, tier, title, message, detail, time: Date.now(), acked: tier === 0, cleared: false };
    this.#items.unshift(item);
    if (this.#items.length > this.limit) this.#items.length = this.limit;
    if (tier >= 2 && !this.muted) chime(tier);
    this.#emit();
    return item;
  }

  acknowledge(id) {
    const item = this.#items.find((a) => a.id === id);
    if (item && !item.acked) {
      item.acked = true;
      this.#emit();
    }
  }

  dismiss(id) {
    this.#items = this.#items.filter((a) => a.id !== id);
    this.#emit();
  }

  /** Drops everything that no longer needs attention. */
  clearHandled() {
    this.#items = this.#items.filter((a) => !a.acked && !a.cleared);
    this.#emit();
  }

  reset() {
    this.#items = [];
    this.#ruleState.clear();
    this.#emit();
  }

  unackedCount(tier) {
    return this.#items.reduce((n, a) => n + (a.tier === tier && !a.acked && !a.cleared ? 1 : 0), 0);
  }

  #resolve(id) {
    const item = this.#items.find((a) => a.id === id);
    if (item) {
      item.cleared = true;
      this.#emit();
    }
  }

  #emit() {
    this.dispatchEvent(new Event('change'));
  }
}
