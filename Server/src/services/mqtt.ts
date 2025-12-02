import mqtt from 'mqtt'
import { lookup } from 'dns/promises'
import type { WifiSniffPayload, SensorReading } from '../types/index.js'
import { BROKER_HOST, BROKER_PORT, TOPIC, CLIENT_ID } from '../config/constants.js'
import { addReading, getRecentReadings } from '../core/readings.js'
import { registerSensor, getSensorBounds, isWithinBounds } from '../core/sensors.js'
import { devicePositions, trilaterate } from '../core/positioning.js'
import { broadcastUpdate } from './websocket.js'

export async function connectMQTT(): Promise<void> {
  const { address } = await lookup(BROKER_HOST, { family: 4 })
  
  console.log(`Resolved ${BROKER_HOST} to ${address}`)
  console.log(`Connecting to MQTT broker at ${address}:${BROKER_PORT} with client ID: ${CLIENT_ID}`)

  const client = mqtt.connect(`mqtt://${address}:${BROKER_PORT}`, {
    clientId: CLIENT_ID,
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

  client.on('message', (_topic, message) => {
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
      
      registerSensor(reading)
      
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
        const readings = getRecentReadings(payload.device_id)
        const currentCount = readings.length
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
}
