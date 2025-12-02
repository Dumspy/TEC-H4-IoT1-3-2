import { devicePositions } from '../core/positioning.js'
import { deviceReadings } from '../core/readings.js'
import { broadcastUpdate } from '../services/websocket.js'
import { DEVICE_TIMEOUT_MS, CLEANUP_INTERVAL_MS } from '../config/constants.js'

export function cleanupStaleDevices(): void {
  const now = Date.now()
  let cleanedCount = 0
  
  for (const [deviceId, position] of devicePositions.entries()) {
    if (now - position.timestamp > DEVICE_TIMEOUT_MS) {
      devicePositions.delete(deviceId)
      deviceReadings.delete(deviceId)
      cleanedCount++
      console.log(`Cleaned up stale device: ${deviceId} (last seen ${Math.floor((now - position.timestamp) / 1000)}s ago)`)
    }
  }
  
  if (cleanedCount > 0) {
    console.log(`Cleanup complete: removed ${cleanedCount} stale device(s)`)
    broadcastUpdate()
  }
}

export function startCleanupInterval(): void {
  setInterval(cleanupStaleDevices, CLEANUP_INTERVAL_MS)
  console.log(`Cleanup interval started: checking for stale devices every ${CLEANUP_INTERVAL_MS / 1000}s`)
}
