import mqtt from 'mqtt'
import { lookup } from 'dns/promises'

const BROKER_HOST = process.env.MQTT_BROKER || 'wilson.local'
const BROKER_PORT = parseInt(process.env.MQTT_PORT || '1883')
const TOPIC = 'temp/devices/position'
const clientId = `ESP32-${Math.random().toString(16).substring(2, 6)}`

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
        
        setTimeout(() => {
          const testMessage = JSON.stringify({
            device: clientId,
            message: "Hello world",
            timestamp: Date.now()
          })
          
          client.publish(TOPIC, testMessage, (err) => {
            if (err) {
              console.error('Publish error:', err)
            } else {
              console.log(`Published test message: ${testMessage}`)
            }
          })
        }, 1000)
      }
    })
  })

  client.on('message', (topic, message) => {
    console.log(`[${topic}] ${message.toString()}`)
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

