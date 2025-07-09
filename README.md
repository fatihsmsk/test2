# Project Requirements & Module Specifications

This document outlines the **updated** and more detailed requirements for each module in the soil-measurement device project. The project includes:

- A device (ESP32 38-pin) inserted into the soil.  
- Measurements for soil moisture, temperature, pH, EC, and NPK values.  
- Periodic data acquisition, calibration, and deep sleep.  
- Battery monitoring.  
- Data logging on an SD card if no connection.  
- Watchdog to reset the device if it becomes unresponsive (including daily resets).  
- OTA updates for the firmware.  
- Communication to a remote server (ThingSpeak) via SIM808.  

**New/Updated Requirements** are integrated below, including:
1. **Exact Communication Protocol Specifications**  
2. **Memory and Resource Constraints**  
3. **Security / Encryption**  
4. **Data Format & Schema**  
5. **Timing & Scheduling**  
6. **Sensor Calibration Process**  
7. (Intentionally omitted from numbering)  
8. **Testing / Validation Requirements**  

---

## System-Level Requirements

### 1. Exact Communication Protocol Specifications

1. **RS485 (SoilSensorManager)**  
   - Follows **Modbus-RTU** standard (reference: *attached images*).  
   - Default address: `0x01` (modifiable).  
   - Baud rate: `4800` (or possibly `9600` if configured), no parity, 8 data bits, 1 stop bit.  
   - Standard Modbus function codes for reading/writing registers.  
   - Query example: reading moisture, temperature, conductivity, pH, etc. from register addresses `0x0000` to `0x0006`.

2. **SIM808 (ServerConnection)**  
   - Reference the [SIM808 GPS Application Note](https://cdn-shop.adafruit.com/datasheets/SIM808_GPS_Application_Note_V1.00.pdf).  
   - Utilizes AT commands for initialization, GPRS connection, and HTTP data transfer.  
   - Must handle potential re-registration attempts, network drops, and ensure robust error handling.

---

### 2. Memory and Resource Constraints
- **No FreeRTOS tasks** are used in the system (single-threaded loop-based or interrupt-based approach).  
- **No dynamic memory allocation** is permitted (use static or global arrays/buffers).  
- **Add a parameter to the DataLogger module**:  
  - **`DataLogger_MaxLogFileSize`** (Default: 1MB). This caps each log file’s size.

---

### 3. Security / Encryption
1. **Encrypted Connection Details**  
   - All connection details (e.g., APN, credentials, ThingSpeak keys) are stored in a **Connection File** at the root of the SD card.  
   - This file **must be encrypted** (e.g., AES or another suitable method).  
2. **Encrypted Logs**  
   - The system logs stored on the SD card (in the `DataLogger_LogFilePath`) must also be **encrypted** to prevent unauthorized access if the SD is removed.

---

### 4. Data Format and Schema
1. **Soil Sensor Data, Calibration Data, Battery Voltage Data**  
   - Stored as `.csv` files using **`;`** (semicolon) as the delimiter.  
   - Example CSV line for soil sensor data:  
     ```
     <timestamp>;<moisture>;<temperature>;<conductivity>;<pH>;<nitrogen>;<phosphorus>;<potassium>
     ```
2. **System Logs**  
   - Plain text files (with encryption as per the Security requirement).  
   - Must **include timestamps** on each log entry.  
   - Timestamp format can be `YYYY-MM-DD HH:MM:SS` or similar.

---

### 5. Timing and Scheduling Details
- The system obtains **time** from the SIM808 module (e.g., using network time or GPS time if supported).  
- Sleep interval is typically 1 hour (controlled by **SleepManager**).  
- Calibration intervals (e.g., 1 month) are also time-based, referencing the SIM808 time.

---

### 6. Sensor Calibration Process
1. **Calibration Interval**  
   - A parameter for the calibration period, default is **1 month**.  
2. **Soil Sensor Power Management**  
   - The SoilSensorManager shall **enable** the sensor’s power pin each time it needs to take a reading, and then **disable** it afterward.  
   - Add parameter **`SoilSensorManager_EnablePin`** to define which GPIO controls sensor power.  
3. **Calibration Parameters from ThingSpeak**  
   - The system shall retrieve updated calibration parameters from ThingSpeak.  
   - These parameters are then applied/stored by **SoilSensorManager** (and can be logged in the SD card if needed).

---

### 7. *(Intentionally Skipped)*

---

### 8. Testing / Validation Requirements
1. **Unit Testing Module**  
   - Create a separate module named **UnitTestingModule**.  
   - This module mocks dependencies (RS485, SIM808, SD card, etc.) to test each other module in isolation.  
   - Each test function returns a **pass/fail** status or a detailed error code.  
2. **Test Plan**  
   - For each module function (e.g., `ServerConnection_InitSim808`, `SoilSensorManager_ReadSoilData`), define success criteria (expected normal result) and fail criteria (handling error codes, invalid data, etc.).  
   - The system should log test results (optionally to the SD card as well).

---

# Detailed Module Specifications

Below are the **revised** module specifications, incorporating the new requirements. All previous details remain, with changes highlighted where applicable.

---

## 1. ServerConnection Module

### 1.1 Description
Handles connectivity with the ThingSpeak server through the SIM808 modem. Initializes the SIM808 module and sends sensor data using HTTP requests.

### 1.2 Responsibilities
1. Initialize the SIM808 modem (UART pins, baud rate, timeouts).
2. Handle HTTP connections to ThingSpeak (reference [SIM808 doc](https://cdn-shop.adafruit.com/datasheets/SIM808_GPS_Application_Note_V1.00.pdf)).
3. Send sensor data (soil measurements) to the remote server.
4. Return specific error codes on failure.

### 1.3 Global Variables
*None specific to this module at the moment.*  
*(Typically, states like connection status could be stored here if needed.)*

### 1.4 Local Variables
- Internal buffers for AT commands and HTTP requests.
- Internal state flags (e.g., “isConnected”, “networkRegistered”).

### 1.5 Parameters (Preprocessor Definitions)
- **`ServerConnection_Pin_TX_Sim808`** (Default: 17)  
- **`ServerConnection_Pin_RX_Sim808`** (Default: 16)  
- **`ServerConnection_Baudrate_Sim808`** (Default: 115200)  
- **`ServerConnection_TimeoutSim808`** (Default: 10000 ms)  
- **`ServerConnection_ThingSpeakAPIKey`** (String; must be valid)  
- **`ServerConnection_ThingSpeakURL`** (String; valid ThingSpeak URL)

### 1.6 Functions

1. **`int ServerConnection_InitSim808()`**  
   - **Description**:  
     Initializes SIM808 using the defined TX/RX pins and baud rate.  
   - **Behavior**:  
     1. Powers on the SIM808 (if managed by a power pin).  
     2. Sends AT commands to verify communication and SIM presence.  
     3. Waits for network registration (checks signal).  
     4. Returns `0` on success or an error code.

2. **`int ServerConnection_Send_NPK_Data(float temperature, float humidity, float EC, float pH, float nitrogen, float phosphorus, float potassium)`**  
   - **Description**:  
     Builds an HTTP request to send soil sensor data (CSV format is prepared outside or inside this function) to ThingSpeak.  
   - **Behavior**:  
     1. Validates that SIM808 is initialized and network-registered.  
     2. Sends data via HTTP POST/GET.  
     3. Monitors for timeouts (`ServerConnection_TimeoutSim808`).  
     4. Returns `0` if successful, or a specific error code otherwise.

### 1.7 Error Handling
Uses the existing global error codes. Possible relevant codes include:

- **1** (`ERR_SIM808_INIT_FAIL`)  
- **2** (`ERR_SIM808_TIMEOUT`)  
- **3** (`ERR_SIM_NOT_INSERTED`)  
- **4** (`ERR_NETWORK_REG_FAIL`)  
- **5** (`ERR_API_KEY_INVALID`)  
- **6** (`ERR_HTTP_CONNECTION_FAIL`)  
- **7** (`ERR_HTTP_SEND_FAIL`)  
- **8** (`ERR_HTTP_RESPONSE_INVALID`)  
- **9** (`ERR_DATA_FORMAT`)  
- **10** (`ERR_UNKNOWN`)  

---

## 2. SoilSensorManager Module

### 2.1 Description
Manages reading data from the **Soil Sensor** via RS485 (Modbus-RTU). Handles periodic calibration, enabling/disabling sensor power, and data validation.

### 2.2 Responsibilities
1. Initialize RS485 communication for the **Soil Sensor**.  
2. Read sensor data (moisture, temperature, pH, EC, nitrogen, phosphorus, potassium).  
3. Maintain and apply calibration coefficients for the **Soil Sensor**.  
4. Enable sensor power pin prior to reading, disable afterward.  
5. Validate sensor data integrity/ranges.

### 2.3 Global Variables
- **`SoilSensorManager_calibrationCoefficients[SoilSensorManager_SensorDataCount][SoilSensorManager_CalibrationCoefficientCount]`**  
  2D array storing calibration coefficients for each sensor data type.  
- **`SoilSensorManager_sensorValues[SoilSensorManager_SensorDataCount]`**  
  Array storing the latest read values from the Soil Sensor.

### 2.4 Local Variables
- Internal buffers for RS485 transactions.
- Status flags (e.g., “isCalibrated”).
- Temporary parsing structures (CRC checks, response buffers, etc.).

### 2.5 Parameters (Preprocessor Definitions or Configuration)
- **`SoilSensorManager_Pin_RS485_RX`**  
- **`SoilSensorManager_Pin_RS485_TX`**  
- **`SoilSensorManager_RS485_REDE_Pin`**  
- **`SoilSensorManager_Baudrate_RS485`** (Default: 4800 or 9600 per sensor specs)  
- **`SoilSensorManager_SensorReadInterval`** (Default: 1 hour)  
- **`SoilSensorManager_CalibrationCoefficientCount`** (Default: 2)  
- **`SoilSensorManager_SensorDataCount`** (Default: 7)  
- **`SoilSensorManager_EnablePin`**: GPIO pin to power on/off the sensor.  
- **`SoilSensorManager_CalibrationPeriod`** (Default: 1 month)

### 2.6 Functions

1. **`int SoilSensorManager_InitRS485()`**  
   - **Description**:  
     Configures UART for RS485 and sets DE/RE pins, referencing attached Modbus documentation.  
   - **Behavior**:  
     1. Sets up RX/TX pins.  
     2. Sets the `SoilSensorManager_RS485_REDE_Pin` to receive mode by default.  
     3. Returns `0` on success, or error code if communication fails.

2. **`int SoilSensorManager_ReadSoilData(float* temperature, float* moisture, float* pH, float* EC, float* nitrogen, float* phosphorus, float* potassium)`**  
   - **Description**:  
     Requests data from the Soil Sensor (Modbus-RTU) and populates the pointers.  
   - **Behavior**:  
     1. **Enable** sensor power pin (`SoilSensorManager_EnablePin`).  
     2. Wait a small delay if needed for sensor startup.  
     3. Sends a Modbus read frame (function code 0x03, registers 0x0000 to 0x0006).  
     4. Receives and parses the response, checks CRC.  
     5. Disables sensor power.  
     6. Returns `0` on success, or an error code on communication/parsing error.

3. **`int SoilSensorManager_CalibrateSensors()`**  
   - **Description**:  
     Applies or updates calibration data in `SoilSensorManager_calibrationCoefficients`, potentially fetched from ThingSpeak or a local store.  
   - **Behavior**:  
     1. Uses the newly retrieved parameters from the server.  
     2. Updates the global calibration array.  
     3. Returns `0` on success, or an error code on failure.

### 2.7 Error Handling
Relevant error codes might include:
- **11** (`ERR_RS485_COMM_FAIL`)
- **12** (`ERR_SENSOR_CALIBRATION`)
- **9**  (`ERR_DATA_FORMAT`)
- **10** (`ERR_UNKNOWN`)

---

## 3. SleepManager Module

### 3.1 Description
Responsible for managing the ESP32 deep sleep cycle: sleeps for an hour, wakes up to perform tasks (sensor reading, data sending), and goes back to sleep.

### 3.2 Responsibilities
1. Configure deep sleep intervals (1 hour).  
2. Manage wake-up sources and triggers.  
3. Integrate with the Watchdog to avoid indefinite wakefulness.

### 3.3 Global Variables
*None specifically required unless persistent across boots is needed.*

### 3.4 Local Variables
- Internal time calculations for next sleep cycle.

### 3.5 Parameters
- **`SleepManager_SleepInterval`** (Default: 1 hour in microseconds)
- **`SleepManager_WakePin`** (Optional, if external wake is used)

### 3.6 Functions

1. **`void SleepManager_ConfigureDeepSleep(unsigned long duration_us)`**  
   - **Description**:  
     Sets up and enters ESP32 deep sleep for `duration_us`.  
   - **Behavior**:  
     1. Retrieves or updates time from SIM808 if needed.  
     2. Configures wake-up sources.  
     3. Calls `esp_deep_sleep_start()`.  

2. **`bool SleepManager_IsWakeFromDeepSleep()`**  
   - **Description**:  
     Determines if the current boot is from deep sleep or a fresh reset.  
   - **Behavior**:  
     - Returns `true` if the device woke from deep sleep; `false` otherwise.

### 3.7 Error Handling
- **13** (`ERR_SLEEP_CONFIG`) if invalid parameters or configuration.

---

## 4. BatteryMonitor Module

### 4.1 Description
Monitors the battery voltage every hour using the ESP32 ADC.

### 4.2 Responsibilities
1. Initialize ADC for battery measurement.  
2. Convert ADC values to actual battery voltage.  
3. Potentially estimate battery percentage.

### 4.3 Global Variables
*None specific to this module by default.*

### 4.4 Local Variables
- Internal calibration offset for ADC (if needed).
- Temporary storage for last read battery voltage.

### 4.5 Parameters
- **`BatteryMonitor_Pin_BatterySense`** (ADC pin)
- **`BatteryMonitor_VoltageDividerFactor`** (Default ratio if using a voltage divider)

### 4.6 Functions

1. **`int BatteryMonitor_Init()`**  
   - **Description**:  
     Sets up the ADC to read the battery voltage.  
   - **Behavior**:  
     1. Configures the ADC channel.  
     2. Returns `0` on success, or an error code otherwise.

2. **`float BatteryMonitor_GetVoltage()`**  
   - **Description**:  
     Reads the ADC and converts it to actual battery voltage.  
   - **Behavior**:  
     1. Takes raw ADC reading.  
     2. Multiplies by `BatteryMonitor_VoltageDividerFactor`.  
     3. Returns the voltage as a float.

### 4.7 Error Handling
- **14** (`ERR_ADC_READ_FAIL`) if the ADC read fails.

---

## 5. DataLogger Module

### 5.1 Description
Handles all data logging to the SD card and console. It also manages file/folder creation, size limits, and offline data storage for soil sensor and battery data. **Requires encryption** for logs and certain connection files.

### 5.2 Responsibilities
1. Initialize the SD card for file operations.  
2. Log diagnostic messages based on a set log level.  
3. Write/read **Soil Sensor** data (CSV with `;` delimiter) when offline or for archival.  
4. Write/read battery data (CSV with `;` delimiter).  
5. Manage log files (text files, encrypted) with rotation upon reaching size limits.  
6. Maintain an encrypted “Connection File” at the SD card root for credentials.

### 5.3 Global Variables
*None by default.*  
*(Most state can be kept local or within log structures.)*

### 5.4 Local Variables
- Internal file handles.
- Buffer for log messages.

### 5.5 Parameters (Preprocessor Definitions or Configuration)
- **`DataLogger_Pin_SD_CS`**: Chip select for the SD card.  
- **`DataLogger_LogLevel`**: Global log level threshold.  
- **`DataLogger_BatteryMonitorFilePath`** (Default: `"Battery"`)  
- **`DataLogger_SoilSensorDataFilePath`** (Default: `"SoilSensor/Data"`)  
- **`DataLogger_SoilSensorCoefficientsFilePath`** (Default: `"SoilSensor/Coeff"`)  
- **`DataLogger_LogFilePath`** (Default: `"Logs"`)  
- **`DataLogger_MaxLogFolderSize`** (Default: `"10GB"`)  
- **`DataLogger_MaxLogFileSize`** (Default: `"1MB"`)  ← **New**  
- **`DataLogger_ConnectionFilePath`** (Default: `"Connection"`)

> **Folder & File Creation Requirement**  
> Every function that writes to a file must:
> - Check if the folder path exists. If not, create it.  
> - Check if the file exists. If not, create it.  
> - **Encrypt** the logs file if it is a log, and the `ConnectionFilePath` file if it contains connection details.

### 5.6 Functions

1. **`int DataLogger_InitSDCard()`**  
   - **Description**:  
     Initializes the SD card.  
   - **Behavior**:  
     1. Verifies card presence, mounts file system.  
     2. Returns `0` on success, or an error code if it fails.

2. **`void DataLogger_LogMessage(int level, const char* message)`**  
   - **Description**:  
     Logs a message if `level >= DataLogger_LogLevel`. Also writes to the encrypted text log file (`DataLogger_LogFilePath`).  
   - **Behavior**:  
     1. Checks if the log folder exists. If not, create it.  
     2. Creates or opens the encrypted log file, ensuring it does not exceed `DataLogger_MaxLogFileSize`.  
     3. Appends a timestamp (from SIM808) and the message.  
     4. If log folder size > `DataLogger_MaxLogFolderSize`, handle rotation/deletion.

3. **`int DataLogger_WriteSoilSensorData(float temperature, float humidity, float EC, float pH, float nitrogen, float phosphorus, float potassium)`**  
   - **Description**:  
     Writes soil sensor data to a CSV in `DataLogger_SoilSensorDataFilePath`, using `;` as delimiter.  
   - **Behavior**:  
     1. Checks/creates folder/file.  
     2. Writes `<timestamp>;<temp>;<humidity>;<EC>;<pH>;<N>;<P>;<K>`.  
     3. Returns `0` on success, or an error code if writing fails.

4. **`int DataLogger_ReadSoilSensorData()`**  
   - **Description**:  
     Reads the CSV file from `DataLogger_SoilSensorDataFilePath`.  
   - **Behavior**:  
     1. Checks if file/folder exist.  
     2. Reads content (could load all or read line by line).  
     3. Returns data or an error code.

5. **`int DataLogger_UpdateSoilSensorCoefficients(float newCoeffs[][2], int sensorCount, int coeffCount)`**  
   - **Description**:  
     Updates the existing soil sensor calibration coefficients on the SD card (CSV or a dedicated format).  
   - **Behavior**:  
     1. Checks/creates folder/file at `DataLogger_SoilSensorCoefficientsFilePath`.  
     2. Writes the array as CSV.  
     3. Returns `0` on success, or an error code.

6. **`int DataLogger_ReadSoilSensorCoefficients(float coeffsOut[][2], int sensorCount, int coeffCount)`**  
   - **Description**:  
     Reads calibration coefficients from the SD card CSV.  
   - **Behavior**:  
     1. Checks if file/folder exist.  
     2. Reads data and populates `coeffsOut`.  
     3. Returns `0` on success, or error code if reading fails.

7. **`int DataLogger_WriteBatteryData(float voltage)`**  
   - **Description**:  
     Appends battery voltage data to `DataLogger_BatteryMonitorFilePath` in CSV form.  
   - **Behavior**:  
     1. Checks/creates folder/file.  
     2. Writes `<timestamp>;<voltage>`.  
     3. Returns `0` on success, or error code.

8. **`int DataLogger_ReadBatteryData()`**  
   - **Description**:  
     Reads battery data from the CSV file.  
   - **Behavior**:  
     1. Checks folder/file.  
     2. Returns the data or an error code.

9. **`int DataLogger_WriteLogs(int level, const char* message)`**  
   - **Description**:  
     Similar to `DataLogger_LogMessage` but specifically ensures new file creation if logs exceed thresholds.  
   - **Behavior**:  
     1. Checks log folder, creates if needed.  
     2. Opens an encrypted file, writes `<timestamp> <message>`.  
     3. Returns `0` if success, or an error code.

10. **`unsigned long DataLogger_MeasureLogVolume()`**  
    - **Description**:  
      Measures total size of the logs folder to compare against `DataLogger_MaxLogFolderSize`.  
    - **Behavior**:  
      1. Recursively sums file sizes in `DataLogger_LogFilePath`.  
      2. Returns the size in bytes.

11. **`int DataLogger_DeleteLogs()`**  
    - **Description**:  
      Deletes older log files or entries when the log folder exceeds `DataLogger_MaxLogFolderSize`.  
    - **Behavior**:  
      1. Identifies oldest logs.  
      2. Removes them until size is below threshold.  
      3. Returns `0` on success.

12. **`int DataLogger_ReadConnectionFile(char* outData, size_t maxLen)`**  
    - **Description**:  
      Decrypts and reads the connection details from the file at `DataLogger_ConnectionFilePath`.  
    - **Behavior**:  
      1. Checks if file exists; if not, error.  
      2. Decrypts content.  
      3. Returns `0` or an error code if it fails.

13. **`int DataLogger_WriteConnectionFile(const char* inData)`**  
    - **Description**:  
      Encrypts and writes connection details to the file at `DataLogger_ConnectionFilePath`.  
    - **Behavior**:  
      1. Checks/creates the file/folder.  
      2. Encrypts `inData` and writes to file.  
      3. Returns `0` on success or an error code.

### 5.7 Error Handling
- **15** (`ERR_SD_INIT_FAIL`)  
- **16** (`ERR_SD_WRITE_FAIL`)  
- **17** (`ERR_SD_READ_FAIL`)  
- **10** (`ERR_UNKNOWN`)

---

## 6. WatchdogManager Module

### 6.1 Description
Implements a watchdog timer to reset the ESP32 if it becomes unresponsive and schedules a daily restart.

### 6.2 Responsibilities
1. Configure the watchdog timer and feed it regularly.  
2. Automatically reset the device if the watchdog is not fed in time.  
3. Perform a forced daily restart of the ESP32.

### 6.3 Global Variables
*None needed unless storing last reset time, etc.*

### 6.4 Local Variables
- Internal timer tracking for daily resets.

### 6.5 Parameters
- **`WatchdogManager_WDT_TIMEOUT_MS`** (Default: 30000 ms)
- **`WatchdogManager_WDT_TASK_INTERVAL_MS`** (Time between feeding)

### 6.6 Functions

1. **`int WatchdogManager_InitWatchdog(unsigned long timeout_ms)`**  
   - **Description**:  
     Initializes the hardware/software watchdog timer with the specified timeout.  
   - **Behavior**:  
     1. Configures the ESP32 watchdog registers.  
     2. Returns `0` if successful, or an error code if it fails.

2. **`void WatchdogManager_FeedWatchdog()`**  
   - **Description**:  
     Resets the watchdog timer countdown to prevent a reset.  
   - **Behavior**:  
     - Called periodically in the main loop or other tasks.

3. **`void WatchdogManager_ForceDailyRestart()`**  
   - **Description**:  
     Triggers a forced restart every 24 hours (or at a configured time).  
   - **Behavior**:  
     1. Tracks the last reset time.  
     2. If 24 hours have elapsed, performs a software or hardware reset.

### 6.7 Error Handling
- **18** (`ERR_WDT_INIT_FAIL`)  
*(Resets are typically handled by hardware if the watchdog is not fed.)*

---

## 7. OTAUpdater Module

### 7.1 Description
Manages Over-The-Air updates for the ESP32 firmware, including checking for new versions, downloading, validating, and flashing.

### 7.2 Responsibilities
1. Periodically check for new firmware at a given URL.  
2. Download firmware securely.  
3. Validate firmware integrity (checksum/signature).  
4. Flash new firmware and reboot if successful.

### 7.3 Global Variables
*None by default.*

### 7.4 Local Variables
- Temporary buffer for downloaded firmware.
- State flags (e.g., “updateInProgress”).

### 7.5 Parameters
- **`OTAUpdater_OTA_SERVER_URL`**  
- **`OTAUpdater_OTA_CHECK_INTERVAL`**  
- **`OTAUpdater_OTA_SECURE`** (boolean, use HTTPS or not)

### 7.6 Functions

1. **`int OTAUpdater_InitOTA()`**  
   - **Description**:  
     Sets up the device for OTA (network or Wi-Fi / SIM808 usage).  
   - **Behavior**:  
     - Returns `0` on success or error code if not possible (memory partition issues, etc.).

2. **`int OTAUpdater_CheckForUpdates()`**  
   - **Description**:  
     Compares the current firmware version with what’s available at `OTAUpdater_OTA_SERVER_URL`.  
   - **Behavior**:  
     - Returns `0` if no update, or a special code if an update is ready, or an error code on failure.

3. **`int OTAUpdater_PerformOTAUpdate()`**  
   - **Description**:  
     Downloads and flashes the new firmware.  
   - **Behavior**:  
     1. Downloads binary from `OTAUpdater_OTA_SERVER_URL`.  
     2. Verifies integrity (checksum/signature).  
     3. Writes to the OTA partition.  
     4. Reboots on success.  
     5. Returns `0` if successful, or an error code otherwise.

### 7.7 Error Handling
- **19** (`ERR_OTA_INIT_FAIL`)  
- **20** (`ERR_OTA_DOWNLOAD_FAIL`)  
- **21** (`ERR_OTA_INVALID_FIRMWARE`)  
- **22** (`ERR_OTA_FLASH_FAIL`)

---

## 8. UnitTestingModule

### 8.1 Description
Provides a framework to test each module’s functions in isolation, mocking any hardware or external dependencies.

### 8.2 Responsibilities
1. Create test functions for **ServerConnection**, **SoilSensorManager**, **SleepManager**, **BatteryMonitor**, **DataLogger**, **WatchdogManager**, and **OTAUpdater**.  
2. Mock dependencies (e.g., RS485 interface, SIM808 responses, SD card writes).  
3. Return pass/fail or specific error codes for each test scenario.

### 8.3 Global Variables
*None typically required; test scaffolding remains local.*

### 8.4 Local Variables
- Mock data buffers (e.g., simulated sensor responses).
- Expected vs. actual results.

### 8.5 Parameters
- **`UnitTestingModule_TestLevel`** (e.g., basic vs. advanced testing).

### 8.6 Functions

1. **`bool UnitTestingModule_RunServerConnectionTests()`**  
   - Mocks SIM808 AT command responses and checks if `ServerConnection_InitSim808` and `ServerConnection_Send_NPK_Data` handle them correctly.  
   - Returns `true` if all tests pass, `false` otherwise.

2. **`bool UnitTestingModule_RunSoilSensorManagerTests()`**  
   - Mocks RS485 responses for reading registers, checks CRC, etc.  
   - Verifies calibration function with dummy data.  
   - Returns `true` on success.

3. **`bool UnitTestingModule_RunSleepManagerTests()`**  
   - Tests function calls but may not actually put device into sleep. Mocks system clock.  
   - Returns `true` if all cases pass.

4. **`bool UnitTestingModule_RunBatteryMonitorTests()`**  
   - Mocks ADC readings.  
   - Verifies correct voltage calculations.  
   - Returns `true` if correct.

5. **`bool UnitTestingModule_RunDataLoggerTests()`**  
   - Mocks SD card reads/writes, checks encryption calls.  
   - Confirms folder/file creation logic.  
   - Tests rotation logic for logs.  
   - Returns `true` if successful.

6. **`bool UnitTestingModule_RunWatchdogManagerTests()`**  
   - Mocks hardware watchdog or feed calls.  
   - Verifies daily restart logic triggers properly.  
   - Returns `true` if successful.

7. **`bool UnitTestingModule_RunOTAUpdaterTests()`**  
   - Mocks a firmware server response, partial/broken downloads, etc.  
   - Checks proper error code for invalid firmware.  
   - Returns `true` if all tests pass.

8. **`void UnitTestingModule_Summary()`**  
   - Prints a summary of pass/fail for each test function.

---

# Expanded Error Codes Reference

| Code | Name                              | Definition                                                          |
|------|-----------------------------------|---------------------------------------------------------------------|
| **0**  | NO_ERROR                          | Everything executed successfully.                                   |
| **1**  | ERR_SIM808_INIT_FAIL              | SIM808 did not respond to initialization (ServerConnection).        |
| **2**  | ERR_SIM808_TIMEOUT                | SIM808 timed out (ServerConnection).                                |
| **3**  | ERR_SIM_NOT_INSERTED              | SIM card missing or unreadable (ServerConnection).                  |
| **4**  | ERR_NETWORK_REG_FAIL              | SIM808 failed to register on network (ServerConnection).            |
| **5**  | ERR_API_KEY_INVALID               | Invalid ThingSpeak API key (ServerConnection).                      |
| **6**  | ERR_HTTP_CONNECTION_FAIL          | Cannot connect to ThingSpeak (ServerConnection).                    |
| **7**  | ERR_HTTP_SEND_FAIL                | Failed to send HTTP request (ServerConnection).                     |
| **8**  | ERR_HTTP_RESPONSE_INVALID         | Invalid or incomplete server response (ServerConnection).           |
| **9**  | ERR_DATA_FORMAT                   | Sensor data out of range or incorrect format.                       |
| **10** | ERR_UNKNOWN                       | An unspecified error occurred.                                      |
| **11** | ERR_RS485_COMM_FAIL               | RS485 communication error (SoilSensorManager).                      |
| **12** | ERR_SENSOR_CALIBRATION            | Sensor calibration failed (SoilSensorManager).                      |
| **13** | ERR_SLEEP_CONFIG                  | Invalid sleep configuration (SleepManager).                         |
| **14** | ERR_ADC_READ_FAIL                 | ADC read failed (BatteryMonitor).                                   |
| **15** | ERR_SD_INIT_FAIL                 | SD card initialization error (DataLogger).                          |
| **16** | ERR_SD_WRITE_FAIL                | Failed to write to SD card (DataLogger).                            |
| **17** | ERR_SD_READ_FAIL                 | Failed to read from SD card (DataLogger).                           |
| **18** | ERR_WDT_INIT_FAIL                | Watchdog initialization failure (WatchdogManager).                  |
| **19** | ERR_OTA_INIT_FAIL                | OTA environment initialization failed (OTAUpdater).                 |
| **20** | ERR_OTA_DOWNLOAD_FAIL            | OTA firmware download failed (OTAUpdater).                          |
| **21** | ERR_OTA_INVALID_FIRMWARE         | OTA firmware checksum/signature mismatch (OTAUpdater).              |
| **22** | ERR_OTA_FLASH_FAIL               | Error writing firmware to flash (OTAUpdater).                       |

---

# Final Notes

1. **Order of Initialization**:  
   - Initialize **Watchdog** → **DataLogger** (SD card) → **BatteryMonitor**, **SoilSensorManager**, **ServerConnection** → Attempt data read/send → Check for OTA updates → Enter deep sleep.  
2. **Daily Reset**:  
   - **WatchdogManager** enforces a forced daily reset.  
3. **Data & Calibration**:  
   - **SoilSensorManager** uses global arrays for sensor values and calibration.  
   - **DataLogger** can update/read them from the SD card as needed.  
4. **Logging & Encryption**:  
   - **DataLogger** module ensures logs/connection info are encrypted.  
   - CSV files for sensor data, calibration, and battery data use `;` delimiter.  
5. **Unit Testing**:  
   - **UnitTestingModule** thoroughly tests all modules with mocks.  
   - This fosters confidence in production software readiness.  
6. **No Dynamic Memory** / **No RTOS Tasks**:  
   - The design must rely on static allocation and a loop-driven structure for reliability and simplicity.  

These comprehensive requirements—with precise communication specs, memory constraints, encryption methods, data formats, timing details, calibration flows, and a testing module—should position the system well for **production-level** development. 
