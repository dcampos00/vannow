import re
import sys
from collections import defaultdict

def arrange_layout(pcb_path):
    print(f"Rearranging layout for: {pcb_path}")
    with open(pcb_path, "r", encoding="utf-8") as f:
        content = f.read()
    
    depth = 0
    in_footprint = False
    footprint_start = -1
    fp_depth = -1
    
    footprints = [] # List of dicts: {start, end, content, ref, atopile_addr, parent}
    
    i = 0
    n = len(content)
    while i < n:
        if content[i] == '(':
            depth += 1
            if not in_footprint and content[i:i+11] == "(footprint ":
                in_footprint = True
                footprint_start = i
                fp_depth = depth
        elif content[i] == ')':
            if in_footprint and depth == fp_depth:
                fp_text = content[footprint_start:i+1]
                
                # Extract ref
                ref = "UNKNOWN"
                atopile_addr = ""
                lines = fp_text.splitlines()
                for line in lines:
                    if '(property "Reference"' in line:
                        m = re.search(r'\(property "Reference" "([^"]+)"', line)
                        if m:
                            ref = m.group(1)
                    if '(property "atopile_address"' in line:
                        m = re.search(r'\(property "atopile_address" "([^"]+)"', line)
                        if m:
                            atopile_addr = m.group(1)
                            
                # Get parent
                if atopile_addr:
                    parts = atopile_addr.split('.')
                    if len(parts) > 1:
                        parent = parts[0]
                    else:
                        parent = "top"
                else:
                    parent = "no_addr"
                    
                footprints.append({
                    "start": footprint_start,
                    "end": i + 1,
                    "content": fp_text,
                    "ref": ref,
                    "atopile_addr": atopile_addr,
                    "parent": parent
                })
                in_footprint = False
            depth -= 1
        i += 1

    print(f"Parsed {len(footprints)} footprints.")

    # Design the module grid
    # We will define a base position (X, Y) for each parent module.
    # X and Y are in mm. A4 size is 297x210, so let's keep X in [30, 260], Y in [30, 180].
    module_bases = {
        "esp32": (30, 30),
        "buck": (30, 110),
        
        "led_zone1_stage": (90, 30),
        "led_zone2_stage": (90, 70),
        "led_zone3_stage": (90, 110),
        "led_zone4_stage": (90, 150),
        
        "pump_stage": (150, 30),
        "aux1_stage": (150, 70),
        "aux2_stage": (150, 110),
        "aux3_stage": (150, 150),
        
        "fan_stage": (210, 30),
        "inverter_opto": (210, 90),
        "dcdc_opto": (210, 130),
        "voltage_sensor": (210, 160),
        
        "top": (260, 30),
        "no_addr": (260, 170)
    }

    # Now, for each module, place its components relative to the base position.
    # We keep track of how many components we have placed for each module to compute offsets.
    placed_counts = defaultdict(int)

    # Let's define specific placement rules for some modules to avoid overlap and look nice.
    def get_relative_pos(parent, ref, index):
        if parent == "esp32":
            # esp32 has J7/left_header and J8/right_header. Space them by 25.4mm.
            if "left_header" in ref or "J1" in ref or "J7" in ref:
                return (0, 0, 90) # Rotate by 90 to be vertical
            else:
                return (25.4, 0, 90) # Rotate by 90, shifted by 25.4mm
                
        elif parent == "buck":
            # buck has vin_header (J1) and vout_header (J2). Space them by 15mm.
            return (index * 15, 0, 0)
            
        elif parent in ["led_zone1_stage", "led_zone2_stage", "led_zone3_stage", "led_zone4_stage", "pump_stage", "aux1_stage", "aux2_stage", "aux3_stage"]:
            # 4 components for HighCurrentSmartHighSideSwitch:
            # u_profet (SOIC-14), r_gate (0805), r_pulldown (0805), r_sense (0805)
            # Let's arrange them in a 2x2 grid
            col = index % 2
            row = index // 2
            return (col * 12, row * 10, 0)
            
        elif parent == "fan_stage":
            # 5 components: q_driver (SOT-23), q_pass (SOIC-8), r_gate, r_pulldown, r_pullup
            # Arrange in a grid:
            if index == 0: # q_driver
                return (0, 0, 0)
            elif index == 1: # q_pass
                return (12, 0, 0)
            else:
                r_idx = index - 2
                return (r_idx * 8, 10, 0)
                
        elif parent in ["inverter_opto", "dcdc_opto"]:
            # u_opto (DIP-4) and r_limit (0805)
            return (index * 12, 0, 0)
            
        elif parent == "voltage_sensor":
            # r_high, r_low, c_filter
            return (index * 8, 0, 90)
            
        elif parent == "top":
            # Let's place top-level components (connectors, LEDs, resistors) in a column grid
            col = index % 2
            row = index // 2
            return (col * 15, row * 12, 0)
            
        else:
            # Default fallback layout
            col = index % 3
            row = index // 3
            return (col * 10, row * 10, 0)

    # Process all footprints and update their contents
    footprints.sort(key=lambda x: x["start"])

    # Group components by parent to assign index
    parent_indices = defaultdict(list)
    for fp in footprints:
        parent_indices[fp["parent"]].append(fp)

    replacements = []
    
    for parent, fps in parent_indices.items():
        fps.sort(key=lambda x: x["ref"])
        base_x, base_y = module_bases.get(parent, (100, 100))
        
        for idx, fp in enumerate(fps):
            # Compute new coordinates
            rel_x, rel_y, rot = get_relative_pos(parent, fp["ref"], idx)
            new_x = round(base_x + rel_x, 3)
            new_y = round(base_y + rel_y, 3)
            
            # Find the main (at X Y [A]) inside fp["content"]
            pattern = r'(?m)^\t\t\(at\s+[-0-9.]+\s+[-0-9.]+(\s+[-0-9.]+)?\)'
            
            if rot != 0:
                new_at = f"\t\t(at {new_x} {new_y} {rot})"
            else:
                new_at = f"\t\t(at {new_x} {new_y})"
                
            m = re.search(pattern, fp["content"])
            if m:
                new_fp_content = fp["content"][:m.start()] + new_at + fp["content"][m.end():]
                replacements.append({
                    "start": fp["start"],
                    "end": fp["end"],
                    "old_content": fp["content"],
                    "new_content": new_fp_content
                })
            else:
                print(f"Warning: Could not find main (at ...) for footprint {fp['ref']}")

    # Apply replacements from right to left (descending start index)
    replacements.sort(key=lambda x: x["start"], reverse=True)
    
    modified_content = content
    for rep in replacements:
        modified_content = modified_content[:rep["start"]] + rep["new_content"] + modified_content[rep["end"]:]
        
    # Write back to file
    with open(pcb_path, "w", encoding="utf-8") as f:
        f.write(modified_content)
        
    print(f"Successfully rearranged footprints in {pcb_path}!")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python arrange_grid.py <pcb_file_path>")
        sys.exit(1)
    arrange_layout(sys.argv[1])
