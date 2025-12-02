import { createServer } from 'http'
import { readFileSync } from 'fs'
import { fileURLToPath } from 'url'
import { dirname, join } from 'path'
import { WS_PORT } from '../config/constants.js'

const __filename = fileURLToPath(import.meta.url)
const __dirname = dirname(__filename)

export function createHTTPServer(): ReturnType<typeof createServer> {
  const httpServer = createServer((req, res) => {
    if (req.url === '/' || req.url === '/index.html') {
      try {
        const html = readFileSync(join(__dirname, '../../public/index.html'), 'utf-8')
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
  
  return httpServer
}
