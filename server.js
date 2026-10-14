const express = require('express');
const mqtt = require('mqtt');
const path = require('path');

const app = express();
const port = 3000;

// MQTT broker settings (local setup to match your ESP32)
const mqttBroker = process.env.MQTT_BROKER || 'mqtt://localhost'; // e.g. MQTT_BROKER=mqtt://192.168.1.10
const dataTopic = 'pomodoro/data';
const clients = [];

// Connect to the MQTT broker
const client = mqtt.connect(mqttBroker);

client.on('connect', () => {
  console.log('Connected to MQTT broker');
  client.subscribe([dataTopic], (err) => {
    if (err) {
      console.error('Subscription error:', err);
    }
  });
});

// Store the last received timer data
let timerData = { minutes: 0, seconds: 0, counter: 0 };

// Handle incoming MQTT messages
client.on('message', (topic, message) => {
  if (topic === dataTopic) {
    try {
      const parsedData = JSON.parse(message.toString());
      timerData = parsedData;

      // Broadcast data to all connected clients
      const payload = JSON.stringify(timerData);
      clients.forEach(client => client.res.write(`data: ${payload}\n\n`));
    } catch (error) {
      console.error('Error parsing MQTT message:', error);
    }
  }
});

// Serve the HTML file
app.use(express.static(path.join(__dirname, 'public')));

// Server-Sent Events (SSE) endpoint
app.get('/events', (req, res) => {
  res.setHeader('Content-Type', 'text/event-stream');
  res.setHeader('Cache-Control', 'no-cache');
  res.setHeader('Connection', 'keep-alive');

  // Send initial data
  res.write(`data: ${JSON.stringify(timerData)}\n\n`);
  clients.push({ res });

  // Handle client disconnect
  req.on('close', () => {
    console.log('Client disconnected');
    const index = clients.findIndex(client => client.res === res);
    if (index !== -1) {
      clients.splice(index, 1);
    }
  });
});

// Start the server
app.listen(port, () => {
  console.log(`Server started at http://localhost:${port}`);
});
