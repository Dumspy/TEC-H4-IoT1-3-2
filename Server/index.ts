import mqtt from 'mqtt'
import { lookup } from 'dns/promises'

const BROKER_HOST = process.env.MQTT_BROKER || 'wilson.local'
const BROKER_PORT = parseInt(process.env.MQTT_PORT || '1883')
const TOPIC = 'wifi/sniff'
const clientId = `ESP32-${Math.random().toString(16).substring(2, 6)}`

interface WifiSniffData {
  device_id: string
  rssi: number
  sensor_x: number
  sensor_y: number
  timestamp: number
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

const deviceReadings = new Map<string, SensorReading[]>()

function addReading(deviceId: string, reading: SensorReading): boolean {
  if (!deviceReadings.has(deviceId)) {
    deviceReadings.set(deviceId, [])
  }
  
  const readings = deviceReadings.get(deviceId)!
  
  const isDuplicatePosition = readings.some(r => 
    r.sensor_x === reading.sensor_x && r.sensor_y === reading.sensor_y
  )
  
  if (isDuplicatePosition) {
    return false
  }
  
  readings.push(reading)
  
  if (readings.length > 3) {
    readings.shift()
  }
  
  return readings.length === 3
}

function rssiToDistance(rssi: number): number {
  const txPower = -59
  const n = 2.0
  return Math.pow(10, (txPower - rssi) / (10 * n))
}

function trilaterate(readings: SensorReading[]): Position | null {
  if (readings.length !== 3) {
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

async function connect() {
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
      const data: WifiSniffData = JSON.parse(message.toString())
      
      const reading: SensorReading = {
        sensor_x: data.sensor_x,
        sensor_y: data.sensor_y,
        rssi: data.rssi,
        timestamp: data.timestamp
      }
      
      const hasThreeReadings = addReading(data.device_id, reading)
      
      if (hasThreeReadings) {
        const readings = deviceReadings.get(data.device_id)!
        const position = trilaterate(readings)
        
        if (position) {
          console.log(`Device ${data.device_id} triangulated position: x=${position.x.toFixed(2)}, y=${position.y.toFixed(2)}`)
        } else {
          console.log(`Failed to triangulate position for device ${data.device_id}`)
        }
      } else {
        const currentCount = deviceReadings.get(data.device_id)?.length || 0
        console.log(`Collected ${currentCount}/3 readings for device ${data.device_id}`)
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
}

connect().catch(console.error)

