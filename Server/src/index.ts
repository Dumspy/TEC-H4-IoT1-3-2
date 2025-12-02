import { createHTTPServer } from './services/http.js'
import { createWebSocketServer } from './services/websocket.js'
import { connectMQTT } from './services/mqtt.js'
import { startCleanupInterval } from './utils/cleanup.js'

async function main(): Promise<void> {
  const httpServer = createHTTPServer()
  createWebSocketServer(httpServer)
  await connectMQTT()
  startCleanupInterval()
}

main().catch(console.error)
