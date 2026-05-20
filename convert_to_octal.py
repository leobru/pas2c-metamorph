#!/usr/bin/env python3
import sys
import re

def decimal_to_octal(match):
    decimal_num = int(match.group(1))
    if 65 <= decimal_num <= 90:
        char = chr(decimal_num)
        return f"char('{char}')"
    octal_num = oct(decimal_num)[2:]
    return f"({octal_num}C)"

def octal_b_to_c(match):
    octal_num = match.group(1).lstrip('0') or '0'
    return f",({octal_num}C)"

def strip_64_prefix(match):
    octal_num = match.group(1).lstrip('0') or '0'
    return f"({octal_num}C)"

input_text = sys.stdin.read()
output_text = re.sub(r'\((\d+)\)', decimal_to_octal, input_text)
output_text = re.sub(r',([0-7]+)B', octal_b_to_c, output_text)
output_text = re.sub(r'\(64([0-7]{14})C\)', strip_64_prefix, output_text)
output_text = re.sub(r'P/WI', 'C/WI', output_text)
output_text = re.sub(r'P/DI', 'C/DI', output_text)
output_text = re.sub(r'P/MD', 'C/MD', output_text)
sys.stdout.write(output_text)
