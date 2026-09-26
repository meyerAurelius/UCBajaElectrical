/* global L */
import { buildCircuit, buildGate } from './track.js';
import { bearing } from './telemetry.js';

const TILE_LAYERS = {
  // Standard OSM tiles; darkened in CSS via .tiles-dark.
  dark: {
    url: 'https://tile.openstreetmap.org/{z}/{x}/{y}.png',
    options: {
      className: 'tiles-dark',
      maxNativeZoom: 19,
      maxZoom: 21,
      attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>',
    },
  },
  satellite: {
    url: 'https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}',
    options: { maxNativeZoom: 19, maxZoom: 21, attribution: 'Imagery &copy; Esri' },
  },
};

const CAR_ICON = L.divIcon({
  className: 'car-marker',
  html: `<span class="car-marker__pulse"></span>
         <span class="car-marker__body"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 3l7 17-7-4-7 4z"/></svg></span>`,
  iconSize: [34, 34],
  iconAnchor: [17, 17],
});

export class TrackMap {
  #map;
  #layers = {};
  #activeLayer = null;
  #car;
  #trail;
  #anim = null;
  #raf = 0;
  #current = null;
  #heading = 0;
  #lastUpdate = 0;

  constructor(el, { outline, gateHalfWidthM, onFollowChange }) {
    this.outline = outline;
    this.follow = true;
    this.onFollowChange = onFollowChange;

    this.#map = L.map(el, { zoomControl: false, zoomSnap: 0.25 });
    this.#map.attributionControl.setPrefix(false);
    L.control.zoom({ position: 'bottomright' }).addTo(this.#map);

    const smooth = buildCircuit(outline).latlngs;
    const ring = [...smooth.filter((_, i) => i % 2 === 0), smooth[0]];
    L.polyline(ring, { color: '#d71925', weight: 12, opacity: 0.2, interactive: false }).addTo(this.#map);
    L.polyline(ring, { color: '#f0484f', weight: 2.5, opacity: 0.95, interactive: false }).addTo(this.#map);
    L.polyline(buildGate(outline, gateHalfWidthM).latlngs, {
      color: '#ffffff', weight: 3, opacity: 0.85, dashArray: '4 4', interactive: false,
    }).addTo(this.#map);
    L.marker(outline[0], {
      icon: L.divIcon({ className: 'sf-marker', html: '<span>S/F</span>', iconSize: [36, 18], iconAnchor: [18, 26] }),
      interactive: false,
      keyboard: false,
    }).addTo(this.#map);

    this.#trail = L.polyline([], { color: '#ffcf48', weight: 3, opacity: 0.85, interactive: false }).addTo(this.#map);
    this.#car = L.marker(outline[0], { icon: CAR_ICON, interactive: false, keyboard: false, zIndexOffset: 1000 });

    this.#map.on('dragstart', () => this.setFollow(false));
    new ResizeObserver(() => this.#map.invalidateSize()).observe(el);
    this.recenter(false);
  }

  setLayer(name) {
    const spec = TILE_LAYERS[name] ?? TILE_LAYERS.dark;
    this.#layers[name] ??= L.tileLayer(spec.url, spec.options);
    if (this.#activeLayer) this.#map.removeLayer(this.#activeLayer);
    this.#activeLayer = this.#layers[name].addTo(this.#map);
  }

  setFollow(on) {
    if (this.follow === on) return;
    this.follow = on;
    if (on && this.#current) this.#map.panTo(this.#current);
    this.onFollowChange?.(on);
  }

  recenter(animate = true) {
    const points = [...this.outline];
    if (this.#current) points.push(this.#current);
    this.#map.fitBounds(L.latLngBounds(points).pad(0.35), { animate, maxZoom: 19 });
  }

  update(lat, lng) {
    const to = L.latLng(lat, lng);
    const now = performance.now();
    if (!this.#current) {
      this.#current = to;
      this.#car.setLatLng(to).addTo(this.#map);
    }
    const from = this.#current;
    if (from.distanceTo(to) > 0.3) {
      const delta = ((bearing(from, to) - (this.#heading % 360) + 540) % 360) - 180;
      this.#heading += delta;
    }
    const duration = Math.min(1500, Math.max(60, now - (this.#lastUpdate || now - 100)));
    this.#anim = { from, to, t0: now, duration };
    this.#lastUpdate = now;
    this.#raf ||= requestAnimationFrame(this.#step);
  }

  /** Draws the recent path, broken wherever GPS fixes are missing for well over the usual fix interval. */
  setTrail(points) {
    const gaps = points.slice(1).map((p, i) => p.t - points[i].t).sort((a, b) => a - b);
    const breakAfter = Math.max(2500, 3 * (gaps[gaps.length >> 1] ?? 0));
    const segments = [];
    let seg = [];
    points.forEach((p, i) => {
      if (i && p.t - points[i - 1].t > breakAfter) {
        segments.push(seg);
        seg = [];
      }
      seg.push([p.lat, p.lng]);
    });
    segments.push(seg);
    this.#trail.setLatLngs(segments);
  }

  reset() {
    cancelAnimationFrame(this.#raf);
    this.#raf = 0;
    this.#anim = null;
    this.#current = null;
    this.#lastUpdate = 0;
    this.#car.remove();
    this.#trail.setLatLngs([]);
  }

  #step = (ts) => {
    const { from, to, t0, duration } = this.#anim;
    const p = Math.min(1, Math.max(0, (ts - t0) / duration));
    this.#current = L.latLng(from.lat + (to.lat - from.lat) * p, from.lng + (to.lng - from.lng) * p);
    this.#car.setLatLng(this.#current);
    const body = this.#car.getElement()?.querySelector('.car-marker__body');
    if (body) body.style.transform = `rotate(${this.#heading}deg)`;
    if (this.follow) this.#map.panTo(this.#current, { animate: false });
    this.#raf = p < 1 ? requestAnimationFrame(this.#step) : 0;
  };
}
