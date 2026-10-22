# Web server <-> Controller protocol

- IPC: POSIX message queue, name `/led_queue`
- Message size: 64 bytes, null-terminated text
- Max messages in queue: 10
- Commands: LED_ON, LED_OFF, START, STOP, SET_INTERVAL:<50-5000>

Browser -> server: plain text command over WebSocket (protocol "cmd")
Server -> browser: {"status":"ok","cmd":"LED_ON"} or {"status":"error","msg":"..."}
