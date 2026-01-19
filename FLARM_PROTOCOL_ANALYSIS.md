# Flarm Protocol Driver Analysis - Inconsistencies Found

## Critical Issues

### 1. Sequence Number Increment Bug in FLARM::PrepareFrameHeader
**Location**: `src/Device/Driver/FLARM/BinaryProtocol.cpp:163`

**Issue**: The namespace function `FLARM::PrepareFrameHeader` increments the `sequence_number` parameter, but since it's passed by value, this increment has no effect on the caller. The increment should be removed from this function.

```cpp
// Current (BUGGY):
FLARM::FrameHeader
FLARM::PrepareFrameHeader(unsigned sequence_number, MessageType message_type,
                          std::span<const std::byte> payload) noexcept
{
  FrameHeader header;
  header.length = 8 + payload.size();
  header.version = 0;
  header.sequence_number = sequence_number++;  // BUG: increments parameter, not caller's variable
  header.type = message_type;
  header.crc = CalculateCRC(header, payload);
  return header;
}
```

**Impact**: While the member function `FlarmDevice::PrepareFrameHeader` correctly increments the member variable before calling this, the increment in the namespace function is dead code and misleading.

**Fix**: Remove the `++` from line 163:
```cpp
header.sequence_number = sequence_number;
```

### 2. Buffer Overflow Risk in Settings.cpp
**Location**: `src/Device/Driver/FLARM/Settings.cpp:24, 52`

**Issue**: `sprintf` is used without bounds checking. If `name` and `value` are long, this could overflow the 64-byte buffer.

```cpp
// Line 24:
char buffer[64];
sprintf(buffer, "PFLAC,S,%s,%s", name, value);  // No bounds checking

// Line 52:
char buffer[64];
sprintf(buffer, "PFLAC,R,%s", name);  // No bounds checking
```

**Impact**: Potential buffer overflow leading to undefined behavior or security issues.

**Fix**: Use `snprintf` with proper bounds:
```cpp
char buffer[64];
snprintf(buffer, sizeof(buffer), "PFLAC,S,%s,%s", name, value);
```

### 3. Sequence Number Never Reset
**Location**: `src/Device/Driver/FLARM/Device.hpp:36`

**Issue**: The `sequence_number` member variable is initialized to 0 but never reset. If the device reconnects or mode switches occur, the sequence number continues from where it left off, which could cause issues if it wraps around or if there are communication errors.

**Impact**: Potential sequence number mismatches after reconnection or mode switches.

**Fix**: Reset sequence_number when:
- Switching to binary mode
- On link timeout
- When mode is set to UNKNOWN due to errors

### 4. Inconsistent Error Handling in WaitForACKOrNACK
**Location**: `src/Device/Driver/FLARM/BinaryProtocol.cpp:195-245`

**Issue**: The function silently continues on various error conditions (CRC mismatch, wrong message type, etc.) without any logging or distinction between different failure modes. All failures result in `MessageType::ERROR`, making debugging difficult.

**Impact**: Difficult to diagnose communication problems.

**Fix**: Consider adding more detailed error reporting or at least distinguishing between timeout and protocol errors.

## Moderate Issues

### 5. Mode State Management Inconsistencies
**Location**: Multiple files

**Issue**: The `mode` variable is set to `UNKNOWN` in various error paths, but there's no consistent pattern:
- `Logger.cpp:273, 355, 359` - sets to UNKNOWN on errors
- `Declare.cpp:19` - sets to UNKNOWN on failure
- `Mode.cpp:34, 57` - sets to UNKNOWN during transitions
- `Device.cpp:19` - sets to UNKNOWN on link timeout

**Impact**: Unclear state management makes it hard to reason about the device state.

**Recommendation**: Document when and why mode should be set to UNKNOWN, or create a helper function.

### 6. Missing Error Handling in BinaryReset
**Location**: `src/Device/Driver/FLARM/BinaryProtocol.cpp:284-296`

**Issue**: `BinaryReset` sends an EXIT message but doesn't wait for ACK or handle errors. The comment says "wait for positive answer" but the code doesn't.

```cpp
void
FlarmDevice::BinaryReset(OperationEnvironment &env,
                         std::chrono::steady_clock::duration _timeout)
{
  TimeoutClock timeout(_timeout);

  // Create header for sending a binary reset request
  FLARM::FrameHeader header = PrepareFrameHeader(FLARM::MessageType::EXIT);

  // Send request and wait for positive answer  <-- COMMENT SAYS THIS BUT CODE DOESN'T
  SendStartByte();
  SendFrameHeader(header, env, timeout.GetRemainingOrZero());
  // Missing: WaitForACK or error handling
}
```

**Impact**: No verification that the reset command was received/processed.

### 7. Inconsistent Timeout Usage
**Location**: Multiple locations

**Issue**: Timeout values are hardcoded in various places with different values:
- `BinaryPing`: 500ms
- `BinaryReset`: 500ms  
- `ReadFlightInfo`: 1s (send), 5s (receive)
- `SelectFlight`: 1s
- `DownloadFlight`: 1s (send), 10s (receive)
- `GetConfig`: 2s
- `SetConfig`: 2s

**Impact**: No consistent timeout strategy, may cause issues with slow devices.

### 8. Comment vs Implementation Mismatch in Declare.cpp
**Location**: `src/Device/Driver/FLARM/Declare.cpp:23-26`

**Issue**: There's a TODO comment about FLARM declaration checks that mentions "183 bytes" but the code comment at line 91-105 mentions "192 bytes". The actual limit according to the comment is 192 bytes.

**Impact**: Confusing documentation.

## Minor Issues

### 9. Inconsistent Return Value Documentation
**Location**: `src/Device/Driver/FLARM/Device.hpp:64-66`

**Issue**: The comment for `RequestSetting` says it returns `true` if successful, but the function signature shows `void`.

```cpp
/**
 * @return true if sending the command has succeeded (it does not
 * indicate whether the FLARM has understood and processed it)
 */
void RequestSetting(const char *name, OperationEnvironment &env);
```

**Fix**: Update the comment to match the implementation (void return).

### 10. Potential Integer Overflow in ParseTime Comment
**Location**: `src/Device/Driver/FLARM/Logger.cpp:52`

**Issue**: Comment says "Parse year" but the code parses "hour". This is a copy-paste error from ParseDate.

```cpp
static bool
ParseTime(const char *str, BrokenTime &time)
{
  char *endptr;

  // Parse year  <-- WRONG COMMENT, should be "Parse hour"
  time.hour = strtoul(str, &endptr, 10);
```

### 11. Missing Sequence Number Validation
**Location**: `src/Device/Driver/FLARM/BinaryProtocol.cpp:240-241`

**Issue**: The sequence number comparison uses a direct cast without checking endianness or alignment. While `FromLE16` is used, the cast to `uint16_t*` assumes proper alignment.

**Impact**: Potential issues on architectures with strict alignment requirements.

### 12. PFLAC Response Format Inconsistency (Emulator vs Implementation)
**Location**: `src/Device/Driver/FLARM/Device.cpp:183-189, 216-221` vs `test/src/FLARMEmulator.cpp:30-33`

**Issue**: There's a discrepancy between what the emulator sends and what the implementation expects:

**Emulator behavior** (`FLARMEmulator.cpp:30-33`):
- PFLAC,S response: `PFLAC,A,<name>,<value>` (includes value)
- PFLAC,R response: `PFLAC,A,<name>,<value>` (includes value)

**Implementation expectation** (`Device.cpp`):
- PFLAC,S response: `PFLAC,A,<name>` (no value expected, line 216-217)
- PFLAC,R response: `PFLAC,A,<name>,<value>` (value expected, line 183-189)

**Impact**: This suggests either:
1. The implementation is wrong and should accept value in PFLAC,S response
2. The emulator is wrong and shouldn't send value in PFLAC,S response
3. The specification allows both formats

**Fix**: Verify against FTD-014 specification which format is correct for PFLAC,S acknowledgment response.

## FTD-014 Configuration Specification Compliance

**Reference**: FTD-014-FLARM-Configuration-Specification-1.18.pdf

The following items need to be verified against the official FLARM Configuration Specification:

### PFLAC Command Format Compliance

#### 1. PFLAC Request Format (PFLAC,R)
**Implementation**: `Settings.cpp:48-54`, `Device.cpp:180-190`

**Current Implementation**:
```cpp
char buffer[64];
sprintf(buffer, "PFLAC,R,%s", name);
Send(buffer, env);
```

**Specification Check Required**:
- [ ] Verify command format matches spec: `$PFLAC,R,<name>*<checksum>`
- [ ] Verify setting name format/case requirements
- [ ] Verify response format: `$PFLAC,A,<name>,<value>*<checksum>` or `$PFLAC,A,ERROR*<checksum>`
- [ ] Check if all setting names are valid per specification

**Implemented Setting Names** (need verification):
- `PILOT` - Pilot name
- `COPIL` - Co-pilot name  
- `GLIDERTYPE` - Aircraft type
- `GLIDERID` - Aircraft registration
- `COMPID` - Competition ID
- `COMPCLASS` - Competition class
- `PRIV` - Privacy/Stealth mode (0/1)
- `RANGE` - Range setting
- `BAUD` - Baud rate
- `BAUD1`, `BAUD2` - PowerFLARM baud rates
- `THRE` - Threshold
- `ACFT` - Aircraft type (numeric?)
- `LOGINT` - Logging interval
- `NOTRACK` - No tracking mode
- `NEWTASK` - Task declaration command
- `ADDWP` - Add waypoint command

#### 2. PFLAC Set Format (PFLAC,S)
**Implementation**: `Settings.cpp:10-26`, `Device.cpp:209-222`

**Current Implementation**:
```cpp
char buffer[64];
sprintf(buffer, "PFLAC,S,%s,%s", name, value);
Send(buffer, env);
```

**Specification Check Required**:
- [ ] Verify command format: `$PFLAC,S,<name>,<value>*<checksum>`
- [ ] Verify response format: `$PFLAC,A,<name>*<checksum>` (acknowledgment)
- [ ] Check value format requirements for each setting type
- [ ] Verify error response format: `$PFLAC,A,ERROR*<checksum>`
- [ ] Check if value length limits are enforced per specification

#### 3. PFLAC Response Parsing
**Implementation**: `Parser.cpp:12-29`

**Current Implementation**:
```cpp
bool FlarmDevice::ParsePFLAC(NMEAInputLine &line)
{
  [[maybe_unused]] const auto responsetype = line.ReadView();  // Should be "A"
  const auto name = line.ReadView();
  if (name == "ERROR"sv)
    return true;  // ignore error responses
  const auto value = line.Rest();
  // Store in settings map
}
```

**Specification Check Required**:
- [ ] Verify response type field handling (currently ignored with `[[maybe_unused]]`)
- [ ] Verify ERROR response handling - should this be logged/returned differently?
- [ ] Check if all response variants are handled (A, E, etc.)
- [ ] Verify value parsing handles all data types correctly

### PFLAR Command (Restart)
**Implementation**: `Device.cpp:260-264`

**Current Implementation**:
```cpp
void FlarmDevice::Restart(OperationEnvironment &env)
{
  Send("PFLAR,0", env);
}
```

**Specification Check Required**:
- [ ] Verify command format: `$PFLAR,<mode>*<checksum>`
- [ ] Verify mode parameter values (0 = restart?)
- [ ] Check if response/acknowledgment is expected
- [ ] Verify timing requirements after restart

### PFLAX Command (Binary Mode Switch)
**Implementation**: `Mode.cpp:53-55`

**Current Implementation**:
```cpp
Send("PFLAX", env);
```

**Specification Check Required**:
- [ ] Verify command format: `$PFLAX*<checksum>`
- [ ] Verify response format (if any)
- [ ] Check timing requirements for mode switch
- [ ] Verify binary mode handshake sequence

### PFLAI Command (Pilot Event)
**Implementation**: `Device.cpp:22-27`

**Current Implementation**:
```cpp
bool FlarmDevice::PutPilotEvent(OperationEnvironment &env)
{
  Send("PFLAI,PILOTEVENT", env);
  return true;
}
```

**Specification Check Required**:
- [ ] Verify command format: `$PFLAI,<event>*<checksum>`
- [ ] Verify event type "PILOTEVENT" is correct
- [ ] Check if other event types are supported
- [ ] Verify response/acknowledgment requirements

### Declaration Commands (Task Declaration)
**Implementation**: `Declare.cpp:31-138`

**Current Implementation**:
- Uses `PFLAC,S,NEWTASK,Task`
- Uses `PFLAC,S,ADDWP,<coordinates>,<name>`
- Uses `PFLAC,S,ADDWP,0000000N,00000000E,L` for landing

**Specification Check Required**:
- [ ] Verify NEWTASK command format and parameters
- [ ] Verify ADDWP format: `DDMMmmmN/S,DDDMMmmmE/W,<type>,<name>`
- [ ] Verify coordinate format (degrees, minutes, thousandths)
- [ ] Verify waypoint type codes (T=takeoff, L=landing, etc.)
- [ ] Check 192-byte total limit enforcement
- [ ] Verify maximum waypoint count (10 waypoints excluding takeoff/landing)
- [ ] Check if waypoint name length limits are enforced (currently limited to 6 chars)

### Setting Value Format Compliance

#### String Settings
**Implementation**: Various `GetConfig`/`SetConfig` calls

**Specification Check Required**:
- [ ] Verify maximum length for string settings (PILOT, COPIL, GLIDERTYPE, etc.)
- [ ] Verify character encoding requirements (UTF-8?)
- [ ] Check if special characters need escaping
- [ ] Verify case sensitivity requirements

#### Numeric Settings
**Implementation**: `GetUnsignedValue`, `GetRange`, `GetBaudRate`, etc.

**Specification Check Required**:
- [ ] Verify valid value ranges for each numeric setting
- [ ] Verify BAUD value mapping (0=4800, 1=9600, 2=19200, 4=38400, 5=57600, etc.)
- [ ] Check if PowerFLARM BAUD1/BAUD2 support additional values (6=115200, 7=230400)
- [ ] Verify THRE (threshold) valid range (0-20 or 255 for auto)
- [ ] Verify ACFT (aircraft type) valid values
- [ ] Verify LOGINT (logging interval) valid range

#### Boolean Settings
**Implementation**: `GetStealthMode`, `SetStealthMode`

**Specification Check Required**:
- [ ] Verify PRIV setting format (0/1 vs true/false)
- [ ] Verify NOTRACK setting format
- [ ] Check if other boolean settings exist

### Error Handling Compliance

**Specification Check Required**:
- [ ] Verify all error response formats match specification
- [ ] Check if PFLAC,A,ERROR responses are properly handled
- [ ] Verify timeout values match specification recommendations
- [ ] Check if retry logic is required per specification

### Binary Protocol Compliance

**Reference**: Should also check against binary protocol specification if available

**Specification Check Required**:
- [ ] Verify frame header structure matches specification
- [ ] Verify escape sequence handling (0x73, 0x78, 0x55, 0x31)
- [ ] Verify CRC calculation algorithm
- [ ] Verify sequence number handling
- [ ] Check message type values (0x00, 0xA0, 0xB7, 0x01, etc.)
- [ ] Verify protocol version handling

## Summary

The most critical issues are:
1. **Sequence number increment bug** (though it may not cause runtime issues due to how it's called)
2. **Buffer overflow risks** in sprintf calls
3. **Sequence number never reset** - could cause issues after reconnection
4. **Missing error handling** in BinaryReset

**Specification Compliance**: Many items need verification against FTD-014:
- PFLAC command/response formats
- Setting name validity
- Value format requirements
- Declaration command formats
- Error handling requirements

The code would benefit from:
- Consistent error handling patterns
- Better state management documentation
- Bounds checking on all string operations
- Sequence number reset on mode changes/timeouts
- **Verification against FTD-014 specification for all PFLAC commands and responses**

