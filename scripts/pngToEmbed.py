import sys
import os

def png_to_cpp_array(png_path, output_var_name="g_ImageData"):
    with open(png_path, "rb") as f:
        data = f.read()
    
    hex_data = ", ".join(f"0x{byte:02x}" for byte in data)
    
    cpp_array = f"const uint8_t {output_var_name}[] = {{\n"
    for i in range(0, len(data), 12):  # Format for readability
        cpp_array += "    " + ", ".join(f"0x{byte:02x}" for byte in data[i:i+12]) + ",\n"
    cpp_array += "};\n"
    cpp_array += f"const size_t {output_var_name}Size = sizeof({output_var_name});\n"
    
    return cpp_array

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python script.py <image.png>")
        sys.exit(1)
    
    png_path = sys.argv[1]
    if not os.path.exists(png_path):
        print("Error: File not found.")
        sys.exit(1)
    
    output_var_name = os.path.splitext(os.path.basename(png_path))[0]
    output_cpp = png_to_cpp_array(png_path, output_var_name)
    
    output_file = output_var_name + ".h"
    with open(output_file, "w") as f:
        f.write(output_cpp)
    
    print(f"Generated {output_file}")
