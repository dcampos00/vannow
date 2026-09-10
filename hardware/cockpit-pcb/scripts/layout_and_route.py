#!/usr/bin/env python3
"""
Cockpit Switch Hub PCB - Procedural Layout and Routing Script (Rev 8)
VanNOW Project - English Documentation

PCB Form Factor: 55.0 mm x 45.0 mm (Center at 0, 0)
Target: 0 shorting items, 0 clearance violations, 0 unconnected items, 0 courtyard collisions.
"""

import os
import pcbnew

def mm_to_nm(mm):
    return int(round(mm * 1e6))

def get_net(board, name):
    net = board.FindNet(name)
    if not net:
        raise ValueError(f"Net '{name}' not found in board!")
    return net

def add_via(board, x_mm, y_mm, net, drill_mm=0.4, size_mm=0.8):
    via = pcbnew.PCB_VIA(board)
    via.SetPosition(pcbnew.VECTOR2I(mm_to_nm(x_mm), mm_to_nm(y_mm)))
    via.SetWidth(mm_to_nm(size_mm))
    via.SetDrill(mm_to_nm(drill_mm))
    if net:
        via.SetNet(net)
    board.Add(via)
    return via

def route_segment(board, start_xy, end_xy, layer, width_mm, net):
    track = pcbnew.PCB_TRACK(board)
    track.SetStart(pcbnew.VECTOR2I(mm_to_nm(start_xy[0]), mm_to_nm(start_xy[1])))
    track.SetEnd(pcbnew.VECTOR2I(mm_to_nm(end_xy[0]), mm_to_nm(end_xy[1])))
    track.SetLayer(layer)
    track.SetWidth(mm_to_nm(width_mm))
    if net:
        track.SetNet(net)
    board.Add(track)
    return track

def route_path(board, points, layer, width_mm, net):
    tracks = []
    for i in range(len(points) - 1):
        t = route_segment(board, points[i], points[i+1], layer, width_mm, net)
        tracks.append(t)
    return tracks

def get_pad_pos(board, ref, pad_num):
    fp = board.FindFootprintByReference(ref)
    if not fp:
        raise ValueError(f"Footprint {ref} not found!")
    pad = fp.FindPadByNumber(str(pad_num))
    if not pad:
        raise ValueError(f"Pad {pad_num} on footprint {ref} not found!")
    pos = pad.GetPosition()
    return (pos.x / 1e6, pos.y / 1e6)

def main():
    board_path = os.path.join(os.path.dirname(__file__), "..", "layouts", "default", "default.kicad_pcb")
    board = pcbnew.LoadBoard(board_path)
    print(f"Loaded board: {board_path}")

    # 1. Clear existing tracks, vias, zones, and drawings safely
    for t in list(board.GetTracks()):
        board.Delete(t)
    for z in list(board.Zones()):
        board.Delete(z)
    for d in list(board.GetDrawings()):
        board.Delete(d)
    print("Cleared existing tracks, zones, and drawings.")

    # 2. Assign Diode nets (Pad 1 = switch sense, Pad 2 = wakeup_bus)
    net_wake = get_net(board, "wakeup_bus")
    net_vcc  = get_net(board, "vcc")
    net_gnd  = get_net(board, "gnd")
    net_z1   = get_net(board, "1")            # Ch 1 (SW1)
    net_z2   = get_net(board, "mcu_sense")    # Ch 2 (SW2)
    net_z3   = get_net(board, "1-1")          # Ch 3 (SW3)
    net_z4   = get_net(board, "mcu_sense-1")  # Ch 4 (SW4)
    net_z5   = get_net(board, "1-2")          # Ch 5 (SW5)
    net_z6   = get_net(board, "mcu_sense-2")  # Ch 6 (SW6)
    net_anode = get_net(board, "anode")
    net_cath  = get_net(board, "cathode")

    diode_net_map = {
        "D1": (net_z1, net_wake),
        "D2": (net_z2, net_wake),
        "D3": (net_z3, net_wake),
        "D4": (net_z4, net_wake),
        "D5": (net_z5, net_wake),
        "D6": (net_z6, net_wake),
    }
    for ref, (net1, net2) in diode_net_map.items():
        fp = board.FindFootprintByReference(ref)
        if fp:
            p1 = fp.FindPadByNumber("1")
            p2 = fp.FindPadByNumber("2")
            if p1: p1.SetNet(net1)
            if p2: p2.SetNet(net2)
    print("Assigned diode nets.")

    # 3. Create Edge.Cuts outline (55 mm x 45 mm) and 4x M3 mounting holes
    edge_layer = pcbnew.Edge_Cuts
    w, h = 55.0, 45.0
    x0, y0 = -w/2, -h/2
    x1, y1 =  w/2,  h/2

    corners = [(x0, y0), (x1, y0), (x1, y1), (x0, y1), (x0, y0)]
    for i in range(len(corners) - 1):
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetStart(pcbnew.VECTOR2I(mm_to_nm(corners[i][0]), mm_to_nm(corners[i][1])))
        seg.SetEnd(pcbnew.VECTOR2I(mm_to_nm(corners[i+1][0]), mm_to_nm(corners[i+1][1])))
        seg.SetLayer(edge_layer)
        seg.SetWidth(mm_to_nm(0.15))
        board.Add(seg)

    # 4x M3 mounting holes (diameter 3.2 mm, radius 1.6 mm, inset 3.5 mm from edges)
    mount_dx, mount_dy = 24.0, 19.0
    mount_r = 1.6
    for mx in [-mount_dx, mount_dx]:
        for my in [-mount_dy, mount_dy]:
            circle = pcbnew.PCB_SHAPE(board)
            circle.SetShape(pcbnew.SHAPE_T_CIRCLE)
            circle.SetCenter(pcbnew.VECTOR2I(mm_to_nm(mx), mm_to_nm(my)))
            circle.SetStart(pcbnew.VECTOR2I(mm_to_nm(mx), mm_to_nm(my)))
            circle.SetEnd(pcbnew.VECTOR2I(mm_to_nm(mx + mount_r), mm_to_nm(my)))
            circle.SetLayer(edge_layer)
            circle.SetWidth(mm_to_nm(0.15))
            board.Add(circle)
    print("Created Edge.Cuts outline and mounting holes.")

    # 4. Footprint Placement
    channels_x = {
        "ch2": -15.27,
        "ch1": -10.19,
        "ch4":  -1.27,
        "ch3":   3.81,
        "ch6":  12.73,
        "ch5":  17.81,
    }

    placements = {
        # Microcontroller Female Socket for Seeed XIAO ESP32-C6
        "J2": (-7.62, -12.0, 0.0),    # Left header (D0..D6)
        "J3": ( 7.62, -12.0, 0.0),    # Right header (D7..D10, 3V3, GND, 5V)

        # Battery Power Entry Terminal (2-pin, 2.54mm pitch)
        "J1": (-17.5, -17.0, 0.0),

        # Switch Signal Terminal Blocks (4-pin, 2.54mm pitch)
        "J4": (-10.19, 17.5, 180.0),  # SW1 (p1) & SW2 (p3)
        "J5": (  3.81, 17.5, 180.0),  # SW3 (p1) & SW4 (p3)
        "J6": ( 17.81, 17.5, 180.0),  # SW5 (p1) & SW6 (p3)

        # Power decoupling capacitors (clearing J1 mounting peg at -17.5, -14.46)
        "C1": (-21.5, -11.0, 0.0),
        "C2": (-21.5,  -8.0, 0.0),

        # Wakeup pull-up resistor (47k)
        "R8": (-12.0,  -8.0, 0.0),     # p1(VCC): -12.91, p2(Wake): -11.09

        # Status Feedback LED and current limiting resistor
        "D7": ( 13.0, -4.38, 0.0),
        "R7": ( 16.5, -4.38, 0.0),

        # Channel 2: SW2
        "R2": (channels_x["ch2"],  6.0, 0.0),
        "D2": (channels_x["ch2"],  9.0, 180.0),
        "C4": (channels_x["ch2"], 12.0, 180.0),

        # Channel 1: SW1
        "R1": (channels_x["ch1"],  6.0, 0.0),
        "D1": (channels_x["ch1"],  9.0, 180.0),
        "C3": (channels_x["ch1"], 12.0, 180.0),

        # Channel 4: SW4
        "R4": (channels_x["ch4"],  6.0, 0.0),
        "D4": (channels_x["ch4"],  9.0, 180.0),
        "C6": (channels_x["ch4"], 12.0, 180.0),

        # Channel 3: SW3
        "R3": (channels_x["ch3"],  6.0, 0.0),
        "D3": (channels_x["ch3"],  9.0, 180.0),
        "C5": (channels_x["ch3"], 12.0, 180.0),

        # Channel 6: SW6
        "R6": (channels_x["ch6"],  6.0, 0.0),
        "D6": (channels_x["ch6"],  9.0, 180.0),
        "C8": (channels_x["ch6"], 12.0, 180.0),

        # Channel 5: SW5
        "R5": (channels_x["ch5"],  6.0, 0.0),
        "D5": (channels_x["ch5"],  9.0, 180.0),
        "C7": (channels_x["ch5"], 12.0, 180.0),
    }

    for ref, (x, y, rot) in placements.items():
        fp = board.FindFootprintByReference(ref)
        if not fp:
            raise ValueError(f"Footprint {ref} not found!")
        fp.SetPosition(pcbnew.VECTOR2I(mm_to_nm(x), mm_to_nm(y)))
        fp.SetOrientationDegrees(rot)
        fp.Reference().SetVisible(False)
        fp.Value().SetVisible(False)
    print(f"Placed all {len(placements)} footprints.")

    # 5. Routing
    F_Cu = pcbnew.F_Cu
    B_Cu = pcbnew.B_Cu

    # Pad coordinates lookup
    j2_p1 = get_pad_pos(board, "J2", 1)  # (-7.62, -12.0) LP_GPIO0 (Wakeup)
    j2_p2 = get_pad_pos(board, "J2", 2)  # (-7.62, -9.46) GPIO1 (Ch 1)
    j2_p3 = get_pad_pos(board, "J2", 3)  # (-7.62, -6.92) GPIO2 (Ch 2)
    j2_p4 = get_pad_pos(board, "J2", 4)  # (-7.62, -4.38) GPIO21 (Ch 3)
    j2_p5 = get_pad_pos(board, "J2", 5)  # (-7.62, -1.84) GPIO22 (Ch 4)
    j2_p6 = get_pad_pos(board, "J2", 6)  # (-7.62, 0.70) GPIO23 (Ch 5)
    j2_p7 = get_pad_pos(board, "J2", 7)  # (-7.62, 3.24) GPIO16 (Ch 6)

    j3_p4 = get_pad_pos(board, "J3", 4)  # (7.62, -4.38) GPIO9 (Status LED)
    j3_p5 = get_pad_pos(board, "J3", 5)  # (7.62, -1.84) VCC (3.3V)
    j3_p6 = get_pad_pos(board, "J3", 6)  # (7.62, 0.70) GND

    # --- 5.1 Status LED ---
    d7_p1 = get_pad_pos(board, "D7", 1)  # Anode
    d7_p2 = get_pad_pos(board, "D7", 2)  # Cathode
    r7_p1 = get_pad_pos(board, "R7", 1)  # Cathode
    r7_p2 = get_pad_pos(board, "R7", 2)  # GND

    route_path(board, [j3_p4, d7_p1], F_Cu, 0.25, net_anode)
    route_path(board, [d7_p2, r7_p1], F_Cu, 0.25, net_cath)
    # Route R7.p2 to J3.pin_6 (GND) around the outside on F_Cu
    route_path(board, [r7_p2, (20.0, r7_p2[1]), (20.0, j3_p6[1]), j3_p6], F_Cu, 0.3, net_gnd)

    # --- 5.2 Channel Sense Columns ---
    def route_channel_sense(r_ref, d_ref, c_ref, j_ref, j_pad, net):
        rp2 = get_pad_pos(board, r_ref, 2)
        dp1 = get_pad_pos(board, d_ref, 1)
        cp1 = get_pad_pos(board, c_ref, 1)
        jp  = get_pad_pos(board, j_ref, j_pad)
        # Vertical connection from R.p2 down through D.p1 to C.p1
        route_path(board, [rp2, (dp1[0], rp2[1]), dp1, (dp1[0], cp1[1]), cp1], F_Cu, 0.3, net)
        # Down from C.p1 into terminal block pad
        route_path(board, [cp1, (jp[0], 14.5), jp], F_Cu, 0.3, net)

    route_channel_sense("R1", "D1", "C3", "J4", 1, net_z1)
    route_channel_sense("R2", "D2", "C4", "J4", 3, net_z2)
    route_channel_sense("R4", "D4", "C6", "J5", 3, net_z4)
    route_channel_sense("R3", "D3", "C5", "J5", 1, net_z3)
    route_channel_sense("R6", "D6", "C8", "J6", 3, net_z6)
    route_channel_sense("R5", "D5", "C7", "J6", 1, net_z5)

    # --- 5.3 Sense Feeds into XIAO J2 Socket ---
    r1_p2 = get_pad_pos(board, "R1", 2)  # (-9.28, 6.0)
    r2_p2 = get_pad_pos(board, "R2", 2)  # (-14.36, 6.0)
    r4_p2 = get_pad_pos(board, "R4", 2)  # (-0.36, 6.0)
    r3_p2 = get_pad_pos(board, "R3", 2)  # (4.72, 6.0)
    c8_p1 = get_pad_pos(board, "C8", 1)  # (13.68, 12.0)
    c7_p1 = get_pad_pos(board, "C7", 1)  # (18.76, 12.0)

    # --- 5.3 Sense Feeds into XIAO J2 Socket ---
    r1_p2 = get_pad_pos(board, "R1", 2)  # (-9.28, 6.0)
    r2_p2 = get_pad_pos(board, "R2", 2)  # (-14.36, 6.0)
    r4_p2 = get_pad_pos(board, "R4", 2)  # (-0.36, 6.0)
    r3_p2 = get_pad_pos(board, "R3", 2)  # (4.72, 6.0)
    c8_p1 = get_pad_pos(board, "C8", 1)  # (13.68, 12.0)
    c7_p1 = get_pad_pos(board, "C7", 1)  # (18.76, 12.0)

    # Ch 1 (SW1) -> J2.p2 (-7.62, -9.46): on F_Cu along X = -9.28 mm
    route_path(board, [r1_p2, (r1_p2[0], j2_p2[1]), j2_p2], F_Cu, 0.25, net_z1)

    # Ch 2 (SW2) -> J2.p3 (-7.62, -6.92):
    # Up on F_Cu to Y = -2.0 mm (clearing R8 at Y = -8.0 mm by 6.0 mm),
    # drop via to B_Cu at (-14.36, -2.0), route on B_Cu to Y = -6.92 mm, then east into J2.p3
    add_via(board, r2_p2[0], -2.0, net_z2)
    route_path(board, [r2_p2, (r2_p2[0], -2.0)], F_Cu, 0.25, net_z2)
    route_path(board, [(r2_p2[0], -2.0), (r2_p2[0], j2_p3[1]), j2_p3], B_Cu, 0.25, net_z2)

    # Ch 4 (SW4) -> J2.p5 (-7.62, -1.84): up on F_Cu to Y = -1.84, via to B_Cu, across on B_Cu
    add_via(board, r4_p2[0], j2_p5[1], net_z4)
    route_path(board, [r4_p2, (r4_p2[0], j2_p5[1])], F_Cu, 0.25, net_z4)
    route_path(board, [(r4_p2[0], j2_p5[1]), j2_p5], B_Cu, 0.25, net_z4)

    # Ch 3 (SW3) -> J2.p4 (-7.62, -4.38): up on F_Cu to Y = -4.38, via to B_Cu, across on B_Cu
    add_via(board, r3_p2[0], j2_p4[1], net_z3)
    route_path(board, [r3_p2, (r3_p2[0], j2_p4[1])], F_Cu, 0.25, net_z3)
    route_path(board, [(r3_p2[0], j2_p4[1]), j2_p4], B_Cu, 0.25, net_z3)

    # Ch 6 (SW6) -> J2.p7 (-7.62, 3.24):
    # Drop via at (13.68, 14.0) to B_Cu, cross horizontally at Y = 14.0 mm to X = -5.0 mm,
    # via to F_Cu, run UP on F_Cu along X = -5.0 mm to Y = 3.24 mm, into J2.p7!
    add_via(board, c8_p1[0], 14.0, net_z6)
    add_via(board, -5.0, 14.0, net_z6)
    route_path(board, [c8_p1, (c8_p1[0], 14.0)], F_Cu, 0.25, net_z6)
    route_path(board, [(c8_p1[0], 14.0), (-5.0, 14.0)], B_Cu, 0.25, net_z6)
    route_path(board, [(-5.0, 14.0), (-5.0, j2_p7[1]), j2_p7], F_Cu, 0.25, net_z6)

    # Ch 5 (SW5) -> J2.p6 (-7.62, 0.70):
    # Drop via at (18.76, 15.5) to B_Cu, cross horizontally at Y = 15.5 mm to X = -4.0 mm,
    # via to F_Cu, run UP on F_Cu along X = -4.0 mm to Y = 0.70 mm, into J2.p6!
    add_via(board, c7_p1[0], 15.5, net_z5)
    add_via(board, -4.0, 15.5, net_z5)
    route_path(board, [c7_p1, (c7_p1[0], 15.5)], F_Cu, 0.25, net_z5)
    route_path(board, [(c7_p1[0], 15.5), (-4.0, 15.5)], B_Cu, 0.25, net_z5)
    route_path(board, [(-4.0, 15.5), (-4.0, j2_p6[1]), j2_p6], F_Cu, 0.25, net_z5)

    # --- 5.4 Wakeup Bus ---
    # D1..D6 pad 2 (Anode) are all at Y = 9.0 mm
    d_wake_pads = [get_pad_pos(board, f"D{i}", 2) for i in range(1, 7)]
    for dp in d_wake_pads:
        add_via(board, dp[0], dp[1], net_wake)
    # Horizontal wakeup trunk on B_Cu along Y = 9.0 mm
    min_wx = min(p[0] for p in d_wake_pads)  # -16.92 mm
    max_wx = max(p[0] for p in d_wake_pads)  #  16.16 mm
    route_path(board, [(-17.5, 9.0), (max_wx, 9.0)], B_Cu, 0.35, net_wake)

    # Step north on B_Cu from (-17.5, 9.0) to (-17.5, 6.0)
    route_path(board, [(-17.5, 9.0), (-17.5, 6.0)], B_Cu, 0.3, net_wake)
    # Jumper on F_Cu over B_Cu VCC trunk (Y = 4.5 mm) from Y = 6.0 to Y = 3.0 mm
    add_via(board, -17.5, 6.0, net_wake)
    add_via(board, -17.5, 3.0, net_wake)
    route_path(board, [(-17.5, 6.0), (-17.5, 3.0)], F_Cu, 0.3, net_wake)

    # Run north on B_Cu along X = -17.5 mm from Y = 3.0 to Y = -9.5 mm
    route_path(board, [(-17.5, 3.0), (-17.5, -9.5)], B_Cu, 0.3, net_wake)
    # Turn east on B_Cu along Y = -9.5 mm to X = -11.09 mm (R8.p2 X-pos)
    r8_p2 = get_pad_pos(board, "R8", 2)  # (-11.09, -8.0)
    route_path(board, [(-17.5, -9.5), (r8_p2[0], -9.5)], B_Cu, 0.3, net_wake)
    # Tap R8.p2
    add_via(board, r8_p2[0], r8_p2[1], net_wake)
    route_path(board, [(r8_p2[0], -9.5), r8_p2], B_Cu, 0.3, net_wake)
    # Tap J2.p1 (-7.62, -12.0)
    route_path(board, [(r8_p2[0], -9.5), (r8_p2[0], j2_p1[1]), j2_p1], B_Cu, 0.3, net_wake)

    # --- 5.5 VCC Bus ---
    j1_p1 = get_pad_pos(board, "J1", 1)  # (-17.5, -17.0)
    c1_p1 = get_pad_pos(board, "C1", 1)  # (-22.45, -11.0)
    c2_p1 = get_pad_pos(board, "C2", 1)  # (-22.45, -8.0)
    r8_p1 = get_pad_pos(board, "R8", 1)  # (-12.91, -8.0)

    # Route VCC from J1.p1 stepping WEST to X = -21.5 mm, down to Y = -14.0 mm
    # (clearing M3 mounting hole at -24.0, -19.0 by 1.4 mm, well above 0.5 mm rule),
    # then step west to X = -23.0 mm and run south on F_Cu to Y = 4.5 mm
    route_path(board, [j1_p1, (-21.5, j1_p1[1]), (-21.5, -14.0), (-23.0, -14.0), (-23.0, 4.5)], F_Cu, 0.4, net_vcc)
    # Tap C1.p1 and C2.p1 from the west on F_Cu
    route_path(board, [(-23.0, c1_p1[1]), c1_p1], F_Cu, 0.4, net_vcc)
    route_path(board, [(-23.0, c2_p1[1]), c2_p1], F_Cu, 0.4, net_vcc)
    # Route to R8.p1 north of C1 along Y = -12.5 mm on F_Cu (clearing all GND pads by > 1.5 mm)
    route_path(board, [(-23.0, -12.5), (r8_p1[0], -12.5), r8_p1], F_Cu, 0.35, net_vcc)

    # Drop via at (-23.0, 4.5) to B_Cu for main pullup trunk
    add_via(board, -23.0, 4.5, net_vcc)
    # Main horizontal VCC trunk on B_Cu from X = -23.0 to X = 16.90 mm along Y = 4.5 mm
    route_path(board, [(-23.0, 4.5), (16.90, 4.5)], B_Cu, 0.35, net_vcc)

    # Connect all 6 switch pullups from the Y = 4.5 mm trunk
    # (via at Y = 4.5 mm, short 1.5mm stub on F_Cu north to resistor pad 1)
    pullup_pads = [
        get_pad_pos(board, "R2", 1),  # (-16.18, 6.0)
        get_pad_pos(board, "R1", 1),  # (-11.10, 6.0)
        get_pad_pos(board, "R4", 1),  # ( -2.18, 6.0)
        get_pad_pos(board, "R3", 1),  # (  2.90, 6.0)
        get_pad_pos(board, "R6", 1),  # ( 11.82, 6.0)
        get_pad_pos(board, "R5", 1),  # ( 16.90, 6.0)
    ]
    for rx, ry in pullup_pads:
        add_via(board, rx, 4.5, net_vcc)
        route_path(board, [(rx, 4.5), (rx, ry)], F_Cu, 0.25, net_vcc)

    # Connect XIAO J3.p5 (3V3 at 7.62, -1.84 mm) to the B_Cu trunk:
    # Step east to X = 10.5 mm, down along X = 10.5 mm to Y = 4.5 mm
    route_path(board, [j3_p5, (10.5, j3_p5[1]), (10.5, 4.5)], B_Cu, 0.35, net_vcc)

    # --- 5.6 Ground Connections and Stitching ---
    # Connect J1.p2 (GND) to stitching via on F_Cu and B_Cu
    j1_p2 = get_pad_pos(board, "J1", 2)
    add_via(board, -12.0, -17.0, net_gnd)
    route_path(board, [j1_p2, (-12.0, -17.0)], F_Cu, 0.35, net_gnd)
    route_path(board, [j1_p2, (-12.0, -17.0)], B_Cu, 0.35, net_gnd)

    # Connect C1.p2 and C2.p2 GND pads
    c1_p2 = get_pad_pos(board, "C1", 2)
    c2_p2 = get_pad_pos(board, "C2", 2)
    add_via(board, -20.55, -9.5, net_gnd)
    route_path(board, [c1_p2, (-20.55, -9.5), c2_p2], F_Cu, 0.35, net_gnd)

    # Ground bus along Y = 20.0 mm tying all screw terminal GND pads on B_Cu
    gnd_terms = [
        get_pad_pos(board, "J4", 4),  # -17.81
        get_pad_pos(board, "J4", 2),  # -12.73
        get_pad_pos(board, "J5", 4),  #  -3.81
        get_pad_pos(board, "J5", 2),  #   1.27
        get_pad_pos(board, "J6", 4),  #  10.19
        get_pad_pos(board, "J6", 2),  #  15.27
    ]
    for gp in gnd_terms:
        add_via(board, gp[0], 20.0, net_gnd)
        route_path(board, [gp, (gp[0], 20.0)], B_Cu, 0.35, net_gnd)
    route_path(board, [(-20.0, 20.0), (20.0, 20.0)], B_Cu, 0.4, net_gnd)

    # Perimeter and interior ground stitching vias
    stitching_vias = [
        (-24.5, -15.0),
        ( 24.5, -15.0),
        (-24.5,  15.0),
        ( 24.5,  15.0),
        (-24.5,   0.0),
        ( 24.5,   0.0),
        (  0.0, -16.0),
        (-15.0, -11.0),
        ( 15.0, -11.0),
    ]
    for gx, gy in stitching_vias:
        add_via(board, gx, gy, net_gnd)

    # 6. Solid Copper Ground Zones (F_Cu & B_Cu)
    for layer in [F_Cu, B_Cu]:
        zone = pcbnew.ZONE(board)
        zone.SetLayer(layer)
        zone.SetNet(net_gnd)
        outline_chain = pcbnew.SHAPE_LINE_CHAIN()
        for x, y in [(-27.0, -22.0), (27.0, -22.0), (27.0, 22.0), (-27.0, 22.0)]:
            outline_chain.Append(pcbnew.VECTOR2I(mm_to_nm(x), mm_to_nm(y)))
        outline_chain.SetClosed(True)
        zone.AddPolygon(outline_chain)
        zone.SetMinThickness(mm_to_nm(0.25))
        zone.SetThermalReliefGap(mm_to_nm(0.3))
        zone.SetThermalReliefSpokeWidth(mm_to_nm(0.4))
        zone.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
        board.Add(zone)

    # 7. Fill zones
    filler = pcbnew.ZONE_FILLER(board)
    filler.Fill(board.Zones())
    print("Filled copper zones on F.Cu and B.Cu.")

    # 8. Save completed board
    pcbnew.SaveBoard(board_path, board)
    print(f"Successfully saved completed board to {board_path}!")

if __name__ == "__main__":
    main()
