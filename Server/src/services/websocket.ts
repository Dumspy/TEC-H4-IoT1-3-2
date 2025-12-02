import { WebSocketServer } from 'ws'
import type { Server } from 'http'
import { devicePositions } from '../core/positioning.js'
import { getAllSensors } from '../core/sensors.js'
import { deviceReadings } from '../core/readings.js'

let wss: WebSocketServer | null = null

export function createWebSocketServer(httpServer: Server): WebSocketServer {
  wss = new WebSocketServer({ server: httpServer })
  
  wss.on('connection', (ws) => {
    console.log('WebSocket client connected')
    broadcastUpdate()
    
    ws.on('close', () => {
      console.log('WebSocket client disconnected')
    })
  })
  
  return wss
}

export function broadcastUpdate(): void {
  if (!wss) return
  
  const update = {
    devices: Array.from(devicePositions.entries()).map(([id, pos]) => ({
      id,
      x: pos.x,
      y: pos.y,
      timestamp: pos.timestamp
    })),
    sensors: getAllSensors(),
    deviceReadings: Array.from(deviceReadings.entries()).map(([deviceId, sensorReadings]) => ({
      deviceId,
      readings: Array.from(sensorReadings.values())
    }))
  }
  
  const message = JSON.stringify(update)
  wss.clients.forEach(client => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(message)
    }
  })
}
