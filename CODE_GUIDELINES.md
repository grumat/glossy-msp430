# Glossy MSP430 - Code Guidelines

## 1. Introduction

This document defines the comprehensive coding standards for the Glossy MSP430 firmware project. These guidelines ensure consistency, maintainability, and professionalism across all code contributions.

**Scope**: Applies to all C/C++ firmware code, BMT library usage, and toolchain code (.NET tools).

**Application**: New code must follow these guidelines. Existing code will be updated on-demand during maintenance.

## 2. Basic Formatting

### 2.1 Line Endings
- **Windows CRLF** (`\r\n`) for all files
- Consistent across entire project
- No mixing of Unix LF line endings

### 2.2 Indentation
- **Hard tabs only** (not spaces)
- **Tab width**: 4 spaces equivalent
- No conversion of tabs to spaces
- Consistent throughout all C/C++ files

### 2.3 Encoding
- UTF-8 encoding preferred
- Byte Order Mark (BOM) permitted but not required
- ASCII for pure C source where appropriate

### 2.4 Line Length
- Maximum 100 characters per line
- Break long lines at logical points
- Continuation lines indented one additional tab

## 3. Naming Conventions

### 3.1 Data Types
- **PascalCase** for all type names:
  ```cpp
  class JtagDev;          // Class
  struct DieInfo;         // Struct  
  enum class DeviceBpType; // Scoped enum
  typedef uint32_t AddressT; // Typedef (PascalCase with T suffix)
  using UsartSettings = ...; // Type alias (PascalCase)
  ```

### 3.2 Variables by Scope

#### Global Variables
- **camelCase** with descriptive names:
  ```cpp
  uint32_t gCurrentDeviceId;
  static JtagDev* sJtagInstance;
  ```

#### File-Static Variables
- **camelCase** with trailing underscore:
  ```cpp
  static uint32_t sInstanceCount_;
  static const char* kConfigFileName_ = "config.bin";
  ```

#### Local Variables
- **snake_case** with descriptive names:
  ```cpp
  uint32_t buffer_size;
  bool is_instruction_load;
  ChipProfile* current_profile;
  ```

#### Function/Method Parameters
- **snake_case** matching local variable convention:
  ```cpp
  void ReadBytes(address_t start_address, uint8_t* data_buffer, uint32_t byte_count);
  ```

### 3.3 Class/Struct Members
- **Private members**: camelCase with trailing underscore
- **Protected members**: camelCase with trailing underscore
- **Public members**: camelCase (no underscore unless static)

Examples:
```cpp
class ExampleClass {
public:
    // Public member (no underscore)
    uint32_t publicValue;
    
    // Static public member (underscore allowed)
    static uint32_t sInstanceCount_;
    
protected:
    // Protected member (trailing underscore)
    uint32_t protectedValue_;
    
private:
    // Private member (trailing underscore)
    uint32_t privateValue_;
    static uint32_t sPrivateStatic_;
};
```

### 3.4 Constants

#### Compile-time Constants (`constexpr`)
- **kPrefix + camelCase**:
  ```cpp
  static constexpr uint32_t kBufferSize = 1024;
  static constexpr uint16_t kSwBkpInstr = 0x4343;
  static constexpr uint32_t kMaxBreakpoints = 20;
  ```

#### Preprocessor Macros
- **ALL_CAPS** with underscores:
  ```cpp
  #define OPT_JTAG_IMPLEMENTATION
  #define MAX_BUFFER_SIZE 1024
  #define DEVICE_FLAG_JTAG 0x01
  ```

#### Enum Values
- **kPrefix + camelCase** for scoped enums:
  ```cpp
  enum class DeviceBpType : uint8_t {
      kBpTypeBreak,
      kBpTypeWatch,
      kBpTypeRead,
      kBpTypeWrite
  };
  ```
- **ALL_CAPS** for traditional enums:
  ```cpp
  enum EraseFlags {
      ERASE = 0x02,
      MERAS = 0x04,
      GMERAS = 0x08
  };
  ```

### 3.5 Functions and Methods
- **camelCase** for function names:
  ```cpp
  void initializeHardware();
  uint32_t calculateChecksum(const uint8_t* data, size_t length);
  bool JtagDev::OnInstrLoad();
  ```

### 3.6 Template Parameters
- **Type parameters**: `typename T` or `class C` for class types
- **Value parameters**: `const Type kName` for non-type parameters
- **Descriptive names** for complex templates:

```cpp
// Good:
template <typename SysClk, const Timer::Unit kTim, const uint32_t kFreq>
class Generator { ... };

// Avoid single letters for complex templates:
template <typename T, typename U, typename V>  // Not descriptive
class ComplexTemplate { ... };
```

## 4. Class and Struct Organization

### 4.1 File Structure
- **One class per file** (exceptions for tightly coupled helper classes)
- **File names match class names**: `ClassName.h` / `ClassName.cpp`
- **Header guards**: Use `#pragma once` (no traditional `#ifndef` guards)

### 4.2 Access Section Ordering
Access sections must appear in this order:
1. `public`
2. `protected` 
3. `private`

### 4.3 Context and Member Organization

#### Interface Files (.h)
```
// [Optional: Class documentation comment]
class ClassName {
// Context: Group description
public:
    /// Member description
    ReturnType methodName(Parameters);
    
    /// Data member description  
    Type dataMember;

// Context: Another group description  
protected:
    /// Protected member description
    ReturnType protectedMethod(Parameters);
    
    /// Protected data description
    Type protectedData_;

// Context: Internal implementation details
private:
    /// Private helper description
    ReturnType privateHelper(Parameters);
    
    /// Private data description
    Type privateData_;
};
```

**Rules:**
1. Each **context group** starts with a left-aligned comment line describing the group
2. Each **access specifier** is explicitly declared, even if repeating
3. **One blank line** between context/access groups
4. **Each member** preceded by at least one comment line (`///` Doxygen style)
5. **One tab indentation** for all members within access section

**Example:**
```cpp
class JtagDev : public ITapInterface {
// Constructors and factory methods
public:
    /// Default constructor initializes JTAG device
    JtagDev();
    
    /// Creates JTAG device with specific configuration
    static JtagDev* Create(ConfigType config);

// Virtual interface implementations
public:
    /// Opens JTAG connection to target device
    virtual bool OnOpen() override;
    
    /// Closes JTAG connection and releases resources
    virtual void OnClose() override;

// Protocol-specific implementations
protected:
    /// Checks if TMS anticipation is required
    virtual bool OnAnticipateTms() const override;
    
    /// Performs IR shift operation
    virtual uint8_t OnIrShift(uint8_t byte) override;

// Internal state management
private:
    /// Current JTAG state
    JtagState current_state_;
    
    /// Buffer for read operations
    static uint32_t read_buf_[kPingPongBufSize_];
};
```

## 5. Implementation Files (.cpp)

### 5.1 File Organization
- **File name matches class**: `ClassName.cpp` for `ClassName.h`
- **Include order**: 
  1. Corresponding header
  2. Project headers
  3. System/library headers
  4. Forward declarations

### 5.2 Method Ordering
Methods should be ordered by:
1. **Constructors and destructors** (always first)
2. **Static methods** and factory functions
3. **Public methods** (grouped by logical purpose)
4. **Protected methods**
5. **Private methods**

Within each group, order by **dependency**: called functions should appear before functions that call them (where possible).

### 5.3 Method Separation
- **≥2 blank lines** between method implementations
- **1 blank line** between logically related method groups
- **0-1 blank lines** within method bodies between logical blocks

**Example:**
```cpp
JtagDev::JtagDev() 
    : current_state_(JtagState::kReset)
    , buffer_index_(0)
{
    InitializeHardware();
    ResetStateMachine();
}



bool JtagDev::OnOpen()
{
    // Hardware initialization sequence
    ConfigureGpioPins();
    SetupSpiInterface();
    
    // JTAG TAP reset
    PerformTapReset();
    
    return VerifyConnection();
}



uint8_t JtagDev::OnIrShift(uint8_t byte)
{
    // Setup shift operation
    PrepareShiftBuffer(byte);
    
    // Execute shift with timing
    StartShiftClock();
    while (!IsShiftComplete()) {
        WaitForClockEdge();
    }
    
    // Read and return result
    return ReadShiftResult();
}
```

### 5.4 Function Body Organization

#### Logical Block Structure
Function bodies should be organized like a book chapter:
- **Comment headers** for major logical sections
- **≤1 blank line** between code blocks
- **Visual separation** that shows structure "from a distance"

**Example:**
```cpp
bool TapDev430::SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
    // Step 1: Establish JTAG connection
    if (!EstablishJtagConnection()) {
        return false;
    }
    
    // Step 2: Perform power-on reset
    ExecutePowerOnReset();
    WaitForStabilization(kResetDelayMs);
    
    // Step 3: Save CPU context
    SaveGeneralPurposeRegisters(ctx);
    SaveStatusRegister(ctx);
    SaveProgramCounter(ctx);
    
    // Step 4: Apply chip-specific configuration
    ApplyChipProfileSettings(prof);
    ConfigureBreakpoints(prof);
    
    return VerifyContextSaved(ctx);
}
```

#### Block Comment Guidelines
- Use `//` for block headers (not `/* */`)
- Brief, descriptive titles
- Align with code indentation level
- One blank line before block comment
- Zero or one blank line after block comment

## 6. Documentation Standards

### 6.1 Doxygen Comments
- Use `///` for single-line comments
- Use `/** ... */` for multi-line comments  
- Place comments **before** the declaration (not after)
- Include at minimum: brief description

**Example:**
```cpp
/// Manages JTAG communication with MSP430 target devices.
/// This class provides hardware-independent interface to JTAG operations.
class JtagDev : public ITapInterface {
public:
    /// Opens connection to JTAG device.
    /// @returns true if connection successful, false otherwise.
    /// @note This method performs hardware initialization.
    virtual bool OnOpen() override;
};
```

### 6.2 Implementation Comments
- **Each functional block** gets a comment header
- **Complex algorithms** get detailed explanations
- **Non-obvious code** gets inline `//` comments
- **TODOs and FIXMEs** must include reference/ticket

### 6.3 Comment Density
- **Header files**: Every member documented
- **Implementation**: Every logical block documented
- **Balance**: Enough comments to understand, not so many to obscure

## 7. BMT Library Compliance

### 7.1 Naming Convention Alignment
The BMT (Bare Metal Templates) submodule should follow the same naming conventions:

```cpp
// BMT examples adhering to guidelines:
namespace Bmt {
namespace Clocks {

template<typename HseClock, uint32_t kTargetFreq>
class AnyPll { ... };

template<const PuPd kPuPd = PuPd::kFloating>
class AnyInPa0 { ... };

} // namespace Clocks
} // namespace Bmt
```

### 7.2 Template Organization
BMT templates should follow similar organization principles:
- Clear context comments for template parameters
- Logical grouping of template specializations
- Consistent use of `typename` vs `class` in parameters

### 7.3 Integration Points
When extending or customizing BMT templates:
- Maintain consistent naming with rest of codebase
- Add appropriate documentation
- Follow same file organization principles

## 8. Platform-Specific Considerations

### 8.1 Visual Studio Integration
- **Solution organization**: `glossy-msp430.sln` with proper project grouping
- **Build configurations**: `Debug|VisualGDB` and `Release|VisualGDB`
- **Property sheets**: Use `stm32.props` for MCU-specific settings

### 8.2 VisualGDB Configuration
- **Toolchain**: ARM GCC managed by VisualGDB
- **Preprocessor definitions**: Set in project properties
- **Include paths**: Properly configured for BMT and firmware

### 8.3 Windows Development Environment
- **CRLF enforcement**: Git configuration to preserve line endings
- **Path handling**: Use forward slashes in `#include` directives
- **Tool integration**: Ensure compatibility with Windows PowerShell

## 9. Error Handling and Preprocessor

### 9.1 Error Directives
- **Concise technical messages**:
  ```cpp
  #error Missing OPT_JTAG_DMA_ISR definition
  #error Platform.h must define JTAG implementation
  #error Unsupported chip architecture
  ```

### 9.2 Preprocessor Organization
- **Group related definitions** with block comments
- **Use `#ifdef`/`#ifndef`** for feature toggles
- **Avoid deep nesting** of preprocessor directives

**Example:**
```cpp
// BLOCK: OPT_JTAG_IMPLEMENTATION values
#define OPT_JTAG_IMPL_OFF            0
#define OPT_JTAG_IMPL_SPI            1
#define OPT_JTAG_IMPL_SPI_DMA        2
#define OPT_JTAG_IMPL_TIM_DMA        3
// ENDBLOCK: OPT_JTAG_IMPLEMENTATION values
```

### 9.3 Static Assertions
- Use `static_assert` for compile-time checks
- Include descriptive error messages:
  ```cpp
  static_assert(sizeof(DeviceBreakpoint) % 4 == 0, 
                "DeviceBreakpoint must be uint32_t aligned for atomic operations");
  ```

## 10. Code Examples

### 10.1 Complete Header Example
```cpp
#pragma once

#include "ITapInterface.h"
#include <util/PingPongBuffer.h>

/// JTAG TAP device controller for MSP430 targets.
/// Implements hardware-abstracted JTAG operations with optimal timing.
class JtagDev : public ITapInterface {
// Construction and initialization
public:
    /// Creates JTAG device with default configuration.
    JtagDev();
    
    /// Creates JTAG device with specific speed grade.
    /// @param speed Grade 2-5 for different performance levels.
    explicit JtagDev(uint8_t speed);

// Core JTAG operations
public:
    /// Establishes connection to target device.
    /// @returns true if device responds, false on error.
    virtual bool OnOpen() override;
    
    /// Releases JTAG connection and resources.
    virtual void OnClose() override;

// Shift operations
protected:
    /// Shifts data into JTAG instruction register.
    /// @param byte Instruction to shift.
    /// @returns Value read from IR during shift.
    virtual uint8_t OnIrShift(uint8_t byte) override;
    
    /// Shifts 8-bit data into JTAG data register.
    virtual uint8_t OnDrShift8(uint8_t data) override;

// Internal state management
private:
    /// Current JTAG TAP state machine state.
    JtagState current_state_;
    
    /// Buffer for read operations (ping-pong buffer).
    static uint32_t read_buf_[kPingPongBufSize_];
    
    /// Checks if instruction load operation is in progress.
    bool IsInstrLoad();
};
```

### 10.2 Complete Implementation Example
```cpp
#include "stdproj.h"
#include "JtagDev.h"

using namespace Bmt::Timer;



JtagDev::JtagDev()
    : current_state_(JtagState::kReset)
{
    // Initialize hardware pins
    ConfigureGpioForJtag();
    
    // Reset internal state
    ResetBuffers();
    ClearErrorFlags();
}



JtagDev::JtagDev(uint8_t speed)
    : current_state_(JtagState::kReset)
{
    // Validate speed parameter
    if (speed < 2 || speed > 5) {
        speed = 4; // Default to grade 4
    }
    
    // Configure for specified speed
    ConfigureSpeedGrade(speed);
    InitializeHardware();
}



bool JtagDev::OnOpen()
{
    // Step 1: Hardware initialization
    if (!InitializeSpiInterface()) {
        return false;
    }
    
    // Step 2: JTAG TAP reset sequence
    PerformTapReset();
    WaitForResetCompletion(kResetTimeoutMs);
    
    // Step 3: Verify device presence
    if (!VerifyDeviceId()) {
        return false;
    }
    
    // Step 4: Configure for operation
    ConfigureJtagMode();
    EnableInterrupts();
    
    return true;
}



uint8_t JtagDev::OnIrShift(uint8_t byte)
{
    // Prepare shift buffer
    tx_buf_.bytes[0] = byte;
    
    // Execute SPI transfer
    StartSpiTransfer();
    WaitForTransferComplete();
    
    // Return captured value
    return rx_buf_.bytes[0];
}



bool JtagDev::IsInstrLoad()
{
    // Query CNTRL_SIG register
    OnIrShift(IR_CNTRL_SIG_CAPTURE);
    
    // Check for instruction load flag
    uint16_t status = OnDrShift16(0);
    return (status & (CNTRL_SIG_INSTRLOAD | CNTRL_SIG_READ)) 
           == (CNTRL_SIG_INSTRLOAD | CNTRL_SIG_READ);
}
```

## 11. Tooling and Validation

### 11.1 Recommended IDE Settings
- **Visual Studio**: Configure tabs (not spaces), UTF-8 encoding
- **EditorConfig**: Use `.editorconfig` for basic formatting
- **Git**: Set `core.autocrlf=true` for Windows development

### 11.2 Validation Checklist
Before submitting code:
- [ ] Naming conventions followed
- [ ] Class organization compliant
- [ ] Method spacing correct (≥2 lines between)
- [ ] All members documented
- [ ] Logical blocks commented
- [ ] Line endings CRLF
- [ ] Tabs used for indentation
- [ ] File names match class names

### 11.3 Code Review Focus Areas
Reviewers should verify:
1. Naming consistency across related files
2. Documentation completeness
3. Organization compliance
4. Visual clarity and structure
5. Adherence to project-specific patterns

## 12. Transition and Application

### 12.1 New Code Development
All new code must follow these guidelines from creation.

### 12.2 Existing Code Updates
When modifying existing files:
1. Update the modified sections to comply
2. Consider updating entire logical unit if making significant changes
3. Document non-compliant sections with `// TODO: Update to guidelines`

### 12.3 File-Wide Updates
Complete file updates should be done:
- During major refactoring
- When adding significant new functionality
- As part of bug fixes that touch much of the file
- With appropriate testing and validation

---

*Last Updated: 2026-04-24*  
*Document Version: 1.0*  
*For Glossy MSP430 Firmware Project*