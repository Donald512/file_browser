import json
import re
import os

# Define the pairs: (input_json, output_header, macro_prefix)
variants = [
    ("FluentSystemIcons-Regular.json", "iconRegular.h", "REG"),
    ("FluentSystemIcons-Filled.json", "iconFilled.h", "FIL")
]

for json_file, header_file, prefix in variants:
    if not os.path.exists(json_file):
        print(f"Skipping {json_file} (file not found in this folder)")
        continue
        
    print(f"Generating {header_file}...")
    with open(json_file, "r", encoding="utf-8") as f:
        data = json.load(f)

    with open(header_file, "w", encoding="utf-8") as f:
        f.write("#pragma once\n\n")
        
        # Get min and max glyph ranges for font loading configuration arrays
        codepoints = list(data.values())
        if codepoints:
            f.write(f"#define ICON_MIN_{prefix} 0x{min(codepoints):04x}\n")
            f.write(f"#define ICON_MAX_{prefix} 0x{max(codepoints):04x}\n\n")
            
        for raw_name, decimal_val in sorted(data.items()):
            # 1. Capture the pixel size group using (\d+) before the suffix
            match = re.search(r'_(\d+)_(regular|filled)$', raw_name)
            
            # 2. Only append the size if it's NOT the standard default size (24)
            if match and match.group(1) != "24":
                size_suffix = f"_{match.group(1)}"
            else:
                size_suffix = ""
            
            # 3. Clean the base name by removing the trailing layout suffix entirely
            base_name = re.sub(r'_\d+_(regular|filled)$', '', raw_name)
            base_name = base_name.replace("ic_fluent_", "").upper()
            
            # 4. Combine the base name with the conditional size suffix
            clean_name = f"{base_name}{size_suffix}"
            
            # Convert raw codepoints to explicit C++ escaped string literals
            utf8_bytes = chr(decimal_val).encode('utf-8')
            utf8_hex = "".join(f"\\x{b:02x}" for b in utf8_bytes)
            
            f.write(f"#define ICON_{prefix}_{clean_name} \"{utf8_hex}\"\n")
            
    print(f"Successfully created {header_file}!")
