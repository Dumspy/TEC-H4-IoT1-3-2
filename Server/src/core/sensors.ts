import type { Position, Bounds, SensorReading } from '../types/index.js'
import { BOUNDARY_PADDING } from '../config/constants.js'

export const knownSensors = new Map<string, { x: number, y: number }>()

export function registerSensor(reading: SensorReading): void {
  const sensorKey = `${reading.sensor_x},${reading.sensor_y}`
  if (!knownSensors.has(sensorKey)) {
    knownSensors.set(sensorKey, { x: reading.sensor_x, y: reading.sensor_y })
  }
}

export function getSensorBounds(): Bounds | null {
  if (knownSensors.size === 0) {
    return null
  }
  
  const sensors = Array.from(knownSensors.values())
  const minX = Math.min(...sensors.map(s => s.x))
  const maxX = Math.max(...sensors.map(s => s.x))
  const minY = Math.min(...sensors.map(s => s.y))
  const maxY = Math.max(...sensors.map(s => s.y))
  
  return {
    minX: minX - BOUNDARY_PADDING,
    maxX: maxX + BOUNDARY_PADDING,
    minY: minY - BOUNDARY_PADDING,
    maxY: maxY + BOUNDARY_PADDING
  }
}

export function isWithinBounds(position: Position, bounds: Bounds): boolean {
  return position.x >= bounds.minX && 
         position.x <= bounds.maxX && 
         position.y >= bounds.minY && 
         position.y <= bounds.maxY
}
