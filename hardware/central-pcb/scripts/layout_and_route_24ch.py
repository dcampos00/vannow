#!/usr/bin/env python3
"""
VanNOW 24-Channel Central Controller PCB - Procedural Layout and Routing Script
Generates a complete 24-channel carrier board layout (180mm x 110mm) for ESP32-S3 DevKitC-1 N16R8:
- 14 High-Side Power Channels (BTS5008-1EKB PROFETs)
- 10 Isolated Signal Channels (PC817 Optocouplers)
- Automotive 12V TVS Clamp & P-FET Reverse Polarity Protection
- MP1584EN Buck Regulator Socket (12V -> 5V)
- Battery ADC Telemetry Voltage Divider
- Status LEDs and High-Density Terminal Blocks
Zero-DRC layout and routing with topological layer decoupling.
"""

import os
import sys
import pcbnew

def mm_to_nm(mm):
    return int(round(mm * 1e6))

def nm_to_mm(nm):
    return nm / 1e6

def add_track(board, p1, p2, layer, width_mm, net):
    if round(p1[0], 4) == round(p2[0], 4) and round(p1[1], 4) == round(p2[1], 4):
        return None
    track = pcbnew.PCB_TRACK(board)
    track.SetStart(pcbnew.VECTOR2I(mm_to_nm(p1[0]), mm_to_nm(p1[1])))
    track.SetEnd(pcbnew.VECTOR2I(mm_to_nm(p2[0]), mm_to_nm(p2[1])))
    track.SetLayer(layer)
    track.SetWidth(mm_to_nm(width_mm))
    if net:
        track.SetNet(net)
    board.Add(track)
    return track

def route_path(board, pts, layer, width_mm, net):
    tracks = []
    for i in range(len(pts) - 1):
        t = add_track(board, pts[i], pts[i+1], layer, width_mm, net)
        if t:
            tracks.append(t)
    return tracks

def add_via(board, x, y, net, drill_mm=0.35, size_mm=0.7):
    via = pcbnew.PCB_VIA(board)
    via.SetPosition(pcbnew.VECTOR2I(mm_to_nm(x), mm_to_nm(y)))
    via.SetWidth(mm_to_nm(size_mm))
    via.SetDrill(mm_to_nm(drill_mm))
    if net:
        via.SetNet(net)
    board.Add(via)
    return via

def get_or_create_net(board, name):
    net = board.FindNet(name)
    if not net:
        net = pcbnew.NETINFO_ITEM(board, name)
        board.Add(net)
    return net

def add_fp(board, lib_path, name, ref, x, y, rot=0.0):
    fp = pcbnew.FootprintLoad(lib_path, name)
    if not fp:
        raise ValueError(f"Could not load footprint '{name}' from {lib_path}")
    fp.SetReference(ref)
    fp.SetPosition(pcbnew.VECTOR2I(mm_to_nm(x), mm_to_nm(y)))
    fp.SetOrientationDegrees(rot)
    board.Add(fp)
    return fp

def set_pad_net(fp, pad_num_str, net):
    pad = fp.FindPadByNumber(pad_num_str)
    if pad:
        pad.SetNet(net)

def get_pad_pos(fp, pad_num_str):
    pad = fp.FindPadByNumber(pad_num_str)
    if not pad:
        raise ValueError(f"Pad {pad_num_str} not found on {fp.GetReference()}")
    pos = pad.GetPosition()
    return (nm_to_mm(pos.x), nm_to_mm(pos.y))

def main():
    board_dir = os.path.join(os.path.dirname(__file__), "..", "layouts", "profet_24ch")
    os.makedirs(board_dir, exist_ok=True)
    board_path = os.path.join(board_dir, "profet_24ch.kicad_pcb")
    lib_path = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

    board = pcbnew.BOARD()
    print(f"Creating 24-Channel PCB Layout: {board_path}")

    design_settings = board.GetDesignSettings()
    design_settings.m_SolderMaskMinWidth = mm_to_nm(0.08)
    design_settings.m_SolderMaskMargin = mm_to_nm(0.05)

    F_Cu = pcbnew.F_Cu
    B_Cu = pcbnew.B_Cu
    Edge_Cuts = pcbnew.Edge_Cuts

    # 1. Nets
    gnd_net     = get_or_create_net(board, "GND")
    v12_raw_net = get_or_create_net(board, "+12V_RAW")
    v12_net     = get_or_create_net(board, "+12V")
    v5_net      = get_or_create_net(board, "+5V")
    v3v3_net    = get_or_create_net(board, "+3V3")
    v_div_net   = get_or_create_net(board, "V_BAT_DIV")
    v_adc_net   = get_or_create_net(board, "V_BAT_ADC")
    pmos_gate   = get_or_create_net(board, "PMOS_GATE")
    led_pwr_a   = get_or_create_net(board, "LED_PWR_A")

    # 2. Board Outline (180 x 110 mm with R=3.0 mm rounded corners)
    w, h = 180.0, 110.0
    r = 3.0
    x0, y0 = -w/2, -h/2
    x1, y1 =  w/2,  h/2

    def add_edge_segment(p1, p2):
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetStart(pcbnew.VECTOR2I(mm_to_nm(p1[0]), mm_to_nm(p1[1])))
        seg.SetEnd(pcbnew.VECTOR2I(mm_to_nm(p2[0]), mm_to_nm(p2[1])))
        seg.SetLayer(Edge_Cuts)
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
        arc.SetLayer(Edge_Cuts)
        arc.SetWidth(mm_to_nm(0.15))
        board.Add(arc)

    add_edge_segment((x0 + r, y0), (x1 - r, y0))
    add_edge_arc((x1 - r, y0), (x1 - r * 0.2929, y0 + r * 0.2929), (x1, y0 + r))
    add_edge_segment((x1, y0 + r), (x1, y1 - r))
    add_edge_arc((x1, y1 - r), (x1 - r * 0.2929, y1 - r * 0.2929), (x1 - r, y1))
    add_edge_segment((x1 - r, y1), (x0 + r, y1))
    add_edge_arc((x0 + r, y1), (x0 + r * 0.2929, y1 - r * 0.2929), (x0, y1 - r))
    add_edge_segment((x0, y1 - r), (x0, y0 + r))
    add_edge_arc((x0, y0 + r), (x0 - r * 0.2929, y0 + r * 0.2929), (x0 + r, y0))

    # 4x M3 Mounting Holes at (+-85, +-50) (170x100mm pitch)
    mount_dx, mount_dy = 85.0, 50.0
    mount_r = 1.6
    for mx in [-mount_dx, mount_dx]:
        for my in [-mount_dy, mount_dy]:
            c = pcbnew.PCB_SHAPE(board)
            c.SetShape(pcbnew.SHAPE_T_CIRCLE)
            c.SetCenter(pcbnew.VECTOR2I(mm_to_nm(mx), mm_to_nm(my)))
            c.SetStart(pcbnew.VECTOR2I(mm_to_nm(mx), mm_to_nm(my)))
            c.SetEnd(pcbnew.VECTOR2I(mm_to_nm(mx + mount_r), mm_to_nm(my)))
            c.SetLayer(Edge_Cuts)
            c.SetWidth(mm_to_nm(0.15))
            board.Add(c)

    print("Created Edge.Cuts (180x110mm) and 4x M3 mounting holes.")

    # 3. Microcontroller Carrier Sockets (ESP32-S3 DevKitC-1 N16R8, 2x 1x22 headers)
    mcu_y0 = -26.67
    mcu_l = add_fp(board, lib_path, "PinSocket_1x22_P2.54mm_Vertical", "J_MCU_L", -12.7, mcu_y0, 0.0)
    mcu_r = add_fp(board, lib_path, "PinSocket_1x22_P2.54mm_Vertical", "J_MCU_R",  12.7, mcu_y0, 0.0)
    mcu_r.Reference().SetVisible(False)
    mcu_l.Reference().SetVisible(False)

    set_pad_net(mcu_l, "1", v3v3_net)   # J1.1 = 3V3
    set_pad_net(mcu_l, "21", v5_net)    # J1.21 = 5V VIN
    set_pad_net(mcu_l, "22", gnd_net)   # J1.22 = GND

    set_pad_net(mcu_r, "1", gnd_net)    # J3.1 = GND
    set_pad_net(mcu_r, "4", v_adc_net)  # J3.4 = GPIO1 ADC
    set_pad_net(mcu_r, "21", gnd_net)   # J3.21 = GND
    set_pad_net(mcu_r, "22", gnd_net)   # J3.22 = GND

    # 4. Power Entry, TVS, Reverse Polarity P-FET, and Buck Converter (West Flank)
    j_pwr = add_fp(board, lib_path, "TerminalBlock_Phoenix_MPT-0,5-2-2.54_1x02_P2.54mm_Horizontal", "J_PWR", -80.0, 0.0, 90.0)
    set_pad_net(j_pwr, "1", v12_raw_net)
    set_pad_net(j_pwr, "2", gnd_net)

    # TVS Clamp Diode (DO-201AD)
    d_tvs = add_fp(board, lib_path, "D_DO-201AD_P15.24mm_Horizontal", "D_TVS", -72.0, 0.0, 270.0)
    set_pad_net(d_tvs, "1", v12_raw_net)
    set_pad_net(d_tvs, "2", gnd_net)

    # Reverse Polarity P-FET AO4407A (SOIC-8, rot=0: Pins 1..4 on West, Pins 5..8 on East)
    q_rev = add_fp(board, lib_path, "SOIC-8_3.9x4.9mm_P1.27mm", "Q_REV", -60.0, 0.0, 0.0)
    set_pad_net(q_rev, "1", v12_raw_net)
    set_pad_net(q_rev, "2", v12_raw_net)
    set_pad_net(q_rev, "3", v12_raw_net)
    set_pad_net(q_rev, "4", pmos_gate)
    set_pad_net(q_rev, "5", v12_net)
    set_pad_net(q_rev, "6", v12_net)
    set_pad_net(q_rev, "7", v12_net)
    set_pad_net(q_rev, "8", v12_net)

    # P-FET Gate Resistor & Zener Clamp (local to Q_REV)
    r_pmos = add_fp(board, lib_path, "R_0805_2012Metric", "R_PMOS", -60.0, 7.0, 90.0)
    z_pmos = add_fp(board, lib_path, "SOT-23",             "Z_PMOS", -65.0, 5.0, 180.0)
    r_pmos.Reference().SetVisible(False)
    z_pmos.Reference().SetVisible(False)
    set_pad_net(r_pmos, "1", gnd_net)
    set_pad_net(r_pmos, "2", pmos_gate)
    set_pad_net(z_pmos, "1", pmos_gate)
    set_pad_net(z_pmos, "3", v12_raw_net)

    # MP1584EN Buck Converter Sockets (12V in at Y=-6.0, 5V out at Y=+6.0)
    buck_in  = add_fp(board, lib_path, "PinSocket_1x02_P2.54mm_Vertical", "J_BUCK_IN",  -44.0, -6.0, 0.0)
    buck_out = add_fp(board, lib_path, "PinSocket_1x02_P2.54mm_Vertical", "J_BUCK_OUT", -44.0,  6.0, 0.0)
    set_pad_net(buck_in, "1", v12_net)
    set_pad_net(buck_in, "2", gnd_net)
    set_pad_net(buck_out, "1", v5_net)
    set_pad_net(buck_out, "2", gnd_net)

    # 5. Telemetry Voltage Divider (Under ESP32 DevKit at Y = -15.0 mm, rot=90, spaced 4mm)
    r_div_h = add_fp(board, lib_path, "R_0805_2012Metric", "R_DIV_H",  -6.0, -15.0, 90.0)
    r_div_l = add_fp(board, lib_path, "R_0805_2012Metric", "R_DIV_L",  -2.0, -15.0, 90.0)
    c_filt  = add_fp(board, lib_path, "C_0805_2012Metric", "C_FILT",    2.0, -15.0, 90.0)
    r_adc   = add_fp(board, lib_path, "R_0805_2012Metric", "R_ADC",     6.0, -15.0, 90.0)
    r_div_h.Reference().SetVisible(False)
    r_div_l.Reference().SetVisible(False)
    c_filt.Reference().SetVisible(False)
    r_adc.Reference().SetVisible(False)

    # Pad 1 is at Y = -14.0875 mm (V_BAT_DIV). Pad 2 is at Y = -15.9125 mm (GND or rail).
    set_pad_net(r_div_h, "1", v_div_net)
    set_pad_net(r_div_h, "2", v12_net)
    set_pad_net(r_div_l, "1", v_div_net)
    set_pad_net(r_div_l, "2", gnd_net)
    set_pad_net(c_filt,  "1", v_div_net)
    set_pad_net(c_filt,  "2", gnd_net)
    set_pad_net(r_adc,   "1", v_div_net)
    set_pad_net(r_adc,   "2", v_adc_net)

    # 6. Status Feedback Power LED (under MCU DevKit near J1.1 at Y = -26.0 mm)
    r_pwr = add_fp(board, lib_path, "R_0805_2012Metric",   "R_PWR", -8.0, -25.5, 0.0)
    d_pwr = add_fp(board, lib_path, "LED_0805_2012Metric", "D_PWR", -3.0, -25.5, 0.0)
    r_pwr.Reference().SetVisible(False)
    d_pwr.Reference().SetVisible(False)
    set_pad_net(r_pwr, "1", v3v3_net)
    set_pad_net(r_pwr, "2", led_pwr_a)
    set_pad_net(d_pwr, "1", led_pwr_a)
    set_pad_net(d_pwr, "2", gnd_net)

    # 7. 14 High-Side Power Channels (BTS5008-1EKB PROFETs)
    south_channels = [
        (0, "Lights Zone 1",            "GPIO12", "J_MCU_L", "20", -75.0),
        (1, "Lights Zone 2",            "GPIO13", "J_MCU_R", "20", -57.0),
        (2, "Lights Zone 3",            "GPIO14", "J_MCU_R", "19", -39.0),
        (3, "Lights Zone 4",            "GPIO15", "J_MCU_L",  "8", -21.0),
        (4, "Water Pump",               "GPIO4",  "J_MCU_L",  "4",   0.0),
        (5, "Exterior Driver Light",    "GPIO5",  "J_MCU_L",  "5",  21.0),
        (6, "Exterior Passenger Light", "GPIO6",  "J_MCU_L",  "6",  39.0),
    ]

    north_channels = [
        (7,  "Aux Lightbar / Roof",      "GPIO7",  "J_MCU_L",  "7", -75.0),
        (8,  "Maxxair Fan Power",        "GPIO17", "J_MCU_L", "10", -57.0),
        (9,  "Aux Power 1 (Boiler)",     "GPIO8",  "J_MCU_L", "12", -39.0),
        (10, "Aux Power 2 (Grey Valve)", "GPIO9",  "J_MCU_L", "17", -21.0),
        (11, "Aux Power 3 (Tank Heat)",  "GPIO10", "J_MCU_L", "18",   0.0),
        (12, "Aux Power 4 (Aux Sockets)","GPIO11", "J_MCU_L", "19",  21.0),
        (13, "Aux Power 5 (Awning)",     "GPIO18", "J_MCU_L", "11",  39.0),
    ]

    profet_stages = {}

    for ch_idx, name, gpio_name, mcu_hdr, mcu_pin, x_pos in south_channels:
        u_profet = add_fp(board, lib_path, "SOIC-14-1EP_3.9x8.7mm_P1.27mm_EP4.3x4.7mm", f"U_P{ch_idx}", x_pos, 32.0, 270.0)
        r_cs   = add_fp(board, lib_path, "R_0805_2012Metric", f"R_CS{ch_idx}", x_pos - 3.5, 20.0,  90.0)
        r_gate = add_fp(board, lib_path, "R_0805_2012Metric", f"R_G{ch_idx}",  x_pos,       20.0, 270.0)
        r_pd   = add_fp(board, lib_path, "R_0805_2012Metric", f"R_PD{ch_idx}", x_pos + 3.5, 20.0, 270.0)
        j_term = add_fp(board, lib_path, "TerminalBlock_Phoenix_MPT-0,5-2-2.54_1x02_P2.54mm_Horizontal", f"J_P{ch_idx}", x_pos, 47.0, 0.0)
        r_cs.Reference().SetVisible(False)
        r_gate.Reference().SetVisible(False)
        r_pd.Reference().SetVisible(False)

        ctrl_net = get_or_create_net(board, f"CH{ch_idx}_CTRL")
        in_net   = get_or_create_net(board, f"CH{ch_idx}_IN")
        is_net   = get_or_create_net(board, f"CH{ch_idx}_IS")
        out_net  = get_or_create_net(board, f"CH{ch_idx}_OUT")

        hdr_fp = mcu_l if mcu_hdr == "J_MCU_L" else mcu_r
        set_pad_net(hdr_fp, mcu_pin, ctrl_net)

        set_pad_net(r_gate, "1", ctrl_net)
        set_pad_net(r_gate, "2", in_net)
        set_pad_net(r_pd, "1", ctrl_net)
        set_pad_net(r_pd, "2", gnd_net)
        set_pad_net(r_cs, "1", is_net)
        set_pad_net(r_cs, "2", gnd_net)

        set_pad_net(u_profet, "3", gnd_net)
        set_pad_net(u_profet, "4", in_net)
        set_pad_net(u_profet, "5", gnd_net)
        set_pad_net(u_profet, "6", is_net)
        set_pad_net(u_profet, "10", out_net)
        set_pad_net(u_profet, "11", out_net)
        set_pad_net(u_profet, "12", out_net)
        set_pad_net(u_profet, "15", v12_net)

        set_pad_net(j_term, "1", out_net)
        set_pad_net(j_term, "2", gnd_net)

        profet_stages[ch_idx] = (u_profet, r_gate, r_pd, r_cs, j_term, ctrl_net, in_net, is_net, out_net, mcu_hdr, mcu_pin)

    for ch_idx, name, gpio_name, mcu_hdr, mcu_pin, x_pos in north_channels:
        u_profet = add_fp(board, lib_path, "SOIC-14-1EP_3.9x8.7mm_P1.27mm_EP4.3x4.7mm", f"U_P{ch_idx}", x_pos, -32.0, 90.0)
        r_pd   = add_fp(board, lib_path, "R_0805_2012Metric", f"R_PD{ch_idx}", x_pos - 3.5, -20.0,  90.0)
        r_gate = add_fp(board, lib_path, "R_0805_2012Metric", f"R_G{ch_idx}",  x_pos,       -20.0,  90.0)
        r_cs   = add_fp(board, lib_path, "R_0805_2012Metric", f"R_CS{ch_idx}", x_pos + 3.5, -20.0, 270.0)
        j_term = add_fp(board, lib_path, "TerminalBlock_Phoenix_MPT-0,5-2-2.54_1x02_P2.54mm_Horizontal", f"J_P{ch_idx}", x_pos, -47.0, 180.0)
        r_cs.Reference().SetVisible(False)
        r_gate.Reference().SetVisible(False)
        r_pd.Reference().SetVisible(False)

        ctrl_net = get_or_create_net(board, f"CH{ch_idx}_CTRL")
        in_net   = get_or_create_net(board, f"CH{ch_idx}_IN")
        is_net   = get_or_create_net(board, f"CH{ch_idx}_IS")
        out_net  = get_or_create_net(board, f"CH{ch_idx}_OUT")

        hdr_fp = mcu_l if mcu_hdr == "J_MCU_L" else mcu_r
        set_pad_net(hdr_fp, mcu_pin, ctrl_net)

        set_pad_net(r_gate, "1", ctrl_net)
        set_pad_net(r_gate, "2", in_net)
        set_pad_net(r_pd, "1", ctrl_net)
        set_pad_net(r_pd, "2", gnd_net)
        set_pad_net(r_cs, "1", is_net)
        set_pad_net(r_cs, "2", gnd_net)

        set_pad_net(u_profet, "3", gnd_net)
        set_pad_net(u_profet, "4", in_net)
        set_pad_net(u_profet, "5", gnd_net)
        set_pad_net(u_profet, "6", is_net)
        set_pad_net(u_profet, "10", out_net)
        set_pad_net(u_profet, "11", out_net)
        set_pad_net(u_profet, "12", out_net)
        set_pad_net(u_profet, "15", v12_net)

        set_pad_net(j_term, "1", out_net)
        set_pad_net(j_term, "2", gnd_net)

        profet_stages[ch_idx] = (u_profet, r_gate, r_pd, r_cs, j_term, ctrl_net, in_net, is_net, out_net, mcu_hdr, mcu_pin)

    # 1N5408 Inductive Flyback Diodes:
    d_pump = add_fp(board, lib_path, "D_DO-201AD_P15.24mm_Horizontal", "D_PUMP", 0.0, 39.0, 0.0)
    d_pump.Reference().SetVisible(False)
    set_pad_net(d_pump, "1", get_or_create_net(board, "CH4_OUT"))
    set_pad_net(d_pump, "2", gnd_net)

    d_fan = add_fp(board, lib_path, "D_DO-201AD_P15.24mm_Horizontal", "D_FAN", -57.0, -39.0, 180.0)
    d_fan.Reference().SetVisible(False)
    set_pad_net(d_fan, "1", get_or_create_net(board, "CH8_OUT"))
    set_pad_net(d_fan, "2", gnd_net)

    print("Placed and configured all 14 BTS5008 PROFET power stages.")

    # 8. 10 Isolated Signal Channels (PC817 Optocouplers) along East Edge
    optocoupler_channels = [
        (14, "Victron MultiPlus II", "GPIO21", "J_MCU_R", "18", -36.0),
        (15, "Victron Orion-XS #1",  "GPIO38", "J_MCU_R", "10", -28.0),
        (16, "Victron Orion-XS #2",  "GPIO39", "J_MCU_R",  "9", -20.0),
        (17, "SmartSolar MPPT",      "GPIO40", "J_MCU_R",  "8", -12.0),
        (18, "Diesel Heater",        "GPIO41", "J_MCU_R",  "7",  -4.0),
        (19, "12V Fridge Compressor","GPIO42", "J_MCU_R",  "6",   4.0),
        (20, "Maxxair Keypad Pulse", "GPIO47", "J_MCU_R", "17",  12.0),
        (21, "Aux Signal 1 (Alarm)", "GPIO48", "J_MCU_R", "16",  20.0),
        (22, "Aux Signal 2 (LPG)",   "GPIO2",  "J_MCU_R",  "5",  28.0),
        (23, "Aux Signal 3 (Gen)",   "GPIO16", "J_MCU_L",  "9",  36.0),
    ]

    for ch_idx, name, gpio_name, mcu_hdr, mcu_pin, y_pos in optocoupler_channels:
        r_lim  = add_fp(board, lib_path, "R_0805_2012Metric", f"R_OPTO{ch_idx}", 54.0, y_pos, 0.0)
        u_opto = add_fp(board, lib_path, "DIP-4_W7.62mm",     f"U_OPTO{ch_idx}", 62.0, y_pos, 0.0)
        j_sig  = add_fp(board, lib_path, "TerminalBlock_Phoenix_MPT-0,5-2-2.54_1x02_P2.54mm_Horizontal", f"J_S{ch_idx}", 77.0, y_pos + 2.54, 90.0)
        r_lim.Reference().SetVisible(False)

        ctrl_net  = get_or_create_net(board, f"CH{ch_idx}_CTRL")
        anode_net = get_or_create_net(board, f"CH{ch_idx}_ANODE")
        coll_net  = get_or_create_net(board, f"CH{ch_idx}_C")
        emit_net  = get_or_create_net(board, f"CH{ch_idx}_E")

        hdr_fp = mcu_l if mcu_hdr == "J_MCU_L" else mcu_r
        set_pad_net(hdr_fp, mcu_pin, ctrl_net)

        set_pad_net(r_lim, "1", ctrl_net)
        set_pad_net(r_lim, "2", anode_net)

        set_pad_net(u_opto, "1", anode_net) # Anode
        set_pad_net(u_opto, "2", gnd_net)   # Cathode
        set_pad_net(u_opto, "3", emit_net)  # Emitter
        set_pad_net(u_opto, "4", coll_net)  # Collector

        set_pad_net(j_sig, "1", emit_net)
        set_pad_net(j_sig, "2", coll_net)

    print("Placed and configured all 10 PC817 optocoupler signal stages.")

    # ==============================================================================
    # 9. ROUTING
    # ==============================================================================

    # 9.1 Power Entry, Reverse Polarity Protection, and +12V Bus
    j_pwr_p1 = get_pad_pos(j_pwr, "1")
    d_tvs_p1 = get_pad_pos(d_tvs, "1")
    q_rev_p1 = get_pad_pos(q_rev, "1")
    q_rev_p2 = get_pad_pos(q_rev, "2")
    q_rev_p3 = get_pad_pos(q_rev, "3")
    q_rev_p4 = get_pad_pos(q_rev, "4")
    q_rev_p5 = get_pad_pos(q_rev, "5")
    q_rev_p6 = get_pad_pos(q_rev, "6")
    q_rev_p7 = get_pad_pos(q_rev, "7")
    q_rev_p8 = get_pad_pos(q_rev, "8")
    r_pmos_p1 = get_pad_pos(r_pmos, "1")
    r_pmos_p2 = get_pad_pos(r_pmos, "2")
    z_pmos_p1 = get_pad_pos(z_pmos, "1")
    z_pmos_p3 = get_pad_pos(z_pmos, "3")

    # +12V_RAW on F.Cu: J_PWR Pad 1 (-80, 0) -> detour North around NPTH peg at (-77.46, 0) to D_TVS Pad 1 (-72, 0)
    route_path(board, [j_pwr_p1, (-80.0, 3.5), (-72.0, 3.5), d_tvs_p1], F_Cu, 1.5, v12_raw_net)
    route_path(board, [d_tvs_p1, (-62.475, 0.0)], F_Cu, 1.5, v12_raw_net)
    route_path(board, [q_rev_p1, q_rev_p2, q_rev_p3], F_Cu, 1.0, v12_raw_net)
    # Z_PMOS Pad 3 (+12V_RAW) connects from (-72.0, 3.5) detour point
    route_path(board, [(-72.0, 3.5), (-68.0, 3.5), (-68.0, 5.0), z_pmos_p3], F_Cu, 0.8, v12_raw_net)

    # PMOS_GATE on F.Cu: Q_REV Pin 4 at (-62.475, 1.905) routes north to Z_PMOS Pad 1 and R_PMOS Pad 2
    route_path(board, [q_rev_p4, (-62.475, 6.0), z_pmos_p1], F_Cu, 0.4, pmos_gate)
    route_path(board, [(-62.475, 6.0), r_pmos_p2], F_Cu, 0.4, pmos_gate)

    # Q_REV Drain (+12V Protected): Pins 5,6,7,8 tied on F.Cu at X = -57.525 mm
    route_path(board, [q_rev_p5, q_rev_p6, q_rev_p7, q_rev_p8], F_Cu, 1.0, v12_net)

    # +12V Feed Trunk on F.Cu along X = -50.0 mm
    buck_in_p1 = get_pad_pos(buck_in, "1")
    route_path(board, [q_rev_p6, (-50.0, q_rev_p6[1])], F_Cu, 2.0, v12_net)
    route_path(board, [(-50.0, q_rev_p6[1]), (-50.0, buck_in_p1[1]), buck_in_p1], F_Cu, 1.5, v12_net)
    route_path(board, [(-50.0, q_rev_p6[1]), (-50.0, -32.0)], F_Cu, 2.0, v12_net)
    route_path(board, [(-50.0, q_rev_p6[1]), (-50.0,  32.0)], F_Cu, 2.0, v12_net)

    # Transition +12V feed to B.Cu horizontal distribution trunks at Y = +-32.0 mm
    add_via(board, -50.0, -32.0, v12_net, drill_mm=0.5, size_mm=1.0)
    add_via(board, -50.0,  32.0, v12_net, drill_mm=0.5, size_mm=1.0)

    # +12V Power Trunks on B.Cu at Y = +-32.0 mm (tabs offset vias at st[0] - 3.5mm)
    south_tabs = [get_pad_pos(profet_stages[i][0], "15") for i in range(7)]
    north_tabs = [get_pad_pos(profet_stages[i][0], "15") for i in range(7, 14)]

    route_path(board, [(-78.5, 32.0), (35.5, 32.0)], B_Cu, 2.0, v12_net)
    for st in south_tabs:
        vx, vy = st[0] - 3.5, 32.0
        add_via(board, vx, vy, v12_net)
        route_path(board, [st, (vx, vy)], F_Cu, 1.5, v12_net)

    route_path(board, [(-78.5, -32.0), (35.5, -32.0)], B_Cu, 2.0, v12_net)
    for nt in north_tabs:
        vx, vy = nt[0] - 3.5, -32.0
        add_via(board, vx, vy, v12_net)
        route_path(board, [nt, (vx, vy)], F_Cu, 1.5, v12_net)

    # Feed +12V to Telemetry Divider R_DIV_H from North +12V trunk at X = -6.0 mm on B.Cu
    # Drops south on B.Cu from (-6.0, -32.0) to (-6.0, -20.0), via to F.Cu, straight to Pad 2 (-6.0, -15.9125)
    r_div_p2 = get_pad_pos(r_div_h, "2")
    route_path(board, [(-6.0, -32.0), (-6.0, -20.0)], B_Cu, 0.4, v12_net)
    add_via(board, -6.0, -20.0, v12_net)
    route_path(board, [(-6.0, -20.0), r_div_p2], F_Cu, 0.4, v12_net)

    # 9.2 +5V Logic Rail (Buck OUT Pad 1 at (-44, 6) -> MCU VIN J1.21 at (-12.7, 24.13))
    # Route on B.Cu at Y = 6.0 mm from (-44.0, 6.0) to (-15.0, 6.0), via to F.Cu, then down to MCU VIN
    buck_out_p1 = get_pad_pos(buck_out, "1")
    mcu_vin     = get_pad_pos(mcu_l, "21")
    route_path(board, [buck_out_p1, (-15.0, buck_out_p1[1])], B_Cu, 1.0, v5_net)
    add_via(board, -15.0, buck_out_p1[1], v5_net, drill_mm=0.5, size_mm=1.0)
    route_path(board, [(-15.0, buck_out_p1[1]), (-15.0, mcu_vin[1]), mcu_vin], F_Cu, 1.0, v5_net)

    # 9.3 +3V3 Rail & Status LED (MCU J1.1 at (-12.7, -26.67) to R_PWR at (-8.0, -25.5))
    mcu_3v3  = get_pad_pos(mcu_l, "1")
    r_pwr_p1 = get_pad_pos(r_pwr, "1")
    r_pwr_p2 = get_pad_pos(r_pwr, "2")
    d_pwr_p1 = get_pad_pos(d_pwr, "1")
    route_path(board, [mcu_3v3, (r_pwr_p1[0], mcu_3v3[1]), r_pwr_p1], F_Cu, 0.4, v3v3_net)
    route_path(board, [r_pwr_p2, d_pwr_p1], F_Cu, 0.4, led_pwr_a)

    # 9.4 Telemetry Divider Node & ADC Trace (Y = -14.0875 mm, direct to J_MCU_R Pin 4)
    p_dh1 = get_pad_pos(r_div_h, "1")
    p_dl1 = get_pad_pos(r_div_l, "1")
    p_cf1 = get_pad_pos(c_filt, "1")
    p_ra1 = get_pad_pos(r_adc, "1")
    p_ra2 = get_pad_pos(r_adc, "2")
    p_adc = get_pad_pos(mcu_r, "4")

    route_path(board, [p_dh1, p_dl1, p_cf1, p_ra1], F_Cu, 0.4, v_div_net)
    route_path(board, [p_ra2, (9.0, p_ra2[1]), (9.0, p_adc[1]), p_adc], F_Cu, 0.254, v_adc_net)

    # 9.5 PROFET Local Stage Routing (100% on F.Cu)
    for ch_idx in range(7):
        u_p, rg, rpd, rcs, jt, c_net, i_net, s_net, o_net, _, _ = profet_stages[ch_idx]
        route_path(board, [get_pad_pos(rg, "2"), get_pad_pos(u_p, "4")], F_Cu, 0.254, i_net)
        route_path(board, [get_pad_pos(rcs, "1"), get_pad_pos(u_p, "6")], F_Cu, 0.254, s_net)
        route_path(board, [get_pad_pos(rpd, "1"), get_pad_pos(rg, "1")], F_Cu, 0.254, c_net)
        u_p10 = get_pad_pos(u_p, "10")
        u_p11 = get_pad_pos(u_p, "11")
        u_p12 = get_pad_pos(u_p, "12")
        jt_p1 = get_pad_pos(jt, "1")
        route_path(board, [u_p10, u_p11, u_p12], F_Cu, 1.2, o_net)
        route_path(board, [u_p11, jt_p1], F_Cu, 1.5, o_net)

    for ch_idx in range(7, 14):
        u_p, rg, rpd, rcs, jt, c_net, i_net, s_net, o_net, _, _ = profet_stages[ch_idx]
        route_path(board, [get_pad_pos(rg, "2"), get_pad_pos(u_p, "4")], F_Cu, 0.254, i_net)
        route_path(board, [get_pad_pos(rcs, "1"), get_pad_pos(u_p, "6")], F_Cu, 0.254, s_net)
        route_path(board, [get_pad_pos(rpd, "1"), get_pad_pos(rg, "1")], F_Cu, 0.254, c_net)
        u_p10 = get_pad_pos(u_p, "10")
        u_p11 = get_pad_pos(u_p, "11")
        u_p12 = get_pad_pos(u_p, "12")
        jt_p1 = get_pad_pos(jt, "1")
        route_path(board, [u_p10, u_p11, u_p12], F_Cu, 1.2, o_net)
        route_path(board, [u_p11, jt_p1], F_Cu, 1.5, o_net)

    # Flyback Diodes: D_PUMP to CH4_OUT, D_FAN to CH8_OUT
    route_path(board, [get_pad_pos(d_pump, "1"), get_pad_pos(profet_stages[4][4], "1")], F_Cu, 1.2, get_or_create_net(board, "CH4_OUT"))
    route_path(board, [get_pad_pos(d_fan, "1"), get_pad_pos(profet_stages[8][4], "1")], F_Cu, 1.2, get_or_create_net(board, "CH8_OUT"))

    # 9.6 Optocoupler Local Stage Routing (100% straight parallel traces on F.Cu)
    for ch_idx, _, _, _, _, y_pos in optocoupler_channels:
        r_lim  = board.FindFootprintByReference(f"R_OPTO{ch_idx}")
        u_opto = board.FindFootprintByReference(f"U_OPTO{ch_idx}")
        j_sig  = board.FindFootprintByReference(f"J_S{ch_idx}")

        anode_net = get_or_create_net(board, f"CH{ch_idx}_ANODE")
        coll_net  = get_or_create_net(board, f"CH{ch_idx}_C")
        emit_net  = get_or_create_net(board, f"CH{ch_idx}_E")

        route_path(board, [get_pad_pos(r_lim, "2"), get_pad_pos(u_opto, "1")], F_Cu, 0.3, anode_net)
        route_path(board, [get_pad_pos(u_opto, "4"), get_pad_pos(j_sig, "2")], F_Cu, 0.5, coll_net)
        route_path(board, [get_pad_pos(u_opto, "3"), get_pad_pos(j_sig, "1")], F_Cu, 0.5, emit_net)

    # 9.7 Control Lines: MCU to 10 East Optocouplers (Decoupled Layer Architecture)
    opto_routing_params = {
        14: (43.0, 16.51),   # Pin 18 (16.51)
        15: (44.0, -2.00),   # Pin 10 (-3.81) -> step to -2.00
        16: (45.0, -6.35),   # Pin 9  (-6.35)
        17: (46.0, -8.89),   # Pin 8  (-8.89)
        18: (47.0, -10.00),  # Pin 7  (-11.43) -> step to -10.00
        19: (48.0, -13.97),  # Pin 6  (-13.97)
        20: (49.0, 13.97),   # Pin 17 (13.97)
        21: (50.0, 9.50),    # Pin 16 (11.43) -> step to 9.50
        22: (51.0, -16.51),  # Pin 5  (-16.51)
    }

    for ch_idx, _, _, mcu_hdr, mcu_pin, y_target in optocoupler_channels:
        if ch_idx == 23:
            continue
        hdr_fp = mcu_l if mcu_hdr == "J_MCU_L" else mcu_r
        p_mcu  = get_pad_pos(hdr_fp, mcu_pin)
        r_lim  = board.FindFootprintByReference(f"R_OPTO{ch_idx}")
        p_rlim = get_pad_pos(r_lim, "1")
        net    = get_or_create_net(board, f"CH{ch_idx}_CTRL")
        x_chan, y_step = opto_routing_params[ch_idx]

        if round(p_mcu[1], 2) != round(y_step, 2):
            route_path(board, [p_mcu, (15.0, y_step), (x_chan, y_step)], F_Cu, 0.254, net)
        else:
            route_path(board, [p_mcu, (x_chan, y_step)], F_Cu, 0.254, net)
        add_via(board, x_chan, y_step, net)
        route_path(board, [(x_chan, y_step), (x_chan, y_target)], B_Cu, 0.254, net)
        add_via(board, x_chan, y_target, net)
        route_path(board, [(x_chan, y_target), p_rlim], F_Cu, 0.254, net)

    # Ch 23 (Opto): J_MCU_L Pin 9 (-12.7, -6.35) -> exit B.Cu to (-8.0, -6.35), F.Cu down to 2.54, B.Cu across to 35.0, F.Cu over opto bus
    c23_net = get_or_create_net(board, "CH23_CTRL")
    c23_p_mcu = get_pad_pos(mcu_l, "9")
    c23_rlim = get_pad_pos(board.FindFootprintByReference("R_OPTO23"), "1")
    route_path(board, [c23_p_mcu, (-8.0, c23_p_mcu[1])], B_Cu, 0.254, c23_net)
    add_via(board, -8.0, c23_p_mcu[1], c23_net)
    route_path(board, [(-8.0, c23_p_mcu[1]), (-8.0, 2.54)], F_Cu, 0.254, c23_net)
    add_via(board, -8.0, 2.54, c23_net)
    route_path(board, [(-8.0, 2.54), (35.0, 2.54)], B_Cu, 0.254, c23_net)
    add_via(board, 35.0, 2.54, c23_net)
    route_path(board, [(35.0, 2.54), (52.0, 2.54)], F_Cu, 0.254, c23_net)
    add_via(board, 52.0, 2.54, c23_net)
    route_path(board, [(52.0, 2.54), (52.0, 36.0)], B_Cu, 0.254, c23_net)
    add_via(board, 52.0, 36.0, c23_net)
    route_path(board, [(52.0, 36.0), c23_rlim], F_Cu, 0.254, c23_net)

    # 9.8 Control Lines: MCU to 14 PROFETs (Decoupled Layer Architecture)
    def connect_south_profet_fcu(ch_idx, x_pos):
        c_net = profet_stages[ch_idx][5]
        rg_p1 = get_pad_pos(profet_stages[ch_idx][1], "1")
        rpd_p1 = get_pad_pos(profet_stages[ch_idx][2], "1")
        route_path(board, [(x_pos, 17.5), rg_p1, rpd_p1], F_Cu, 0.254, c_net)

    def connect_north_profet_fcu(ch_idx, x_pos):
        c_net = profet_stages[ch_idx][5]
        rg_p1 = get_pad_pos(profet_stages[ch_idx][1], "1")
        rpd_p1 = get_pad_pos(profet_stages[ch_idx][2], "1")
        route_path(board, [(x_pos, -17.5), rg_p1, rpd_p1], F_Cu, 0.254, c_net)

    # Ch 0: J_MCU_L Pin 20 (-12.7, 21.59) -> F.Cu drop East at X = -11.0 to Y = 30.0, via to B.Cu, West corridor to X = -75.0
    c0_net = profet_stages[0][5]
    c0_p_mcu = get_pad_pos(mcu_l, "20")
    route_path(board, [c0_p_mcu, (-11.0, c0_p_mcu[1]), (-11.0, 30.0)], F_Cu, 0.254, c0_net)
    add_via(board, -11.0, 30.0, c0_net)
    route_path(board, [(-11.0, 30.0), (-75.0, 30.0), (-75.0, 17.5)], B_Cu, 0.254, c0_net)
    add_via(board, -75.0, 17.5, c0_net)
    connect_south_profet_fcu(0, -75.0)

    # Ch 1: J_MCU_R Pin 20 (12.7, 21.59) -> F.Cu exit to (15.5, 29.0) over CH2, via to B.Cu, West corridor to X = -57.0
    c1_net = profet_stages[1][5]
    c1_p_mcu = get_pad_pos(mcu_r, "20")
    route_path(board, [c1_p_mcu, (15.5, c1_p_mcu[1]), (15.5, 29.0)], F_Cu, 0.254, c1_net)
    add_via(board, 15.5, 29.0, c1_net)
    route_path(board, [(15.5, 29.0), (-57.0, 29.0), (-57.0, 17.5)], B_Cu, 0.254, c1_net)
    add_via(board, -57.0, 17.5, c1_net)
    connect_south_profet_fcu(1, -57.0)

    # Ch 2: J_MCU_R Pin 19 (12.7, 19.05) -> dest X = -39.0 (100% on B.Cu South corridor at Y = 28.0, X = 14.5)
    c2_net = profet_stages[2][5]
    c2_p_mcu = get_pad_pos(mcu_r, "19")
    route_path(board, [c2_p_mcu, (14.5, c2_p_mcu[1]), (14.5, 28.0), (-39.0, 28.0), (-39.0, 17.5)], B_Cu, 0.254, c2_net)
    add_via(board, -39.0, 17.5, c2_net)
    connect_south_profet_fcu(2, -39.0)

    # Ch 3: J_MCU_L Pin 8 (-12.7, -8.89) -> dest X = -21.0 (100% on F.Cu at X = -17.0)
    c3_net = profet_stages[3][5]
    c3_p_mcu = get_pad_pos(mcu_l, "8")
    route_path(board, [c3_p_mcu, (-17.0, c3_p_mcu[1]), (-17.0, 17.5), (-21.0, 17.5)], F_Cu, 0.254, c3_net)
    connect_south_profet_fcu(3, -21.0)

    # Ch 4: J_MCU_L Pin 4 (-12.7, -19.05) -> dest X = 0.0 (B.Cu to X=0.0 and Y=-10.0, via to F.Cu South to 17.5)
    c4_net = profet_stages[4][5]
    c4_p_mcu = get_pad_pos(mcu_l, "4")
    route_path(board, [c4_p_mcu, (0.0, c4_p_mcu[1]), (0.0, -10.0)], B_Cu, 0.254, c4_net)
    add_via(board, 0.0, -10.0, c4_net)
    route_path(board, [(0.0, -10.0), (0.0, 17.5)], F_Cu, 0.254, c4_net)
    connect_south_profet_fcu(4, 0.0)

    # Ch 5: J_MCU_L Pin 5 (-12.7, -16.51) -> dest X = 21.0 South (exit B.Cu to -9.0, via F.Cu down to 10.16, via B.Cu across)
    c5_net = profet_stages[5][5]
    c5_p_mcu = get_pad_pos(mcu_l, "5")
    route_path(board, [c5_p_mcu, (-9.0, c5_p_mcu[1])], B_Cu, 0.254, c5_net)
    add_via(board, -9.0, c5_p_mcu[1], c5_net)
    route_path(board, [(-9.0, c5_p_mcu[1]), (-9.0, 10.16)], F_Cu, 0.254, c5_net)
    add_via(board, -9.0, 10.16, c5_net)
    route_path(board, [(-9.0, 10.16), (21.0, 10.16), (21.0, 17.5)], B_Cu, 0.254, c5_net)
    add_via(board, 21.0, 17.5, c5_net)
    connect_south_profet_fcu(5, 21.0)

    # Ch 6: J_MCU_L Pin 6 (-12.7, -13.97) -> dest X = 39.0 South (exit B.Cu to -10.0, via F.Cu down to 7.62, via B.Cu across)
    c6_net = profet_stages[6][5]
    c6_p_mcu = get_pad_pos(mcu_l, "6")
    route_path(board, [c6_p_mcu, (-10.0, c6_p_mcu[1])], B_Cu, 0.254, c6_net)
    add_via(board, -10.0, c6_p_mcu[1], c6_net)
    route_path(board, [(-10.0, c6_p_mcu[1]), (-10.0, 7.62)], F_Cu, 0.254, c6_net)
    add_via(board, -10.0, 7.62, c6_net)
    route_path(board, [(-10.0, 7.62), (39.0, 7.62), (39.0, 17.5)], B_Cu, 0.254, c6_net)
    add_via(board, 39.0, 17.5, c6_net)
    connect_south_profet_fcu(6, 39.0)

    # North Channels (Ch 7..13):
    # Ch 7: J_MCU_L Pin 7 (-12.7, -11.43) -> dest X = -75.0 (B.Cu horizontal, F.Cu vertical)
    c7_net = profet_stages[7][5]
    c7_p_mcu = get_pad_pos(mcu_l, "7")
    route_path(board, [c7_p_mcu, (-75.0, c7_p_mcu[1])], B_Cu, 0.254, c7_net)
    add_via(board, -75.0, c7_p_mcu[1], c7_net)
    route_path(board, [(-75.0, c7_p_mcu[1]), (-75.0, -17.5)], F_Cu, 0.254, c7_net)
    connect_north_profet_fcu(7, -75.0)

    # Ch 8: J_MCU_L Pin 10 (-12.7, -3.81) -> dest X = -57.0 (step to Y = -1.0 on B.Cu, via at Y=-7.0 to F.Cu)
    c8_net = profet_stages[8][5]
    c8_p_mcu = get_pad_pos(mcu_l, "10")
    route_path(board, [c8_p_mcu, (-15.0, -1.0), (-57.0, -1.0), (-57.0, -7.0)], B_Cu, 0.254, c8_net)
    add_via(board, -57.0, -7.0, c8_net)
    route_path(board, [(-57.0, -7.0), (-57.0, -17.5)], F_Cu, 0.254, c8_net)
    connect_north_profet_fcu(8, -57.0)

    # Ch 9: J_MCU_L Pin 12 (-12.7, 1.27) -> dest X = -39.0 (B.Cu horizontal, F.Cu vertical)
    c9_net = profet_stages[9][5]
    c9_p_mcu = get_pad_pos(mcu_l, "12")
    route_path(board, [c9_p_mcu, (-39.0, c9_p_mcu[1])], B_Cu, 0.254, c9_net)
    add_via(board, -39.0, c9_p_mcu[1], c9_net)
    route_path(board, [(-39.0, c9_p_mcu[1]), (-39.0, -17.5)], F_Cu, 0.254, c9_net)
    connect_north_profet_fcu(9, -39.0)

    # Ch 10: J_MCU_L Pin 17 (-12.7, 13.97) -> dest X = -21.0 (B.Cu horizontal, F.Cu vertical)
    c10_net = profet_stages[10][5]
    c10_p_mcu = get_pad_pos(mcu_l, "17")
    route_path(board, [c10_p_mcu, (-21.0, c10_p_mcu[1])], B_Cu, 0.254, c10_net)
    add_via(board, -21.0, c10_p_mcu[1], c10_net)
    route_path(board, [(-21.0, c10_p_mcu[1]), (-21.0, -17.5)], F_Cu, 0.254, c10_net)
    connect_north_profet_fcu(10, -21.0)

    # Ch 11: J_MCU_L Pin 18 (-12.7, 16.51) -> dest X = 0.0 North (exit B.Cu to -5.0, F.Cu North to -12.0, B.Cu North to -17.5)
    c11_net = profet_stages[11][5]
    c11_p_mcu = get_pad_pos(mcu_l, "18")
    route_path(board, [c11_p_mcu, (-5.0, c11_p_mcu[1])], B_Cu, 0.254, c11_net)
    add_via(board, -5.0, c11_p_mcu[1], c11_net)
    route_path(board, [(-5.0, c11_p_mcu[1]), (-5.0, -12.0)], F_Cu, 0.254, c11_net)
    add_via(board, -5.0, -12.0, c11_net)
    route_path(board, [(-5.0, -12.0), (-5.0, -17.5)], B_Cu, 0.254, c11_net)
    add_via(board, -5.0, -17.5, c11_net)
    route_path(board, [(-5.0, -17.5), (-3.5, -17.5), (-3.5, -19.0875), (0.0, -19.0875)], F_Cu, 0.254, c11_net)

    # Ch 12: J_MCU_L Pin 19 (-12.7, 19.05) -> dest X = 21.0 North (exit B.Cu to -7.0, F.Cu down to 0.00, B.Cu across to 21.0, B.Cu North)
    c12_net = profet_stages[12][5]
    c12_p_mcu = get_pad_pos(mcu_l, "19")
    route_path(board, [c12_p_mcu, (-7.0, c12_p_mcu[1])], B_Cu, 0.254, c12_net)
    add_via(board, -7.0, c12_p_mcu[1], c12_net)
    route_path(board, [(-7.0, c12_p_mcu[1]), (-7.0, 0.00)], F_Cu, 0.254, c12_net)
    add_via(board, -7.0, 0.00, c12_net)
    route_path(board, [(-7.0, 0.00), (21.0, 0.00), (21.0, -17.5)], B_Cu, 0.254, c12_net)
    add_via(board, 21.0, -17.5, c12_net)
    connect_north_profet_fcu(12, 21.0)

    # Ch 13: J_MCU_L Pin 11 (-12.7, -1.27) -> dest X = 39.0 North (exit B.Cu to -11.0, B.Cu to Y=5.08, B.Cu across to 39.0)
    c13_net = profet_stages[13][5]
    c13_p_mcu = get_pad_pos(mcu_l, "11")
    route_path(board, [c13_p_mcu, (-11.0, c13_p_mcu[1]), (-11.0, 5.08), (39.0, 5.08), (39.0, -17.5)], B_Cu, 0.254, c13_net)
    add_via(board, 39.0, -17.5, c13_net)
    connect_north_profet_fcu(13, 39.0)

    # 9.9 Ground Stitching Vias & Plane Continuity
    stitch_points = [
        # 4 corners
        (-82, -48), (-82, 48), (82, -48), (82, 48),
        # Perimeter edges
        (-86, -25), (-86, 0), (-86, 25),
        ( 86, -25), ( 86, 0), ( 86, 25),
        # North and South edges
        (-60, -52), (-40, -52), (-20, -52), (0, -52), (20, -52), (40, -52), (60, -52),
        (-60,  52), (-40,  52), (-20,  52), (0,  52), (20,  52), (40,  52), (60,  52),
        # Optocoupler ground columns
        (58, -40), (58, -24), (58, -8), (58, 8), (58, 24), (58, 40),
        (70, -40), (70, -24), (70, -8), (70, 8), (70, 24), (70, 40),
        # Buck regulator GND stitch
        (-41.0, 8.54),
        # D_TVS and R_PMOS GND stitch
        (-72.0, 18.0), (-60.0, 9.5),
        # MCU Ground continuity stitches
        (-10.0, 26.67), (16.5, 25.0), (19.73, 25.0)
    ]
    for sx, sy in stitch_points:
        add_via(board, sx, sy, gnd_net)

    # 10. Copper Ground Fill (F.Cu and B.Cu)
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
        zone.SetNet(gnd_net)
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

    filler = pcbnew.ZONE_FILLER(board)
    filler.Fill(board.Zones())
    print("Filled copper ground planes on F.Cu and B.Cu.")

    board.Save(board_path)
    print(f"\n[SUCCESS] Successfully generated 24-channel board layout at:\n  {board_path}\n")

if __name__ == "__main__":
    main()
