// Central place for everything a team member might want to tweak between events.

export const CONFIG = {
  eventName: 'UCalgary Baja SAE',

  live: {
    // Colin's test server. Swap for the Raspberry Pi (http://192.168.0.67) or the competition middleman.
    baseUrl: 'https://baja.403587.xyz/logs/json_received',
    pollMs: 1000,
    timeoutMs: 4000,
    // How many records to replay from an existing log file on first connect.
    initialBacklog: 200,
  },

  demo: { hz: 10 },

  track: {
    // Start/finish is the first point. Points are joined in order and closed automatically.
    outline: [
      [51.083149, -114.130161],
      [51.082957, -114.129689],
      [51.082695, -114.130145],
      [51.083051, -114.130440],
    ],
    // Half the width of the start/finish line used for lap timing.
    gateHalfWidthM: 15,
  },

  // Explicit device_id -> channel routing for generic `temp` and `rpm` readings. Anything not listed
  // falls back to name matching ("eng" -> engine, "trans"/"cvt"/"sec" -> transmission / secondary).
  devices: {
    eng_temp_1: 'engineTemp',
    eng_rpm_1: 'primaryRpm',
  },

  imu: {
    // Which IMU device_id drives the G-force panel. null = first IMU that reports.
    device: null,
    // How the sensor is mounted. Prefix with "-" to flip an axis, e.g. longitudinal: '-y'.
    // Parked on flat ground, vertical must read about +1 g and longitudinal must go positive when accelerating.
    axes: { longitudinal: 'x', lateral: 'y', vertical: 'z' },
  },

  // Tip-over estimate. Tip Risk is 100% when the inside (or front/rear) wheels would lift:
  // sideways G / vertical G = (track / 2) / CG height, and likewise fore-aft against the axle distances.
  // Placeholder dimensions: measure the real car (CG height from a tilt test) and update.
  stability: {
    trackWidthM: 1.35,
    wheelbaseM: 1.45,
    cgHeightM: 0.55,
    cgToRearAxleM: 0.6,
    // Risk has to hold for this long to count, so single bumps and landings don't trip it.
    sustainMs: 250,
  },

  historyMs: 60_000,
  // The car is flagged stale after max(staleMs, 2.5 x its observed upload interval).
  staleMs: 5_000,

  // Every channel below is fed by a reading type defined in the car firmware
  // (BAJA/digital_dash/gps_module/main/*_sensor_reading.c) or the legacy Data_Falsifier format,
  // or is calculated from those readings.
  channels: {
    // gps: [lat, lon, alt, speed, satellites, accuracy]
    speed:        { label: 'Speed',          unit: 'km/h', decimals: 0, min: 0,   max: 60 },
    altitude:     { label: 'Altitude',       unit: 'm',    decimals: 0 },
    gpsSats:      { label: 'GPS Satellites', unit: 'sats', decimals: 0 },
    gpsAccuracy:  { label: 'GPS Accuracy',   unit: 'm',    decimals: 1 },
    // rpm (eng_rpm_1) / EngineRPM, TransRPM
    primaryRpm:   { label: 'Engine RPM',     unit: 'rpm',  decimals: 0, min: 0,   max: 4000, redline: 3800 },
    secondaryRpm: { label: 'Secondary RPM',  unit: 'rpm',  decimals: 0, min: 0,   max: 4000 },
    cvtRatio:     { label: 'CVT Ratio',      unit: ':1',   decimals: 2, min: 0.5, max: 4 },
    // temp (eng_temp_1) / EnigneTemp, TransTemp
    engineTemp:   { label: 'Engine Temp',    unit: '°C',   decimals: 0, min: 20,  max: 150 },
    transTemp:    { label: 'Trans Temp',     unit: '°C',   decimals: 0, min: 20,  max: 130 },
    // voltage (batt_volt_1) / Voltage
    voltage:      { label: 'Battery',        unit: 'V',    decimals: 2 },
    // pressure: [pressure, temperature, raw]; label is taken from device_id (e.g. oil_pressure_1)
    pressure:     { label: 'Pressure',       unit: 'kPa',  decimals: 0 },
    // imu: [x, y, z, gyro_x, gyro_y, gyro_z] in g (MPU6050, +/-8 g range)
    gx:           { label: 'Longitudinal',   unit: 'g',    decimals: 2 },
    gy:           { label: 'Lateral',        unit: 'g',    decimals: 2 },
    gz:           { label: 'Vertical',       unit: 'g',    decimals: 2 },
    gTotal:       { label: 'Combined',       unit: 'g',    decimals: 2 },
    yawRate:      { label: 'Yaw Rate',       unit: '°/s',  decimals: 0 },
    // Derived from imu x/y/z: see `stability`
    tipRisk:      { label: 'Tip Risk',       unit: '%',    decimals: 0 },
    rollAngle:    { label: 'Lean',           unit: '°',    decimals: 0 },
    pitchAngle:   { label: 'Pitch',          unit: '°',    decimals: 0 },
  },

  // Cards appear only once their channel has reported. `sub` picks the derived line under the value.
  metricCards: [
    { key: 'tipRisk',    sub: 'tip' },
    { key: 'engineTemp', sub: 'rate' },
    { key: 'transTemp',  sub: 'rate' },
    { key: 'voltage',    sub: 'low' },
    { key: 'cvtRatio',   sub: 'range' },
    { key: 'pressure',   sub: 'peak' },
    { key: 'altitude',   sub: 'range' },
    { key: 'gpsSats',    sub: 'accuracy' },
  ],

  trend: [
    { key: 'speed',        color: '#ffcf48' },
    { key: 'primaryRpm',   color: '#f0484f' },
    { key: 'secondaryRpm', color: '#a855f7' },
    { key: 'engineTemp',   color: '#38bdf8' },
    { key: 'transTemp',    color: '#fb923c' },
  ],
};
