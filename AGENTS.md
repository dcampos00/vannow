# AI Agent Instructions

This repository follows strict software development guidelines. When working on this project, you must adhere to the following fundamental directive:

## Main Directive

> [!IMPORTANT]
> **Always prioritize high-performance, generic, and easy-to-maintain code over quick or easy-to-implement solutions.**

### Development Principles

1. **High Performance:**
   - Avoid premature optimizations, but design with efficiency in mind.
   - Choose optimal algorithms and data structures for the problem.
   - Minimize unnecessary resource usage (memory, CPU, network).

2. **Generic & Reusable Code:**
   - Design solutions that solve the problem generally, not just for the immediate use case.
   - Avoid code duplication by applying clean and parameterized abstractions when appropriate.

3. **Ease of Maintenance:**
   - Code must be clean, self-documented, and follow principles like SOLID.
   - Prefer readability and robust structure over temporary "shortcuts" or quick hacks.
   - Include unit tests and adequate documentation to ensure long-term stability.

4. **Language Requirement:**
   - All code (including variables, functions, classes, and comments) and documentation must be written entirely in English.

### Hardware & Prototyping Principles

1. **Safety First (Logic-Level MOSFET Selection):**
   - Avoid recommending or using basic IRF520 MOSFET modules with 3.3V logic microcontrollers (e.g., ESP32-C6). The IRF520 is not a true logic-level device and will fail to saturate at 3.3V, causing it to overheat and fail under load.
   - Always recommend true logic-level MOSFET modules (e.g., LR7843 or D4184) for low-voltage gates to minimize heat generation.

2. **Automotive High-Side Switching & Protection:**
   - For automotive or van power systems, prefer high-side switching (switching the positive rail) rather than low-side (switching the ground). This prevents hazardous bypasses if a wire short-circuits to the metal chassis.
   - Recommend smart high-side switches (e.g., Infineon PROFET) for integrated short-circuit, over-current, and over-temperature protection.

3. **Modular Reusability (Carrier Board Design):**
   - For DIY hardware projects, design custom final PCBs as carrier boards using female headers to accept the exact pre-assembled modules used in the breadboard prototype. This facilitates 100% reuse of components and avoids complex SMD soldering.

4. **Component Quality & Compatibility:**
   - Always prioritize high-quality, verified compatible components (e.g., modules from reputable brands like Pololu, Adafruit, SparkFun, or original components from distributors like Mouser/DigiKey) over cheap unbranded clones.
   - Avoid generic, counterfeit buck converters (e.g., fake LM2596 boards) which suffer from high voltage ripple, low efficiency, and high failure rates. Instead, recommend robust, high-efficiency regulators (like Pololu regulators or genuine MP1584EN boards) to prevent damage to downstream microcontrollers.
