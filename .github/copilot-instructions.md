# Copilot Instructions for AI Coding Agents

## Project Overview
This repository is for an IoT motion detector project using ESP32, Photon 2, and Azure. It is designed for a Master-level course and aims to demonstrate a connected motion detection system.

## Architecture & Components
- **Hardware:** ESP32 and Photon 2 microcontrollers are used for motion sensing and data transmission.
- **Cloud Integration:** Azure is the primary cloud platform for data ingestion, processing, and visualization.
- **Data Flow:** Sensor data is collected by microcontrollers and sent to Azure for further processing.

## Developer Workflows
- **Build & Flash:** Use platform-specific tools (e.g., PlatformIO, Particle CLI) to build and flash firmware to ESP32/Photon 2.
- **Cloud Setup:** Azure resources (IoT Hub, Functions, Storage) must be provisioned and configured for device connectivity.
- **Testing:** Hardware-in-the-loop testing is required. Simulate motion events and verify cloud data ingestion.
- **Debugging:** Use serial monitors and Azure diagnostics for troubleshooting device and cloud issues.

## Project-Specific Conventions
- **Device Naming:** Devices should be named according to their location and type (e.g., `esp32-lab1`, `photon2-hallway`).
- **Data Format:** Sensor payloads must be JSON with fields: `device_id`, `timestamp`, `motion_detected`.
- **Azure Integration:** All cloud communication uses secure MQTT or HTTPS protocols.

## Key Files & Directories
- `README.md`: Project summary and hardware/cloud overview.
- `.github/copilot-instructions.md`: AI agent instructions (this file).
- `LICENSE`: Licensing information.
- **Firmware source files** and **cloud scripts** should be added in future commits for full workflow documentation.

## Integration Points
- **ESP32/Photon 2 <-> Azure IoT Hub:** Devices send telemetry to Azure.
- **Azure Functions:** Process incoming data and trigger alerts or storage actions.

## Examples
- Device payload: `{ "device_id": "esp32-lab1", "timestamp": "2025-10-15T12:00:00Z", "motion_detected": true }`
- Azure IoT Hub connection string usage for device provisioning.

---

**Update this file as new components and workflows are added.**
