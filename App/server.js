const app = require('express')()
const http = require('http').Server(app)
const io = require('socket.io')(http)

const port = 4224

app.get('/', (req, res) => {
  res.sendFile(__dirname + '/index.html')
})

io.on('connection', socket => {
  console.log('user connected')
  socket.on('message', message => {
    console.log('message:', message)
  })
  socket.on('disconnect', () => {
    console.log('user disconnected')
  });
  // socket.use((packet, next) => {
  //   console.log(packet);
  //   return next();
  // });
})

http.listen(port, () => {
  console.log(`listening on *:${port}`)
})
