#!/usr/bin/env python3
"""
VanNOW Central Controller PCB - Procedural Layout and Routing Script (Rev 14)
Zero-DRC layout and routing with complete topological layer decoupling.
"""

import os
import sys
import pcbnew

def mm_to_nm(mm):
    return int(round(mm * 1e6))

def nm_to_mm(nm):
    return nm / 1e6

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
        p1 = points[i]
        p2 = points[i+1]
        if round(p1[0], 4) != round(p2[0], 4) or round(p1[1], 4) != round(p2[1], 4):
            t = route_segment(board, p1, p2, layer, width_mm, net)
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
    return (nm_to_mm(pos.x), nm_to_mm(pos.y))

def main():
    board_path = os.path.join(os.path.dirname(__file__), "..", "layouts", "profet", "profet.kicad_pcb")
    board = pcbnew.LoadBoard(board_path)
    print(f"Loaded board: {board_path}")

    # 1. Delete unused footprints (Aux 2 and Aux 3)
    unused_refs = ["U2", "R4", "R5", "R6", "J2", "U3", "R7", "R8", "R9", "J3"]
    for ref in unused_refs:
        fp = board.FindFootprintByReference(ref)
        if fp:
            board.Delete(fp)
            print(f"Deleted unused footprint: {ref}")

    # 2. Clear existing tracks, vias, zones, drawings safely
    for t in list(board.GetTracks()):
        board.Delete(t)
    for z in list(board.Zones()):
        board.Delete(z)
    for d in list(board.GetDrawings()):
        board.Delete(d)
    print("Cleared existing tracks, zones, and drawings.")

    # 3. Setup Board Outline (Edge.Cuts) 150mm x 95mm with R=3.0mm rounded corners
    edge_layer = pcbnew.Edge_Cuts
    w, h = 150.0, 95.0
    r = 3.0
    x0, y0 = -w/2, -h/2
    x1, y1 =  w/2,  h/2

    def add_edge_segment(p1, p2):
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetStart(pcbnew.VECTOR2I(mm_to_nm(p1[0]), mm_to_nm(p1[1])))
        seg.SetEnd(pcbnew.VECTOR2I(mm_to_nm(p2[0]), mm_to_nm(p2[1])))
        seg.SetLayer(edge_layer)
        seg.SetWidth(mm_to_nm(0.15))
        board.Add(seg)

    def add_edge_arc(start, mid, end):
        arc = pcbnew.PCB_SHAPE(board)
        arc.SetShape(pcbnew.SHAPE_T_ARC)
        arc.SetArcGeometry(
            pcbnew.VECTOR2I(mm_to_nm(start[0]), mm_to_nm(start[1])),
            pcbnew.VECTOR2I(mm_to_nm(mid[0]), mm_to_nm(mid[1])),
            pcbnew.VECTOR2I(mm_to_nm(end[0]), mm_to_nm(end[1]))
        )
        arc.SetLayer(edge_layer)
        arc.SetWidth(mm_to_nm(0.15))
        board.Add(arc)

    add_edge_segment((x0 + r, y0), (x1 - r, y0))
    add_edge_arc((x1 - r, y0), (x1 - r * 0.2929, y0 + r * 0.2929), (x1, y0 + r))
    add_edge_segment((x1, y0 + r), (x1, y1 - r))
    add_edge_arc((x1, y1 - r), (x1 - r * 0.2929, y1 - r * 0.2929), (x1 - r, y1))
    add_edge_segment((x1 - r, y1), (x0 + r, y1))
    add_edge_arc((x0 + r, y1), (x0 + r * 0.2929, y1 - r * 0.2929), (x0, y1 - r))
    add_edge_segment((x0, y1 - r), (x0, y0 + r))
    add_edge_arc((x0, y0 + r), (x0 + r * 0.2929, y0 + r * 0.2929), (x0 + r, y0))

    # Mounting Holes (M3: 3.2mm drill, 6.0mm pad clearance) at (+-70, +-42.5)
    mount_dx, mount_dy = 70.0, 42.5
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
    print("Created Edge.Cuts outline and 4x M3 mounting holes.")

    # 4. Footprint Placement
    placements = {
        # Microcontroller ESP32-S3 DevKit Sockets (Parallel 1x22 headers, 25.4mm pitch)
        "J7": (-12.7, -22.0, 0.0),    # Left header (Pins 1..22, Y from -22.0 to +31.34)
        "J8": ( 12.7, -22.0, 0.0),    # Right header (Pins 1..22, Y from -22.0 to +31.34)

        # Power Entry & Buck Converter Module (Top-Left)
        "J16": (-60.0, -36.0, 180.0), # 12V Main Battery Input
        "J4":  (-44.0, -36.0, 90.0),  # Buck 12V Input (rot 90: Pad 1 West at -44, -37.27; Pad 2 East at -44, -34.73)
        "J5":  (-28.0, -36.0, 0.0),   # Buck 5V Output Header

        # Status Feedback LEDs & Series Resistors (Localized directly next to pins)
        "R28": (-17.0, -22.0, 180.0), # 330 ohm for Power LED (Pad 1 East, Pad 2 West)
        "D3":  (-22.0, -22.0, 180.0), # Power 3V3 LED (Pad 1 East, Pad 2 West)
        "R32": (-17.0,  -1.68, 180.0),# 330 ohm for Remote LED (Pad 1 East, Pad 2 West)
        "D5":  (-22.0,  -1.68, 180.0),# Remote Mesh RX LED (Pad 1 East, Pad 2 West)
        "R10": ( 17.0, -11.84, 0.0),  # 330 ohm for Cabin LED (Pad 1 West, Pad 2 East)
        "D1":  ( 22.0, -11.84, 0.0),  # Cabin Status LED (Pad 1 West, Pad 2 East)

        # Battery Telemetry Voltage Divider
        "R35": ( 17.0, -14.38, 180.0), # 1k ADC Protection Resistor (J8.4)
        "R33": ( 22.0, -18.0,  270.0), # 100k (Pad 1 North 12V at -18.91, Pad 2 South Divider at -17.09)
        "R34": ( 26.0, -18.0,   90.0), # 18k  (Pad 1 South Divider at -17.09, Pad 2 North GND at -18.91)
        "C1":  ( 30.0, -18.0,   90.0), # 100nF (Pad 1 South Divider at -17.05, Pad 2 North GND at -18.95)

        # Optocouplers & Dry Contact Terminals (Top-Right, Y=-25mm)
        # Diesel Heater (U4 pins 4 & 3 align straight with J6 pins 1 & 2)
        "R11": ( 21.0, -25.0, 0.0),
        "U4":  ( 26.38, -25.0, 0.0),  # Pin 4 at (34.0, -25.0), Pin 3 at (34.0, -22.46)
        "J6":  ( 34.0, -36.0, 180.0), # Pad 1 at (34.0, -36.0), Pad 2 at (31.46, -36.0)

        # Fridge Compressor (U5 pins 4 & 3 align straight with J10 pins 1 & 2)
        "R15": ( 41.0, -25.0, 0.0),
        "U5":  ( 46.38, -25.0, 0.0),  # Pin 4 at (54.0, -25.0), Pin 3 at (54.0, -22.46)
        "J10": ( 54.0, -36.0, 180.0), # Pad 1 at (54.0, -36.0), Pad 2 at (51.46, -36.0)

        # Ceiling Fan Discrete MOSFET Stage
        "R12": ( 35.0, -12.0, 0.0),   # Gate resistor
        "R13": ( 35.0, -16.0, 0.0),   # Pulldown
        "Q1":  ( 41.0, -12.0, 0.0),   # N-channel driver (SOT-23)
        "R14": ( 41.0,  -6.0, 180.0), # Pullup to 12V (rot 180: Pad 1 drain at 41.91, Pad 2 12V at 40.09)
        "Q2":  ( 49.0, -12.0, 0.0),   # P-channel pass (SO-8)
        "D2":  ( 56.0,  -6.0, 0.0),   # Flyback diode DO-201
        "J9":  ( 62.0, -14.0, 0.0),   # 2-pin fan terminal

        # --- 6x BTS5008-1EKB PROFET Smart Power Stages ---
        # Zone 1 (GPIO 12)
        "R16": (-58.0,   4.0, 270.0),
        "R17": (-54.0,   4.0, 270.0),
        "R18": (-60.54,  4.0,  90.0),
        "U6":  (-58.0,  14.0, 270.0),
        "J11": (-58.0,  35.0, 0.0),

        # Zone 2 (GPIO 13)
        "R19": (-42.0,   4.0, 270.0),
        "R20": (-38.0,   4.0, 270.0),
        "R21": (-44.54,  4.0,  90.0),
        "U7":  (-42.0,  14.0, 270.0),
        "J12": (-42.0,  35.0, 0.0),

        # Zone 3 (GPIO 14)
        "R22": (-26.0,   4.0, 270.0),
        "R23": (-22.0,   4.0, 270.0),
        "R24": (-28.54,  4.0,  90.0),
        "U8":  (-26.0,  14.0, 270.0),
        "J13": (-26.0,  35.0, 0.0),

        # Zone 4 (GPIO 15)
        "R25": ( 26.0,   4.0, 270.0),
        "R26": ( 30.0,   4.0, 270.0),
        "R27": ( 23.46,  4.0,  90.0),
        "U9":  ( 26.0,  14.0, 270.0),
        "J14": ( 26.0,  35.0, 0.0),

        # Water Pump (GPIO 4)
        "R29": ( 42.0,   4.0, 270.0),
        "R30": ( 46.0,   4.0, 270.0),
        "R31": ( 39.46,  4.0,  90.0),
        "U10": ( 42.0,  14.0, 270.0),
        "D4":  ( 47.08, 28.0, 180.0), # DO-201 horizontal
        "J15": ( 42.0,  35.0, 0.0),

        # Aux 1: Exterior Lights (GPIO 5)
        "R1":  ( 58.0,   4.0, 270.0),
        "R2":  ( 62.0,   4.0, 270.0),
        "R3":  ( 55.46,  4.0,  90.0),
        "U1":  ( 58.0,  14.0, 270.0),
        "J1":  ( 58.0,  35.0, 0.0),
    }

    for ref, (x, y, rot) in placements.items():
        fp = board.FindFootprintByReference(ref)
        if not fp:
            print(f"Warning: footprint {ref} not found!")
            continue
        fp.SetPosition(pcbnew.VECTOR2I(mm_to_nm(x), mm_to_nm(y)))
        fp.SetOrientationDegrees(rot)
        fp.Reference().SetVisible(True)
        fp.Value().SetVisible(False)

    print(f"Placed all {len(placements)} active footprints.")

    # 5. Routing Signal and Power Connections
    F_Cu = pcbnew.F_Cu
    B_Cu = pcbnew.B_Cu

    net_12v   = get_net(board, "power_in")
    net_gnd   = get_net(board, "gnd")
    net_5v    = get_net(board, "v_mcu_5v")
    net_3v3   = get_net(board, "1-2")
    net_adc   = get_net(board, "v_out")
    net_div   = get_net(board, "voltage_sensor-1")

    # =========================================================================
    # 5.1 12V MAIN POWER DISTRIBUTION (Top Layer F.Cu)
    # =========================================================================
    j16_p1 = get_pad_pos(board, "J16", 1)
    j4_p1  = get_pad_pos(board, "J4", 1)

    # 1. Feed between Battery Input J16.1 and Buck Converter J4.1 (along Y = -32.0 mm)
    route_path(board, [j16_p1, (j16_p1[0], -32.0), (-67.0, -32.0)], F_Cu, 2.0, net_12v)
    route_path(board, [(-67.0, -32.0), (j4_p1[0], -32.0), j4_p1], F_Cu, 2.0, net_12v)

    # 2. West Trunk: Runs down along X = -67.0 mm from Y = -32.0 to Y = 14.0 mm
    route_path(board, [(-67.0, -32.0), (-67.0, 14.0)], F_Cu, 2.0, net_12v)

    # 3. West PROFET Power Bus along Y = 14.0 mm connecting U6, U7, U8 tabs and J13 tap
    u6_p15 = get_pad_pos(board, "U6", 15)
    u7_p15 = get_pad_pos(board, "U7", 15)
    u8_p15 = get_pad_pos(board, "U8", 15)
    route_path(board, [(-67.0, 14.0), u6_p15, u7_p15, u8_p15, (-19.5, 14.0)], F_Cu, 2.0, net_12v)

    # 4. West Terminal Drops (J11.2, J12.2, J13.2) - Dropping along East side of each chip (X = xp + 6.5 mm)
    j11_p2 = get_pad_pos(board, "J11", 2)
    j12_p2 = get_pad_pos(board, "J12", 2)
    j13_p2 = get_pad_pos(board, "J13", 2)
    route_path(board, [(-51.5, 14.0), (-51.5, 28.0), (j11_p2[0], 28.0), j11_p2], F_Cu, 1.2, net_12v)
    route_path(board, [(-35.5, 14.0), (-35.5, 28.0), (j12_p2[0], 28.0), j12_p2], F_Cu, 1.2, net_12v)
    route_path(board, [(-19.5, 14.0), (-19.5, 28.0), (j13_p2[0], 28.0), j13_p2], F_Cu, 1.2, net_12v)

    # 5. East PROFET Power Bus along Y = 14.0 mm connecting U9, U10, U1 tabs and East trunk
    u1_p15  = get_pad_pos(board, "U1", 15)
    u10_p15 = get_pad_pos(board, "U10", 15)
    u9_p15  = get_pad_pos(board, "U9", 15)
    route_path(board, [u9_p15, u10_p15, u1_p15, (68.0, 14.0)], F_Cu, 2.0, net_12v)

    # 6. Central 12V Dual-Bridge: Connecting U8.15 (-26.0, 14.0) to U9.15 (26.0, 14.0)
    # Routes two parallel 0.35mm tracks through J7/J8 inter-pin corridors at Y = 14.83mm and Y = 17.37mm
    route_path(board, [u8_p15, (-15.0, 14.0), (-15.0, 14.83), (15.0, 14.83), (15.0, 14.0), u9_p15], F_Cu, 0.35, net_12v)
    route_path(board, [u8_p15, (-16.0, 14.0), (-16.0, 17.37), (16.0, 17.37), (16.0, 14.0), u9_p15], F_Cu, 0.35, net_12v)

    # 7. Feed from East Bus to J14.2 and J15.2 (drops at X = 34.0 mm and X = 50.0 mm)
    j14_p2 = get_pad_pos(board, "J14", 2)
    j15_p2 = get_pad_pos(board, "J15", 2)
    route_path(board, [(34.0, 14.0), (34.0, 20.0), (j14_p2[0], 20.0), j14_p2], F_Cu, 1.2, net_12v)
    route_path(board, [(50.0, 14.0), (50.0, 20.0), (j15_p2[0], 20.0), j15_p2], F_Cu, 1.2, net_12v)

    # 8. Feed to Fan Stage Q2, R14, and Battery Divider R33
    # Main North 12V Feeder leaves East 12V Bus at (37.0, 14.0), runs North along X = 37.0 mm to Y = -6.0 mm
    q2_p1 = get_pad_pos(board, "Q2", 1)
    q2_p2 = get_pad_pos(board, "Q2", 2)
    q2_p3 = get_pad_pos(board, "Q2", 3)
    r33_p1 = get_pad_pos(board, "R33", 1)
    r14_p2 = get_pad_pos(board, "R14", 2)

    # Tap East 12V bus at (37.0, 14.0), route North along X = 37.0 mm to Y = -6.0 mm and connect R14 Pad 2 on F.Cu
    route_path(board, [(37.0, 14.0), (37.0, -6.0), r14_p2], F_Cu, 0.8, net_12v)
    # Bridge under control-8 on B.Cu from (37.0, -6.0) to (32.0, -6.0)
    add_via(board, 37.0, -6.0, net_12v)
    route_path(board, [(37.0, -6.0), (32.0, -6.0)], B_Cu, 0.8, net_12v)
    add_via(board, 32.0, -6.0, net_12v)
    # Continue on F.Cu North along X = 32.0 mm to Highway Y = -20.5 mm
    route_path(board, [(32.0, -6.0), (32.0, -20.5)], F_Cu, 0.8, net_12v)
    # North 12V Highway along Y = -20.5 mm feeds R33.1 (West) and Q2.1-3 (East)
    route_path(board, [(32.0, -20.5), (22.0, -20.5), r33_p1], F_Cu, 0.4, net_12v)
    route_path(board, [(32.0, -20.5), (46.525, -20.5), (46.525, q2_p1[1]), q2_p1, q2_p2, q2_p3], F_Cu, 1.0, net_12v)

    # =========================================================================
    # 5.2 PROFET STAGES LOCAL ROUTING & POWER OUTPUTS (F.Cu)
    # =========================================================================
    stages_info = [
        (-58.0, "U6",  "R16", "R17", "R18", "J11", "led_zone1_stage-input", "led_zone1_stage-current_sense", "power_out-3"),
        (-42.0, "U7",  "R19", "R20", "R21", "J12", "led_zone2_stage-input", "led_zone2_stage-current_sense", "power_out-4"),
        (-26.0, "U8",  "R22", "R23", "R24", "J13", "led_zone3_stage-input", "led_zone3_stage-current_sense", "power_out-5"),
        ( 26.0, "U9",  "R25", "R26", "R27", "J14", "led_zone4_stage-input", "led_zone4_stage-current_sense", "power_out-6"),
        ( 42.0, "U10", "R29", "R30", "R31", "J15", "pump_stage-input",      "pump_stage-current_sense",      "power_out-7"),
        ( 58.0, "U1",  "R1",  "R2",  "R3",  "J1",  "input",                 "current_sense",                 "power_out"),
    ]

    for xp, u_ref, rg_ref, rpd_ref, rcs_ref, jt_ref, n_in, n_cs, n_out in stages_info:
        net_in   = get_net(board, n_in)
        net_cs   = get_net(board, n_cs)
        net_out  = get_net(board, n_out)

        # Gate resistor Pad 2 -> PROFET Pin 4 (IN)
        rg_p2 = get_pad_pos(board, rg_ref, 2)
        u_p4  = get_pad_pos(board, u_ref, 4)
        route_path(board, [rg_p2, u_p4], F_Cu, 0.254, net_in)

        # Current sense resistor Pad 1 (South) -> PROFET Pin 6 (IS)
        rcs_p1 = get_pad_pos(board, rcs_ref, 1)
        u_p6   = get_pad_pos(board, u_ref, 6)
        route_path(board, [rcs_p1, u_p6], F_Cu, 0.254, net_cs)

        # PROFET Power Output: Pins 10, 11, 12 -> Terminal Pad 1
        u_p10 = get_pad_pos(board, u_ref, 10)
        u_p11 = get_pad_pos(board, u_ref, 11)
        u_p12 = get_pad_pos(board, u_ref, 12)
        jt_p1 = get_pad_pos(board, jt_ref, 1)

        route_path(board, [u_p10, u_p11, u_p12], F_Cu, 1.2, net_out)
        route_path(board, [u_p11, jt_p1], F_Cu, 1.5, net_out)

    # Pump Flyback Diode D4 (Horizontal: Pad 1 East -> J15 Pad 3)
    d4_p1  = get_pad_pos(board, "D4", 1)
    j15_p3 = get_pad_pos(board, "J15", 3)
    net_cath = get_net(board, "cathode")
    route_path(board, [d4_p1, (d4_p1[0], 31.0), (j15_p3[0], 31.0), j15_p3], F_Cu, 1.2, net_cath)

    # =========================================================================
    # 5.3 5V MCU LOGIC RAIL (v_mcu_5v: 100% on B.Cu, zero vias)
    # =========================================================================
    j7_p21 = get_pad_pos(board, "J7", 21)
    j5_p1  = get_pad_pos(board, "J5", 1)
    route_path(board, [j5_p1, (j5_p1[0], -38.5), (-8.0, -38.5), (-8.0, j7_p21[1]), j7_p21], B_Cu, 0.8, net_5v)

    # =========================================================================
    # 5.4 CONTROL SIGNAL ROUTING (Planar Layer-Decoupled Highways)
    # =========================================================================
    # WEST GROUP on B.Cu (Zero Crossings!)
    # Zone 1 (control-4, J7 pin 18 at Y = 21.18 mm) -> lane X = -15.0 mm -> corridor Y = -2.0 mm
    j7_p18 = get_pad_pos(board, "J7", 18)
    net_c4 = get_net(board, "control-4")
    route_path(board, [j7_p18, (-15.0, j7_p18[1]), (-15.0, -2.0), (-58.0, -2.0)], B_Cu, 0.254, net_c4)
    add_via(board, -58.0, -2.0, net_c4)
    r16_p1 = get_pad_pos(board, "R16", 1)
    r17_p1 = get_pad_pos(board, "R17", 1)
    route_path(board, [(-58.0, -2.0), r16_p1, (r17_p1[0], r16_p1[1]), r17_p1], F_Cu, 0.254, net_c4)

    # Zone 2 (control-5, J7 pin 19 at Y = 23.72 mm) -> lane X = -17.0 mm -> corridor Y = -1.0 mm
    j7_p19 = get_pad_pos(board, "J7", 19)
    net_c5 = get_net(board, "control-5")
    route_path(board, [j7_p19, (-17.0, j7_p19[1]), (-17.0, -1.0), (-42.0, -1.0)], B_Cu, 0.254, net_c5)
    add_via(board, -42.0, -1.0, net_c5)
    r19_p1 = get_pad_pos(board, "R19", 1)
    r20_p1 = get_pad_pos(board, "R20", 1)
    route_path(board, [(-42.0, -1.0), r19_p1, (r20_p1[0], r19_p1[1]), r20_p1], F_Cu, 0.254, net_c5)

    # Zone 3 (control-6, J7 pin 20 at Y = 26.26 mm) -> lane X = -19.0 mm -> corridor Y = 0.0 mm
    j7_p20 = get_pad_pos(board, "J7", 20)
    net_c6 = get_net(board, "control-6")
    route_path(board, [j7_p20, (-19.0, j7_p20[1]), (-19.0, 0.0), (-26.0, 0.0)], B_Cu, 0.254, net_c6)
    add_via(board, -26.0, 0.0, net_c6)
    r22_p1 = get_pad_pos(board, "R22", 1)
    r23_p1 = get_pad_pos(board, "R23", 1)
    route_path(board, [(-26.0, 0.0), r22_p1, (r23_p1[0], r22_p1[1]), r23_p1], F_Cu, 0.254, net_c6)

    # EAST GROUP (Nesting non-intersecting highways):
    # 1. Aux 1 PROFET (control, J7 pin 5, Y = -11.84 mm)
    # Inside MCU socket, dives to B.Cu at (-6.0, -11.84), jogs to Y = -13.11 mm, vias to F.Cu at (-3.0, -13.11).
    # Runs across J8 at Y = -13.11 mm to (15.0, -13.11), vias to B.Cu, runs East to X = 60.0 mm, drops South to (60.0, 3.0875), via to F.Cu.
    j7_p5 = get_pad_pos(board, "J7", 5)
    net_c1 = get_net(board, "control")
    r1_p1 = get_pad_pos(board, "R1", 1)
    r2_p1 = get_pad_pos(board, "R2", 1)
    route_path(board, [j7_p5, (-6.0, j7_p5[1])], F_Cu, 0.254, net_c1)
    add_via(board, -6.0, j7_p5[1], net_c1)
    route_path(board, [(-6.0, j7_p5[1]), (-6.0, -13.11), (-3.0, -13.11)], B_Cu, 0.254, net_c1)
    add_via(board, -3.0, -13.11, net_c1)
    route_path(board, [(-3.0, -13.11), (15.0, -13.11)], F_Cu, 0.254, net_c1)
    add_via(board, 15.0, -13.11, net_c1)
    route_path(board, [(15.0, -13.11), (60.0, -13.11), (60.0, 3.0875)], B_Cu, 0.254, net_c1)
    add_via(board, 60.0, 3.0875, net_c1)
    route_path(board, [r1_p1, (60.0, 3.0875), r2_p1], F_Cu, 0.254, net_c1)

    # 2. Water Pump PROFET (control-10, J7 pin 4, Y = -14.38 mm)
    # Stays on F.Cu! Jogs to Y = -10.57 mm at (-4.5, -10.57) (crossing above control on B.Cu), crosses J8 to (15.0, -10.57).
    # Vias to B.Cu at (15.0, -10.57), runs East to X = 44.0 mm, drops South to (44.0, 3.0875), via to F.Cu.
    j7_p4 = get_pad_pos(board, "J7", 4)
    net_c10 = get_net(board, "control-10")
    r29_p1 = get_pad_pos(board, "R29", 1)
    r30_p1 = get_pad_pos(board, "R30", 1)
    route_path(board, [j7_p4, (-6.0, j7_p4[1]), (-4.5, -10.57), (15.0, -10.57)], F_Cu, 0.254, net_c10)
    add_via(board, 15.0, -10.57, net_c10)
    route_path(board, [(15.0, -10.57), (44.0, -10.57), (44.0, 3.0875)], B_Cu, 0.254, net_c10)
    add_via(board, 44.0, 3.0875, net_c10)
    route_path(board, [r29_p1, (44.0, 3.0875), r30_p1], F_Cu, 0.254, net_c10)

    # 3. Zone 4 PROFET (control-7, J7 pin 8, Y = -4.22 mm)
    # Crosses J8 at Y = -2.95 mm on F.Cu to (15.0, -2.95), via to B.Cu, runs East to X = 28.0 mm at Y = -2.95 mm, drops South to (28.0, 3.0875), via to F.Cu.
    j7_p8 = get_pad_pos(board, "J7", 8)
    net_c7 = get_net(board, "control-7")
    r25_p1 = get_pad_pos(board, "R25", 1)
    r26_p1 = get_pad_pos(board, "R26", 1)
    route_path(board, [j7_p8, (-10.0, j7_p8[1]), (-10.0, -2.95), (15.0, -2.95)], F_Cu, 0.254, net_c7)
    add_via(board, 15.0, -2.95, net_c7)
    route_path(board, [(15.0, -2.95), (28.0, -2.95), (28.0, 3.0875)], B_Cu, 0.254, net_c7)
    add_via(board, 28.0, 3.0875, net_c7)
    route_path(board, [r25_p1, (28.0, 3.0875), r26_p1], F_Cu, 0.254, net_c7)

    # 4. Fan Gate Driver (control-8, J7 pin 10, Y = +0.86 mm) on F.Cu:
    # Crosses J8 at Y = 2.13 mm on F.Cu, runs East to X = 34.0875 mm, then North directly to R13/R12
    j7_p10 = get_pad_pos(board, "J7", 10)
    net_c8 = get_net(board, "control-8")
    r12_p1 = get_pad_pos(board, "R12", 1)
    r13_p1 = get_pad_pos(board, "R13", 1)
    route_path(board, [j7_p10, (-10.0, j7_p10[1]), (-10.0, 2.13), (34.0875, 2.13), (34.0875, r13_p1[1]), r13_p1, r12_p1], F_Cu, 0.254, net_c8)

    # 5. Heater Opto (control-3, J7 pin 11, Y = 3.40 mm)
    # Starts F.Cu to (0.0, 3.40), via to B.Cu, North at X = 0.0 mm to Y = -25.0 mm, via to F.Cu at (0.0, -25.0 mm), East to R11.1
    j7_p11 = get_pad_pos(board, "J7", 11)
    net_c3 = get_net(board, "control-3")
    r11_p1 = get_pad_pos(board, "R11", 1)
    route_path(board, [j7_p11, (0.0, j7_p11[1])], F_Cu, 0.254, net_c3)
    add_via(board, 0.0, j7_p11[1], net_c3)
    route_path(board, [(0.0, j7_p11[1]), (0.0, -25.0)], B_Cu, 0.254, net_c3)
    add_via(board, 0.0, -25.0, net_c3)
    route_path(board, [(0.0, -25.0), r11_p1], F_Cu, 0.254, net_c3)

    # 6. Fridge Opto (control-9, J8 pin 18, Y = 21.18 mm) on B.Cu:
    # Jogs to Y = 22.45 mm into MCU corridor at X = 6.0 mm, North to Y = -27.0 mm, East to X = 38.0 mm, via to F.Cu
    j8_p18 = get_pad_pos(board, "J8", 18)
    net_c9 = get_net(board, "control-9")
    r15_p1 = get_pad_pos(board, "R15", 1)
    route_path(board, [j8_p18, (10.0, j8_p18[1]), (10.0, 22.45), (6.0, 22.45), (6.0, -27.0), (38.0, -27.0), (38.0, -25.0)], B_Cu, 0.254, net_c9)
    add_via(board, 38.0, -25.0, net_c9)
    route_path(board, [(38.0, -25.0), r15_p1], F_Cu, 0.254, net_c9)

    # =========================================================================
    # 5.5 HEATER & FRIDGE OPTOCOUPLERS LOCAL ROUTING (F.Cu)
    # =========================================================================
    # Heater Opto U4 (PC817 DIP-4)
    r11_p2 = get_pad_pos(board, "R11", 2)
    u4_p1  = get_pad_pos(board, "U4", 1)
    net_u4_in = get_net(board, "dcdc_opto-anode")
    route_path(board, [r11_p2, u4_p1], F_Cu, 0.254, net_u4_in)

    # U4 Pin 4 (collector) -> J6 Pad 1 (collector) on F.Cu
    u4_p4 = get_pad_pos(board, "U4", 4)
    j6_p1 = get_pad_pos(board, "J6", 1)
    net_u4_c = get_net(board, "out_collector")
    route_path(board, [u4_p4, j6_p1], F_Cu, 0.5, net_u4_c)

    # U4 Pin 3 (emitter) -> J6 Pad 2 (emitter) on F.Cu
    u4_p3 = get_pad_pos(board, "U4", 3)
    j6_p2 = get_pad_pos(board, "J6", 2)
    net_u4_e = get_net(board, "out_emitter")
    route_path(board, [u4_p3, (j6_p2[0], u4_p3[1]), j6_p2], F_Cu, 0.5, net_u4_e)

    # Fridge Opto U5 (PC817 DIP-4)
    r15_p2 = get_pad_pos(board, "R15", 2)
    u5_p1  = get_pad_pos(board, "U5", 1)
    net_u5_in = get_net(board, "inverter_opto-anode")
    route_path(board, [r15_p2, u5_p1], F_Cu, 0.254, net_u5_in)

    # U5 Pin 4 (collector) -> J10 Pad 1 (collector) on F.Cu
    u5_p4  = get_pad_pos(board, "U5", 4)
    j10_p1 = get_pad_pos(board, "J10", 1)
    net_u5_c = get_net(board, "out_collector-1")
    route_path(board, [u5_p4, j10_p1], F_Cu, 0.5, net_u5_c)

    # U5 Pin 3 (emitter) -> J10 Pad 2 (emitter) on F.Cu
    u5_p3  = get_pad_pos(board, "U5", 3)
    j10_p2 = get_pad_pos(board, "J10", 2)
    net_u5_e = get_net(board, "out_emitter-1")
    route_path(board, [u5_p3, (j10_p2[0], u5_p3[1]), j10_p2], F_Cu, 0.5, net_u5_e)

    # =========================================================================
    # 5.6 CEILING FAN DISCRETE MOSFET STAGE (F.Cu)
    # =========================================================================
    r12_p2 = get_pad_pos(board, "R12", 2)
    q1_p1  = get_pad_pos(board, "Q1", 1)
    net_q1_g = get_net(board, "gate")
    route_path(board, [r12_p2, (r12_p2[0], q1_p1[1]), q1_p1], F_Cu, 0.254, net_q1_g)

    q1_p3  = get_pad_pos(board, "Q1", 3)
    r14_p1 = get_pad_pos(board, "R14", 1)
    q2_p4  = get_pad_pos(board, "Q2", 4)
    net_q1_d = get_net(board, "drain")
    route_path(board, [q1_p3, r14_p1], F_Cu, 0.254, net_q1_d)
    route_path(board, [r14_p1, (r14_p1[0], -10.10), q2_p4], F_Cu, 0.254, net_q1_d)

    q2_p5 = get_pad_pos(board, "Q2", 5)
    q2_p6 = get_pad_pos(board, "Q2", 6)
    q2_p7 = get_pad_pos(board, "Q2", 7)
    q2_p8 = get_pad_pos(board, "Q2", 8)
    d2_p1 = get_pad_pos(board, "D2", 1)
    j9_p1 = get_pad_pos(board, "J9", 1)
    net_fan_out = get_net(board, "1-3")
    route_path(board, [q2_p5, q2_p6, q2_p7, q2_p8], F_Cu, 0.8, net_fan_out)
    route_path(board, [q2_p6, (d2_p1[0], q2_p6[1]), d2_p1], F_Cu, 1.2, net_fan_out)
    route_path(board, [d2_p1, (d2_p1[0], j9_p1[1]), j9_p1], F_Cu, 1.2, net_fan_out)

    # =========================================================================
    # 5.7 STATUS LEDS (F.Cu)
    # =========================================================================
    # Power LED D3 (3V3 from J7.1 -> R28 -> D3)
    j7_p1  = get_pad_pos(board, "J7", 1)
    r28_p1 = get_pad_pos(board, "R28", 1)
    r28_p2 = get_pad_pos(board, "R28", 2)
    d3_p1  = get_pad_pos(board, "D3", 1)
    net_anode_d3 = get_net(board, "anode-1")
    route_path(board, [j7_p1, r28_p1], F_Cu, 0.254, net_3v3)
    route_path(board, [r28_p2, d3_p1], F_Cu, 0.254, net_anode_d3)

    # Remote Mesh RX LED D5 (GPIO 16 on J7.9 -> R32 -> D5)
    j7_p9  = get_pad_pos(board, "J7", 9)
    r32_p1 = get_pad_pos(board, "R32", 1)
    r32_p2 = get_pad_pos(board, "R32", 2)
    d5_p1  = get_pad_pos(board, "D5", 1)
    net_led5 = get_net(board, "1-1")
    net_anode_d5 = get_net(board, "anode-2")
    route_path(board, [j7_p9, r32_p1], F_Cu, 0.254, net_led5)
    route_path(board, [r32_p2, d5_p1], F_Cu, 0.254, net_anode_d5)

    # Cabin Status LED D1 (GPIO 2 on J8.5 -> R10 -> D1)
    j8_p5  = get_pad_pos(board, "J8", 5)
    r10_p1 = get_pad_pos(board, "R10", 1)
    r10_p2 = get_pad_pos(board, "R10", 2)
    d1_p1  = get_pad_pos(board, "D1", 1)
    net_led1 = get_net(board, "1")
    net_anode_d1 = get_net(board, "anode")
    route_path(board, [j8_p5, r10_p1], F_Cu, 0.254, net_led1)
    route_path(board, [r10_p2, d1_p1], F_Cu, 0.254, net_anode_d1)

    # =========================================================================
    # 5.8 BATTERY TELEMETRY VOLTAGE DIVIDER (F.Cu)
    # =========================================================================
    r33_p2 = get_pad_pos(board, "R33", 2)
    r34_p1 = get_pad_pos(board, "R34", 1)
    c1_p1  = get_pad_pos(board, "C1", 1)
    r35_p1 = get_pad_pos(board, "R35", 1)
    r35_p2 = get_pad_pos(board, "R35", 2)
    j8_p4  = get_pad_pos(board, "J8", 4)

    # Connect divider node along Y = -17.087 mm
    route_path(board, [c1_p1, r34_p1, r33_p2], F_Cu, 0.4, net_div)
    route_path(board, [r33_p2, (r35_p1[0], r33_p2[1]), r35_p1], F_Cu, 0.4, net_div)
    # Connect ADC protection resistor to J8.4
    route_path(board, [r35_p2, j8_p4], F_Cu, 0.254, net_adc)

    # Connect R34 Pad 2 and C1 Pad 2 GND node with local stitching via
    r34_p2 = get_pad_pos(board, "R34", 2)
    c1_p2  = get_pad_pos(board, "C1", 2)
    route_path(board, [r34_p2, (28.0, -18.913), c1_p2], F_Cu, 0.35, net_gnd)
    add_via(board, 28.0, -18.913, net_gnd)

    # =========================================================================
    # 5.9 GROUND CONNECTIONS & FUNCTIONAL GROUND VIAS
    # =========================================================================
    # Pulldown resistors Pad 2 -> GND
    pd_refs = ["R17", "R20", "R23", "R26", "R30", "R2"]
    for r_ref in pd_refs:
        p2 = get_pad_pos(board, r_ref, 2)
        add_via(board, p2[0], p2[1] + 1.2, net_gnd)
        route_path(board, [p2, (p2[0], p2[1] + 1.2)], F_Cu, 0.254, net_gnd)

    # Current sense resistors Pad 2 (North) -> GND (via placed to the West of pad)
    cs_refs = ["R18", "R21", "R24", "R27", "R31", "R3"]
    for r_ref in cs_refs:
        p2 = get_pad_pos(board, r_ref, 2)
        add_via(board, p2[0] - 1.2, p2[1], net_gnd)
        route_path(board, [p2, (p2[0] - 1.2, p2[1])], F_Cu, 0.254, net_gnd)

    # PROFET Pad 3 and Pad 5 Solid Ground Stitching (vias behind each individual pad)
    profet_refs = ["U6", "U7", "U8", "U9", "U10", "U1"]
    for u_ref in profet_refs:
        p3 = get_pad_pos(board, u_ref, 3)
        p5 = get_pad_pos(board, u_ref, 5)
        add_via(board, p3[0], 8.5, net_gnd)
        route_path(board, [p3, (p3[0], 8.5)], F_Cu, 0.254, net_gnd)
        add_via(board, p5[0], 8.5, net_gnd)
        route_path(board, [p5, (p5[0], 8.5)], F_Cu, 0.254, net_gnd)

    # Opto GNDs (U4.2, U5.2)
    for opto in ["U4", "U5"]:
        p2 = get_pad_pos(board, opto, 2)
        add_via(board, p2[0] - 1.5, p2[1], net_gnd)
        route_path(board, [p2, (p2[0] - 1.5, p2[1])], F_Cu, 0.254, net_gnd)

    # Fan Q1.2 & R13.2 (Q1 Pad 2 GND via placed to the South at Y = -9.0 mm)
    q1_p2 = get_pad_pos(board, "Q1", 2)
    add_via(board, q1_p2[0], -9.0, net_gnd)
    route_path(board, [q1_p2, (q1_p2[0], -9.0)], F_Cu, 0.254, net_gnd)

    r13_p2 = get_pad_pos(board, "R13", 2)
    add_via(board, r13_p2[0], r13_p2[1] - 1.2, net_gnd)
    route_path(board, [r13_p2, (r13_p2[0], r13_p2[1] - 1.2)], F_Cu, 0.254, net_gnd)



    # Status LEDs Cathodes (D3.2, D5.2, D1.2)
    d3_p2 = get_pad_pos(board, "D3", 2)
    add_via(board, d3_p2[0] - 1.5, d3_p2[1], net_gnd)
    route_path(board, [d3_p2, (d3_p2[0] - 1.5, d3_p2[1])], F_Cu, 0.254, net_gnd)

    d5_p2 = get_pad_pos(board, "D5", 2)
    add_via(board, d5_p2[0], d5_p2[1] - 1.8, net_gnd)
    route_path(board, [d5_p2, (d5_p2[0], d5_p2[1] - 1.8)], F_Cu, 0.254, net_gnd)

    d1_p2 = get_pad_pos(board, "D1", 2)
    add_via(board, d1_p2[0] + 1.5, d1_p2[1], net_gnd)
    route_path(board, [d1_p2, (d1_p2[0] + 1.5, d1_p2[1])], F_Cu, 0.254, net_gnd)

    # Buck converter GNDs (J16.2, J4.2, J5.2)
    j16_p2 = get_pad_pos(board, "J16", 2)
    add_via(board, j16_p2[0] - 2.0, j16_p2[1], net_gnd)
    route_path(board, [j16_p2, (j16_p2[0] - 2.0, j16_p2[1])], F_Cu, 0.8, net_gnd)

    j4_p2 = get_pad_pos(board, "J4", 2)
    add_via(board, j4_p2[0] + 2.0, j4_p2[1], net_gnd)
    route_path(board, [j4_p2, (j4_p2[0] + 2.0, j4_p2[1])], F_Cu, 0.8, net_gnd)

    j5_p2 = get_pad_pos(board, "J5", 2)
    add_via(board, j5_p2[0] + 2.0, j5_p2[1], net_gnd)
    route_path(board, [j5_p2, (j5_p2[0] + 2.0, j5_p2[1])], F_Cu, 0.8, net_gnd)

    # D4 Flyback Anode GND (Pad 2)
    d4_p2 = get_pad_pos(board, "D4", 2)
    add_via(board, d4_p2[0], d4_p2[1] - 2.5, net_gnd)
    route_path(board, [d4_p2, (d4_p2[0], d4_p2[1] - 2.5)], F_Cu, 0.8, net_gnd)

    # MCU Header GND Pins
    j8_p1 = get_pad_pos(board, "J8", 1)
    add_via(board, 10.0, j8_p1[1], net_gnd)
    route_path(board, [j8_p1, (10.0, j8_p1[1])], F_Cu, 0.5, net_gnd)

    j7_p22 = get_pad_pos(board, "J7", 22)
    add_via(board, -10.0, j7_p22[1], net_gnd)
    route_path(board, [j7_p22, (-10.0, j7_p22[1])], F_Cu, 0.5, net_gnd)

    j8_p21 = get_pad_pos(board, "J8", 21)
    j8_p22 = get_pad_pos(board, "J8", 22)
    add_via(board, 10.0, 30.0, net_gnd)
    route_path(board, [j8_p21, (10.0, 30.0), j8_p22], F_Cu, 0.5, net_gnd)

    # Terminal GND Pins (Pad 4 of J11-J15, Pad 2 of J1, Pad 2 of J9)
    for term_gnd, pad_num in [("J11", 4), ("J12", 4), ("J13", 4), ("J14", 4), ("J15", 4)]:
        gp = get_pad_pos(board, term_gnd, pad_num)
        add_via(board, gp[0], 41.0, net_gnd)
        route_path(board, [gp, (gp[0], 41.0)], F_Cu, 0.8, net_gnd)

    # J1 Pad 2 GND (routed East of mechanical peg to avoid collision)
    j1_p2 = get_pad_pos(board, "J1", 2)
    add_via(board, j1_p2[0] + 2.0, 41.0, net_gnd)
    route_path(board, [j1_p2, (j1_p2[0] + 2.0, j1_p2[1]), (j1_p2[0] + 2.0, 41.0)], F_Cu, 0.8, net_gnd)

    # J9 Pad 2 GND
    j9_p2 = get_pad_pos(board, "J9", 2)
    add_via(board, j9_p2[0], j9_p2[1] - 3.0, net_gnd)
    route_path(board, [j9_p2, (j9_p2[0], j9_p2[1] - 3.0)], F_Cu, 0.8, net_gnd)

    # Safe Edge Ground Stitching Vias (safely offset from M3 mounting hole cutouts at +-70, +-42.5)
    edge_stitching = [
        (-64.0, -35.0), (-64.0, 0.0), (-64.0, 35.0),
        ( 64.0, -35.0), ( 64.0, 0.0), ( 64.0, 35.0),
        (-50.0, -42.0), (-30.0, -42.0), ( 10.0, -42.0),
        (-50.0,  42.0), (-30.0,  42.0), ( 10.0,  42.0), ( 30.0, 42.0), ( 50.0, 42.0)
    ]
    for sx, sy in edge_stitching:
        add_via(board, sx, sy, net_gnd)

    print("Completed all signal, power, and ground routing.")

    # 6. Filled Copper Ground Planes (F.Cu and B.Cu)
    zone_margin = 0.5
    zone_pts = [
        (-w/2 + zone_margin, -h/2 + zone_margin),
        ( w/2 - zone_margin, -h/2 + zone_margin),
        ( w/2 - zone_margin,  h/2 - zone_margin),
        (-w/2 + zone_margin,  h/2 - zone_margin)
    ]

    for layer in [F_Cu, B_Cu]:
        zone = pcbnew.ZONE(board)
        lset = pcbnew.LSET()
        lset.AddLayer(layer)
        zone.SetLayerSet(lset)
        zone.SetLayer(layer)
        zone.SetNet(net_gnd)
        zone.SetAssignedPriority(0)
        zone.SetMinThickness(mm_to_nm(0.25))
        zone.SetPadConnection(pcbnew.ZONE_CONNECTION_THT_THERMAL)
        zone.SetThermalReliefGap(mm_to_nm(0.25))
        zone.SetThermalReliefSpokeWidth(mm_to_nm(0.25))
        zone.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)

        outline = zone.Outline()
        outline.NewOutline()
        for zx, zy in zone_pts:
            outline.Append(mm_to_nm(zx), mm_to_nm(zy))

        board.Add(zone)

    # Refill all zones
    filler = pcbnew.ZONE_FILLER(board)
    filler.Fill(board.Zones())
    print("Filled copper ground planes on F.Cu and B.Cu.")

    # 7. Save Board and sync to default layout
    board.Save(board_path)
    print(f"Successfully saved completed board to {board_path}!")

    default_path = os.path.join(os.path.dirname(__file__), "..", "layouts", "default", "default.kicad_pcb")
    board.Save(default_path)
    print(f"Synced board to: {default_path}")

if __name__ == "__main__":
    main()
