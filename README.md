# Empress

A header-only C++ library that prevents debugger functionality by restricting process memory operations through [Windows Job Objects](https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects).

## How It Works
The technique leverages Windows Job Objects to set a process memory limit that's just enough for your code to run, but not enough for debuggers to function.

## Breakpoint Types
### Software Breakpoints (int 3/0xCC)
- Requires memory writes to replace instructions
- Most common debugging method
- Default choice for user-mode debuggers
  
### Hardware Breakpoints
- Utilizes CPU debug registers (DR0-DR7)
- No memory modification required
- Exception handlers still need memory allocation
- Job object prevents exception handling while allowing breakpoints
  
## Memory Management
### Memory Types
| Type | Description | Characteristics |
|------|-------------|-----------------|
| Private Pages | Process-specific memory | - Heap and stack storage<br>- Copy-on-write when modified<br>- Required for debugger operations |
| Shared Pages | Mapped files and shared memory | - Accessible by multiple processes<br>- Usually read-only for code sections |
| Committed Pages | Physical or page file backed | - Contains existing code<br>- Unaffected by job object restrictions |

### Job Object Effects
The job object restriction:
- Prevents new memory commitments
- Blocks private page allocations needed by debuggers
- Preserves existing committed pages
- Maintains shared page accessibility
  
## Protection Mechanism
The 0x1000 (4KB) memory limit is effective because:
- Matches minimum system page size
- Allows existing committed memory to continue execution (your code)
- Prevents debuggers from:
  - Allocating private pages for operation
  - Creating exception handling threads
  - Modifying memory protection flags

## Usage

```cpp
#include "empress.hpp"

int main() {
  if (empress::protection::enable()) {
    // Protection active - debuggers can't modify memory
  }

  return 0;
}
```

## Results

Breakpoints:

![{0727F23B-F4EA-4664-B687-A80069C6E795}](https://github.com/user-attachments/assets/905971b9-8716-4d6a-9509-21ce874f367e)

Memory editing:
![{F2A69C4A-914D-47B6-99DD-D515A1121441}](https://github.com/user-attachments/assets/64b3cbf2-f655-4bd0-b70f-26b5817e2deb)

## Limitations

- Not effective against kernel-mode debuggers (they bypass user-mode restrictions)
- Your code must minimize dynamic memory allocations after protection
- Very small programs (<4KB) might not be protected if entirely private
- Some debuggers might still attach but won't be able to function

## Requirements

- Windows OS
- C++20 or later
- MSVC compiler

## References

Based on the technique described in "Process on a diet: anti-debug using job objects" by Justas Masiulis.
