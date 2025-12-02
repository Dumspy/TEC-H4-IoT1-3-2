import type { SensorReading } from '../types/index.js'
import { MAX_SENSORS_PER_DEVICE } from '../config/constants.js'

export const deviceReadings = new Map<string, Map<string, SensorReading>>()

export function addReading(deviceId: string, reading: SensorReading): boolean {
  const sensorKey = `${reading.sensor_x},${reading.sensor_y}`
  
  if (!deviceReadings.has(deviceId)) {
    deviceReadings.set(deviceId, new Map())
  }
  
  const readings = deviceReadings.get(deviceId)!
  
  readings.set(sensorKey, reading)
  
  if (readings.size > MAX_SENSORS_PER_DEVICE) {
    const sortedReadings = Array.from(readings.entries())
      .sort((a, b) => a[1].timestamp - b[1].timestamp)
    
    const oldestKey = sortedReadings[0]![0]
    readings.delete(oldestKey)
  }
  
  return readings.size >= 3
}

export function getRecentReadings(deviceId: string, count: number = 3): SensorReading[] {
  const readings = deviceReadings.get(deviceId)
  if (!readings) {
    return []
  }
  
  return Array.from(readings.values())
    .sort((a, b) => b.timestamp - a.timestamp)
    .slice(0, count)
}
