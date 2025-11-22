# TemperatureMonitor
ESP32-C3 based temperature controller for a fermentation chamber, with DS18B20 sensor, web interface, and automatic heating control.

## Project Status
This project is currently under development and follows a gradual approach:

### 1. Initial monolithic version
The first release will serve as a working proof-of-concept, with all ESP32 code concentrated in main. This approach simplifies initial development and allows rapid testing of core features:

- Reading temperature from the DS18B20 sensor
- Controlling a relay for heating
- Basic web interface with setpoint and manual buttons
- Saving setpoints in non-volatile memory
- Captive portal for WiFi configuration

### 2. Future structured version
Once the monolithic version is validated, the code will be refactored into a modular structure, improving maintainability, readability, and scalability for additional features.