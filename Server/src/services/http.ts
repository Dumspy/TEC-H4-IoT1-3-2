import { createServer } from 'http'
import { readFileSync } from 'fs'
import { fileURLToPath } from 'url'
import { dirname, join } from 'path'
import { WS_PORT } from '../config/constants.js'
import { setSensorOverride, clearSensorOverride } from '../core/sensors.js'
import { broadcastUpdate } from './websocket.js'
import { deviceReadings } from '../core/readings.js'

const __filename = fileURLToPath(import.meta.url)
const __dirname = dirname(__filename)

export function createHTTPServer(): ReturnType<typeof createServer> {
  const httpServer = createServer((req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*')
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, DELETE')
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type')

    if (req.method === 'OPTIONS') {
      res.writeHead(200)
      res.end()
      return
    }

    if (req.url === '/' || req.url === '/index.html') {
      try {
        const html = readFileSync(join(__dirname, '../../public/index.html'), 'utf-8')
        res.writeHead(200, { 'Content-Type': 'text/html' })
        res.end(html)
      } catch (err) {
        res.writeHead(404)
        res.end('index.html not found')
      }
    } else if (req.url === '/readings' || req.url === '/readings.html') {
      try {
        const html = readFileSync(join(__dirname, '../../public/readings.html'), 'utf-8')
        res.writeHead(200, { 'Content-Type': 'text/html' })
        res.end(html)
      } catch (err) {
        res.writeHead(404)
        res.end('readings.html not found')
      }
    } else if (req.url === '/api/devices/readings' && req.method === 'GET') {
      const readings = Array.from(deviceReadings.entries()).map(([deviceId, sensorReadings]) => ({
        deviceId,
        readings: Array.from(sensorReadings.values())
      }))
      res.writeHead(200, { 'Content-Type': 'application/json' })
      res.end(JSON.stringify(readings))
    } else if (req.url === '/api/sensors' && req.method === 'POST') {
      let body = ''
      req.on('data', chunk => {
        body += chunk.toString()
      })
      req.on('end', () => {
        try {
          const { sensorId, x, y } = JSON.parse(body)
          if (!sensorId || typeof x !== 'number' || typeof y !== 'number') {
            res.writeHead(400, { 'Content-Type': 'application/json' })
            res.end(JSON.stringify({ error: 'Invalid request body' }))
            return
          }
          
          const success = setSensorOverride(sensorId, x, y)
          if (success) {
            broadcastUpdate()
            res.writeHead(200, { 'Content-Type': 'application/json' })
            res.end(JSON.stringify({ success: true }))
          } else {
            res.writeHead(404, { 'Content-Type': 'application/json' })
            res.end(JSON.stringify({ error: 'Sensor not found' }))
          }
        } catch (err) {
          res.writeHead(400, { 'Content-Type': 'application/json' })
          res.end(JSON.stringify({ error: 'Invalid JSON' }))
        }
      })
    } else if (req.url?.startsWith('/api/sensors/') && req.method === 'DELETE') {
      const sensorId = decodeURIComponent(req.url.split('/api/sensors/')[1] || '')
      if (sensorId) {
        const success = clearSensorOverride(sensorId)
        console.log(`Clearing override for sensor ${sensorId}: ${success}`)
        broadcastUpdate()
        res.writeHead(200, { 'Content-Type': 'application/json' })
        res.end(JSON.stringify({ success }))
      } else {
        res.writeHead(400, { 'Content-Type': 'application/json' })
        res.end(JSON.stringify({ error: 'Invalid sensor ID' }))
      }
    } else {
      res.writeHead(404)
      res.end('Not found')
    }
  })
  
  httpServer.listen(WS_PORT, () => {
    console.log(`HTTP server listening on http://localhost:${WS_PORT}`)
  })
  
  return httpServer
}
