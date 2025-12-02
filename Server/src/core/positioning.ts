import type { Position, SensorReading } from '../types/index.js'
import { MAX_READING_TIME_DIFF_MS } from '../config/constants.js'
import { getSensorPosition } from './sensors.js'

export const devicePositions = new Map<string, Position & { timestamp: number }>()

export function rssiToDistance(rssi: number): number {
  const txPower = -59
  const n = 2.0
  return Math.pow(10, (txPower - rssi) / (10 * n))
}

export function trilaterate(readings: SensorReading[]): Position | null {
  if (readings.length !== 3) {
    return null
  }

  const timestamps = readings.map(r => r.timestamp)
  const maxTime = Math.max(...timestamps)
  const minTime = Math.min(...timestamps)
  
  if (maxTime - minTime > MAX_READING_TIME_DIFF_MS) {
    return null
  }

  const r1 = readings[0]!
  const r2 = readings[1]!
  const r3 = readings[2]!
  
  const d1 = rssiToDistance(r1.rssi)
  const d2 = rssiToDistance(r2.rssi)
  const d3 = rssiToDistance(r3.rssi)

  const sensor1Key = `${r1.sensor_x},${r1.sensor_y}`
  const sensor2Key = `${r2.sensor_x},${r2.sensor_y}`
  const sensor3Key = `${r3.sensor_x},${r3.sensor_y}`

  const sensor1Pos = getSensorPosition(sensor1Key) || { x: r1.sensor_x, y: r1.sensor_y }
  const sensor2Pos = getSensorPosition(sensor2Key) || { x: r2.sensor_x, y: r2.sensor_y }
  const sensor3Pos = getSensorPosition(sensor3Key) || { x: r3.sensor_x, y: r3.sensor_y }

  const x1 = sensor1Pos.x, y1 = sensor1Pos.y
  const x2 = sensor2Pos.x, y2 = sensor2Pos.y
  const x3 = sensor3Pos.x, y3 = sensor3Pos.y

  const A = 2 * (x2 - x1)
  const B = 2 * (y2 - y1)
  const C = d1 * d1 - d2 * d2 - x1 * x1 + x2 * x2 - y1 * y1 + y2 * y2
  const D = 2 * (x3 - x2)
  const E = 2 * (y3 - y2)
  const F = d2 * d2 - d3 * d3 - x2 * x2 + x3 * x3 - y2 * y2 + y3 * y3

  const denominator = A * E - B * D
  
  if (Math.abs(denominator) < 0.001) {
    return null
  }

  const x = (C * E - F * B) / denominator
  const y = (A * F - D * C) / denominator

  return { x, y }
}
