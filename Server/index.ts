import mqtt from 'mqtt'
import { lookup } from 'dns/promises'
import { WebSocketServer } from 'ws'
import { createServer } from 'http'
import { readFileSync } from 'fs'
import { fileURLToPath } from 'url'
import { dirname, join } from 'path'

const __filename = fileURLToPath(import.meta.url)
const __dirname = dirname(__filename)

const BROKER_HOST = 'wilson.local'
const BROKER_PORT = 1883
const WS_PORT = 8080
const TOPIC = 'wifi/sniff'
const clientId = `ESP32-${Math.random().toString(16).substring(2, 6)}`
const DEVICE_TIMEOUT_MS = 5 * 60 * 1000
const CLEANUP_INTERVAL_MS = 60 * 1000
const BOUNDARY_PADDING = 1.0
const MAX_READING_TIME_DIFF_MS = 2000

interface WifiSniffPayload {
  device_id: string
  rssi: number
  sensor_x: number
  sensor_y: number
  timestamp: string
}

interface WifiSniffData {
  timestamp: string
  topic: string
  payload: string
}

interface Position {
  x: number
  y: number
}

interface SensorReading {
  sensor_x: number
  sensor_y: number
  rssi: number
  timestamp: number
}

interface Bounds {
  minX: number
  maxX: number
  minY: number
  maxY: number
}

const deviceReadings = new Map<string, Map<string, SensorReading>>()
const devicePositions = new Map<string, Position & { timestamp: number }>()
const knownSensors = new Map<string, { x: number, y: number }>()

let wss: WebSocketServer | null = null

function broadcastUpdate(): void {
  if (!wss) return
  
  const update = {
    devices: Array.from(devicePositions.entries()).map(([id, pos]) => ({
      id,
      x: pos.x,
      y: pos.y,
      timestamp: pos.timestamp
    })),
    sensors: Array.from(knownSensors.entries()).map(([id, pos]) => ({
      id,
      x: pos.x,
      y: pos.y
    }))
  }
  
  const message = JSON.stringify(update)
  wss.clients.forEach(client => {
    if (client.readyState === 1) {
      client.send(message)
    }
  })
}

function cleanupStaleDevices(): void {
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

const MAX_SENSORS_PER_DEVICE = 5

function addReading(deviceId: string, reading: SensorReading): boolean {
  const sensorKey = `${reading.sensor_x},${reading.sensor_y}`
  if (!knownSensors.has(sensorKey)) {
    knownSensors.set(sensorKey, { x: reading.sensor_x, y: reading.sensor_y })
  }
  
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

function rssiToDistance(rssi: number): number {
  const txPower = -59
  const n = 2.0
  return Math.pow(10, (txPower - rssi) / (10 * n))
}

function getRecentReadings(deviceId: string, count: number = 3): SensorReading[] {
  const readings = deviceReadings.get(deviceId)
  if (!readings) {
    return []
  }
  
  return Array.from(readings.values())
    .sort((a, b) => b.timestamp - a.timestamp)
    .slice(0, count)
}

function trilaterate(readings: SensorReading[]): Position | null {
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

  const x1 = r1.sensor_x, y1 = r1.sensor_y
  const x2 = r2.sensor_x, y2 = r2.sensor_y
  const x3 = r3.sensor_x, y3 = r3.sensor_y

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

function getSensorBounds(): Bounds | null {
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

function isWithinBounds(position: Position, bounds: Bounds): boolean {
  return position.x >= bounds.minX && 
         position.x <= bounds.maxX && 
         position.y >= bounds.minY && 
         position.y <= bounds.maxY
}

async function connect() {
  const httpServer = createServer((req, res) => {
    if (req.url === '/' || req.url === '/index.html') {
      try {
        const html = readFileSync(join(__dirname, 'index.html'), 'utf-8')
        res.writeHead(200, { 'Content-Type': 'text/html' })
        res.end(html)
      } catch (err) {
        res.writeHead(404)
        res.end('index.html not found')
      }
    } else {
      res.writeHead(404)
      res.end('Not found')
    }
  })
  
  httpServer.listen(WS_PORT, () => {
    console.log(`HTTP server listening on http://localhost:${WS_PORT}`)
  })
  
  wss = new WebSocketServer({ server: httpServer })
  
  wss.on('connection', (ws) => {
    console.log('WebSocket client connected')
    broadcastUpdate()
    
    ws.on('close', () => {
      console.log('WebSocket client disconnected')
    })
  })
  
  const { address } = await lookup(BROKER_HOST, { family: 4 })
  
  console.log(`Resolved ${BROKER_HOST} to ${address}`)
  console.log(`Connecting to MQTT broker at ${address}:${BROKER_PORT} with client ID: ${clientId}`)

  const client = mqtt.connect(`mqtt://${address}:${BROKER_PORT}`, {
    clientId,
    protocolVersion: 4,
    clean: true,
    keepalive: 60,
    reconnectPeriod: 5000,
    connectTimeout: 30000
  })

  client.on('connect', () => {
    console.log('Connected to MQTT broker')
    client.subscribe(TOPIC, (err) => {
      if (err) {
        console.error('Subscription error:', err)
      } else {
        console.log(`Subscribed to ${TOPIC}`)
      }
    })
  })

  client.on('message', (topic, message) => {
    try {
      const payload: WifiSniffPayload = JSON.parse(message.toString())
      
      const timestampStr = payload.timestamp
        .replace(/\//g, '-')
        .replace(' ', 'T')
        .replace(/(\d{2}):(\d{2}):(\d{2}):(\d{3})$/, '$1:$2:$3.$4')
      const payloadTimestamp = new Date(timestampStr).getTime()
      
      const reading: SensorReading = {
        sensor_x: payload.sensor_x,
        sensor_y: payload.sensor_y,
        rssi: payload.rssi,
        timestamp: payloadTimestamp
      }
      
      const hasThreeReadings = addReading(payload.device_id, reading)
      
      if (hasThreeReadings) {
        const readings = getRecentReadings(payload.device_id, 3)
        const position = trilaterate(readings)
        
        if (position) {
          const bounds = getSensorBounds()
          
          if (bounds && isWithinBounds(position, bounds)) {
            devicePositions.set(payload.device_id, { 
              ...position, 
              timestamp: payloadTimestamp 
            })
            console.log(`Device ${payload.device_id} triangulated position: x=${position.x.toFixed(2)}, y=${position.y.toFixed(2)}`)
            broadcastUpdate()
          } else {
            console.log(`Device ${payload.device_id} position out of bounds: x=${position.x.toFixed(2)}, y=${position.y.toFixed(2)} - ignoring`)
          }
        } else {
          console.log(`Failed to triangulate position for device ${payload.device_id}`)
        }
      } else {
        const readings = deviceReadings.get(payload.device_id)
        const currentCount = readings ? readings.size : 0
        console.log(`Collected ${currentCount}/3 readings for device ${payload.device_id}`)
        broadcastUpdate()
      }
    } catch (err) {
      console.error('Error processing message:', err)
      console.log(`Raw message: ${message.toString()}`)
    }
  })

  client.on('error', (err) => {
    console.error('Connection error:', err)
  })

  client.on('offline', () => {
    console.log('Client went offline')
  })

  client.on('reconnect', () => {
    console.log('Attempting to reconnect...')
  })
  
  setInterval(cleanupStaleDevices, CLEANUP_INTERVAL_MS)
  console.log(`Cleanup interval started: checking for stale devices every ${CLEANUP_INTERVAL_MS / 1000}s`)
}

connect().catch(console.error)

