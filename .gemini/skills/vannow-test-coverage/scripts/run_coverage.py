#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil

def run_cmd(cmd, cwd=None):
    res = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=cwd)
    if res.returncode != 0:
        print(f"Error running command: {cmd}", file=sys.stderr)
        print(f"Stdout:\n{res.stdout}", file=sys.stderr)
        print(f"Stderr:\n{res.stderr}", file=sys.stderr)
        return False
    return True

def parse_gcov_file(filepath):
    if not os.path.exists(filepath):
        return None
    executable = 0
    covered = 0
    with open(filepath, 'r') as f:
        for line in f:
            parts = line.split(':', 2)
            if len(parts) < 2:
                continue
            count_str = parts[0].strip()
            if count_str == '-':
                continue
            executable += 1
            if count_str != '#####' and count_str != '$$$$$':
                covered += 1
    return executable, covered

def main():
    # Detect firmware root directory
    cwd = os.getcwd()
    if os.path.isdir(os.path.join(cwd, 'firmware')):
        fw_dir = os.path.join(cwd, 'firmware')
    elif os.path.basename(cwd) == 'firmware':
        fw_dir = cwd
    else:
        # Check if we can find it in parent dirs
        found = False
        parts = cwd.split(os.sep)
        for i in range(len(parts), 0, -1):
            path = os.sep.join(parts[:i])
            if os.path.isdir(os.path.join(path, 'firmware')):
                fw_dir = os.path.join(path, 'firmware')
                found = True
                break
        if not found:
            print("Error: Could not find 'firmware' directory. Please run this script from the project root or the firmware directory.", file=sys.stderr)
            sys.exit(1)

    central_dir = os.path.join(fw_dir, 'central')
    remote_dir = os.path.join(fw_dir, 'remote')

    results = []
    
    # 1. Process Central
    print("Gathering coverage for Central firmware...")
    central_compile = (
        "g++ -O0 -g --coverage "
        "-I src -I test/test_channels -I ../lib/arduino_mock/src -I ../lib/protocol -I .pio/libdeps/native/Unity/src "
        "src/Channel.cpp src/DigitalChannel.cpp src/DimmableChannel.cpp src/SystemController.cpp "
        "../lib/arduino_mock/src/Arduino.cpp .pio/libdeps/native/Unity/src/unity.c "
        "test/test_channels/test_channels.cpp -o test_channels_cov"
    )
    if run_cmd(central_compile, cwd=central_dir):
        if run_cmd("./test_channels_cov", cwd=central_dir):
            # Run gcov
            gcov_cmd = "gcov test_channels_cov-Channel.gcda test_channels_cov-DigitalChannel.gcda test_channels_cov-DimmableChannel.gcda test_channels_cov-SystemController.gcda"
            run_cmd(gcov_cmd, cwd=central_dir)
            
            # Parse
            for src_name, gcov_name in [
                ('src/Channel.cpp', 'Channel.cpp.gcov'),
                ('src/DigitalChannel.cpp', 'DigitalChannel.cpp.gcov'),
                ('src/DimmableChannel.cpp', 'DimmableChannel.cpp.gcov'),
                ('src/SystemController.cpp', 'SystemController.cpp.gcov')
            ]:
                gcov_path = os.path.join(central_dir, gcov_name)
                res = parse_gcov_file(gcov_path)
                if res:
                    results.append(('Central', src_name, res[1], res[0]))
                else:
                    results.append(('Central', src_name, 0, 0))
            
            # Cleanup
            for f in os.listdir(central_dir):
                if f.endswith('.gcov') or f.startswith('test_channels_cov'):
                    try:
                        p = os.path.join(central_dir, f)
                        if os.path.isdir(p):
                            shutil.rmtree(p)
                        else:
                            os.remove(p)
                    except Exception:
                        pass

    # 2. Process Remote
    print("Gathering coverage for Remote firmware...")
    remote_compile = (
        "g++ -O0 -g --coverage "
        "-I src -I test/test_remote_handlers -I ../lib/arduino_mock/src -I ../lib/protocol -I .pio/libdeps/native/Unity/src "
        "src/ButtonHandler.cpp src/EncoderHandler.cpp src/PowerManager.cpp "
        "../lib/arduino_mock/src/Arduino.cpp .pio/libdeps/native/Unity/src/unity.c "
        "test/test_remote_handlers/test_remote_handlers.cpp -o test_remote_handlers_cov"
    )
    if run_cmd(remote_compile, cwd=remote_dir):
        if run_cmd("./test_remote_handlers_cov", cwd=remote_dir):
            # Run gcov
            gcov_cmd = "gcov test_remote_handlers_cov-ButtonHandler.gcda test_remote_handlers_cov-EncoderHandler.gcda test_remote_handlers_cov-PowerManager.gcda"
            run_cmd(gcov_cmd, cwd=remote_dir)
            
            # Parse
            for src_name, gcov_name in [
                ('src/ButtonHandler.cpp', 'ButtonHandler.cpp.gcov'),
                ('src/EncoderHandler.cpp', 'EncoderHandler.cpp.gcov'),
                ('src/PowerManager.cpp', 'PowerManager.cpp.gcov')
            ]:
                gcov_path = os.path.join(remote_dir, gcov_name)
                res = parse_gcov_file(gcov_path)
                if res:
                    results.append(('Remote', src_name, res[1], res[0]))
                else:
                    results.append(('Remote', src_name, 0, 0))
            
            # Cleanup
            for f in os.listdir(remote_dir):
                if f.endswith('.gcov') or f.startswith('test_remote_handlers_cov'):
                    try:
                        p = os.path.join(remote_dir, f)
                        if os.path.isdir(p):
                            shutil.rmtree(p)
                        else:
                            os.remove(p)
                    except Exception:
                        pass

    # 3. Print markdown report
    print("\n## Code Coverage Summary Report\n")
    print("| Component | File | Covered Lines | Executable Lines | Coverage |")
    print("| :--- | :--- | :---: | :---: | :---: |")
    
    total_exec = 0
    total_cov = 0
    
    current_comp = None
    comp_exec = 0
    comp_cov = 0
    
    for comp, name, cov, exec_l in results:
        if current_comp and current_comp != comp:
            # print subtotal
            sub_pct = (comp_cov / comp_exec) * 100 if comp_exec > 0 else 0
            print(f"| ***{current_comp} Subtotal*** | | **{comp_cov}** | **{comp_exec}** | **{sub_pct:.2f}%** |")
            comp_exec = 0
            comp_cov = 0
        
        current_comp = comp
        comp_exec += exec_l
        comp_cov += cov
        total_exec += exec_l
        total_cov += cov
        
        pct = (cov / exec_l) * 100 if exec_l > 0 else 0
        print(f"| {comp} | {name} | {cov} | {exec_l} | {pct:.2f}% |")
        
    if current_comp:
        sub_pct = (comp_cov / comp_exec) * 100 if comp_exec > 0 else 0
        print(f"| ***{current_comp} Subtotal*** | | **{comp_cov}** | **{comp_exec}** | **{sub_pct:.2f}%** |")
        
    total_pct = (total_cov / total_exec) * 100 if total_exec > 0 else 0
    print(f"| **Total (Tested Code)** | **-** | **{total_cov}** | **{total_exec}** | **{total_pct:.2f}%** |")

if __name__ == '__main__':
    main()
