export interface WifiSniffPayload {
  device_id: string
  rssi: number
  sensor_x: number
  sensor_y: number
  timestamp: string
}

export interface Position {
  x: number
  y: number
}

export interface SensorReading {
  sensor_x: number
  sensor_y: number
  rssi: number
  timestamp: number
}

export interface Bounds {
  minX: number
  maxX: number
  minY: number
  maxY: number
}
