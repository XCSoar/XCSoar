# FLARM FTD-014 Configuration Specification Compliance Checklist

**Document**: FTD-014-FLARM-Configuration-Specification-1.18.pdf  
**Date**: December 19, 2024  
**Version**: 1.18

This checklist should be used to verify the XCSoar FLARM driver implementation against the official specification.

## PFLAC Command Format

### PFLAC,R (Read Setting)

**Implementation Location**: 
- `src/Device/Driver/FLARM/Settings.cpp:48-54`
- `src/Device/Driver/FLARM/Device.cpp:177-190`

**Checklist**:
- [ ] Command format: `$PFLAC,R,<name>*<checksum><CR><LF>`
- [ ] Setting name format (case sensitivity, allowed characters)
- [ ] Response format: `$PFLAC,A,<name>,<value>*<checksum><CR><LF>`
- [ ] Error response format: `$PFLAC,A,ERROR*<checksum><CR><LF>` or `$PFLAC,A,<name>,ERROR*<checksum>`
- [ ] Timeout handling matches specification
- [ ] Checksum calculation/verification correct

**Current Implementation Notes**:
- Uses `sprintf` without bounds checking (64-byte buffer)
- Response parsing in `Parser.cpp:12-29`
- Response type field is read but marked `[[maybe_unused]]` - verify if this should be checked

### PFLAC,S (Set Setting)

**Implementation Location**:
- `src/Device/Driver/FLARM/Settings.cpp:10-26`
- `src/Device/Driver/FLARM/Device.cpp:209-222`

**Checklist**:
- [ ] Command format: `$PFLAC,S,<name>,<value>*<checksum><CR><LF>`
- [ ] Response format: `$PFLAC,A,<name>*<checksum><CR><LF>` (acknowledgment)
  - **CRITICAL**: Verify if response should include value: `$PFLAC,A,<name>,<value>*<checksum>`
  - Current implementation expects NO value in response
  - Test emulator sends value in response
  - **MUST VERIFY AGAINST SPECIFICATION**
- [ ] Error response format when setting is invalid
- [ ] Value format requirements for each setting type
- [ ] Value length limits per setting
- [ ] Special character handling in values
- [ ] Timeout handling matches specification

**Current Implementation Notes**:
- Uses `sprintf` without bounds checking (64-byte buffer)
- Expects response with 'A' replacing 'S' in position 6
- Expects response format: `PFLAC,A,<name>` (without value)
- Verifies checksum after response
- **INCONSISTENCY**: Test emulator (`FLARMEmulator.cpp`) sends value in response, but implementation doesn't expect it

## Configuration Settings

### Setting Names to Verify

For each setting, verify:
- [ ] Setting name spelling/case matches specification
- [ ] Setting is documented in FTD-014
- [ ] Value format matches specification
- [ ] Valid value ranges match specification
- [ ] Default values match specification

#### String Settings
- [ ] `PILOT` - Pilot name (max length, encoding)
- [ ] `COPIL` - Co-pilot name (max length, encoding)
- [ ] `GLIDERTYPE` - Aircraft type (max length, encoding)
- [ ] `GLIDERID` - Aircraft registration (max length, encoding)
- [ ] `COMPID` - Competition ID (max length, encoding)
- [ ] `COMPCLASS` - Competition class (max length, encoding)

#### Numeric Settings
- [ ] `BAUD` - Baud rate (valid values: 0=4800, 1=9600, 2=19200, 4=38400, 5=57600)
- [ ] `BAUD1` - PowerFLARM port 1 baud rate (additional: 6=115200, 7=230400?)
- [ ] `BAUD2` - PowerFLARM port 2 baud rate (additional: 6=115200, 7=230400?)
- [ ] `RANGE` - Range setting (valid range)
- [ ] `THRE` - Speed threshold (0-20 or 255 for auto)
- [ ] `ACFT` - Aircraft type numeric code (valid values)
- [ ] `LOGINT` - Logging interval (valid range, units)

#### Boolean Settings
- [ ] `PRIV` - Privacy/Stealth mode (0/1 format)
- [ ] `NOTRACK` - No tracking mode (0/1 format)

#### Task Declaration Settings
- [ ] `NEWTASK` - Start new task declaration (parameter format)
- [ ] `ADDWP` - Add waypoint (coordinate format, type codes, name format)

## PFLAR Command (Restart)

**Implementation Location**: `src/Device/Driver/FLARM/Device.cpp:260-264`

**Checklist**:
- [ ] Command format: `$PFLAR,<mode>*<checksum><CR><LF>`
- [ ] Mode parameter values (0 = restart?)
- [ ] Response/acknowledgment expected?
- [ ] Timing requirements after restart
- [ ] Device state after restart

**Current Implementation Notes**:
- Sends `PFLAR,0` - verify if mode parameter is correct
- No response handling - verify if needed

## PFLAX Command (Binary Mode Switch)

**Implementation Location**: `src/Device/Driver/FLARM/Mode.cpp:53-55`

**Checklist**:
- [ ] Command format: `$PFLAX*<checksum><CR><LF>`
- [ ] Response format (if any)
- [ ] Timing requirements for mode switch
- [ ] Binary mode handshake sequence
- [ ] Timeout for mode switch completion

**Current Implementation Notes**:
- Sends `PFLAX` command
- Waits up to 10 attempts (5 seconds total) for binary ping response
- Sets mode to UNKNOWN during transition

## PFLAI Command (Pilot Event)

**Implementation Location**: `src/Device/Driver/FLARM/Device.cpp:22-27`

**Checklist**:
- [ ] Command format: `$PFLAI,<event>*<checksum><CR><LF>`
- [ ] Event type "PILOTEVENT" is valid
- [ ] Other supported event types
- [ ] Response/acknowledgment requirements
- [ ] Timing requirements

**Current Implementation Notes**:
- Sends `PFLAI,PILOTEVENT`
- Always returns true (no error checking)

## Task Declaration (PFLAC,S with NEWTASK/ADDWP)

**Implementation Location**: `src/Device/Driver/FLARM/Declare.cpp:31-138`

### NEWTASK Command
**Checklist**:
- [ ] Command format: `$PFLAC,S,NEWTASK,<parameter>*<checksum>`
- [ ] Parameter value ("Task" - verify if correct)
- [ ] Response format
- [ ] Error handling

### ADDWP Command
**Checklist**:
- [ ] Command format: `$PFLAC,S,ADDWP,<coordinates>,<type>,<name>*<checksum>`
- [ ] Coordinate format: `DDMMmmmN/S,DDDMMmmmE/W`
  - [ ] Degrees (DD/DDD)
  - [ ] Minutes (MM)
  - [ ] Thousandths of minutes (mmm)
  - [ ] Direction (N/S, E/W)
- [ ] Waypoint type codes:
  - [ ] `T` - Takeoff
  - [ ] `L` - Landing
  - [ ] Other types?
- [ ] Waypoint name format:
  - [ ] Maximum length (currently limited to 6 chars)
  - [ ] Allowed characters
  - [ ] Encoding requirements
- [ ] Total declaration size limit: 192 bytes
  - [ ] Calculation: 7 + (Number of Waypoints * 9) + (sum of all descriptions)
- [ ] Maximum waypoint count: 10 waypoints (excluding takeoff/landing)
- [ ] Takeoff waypoint format: `0000000N,00000000E,T`
- [ ] Landing waypoint format: `0000000N,00000000E,L`

**Current Implementation Notes**:
- Uses `CopyCleanFlarmString` to limit waypoint names to 6 characters
- Comment mentions 192-byte limit but TODO mentions 183 bytes (inconsistency)
- No explicit size checking before sending

## Response Parsing

**Implementation Location**: `src/Device/Driver/FLARM/Parser.cpp:12-29`

**Checklist**:
- [ ] Response type field handling (currently 'A' for acknowledgment)
- [ ] ERROR response handling
- [ ] All response variants handled (A, E, etc.)
- [ ] Value parsing for all data types
- [ ] Empty value handling
- [ ] Multi-value responses (if any)

**Current Implementation Notes**:
- Response type is read but marked `[[maybe_unused]]`
- ERROR responses are ignored (return true)
- Value is stored as string in settings map

## Error Handling

**Checklist**:
- [ ] All error response formats match specification
- [ ] PFLAC,A,ERROR handling
- [ ] Timeout values match specification recommendations
- [ ] Retry logic per specification
- [ ] Error reporting to user

## Binary Protocol (if covered in FTD-014)

**Implementation Location**: `src/Device/Driver/FLARM/BinaryProtocol.*`

**Checklist**:
- [ ] Frame structure matches specification
- [ ] Start byte: 0x73
- [ ] Escape sequences: 0x78 (escape), 0x55 (escape-escape), 0x31 (escape-start)
- [ ] Frame header structure (8 bytes)
- [ ] CRC calculation algorithm
- [ ] Sequence number handling
- [ ] Message types match specification
- [ ] Protocol version handling

## General Protocol Compliance

**Checklist**:
- [ ] All NMEA sentences end with `<CR><LF>`
- [ ] Checksum calculation matches specification
- [ ] Checksum format: `*<2-hex-digits>`
- [ ] Sentence format: `$<talker><sentence>,<fields>*<checksum><CR><LF>`
- [ ] Timeout values are appropriate
- [ ] Character encoding (ASCII/UTF-8?)

## Implementation Issues Found (Separate from Spec Compliance)

See `FLARM_PROTOCOL_ANALYSIS.md` for code quality and consistency issues.

## Notes for Specification Review

When reviewing the FTD-014 document, pay special attention to:

1. **Setting Name Case Sensitivity**: Are setting names case-sensitive? (Current code uses uppercase)
2. **Value Format**: String vs numeric vs boolean formats
3. **Response Variants**: Are there response types other than 'A'?
4. **Error Responses**: Exact format of error responses
5. **Declaration Limits**: Exact calculation for 192-byte limit
6. **Waypoint Name Length**: Maximum allowed length per specification
7. **Timing Requirements**: Any specific timing requirements for commands
8. **PowerFLARM Differences**: Are BAUD1/BAUD2 and other PowerFLARM-specific settings documented?

