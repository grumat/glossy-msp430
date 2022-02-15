# Step By Step TI Code Tracing

This doc forms a trace of each TI step for the MSP430.dll and the UIF v1 
firmware. We use a python-styled algorithm to broadly describe involved 
methods.

Sample Program:

```cpp
int32_t lVersion; // MSPDebugStack version

// init JTAG interface – TIUSB will use first connected debugger
if(MSP430_Initialize("TIUSB", &lVersion) == STATUS_ERROR)
{
    printf("Error: %s\n", MSP430_Error_String(MSP430_Error_Number())); // print error string
    MSP430_Close(1); // close the debug session
}

// Set target architecture
if (MSP430_SetTargetArchitecture(MSP430) == STATUS_ERROR)
{
    printf("Error: %s\n", MSP430_Error_String(MSP430_Error_Number())); // print error string
    MSP430_Close(1); // close the debug session and turn VCC off
}

// Check firmware compatibility
if(lVersion < 0) // firmware outdated?
{
    // perform firmware update
    if(MSP430_FET_FwUpdate(NULL, NULL, NULL) == STATUS_ERROR)
    {
        printf("Error: %s\n", MSP430_Error_String(MSP430_Error_Number())); // print error string
        MSP430_Close(1); // close the debug session and turn VCC off
    } 
}

// power up the target device
if(MSP430_VCC(3000) == STATUS_ERROR) // target VCC in millivolts
{ 
    printf("Error: %s\n", MSP430_Error_String(MSP430_Error_Number())); // print error string 
    MSP430_Close(1); // close the debug session and turn VCC off 
}

// configure interface - this is optional! automatic interface selection is the default
if(MSP430_Configure(INTERFACE_MODE, AUTOMATIC_IF) == STATUS_ERROR)
{
    printf("Error: %s\n", MSP430_Error_String(MSP430_Error_Number())); // print error string
    MSP430_Close(1); // close the debug session and turn VCC off
}

// If the device is password protected, use MSP430_OpenDevice with appropriate password
if(passwordLen > 0)
{
    if(MSP430_OpenDevice("DEVICE_UNKNOWN", password, passwordLen, 0, DEVICE_UNKNOWN)
        == STATUS_ERROR)
    {
        printf("Error: %s\n", MSP430_Error_String(MSP430_Error_Number())); // print error string 
        MSP430_Close(1); // close the debug session and turn VCC off
    }
}
else 
{
    if(MSP430_OpenDevice("DEVICE_UNKNOWN", "", 0, 0, DEVICE_UNKNOWN) == STATUS_ERROR)
    {
        printf("Error: %s\n", MSP430_Error_String(MSP430_Error_Number())); // print error string
        MSP430_Close(1); // close the debug session and turn VCC off
    }
}

// program .txt file into device memory (optional) 
if(MSP430_ProgramFile("C:\file.txt", ERASE_ALL, verify) == STATUS_ERROR)
{
    printf("Error: %s\n", MSP430_Error_String(MSP430_Error_Number())); // print error string
    MSP430_Close(1); // close the debug session and turn VCC off
}
/**************************** debug session is started ****************************/
```

# MSP430_Initialize

This method just initializes system and creates a connection to the windows 
device. It also checks the firmware version.

The method is not directly required fo the MSPBMP device.


# MSP430_SetTargetArchitecture

This method is also not required by the MSPBMP, as it enables support for 
the MSP432.


# MSP430_FET_FwUpdate

This method is not required by the MSPBMP, as it implements the firmware 
update for TI devices.

# MSP430_VCC

This method is also not required by the MSPBMP.

# MSP430_Configure

This method is sued to specify the device interface, such as JTAG SBW or 
automatic. Similar feature will be developed in the future, so this will not 
be covered now.

# Passwords

Currently unsupported.

# MSP430_OpenDevice

```cpp
bool OpenDevice(const char* Device  // "DEVICE_UNKNOWN"
    , const char* Password          // ""
    , int32_t PwLength              // 0
    , int32_t DeviceCode            // 0
    , int32_t setId)                // DEVICE_UNKNOWN (0)
```

Very important for the MSPBMP for device initialization.

```python
# Handle Special devices
if Device ~ "MSP430C09":
    Set DeviceCode to 0xDEADBABE
elif Device ~ "MSP430I":
    Set DeviceCode to 0x20404020
elif Device one of "MSP430L09", "MSP430C09":
    The DeviceCode parameter shall specify the device code
    Set Interface Mode to JTAG

if Identify():
    Read Memory Area
```

## 1) `Identify()`

```cpp
bool Identify(uint8_t* buffer       // DEVICE_T structure (see MSP430.h)
    , int32_t count                 // sizeof(DEVICE_T)
    , int32_t setId                 // argument DEVICE_UNKNOWN (0)
    , const char* Password          // argument Password
    , int32_t PwLength              // argument PwLength
    , int32_t code)                 // argument DeviceCode
```

The step summary follows below (without MSP432 stuff).

```python
# Validate device database stuff
if Device Database is not valid:
    return Error
if setId not in range of Device Database:
    return Error

# Hardware may draw to much current at startup
Disable over-current protection

# Resolve automatic JTAG interface mode
if Jtag Mode is automatic:
    ifMode = getInterfaceMode   # ID_GetInterfaceMode
    setJtagMode(ifMode)

setDeviceCode(code)     # default is 0

if not start():
    Handle Error states
# After this point, call stop() for new errors
# using the scope of a smart pointer

createDeviceHandle()    # This clears the EemTimer values

if setId is specified:
    # attach to running target
    if getJtagId() fails:
        # if no jtag id was found the device is locked or lpmx.5
        setId = DEVICE_UNKNOWN

if setId is not specified:
    # This will reset and pause the target MCU
    setId = identifyDevice()    # afterMagicPattern = false
    if setId is failure:
        isFuseBlown = isJtagFuseBlown()

        if deviceCode is 0x20404020:
            MSP430I_MagicPattern()
        if no errors and JtagID != 0x89:    # except original MSP430
            magicPatternSend()
        if JtagID == 0x89:
            call HIL_CMD_BSL_1XX_4XX
            start()

        Handle Error States

        # If the device is locked by a password, unlock it
        if has password:
            start()
        # try to identify the device again
        setId = identifyDevice()    # afterMagicPattern = true

        if isJtagFuseBlown()
            return Error
        debug.state = STOPPED

else:
    # this is Attach to running target
    setDeviceId(setId)
    debug.state=RUNNING;

if Device is Legacy:
    return Error

Setup Debug Manager

# Loads device information from database
DeviceDb::Database().getDeviceInfo()

return Device()
```

## 2) `ConfigManager::start()`

```cpp
int16_t ConfigManager::start(
    const string& pwd
    , uint32_t deviceCode)
```
ref: `DLL430_v3\src\TI\DLL430\ConfigManager.cpp`

```python
# if we have an L092 Rom device
if deviceCode == 0xDEADBABE:
    call ID_UnlockC092
    return 1
# if we have an L092 device
elif deviceCode in (0xA55AA55A, 0x5AA55AA5):
    call ID_StartJtagActivationCode
    return 1
# if we have a device locked with a custom password
elif has password:
    call ID_UnlockDeviceXv2
    return 1
# if we have a "normal" msp430 device without special handling
else:
    call ID_StartJtag
    return numberOfDevices
```


## 3) ID_StartJtag

```cpp
HAL_FUNCTION(_hal_StartJtag)
```
ref: `Bios\src\hal\macros\StartJtag.c`

```python
IHIL_Open(RSTHIGH)          # General output activation
    _hil_Open()             # TapMcu::InitDevice()
    # Reset entry sequences.
    _hil_Connect()          # JtagDev::OnEnterTap() - Needs improvement.
IHIL_TapReset()             # ref: Bios\src\hil\uifv1\hil_4w.c
    # six pulses for *Test-Logic-Reset*
    _hil_4w_TapReset()      # JtagDev::OnResetTap()
IHIL_CheckJtagFuse()
    _hil_4w_CheckJtagFuse() # JtagDev::OnResetTap()
```

> This code captures device by means of the debug core.


## 4) `identifyDevice()`

```cpp
int32_t DeviceHandleMSP430::identifyDevice (
    uint32_t activationKey
    , bool afterMagicPattern)       // false for the first-time-call only
```

Ref: `DLL430_v3\src\TI\DLL430\DeviceHandleMSP430.cpp`

Identifies device by calling `getDeviceIdentity()`, then loads PC and SR and combines them
with the other registers.

```python
Send Configuration

# General validation
if isJtagFuseBlown():
    return Error
if jtagId is not valid:
    return Error
# Read identity values
devId = getDeviceIdentity()
if devId is not valid:
    return Error
# Loads device configuration
setDeviceId(devId)
# Read general registers
getCpuRegisters(devId)
# Cache RAM
getMemoryArea()
```

## 5) `getDeviceIdentity()`

```cpp
long DeviceHandleMSP430::getDeviceIdentity(
    uint32_t activationKey
    , uint32_t* pc
    , uint32_t* sr
    , bool afterMagicPattern)       // false for the first-time-call only
```

Ref: `DLL430_v3\src\TI\DLL430\DeviceHandleMSP430.cpp`

Core of target initialization. Selects the FET procedure, which performs almost 
everything independently.

> A very different behavior happens depending on `afterMagicPattern` flag. IMHO
> this should be split into two distinct routines.  
> Abstraction comes in trouble comes when **jtagId is 0x99**.

```python
# Selects the FET service ID
select hal_id:
    ID_SyncJtag_Conditional_SaveContextXv2 for isXv2 and afterMagicPattern and jtagId != 0x99
    ID_SyncJtag_AssertPor_SaveContextXv2 for isXv2
    ID_SyncJtag_AssertPor_SaveContext for any other case
# Watchdog hardware register
addHoldParamsTo()
# Copy EEM timers (in etwCodes global var)
for eemTimer in etwCodes:
    appendInputData8(eemTimer)
# Enqueue a double sequence of calls
call hal_id
call ID_GetDeviceIdPtr
# Return values depends on hal_id value
wdtCtrl = pop() # WDT original value.
if hal_id != ID_SyncJtag_Conditional_SaveContextXv2:
    *pc = pop()
    *sr = pop()
else:
    *pc = ReamMemWord(0xFFFE)   # reset interrupt vector
    *sr = 0
# Load device identity information
deviceIdPtr = pop()
if not Xv2:
    idDataAddr = pop()
    idCode.version = ReadMemWord(idDataAddr)
    idCode.subversion = 0x0000
    idCode.revision = ReadMemWord(idDataAddr + 2).low()
    idCode.fab = ReadMemWord(idDataAddr + 2).high()
    idCode.self = ReadMemWord(idDataAddr + 4)
    idCode.config = ReadMemWord(idDataAddr + 12).high() & 0x7f
    idCode.fuses = GetFuses()
    idCode.activationKey = 0
else:
    # must be xv2 CPU device 99, 95, 91
    idCode.version = ReadMemWord(deviceIdPtr + 4)
    idCode.subversion = 0x0000                              # init with zero = no sub id
    idCode.revision = ReadMemWord(deviceIdPtr + 6).low()    # HW Revision
    idCode.config = ReadMemWord(deviceIdPtr + 6).high()     # SW Revision
    # Some irrelevant "no open source" comes here
    idCode.subversion = getSubID()
    call ID_EemDataExchangeXv2
    eemVersion = pop()      # This is nowhere used!!!
    # Other values are irrelevant. Set to unlikely values...
    idCode.fab = 0x55
    idCode.self = 0x5555
    idCode.fuses = 0x55
    idCode.activationKey = activationKey

# Query the database
return DeviceDb::Database().findDevice(idCode)
```

## MSPBSP Equivalence Table

| FET | MSPBSP |
| --- | ------ |
| MSP430_OpenDevice() | TapMcu::Open() |
| Identify()          |  |
| start()             | TapMcu::InitDevice() |
| ID_UnlockDeviceXv2  |  *not ported* |
| ID_StartJtag        | TapMcu::OnConnectJtag() |
| IHIL_Open           | JtagDev::OnEnterTap() |
| IHIL_TapReset       | JtagDev::OnResetTap() |
| IHIL_CheckJtagFuse  | JtagDev::OnResetTap() |
| IdentifyDevice      |  |

## 6) `ID_GetDeviceIdPtr`

## 7) `getSubID()`

## 8) `ID_EemDataExchangeXv2`

## 9) `setDeviceId`

```cpp
int32_t DeviceHandleMSP430::setDeviceId (long id)
```

Device is located in the database and all configurations are set
in this routine, such as the EEM timers.

```python
if DeviceDb.getDeviceInfo():
    configure()
```

## 10) `Device()`

Fills the `Device_t` structure returned by the driver.

## Rough Call Graph

A rough call graph for Xv2 CPU case is shown below:

![fn_call.svg](images/fn_call.svg)

# Conclusion

Based on this analysis the EEM Timers are never set, as the only 
possible way to have it's values occurs after the **DB.findDevice**
and **setDeviceId** steps, which occurs after 
`ID_SyncJtag_AssertPor_SaveContextXv2`.

> At the moment these are already ported. On a extreme case we can 
> disable this information and free near 1K flash memory.
