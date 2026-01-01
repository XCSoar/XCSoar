# FLARM FTD-014 Specification Analysis

**Document**: FTD-014.txt (appears to be header/table of contents only)  
**Version**: 1.18 (2024-12-19)  
**Date in file**: 2022-06-06 (Version 1.1)

## Available Information from FTD-014.txt

The provided text file contains:
- Document header and version history
- Table of contents showing configuration items
- References to PFLAC commands in changelog

### PFLAC Command References Found

1. **$PFLAC,R,LIC** (line 35)
   - Indicates read command format: `$PFLAC,R,<name>`
   - Confirms setting names are case-sensitive (uppercase)

2. **$PFLAC,A,TASK** (line 54)
   - Indicates acknowledgment response format: `$PFLAC,A,<name>`
   - Version 1.11 added "command termination" for this response
   - **IMPORTANT**: This suggests the response format is `$PFLAC,A,<name>` (without value)
   - This supports the current implementation's expectation

### Configuration Items Listed in Table of Contents

The document lists these configuration items (matching implementation):

#### General Settings (Section 3.1)
- ID
- NMEAOUT (NMEAOUT1, NMEAOUT2)
- BAUD (BAUD1, BAUD2) ✓ (implemented)
- ACFT ✓ (implemented)
- RANGE ✓ (implemented)
- VRANGE
- PRIV ✓ (implemented)
- NOTRACK ✓ (implemented)
- THRE ✓ (implemented)
- LOGINT ✓ (implemented)
- PILOT ✓ (implemented)
- COPIL ✓ (implemented)
- GLIDERID ✓ (implemented)

## Analysis: Implementation vs Specification

### PFLAC Response Format - RESOLVED

**Finding**: The specification reference to `$PFLAC,A,TASK` with "command termination" suggests the acknowledgment format is:
```
$PFLAC,A,<name>*<checksum>
```

**Current Implementation** (`Device.cpp:216-217`):
```cpp
NarrowString<90> expected_answer(buffer);
expected_answer[6u] = 'A';  // Changes 'S' to 'A'
// Expects: PFLAC,A,<name> (no value)
```

**Test Emulator** (`FLARMEmulator.cpp:30-33`):
```cpp
snprintf(buffer, ARRAY_SIZE(buffer), "PFLAC,A,%.*s,%s",
         (int)name.size(), name.data(), value_buffer.c_str());
// Sends: PFLAC,A,<name>,<value> (with value)
```

**Conclusion**: 
- ✅ **Implementation appears CORRECT** - specification suggests `$PFLAC,A,<name>` format
- ❌ **Test emulator is INCORRECT** - it includes the value in the acknowledgment
- **Action Required**: Fix the test emulator to match specification

### Setting Names Verification

All implemented setting names appear in the table of contents:
- ✅ PILOT (3.1.11)
- ✅ COPIL (3.1.12)
- ✅ GLIDERID (3.1.13)
- ✅ BAUD/BAUD1/BAUD2 (3.1.3)
- ✅ ACFT (3.1.4)
- ✅ RANGE (3.1.5)
- ✅ PRIV (3.1.7)
- ✅ NOTRACK (3.1.8)
- ✅ THRE (3.1.9)
- ✅ LOGINT (3.1.10)

**Missing from implementation** (but in specification):
- ID (3.1.1)
- NMEAOUT/NMEAOUT1/NMEAOUT2 (3.1.2)
- VRANGE (3.1.6)

**Not in specification table of contents** (but used in implementation):
- GLIDERTYPE - Need to verify if this is correct name or should be ACFT
- COMPID - Competition ID (need to verify)
- COMPCLASS - Competition class (need to verify)
- NEWTASK - Task declaration command (need to verify)
- ADDWP - Add waypoint command (need to verify)

### Version Information

**Specification Version**: 1.18 (2024-12-19)  
**File Header Version**: 1.1 (2022-06-06) - **INCONSISTENCY**

The file header shows version 1.1 from 2022, but the changelog goes up to 1.18 from 2024. This suggests the file may be incomplete or the header wasn't updated.

## Missing Information (Need Full Specification)

The provided text file is incomplete. To fully verify compliance, we need:

1. **Command Format Specifications**:
   - Exact format for `$PFLAC,R,<name>`
   - Exact format for `$PFLAC,S,<name>,<value>`
   - Response format details
   - Error response formats

2. **Setting Value Specifications**:
   - Valid value ranges for each setting
   - Value format requirements (string length, encoding, etc.)
   - Default values

3. **Task Declaration Specifications**:
   - NEWTASK command format
   - ADDWP command format and coordinate system
   - Waypoint name length limits
   - Total declaration size calculation (192 bytes)

4. **Other Commands**:
   - PFLAR (restart) format
   - PFLAX (binary mode) format
   - PFLAI (pilot event) format

5. **Error Handling**:
   - Error response formats
   - Timeout requirements
   - Retry logic

## Recommendations

### Immediate Actions

1. **Fix Test Emulator** (`test/src/FLARMEmulator.cpp`):
   - Change PFLAC,S response to NOT include value
   - Response should be: `PFLAC,A,<name>` not `PFLAC,A,<name>,<value>`

2. **Verify Setting Names**:
   - Check if GLIDERTYPE should be ACFT or if both exist
   - Verify COMPID and COMPCLASS are valid setting names
   - Verify NEWTASK and ADDWP command formats

3. **Obtain Full Specification**:
   - Request complete FTD-014 document (54 pages)
   - Focus on sections 2 (Configuration Overview) and 3 (Configuration Items)

### Code Issues Confirmed

Based on the specification fragment, the following issues from the original analysis remain valid:

1. ✅ Buffer overflow risks (sprintf without bounds)
2. ✅ Sequence number handling issues
3. ✅ Missing error handling in BinaryReset
4. ✅ Test emulator response format mismatch (NEW FINDING)

## Next Steps

1. Obtain the complete FTD-014 specification document
2. Fix the test emulator to match specification
3. Verify all setting names against full specification
4. Complete the compliance checklist with full specification details



