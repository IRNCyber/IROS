## IROS v0.0.1 – Initial Kernel Release

### Overview  
This release marks the first operational milestone of IROS (Intelligent Real-time Operating System).  
The system successfully boots, initializes the kernel, and demonstrates foundational low-level functionality.

---

### Key Features

- Bootable Kernel
  - Custom bootloader successfully loads the kernel into memory  
  - Verified execution in emulator environment  

- Kernel Initialization
  - Proper transition from bootloader to kernel  
  - Stack and execution environment setup  

- Basic VGA Output
  - Direct hardware-level text rendering  
  - Displays kernel status messages on boot  

- Memory Management (Initial)
  - Basic kmalloc implementation  
  - Successful allocation test:
    kmalloc(64) = 0x000016A0

---

### Validation

- Boot tested using QEMU emulator  
- Kernel execution confirmed without crashes  
- Memory allocation functioning as expected  

---

### Known Limitations

- No keyboard input support  
- No interrupt handling (IDT/IRQ not implemented)  
- No interactive shell or command processing  
- Static execution (no dynamic runtime loop beyond initialization)  

---

### Next Milestones

- Keyboard driver implementation  
- Interrupt handling (IDT + IRQ)  
- Interactive shell (CLI)  
- Enhanced memory management  
- Modular kernel architecture  

---

### Full Changelog  
https://github.com/IRNCyber/IROS/commits/v0.0.1

---

### Summary  
v0.0.1 establishes the core foundation of IROS, proving control over boot process, memory, and hardware-level output. Subsequent releases will focus on interactivity, stability, and expanded system capabilities.
