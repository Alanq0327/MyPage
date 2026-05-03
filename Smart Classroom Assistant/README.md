# 🏫 Raspberry Pi Based Smart Classroom Assistant

A distributed embedded control system for smart classroom scenarios, integrating Raspberry Pi, ESP8266, MQTT communication, PiTFT touchscreen interaction, voice input, environmental sensing, and actuator control.

This project aims to unify classroom device control into a single intelligent assistant system. Instead of operating temperature, lighting, and projection devices separately, the system enables users to manage classroom environments through a touchscreen UI and voice commands.

---

## 🎥 Demo Video

[![Smart Classroom Assistant Demo](https://img.youtube.com/vi/xawhwiLCINI/0.jpg)](https://www.youtube.com/watch?v=xawhwiLCINI)

> Click the image above to watch the demo video on YouTube.

---

## ✨ Project Highlights

- Designed a distributed embedded system with **Raspberry Pi as the central controller** and **two ESP8266 boards as peripheral execution nodes**.
- Implemented **MQTT-based communication** for real-time command transmission and device state synchronization.
- Built a **PiTFT touchscreen interface** for direct classroom device control.
- Integrated **speech recognition** to support hands-free interaction.
- Collected environmental data including **temperature, humidity, and illuminance**.
- Controlled classroom actuators such as **fan, heater, LED lighting, and projection-screen motor**.
- Designed scenario-based control modes including **Projection Mode**, **Movie Mode**, and **Off Mode**.
- Applied state management and command-priority logic to improve stability and avoid conflicting operations.

---

## 🧩 System Architecture

```text
User
├── Touchscreen Input
└── Voice Input
        │
        ▼
Raspberry Pi Central Controller
├── PiTFT User Interface
├── Speech Recognition
├── Global State Management
└── MQTT Message Publishing
        │
        ▼
Mosquitto MQTT Broker
        │
        ├─────────────── ESP8266 Node 1
        │                ├── Temperature / Humidity Sensing
        │                ├── Fan Control
        │                └── Heater Control
        │
        └─────────────── ESP8266 Node 2
                         ├── Illuminance Sensing
                         ├── WS2812B LED Control
                         └── Projection Screen Motor Control
