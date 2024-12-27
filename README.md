# Empress

A header-only C++ library that prevents debugger functionality by restricting process memory operations through Windows Job Objects.

## How It Works

The technique leverages Windows Job Objects to set a process memory limit that's just enough for your code to run, but not enough for debuggers to function.

1. **Breakpoint Types & Memory Operations**
   - Software Breakpoints (int 3/0xCC):
     - Requires memory writes to replace instructions
     - Most common, used by default in user-mode debuggers
   - Hardware Breakpoints:
     - Uses CPU debug registers (DR0-DR7)
     - Doesn't require memory modification
     - BUT: Debugger still needs memory for exception handlers
     - Job object won't block the breakpoint itself, but prevents debugger from handling it

2. **Memory Pages & Protection**
   - Windows Memory Types:
     - Private Pages: Process-specific memory (heap, stack)
       - Copy-on-write: New private copy when modified
       - Most debugger operations need private pages
     - Shared Pages: Mapped files, shared memory
       - Multiple processes can access
       - Usually read-only for code sections
     - Committed Pages: Backed by physical memory or page file
       - Your existing code stays in these
       - Job object won't affect already committed memory
   - What Job Object Affects:
     - Blocks NEW memory commitments
     - Prevents private page allocations needed by debuggers
     - Existing committed pages (your code) stay untouched
     - Shared pages remain accessible

3. **Why 0x1000 Limit Works**
   - 0x1000 (4KB) is minimum page size
   - Already committed memory (your code) keeps running
   - Debuggers can't:
     - Allocate new private pages for their data
     - Create exception handling threads
     - Modify page protections

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
