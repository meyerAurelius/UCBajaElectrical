/**
 * Polls the logging server: list_files -> open newest file -> forward only records not seen yet.
 * Handlers: onRecords(rawRecords[], { backlog, sessionId }), onStatus({ ok, latency?, detail?, error? }).
 * `backlog` is true for the history replayed from an existing file on first connect.
 */
export class LiveSource {
  #timer = 0;
  #abort = null;
  #running = false;
  #file = null;
  #count = 0;

  constructor(config, handlers) {
    this.config = config;
    this.handlers = handlers;
  }

  start() {
    this.#running = true;
    this.#file = null;
    this.#count = 0;
    this.#poll();
  }

  stop() {
    this.#running = false;
    clearTimeout(this.#timer);
    this.#abort?.abort();
  }

  async #poll() {
    if (!this.#running) return;
    const { baseUrl, pollMs, initialBacklog } = this.config;
    const started = performance.now();

    try {
      const list = await this.#getJson(`${baseUrl}/list_files`);
      const first = list?.files?.[0] ?? list?.[0];
      const name = typeof first === 'string' ? first : first?.name;
      if (!name) throw new Error('No log files on server');

      const doc = await this.#getJson(`${baseUrl}/open?name=${encodeURIComponent(name)}`);
      const envelope = doc?.received_json ?? doc;
      const records = envelope?.Logging_Data;
      if (!Array.isArray(records)) throw new Error('Unexpected response format');
      const sessionId = typeof envelope.session_id === 'string' ? envelope.session_id : null;

      const backlog = this.#file === null;
      let fresh;
      if (name !== this.#file || records.length < this.#count) {
        fresh = backlog ? records.slice(-initialBacklog) : records;
        this.#file = name;
      } else {
        fresh = records.slice(this.#count);
      }
      this.#count = records.length;

      if (!this.#running) return;
      if (fresh.length) this.handlers.onRecords(fresh, { backlog, sessionId });
      this.handlers.onStatus({ ok: true, latency: Math.round(performance.now() - started), detail: name });
    } catch (err) {
      if (!this.#running) return;
      const error = err.name === 'AbortError' ? 'Request timed out' : err.message || 'Network error';
      this.handlers.onStatus({ ok: false, error });
    }

    this.#timer = setTimeout(() => this.#poll(), Math.max(0, pollMs - (performance.now() - started)));
  }

  async #getJson(url) {
    this.#abort = new AbortController();
    const timeout = setTimeout(() => this.#abort.abort(), this.config.timeoutMs);
    try {
      const res = await fetch(url, { cache: 'no-store', signal: this.#abort.signal });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      return await res.json();
    } finally {
      clearTimeout(timeout);
    }
  }
}
