#!/usr/bin/env python3
"""
C++ON Header Generator
This script generates both modular and uber header files for the C++ON library.
"""

import os
import re
import json

def read_file(path):
    """Read a file with multiple encoding fallbacks."""
    encodings = ['utf-8', 'latin1', 'cp1252']
    for encoding in encodings:
        try:
            with open(path, 'r', encoding=encoding) as f:
                return f.read()
        except UnicodeDecodeError:
            continue
    with open(path, 'r', encoding='latin1') as f:
        return f.read()

def write_file(path, content):
    """Write content to a file, creating directories if needed."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

def _replace_tag(template_content, tag_text, replacement):
    """Replace a tag; if replacement is empty/whitespace, drop the whole line (and one following blank line)."""
    if replacement.strip() == "":
        # Line containing the tag + optional line ending + ONE following blank line
        pattern = r'^[ \t]*' + re.escape(tag_text) + r'[ \t]*(?:\r?\n)?(?:[ \t]*\r?\n)?'
        return re.sub(pattern, '', template_content, flags=re.MULTILINE)
    else:
        return template_content.replace(tag_text, replacement)

def _cleanup_content(content):
    """Minimal final cleanup: trim trailing spaces (leaves empty lines intact)."""
    # Remove trailing spaces
    content = re.sub(r'[ \t]+(\r?\n)', r'\1', content)
    return content

def process_template(template_content, fragments, headers, is_uber, std_includes=None, base_path=None, processed_templates=None):
    """
    Process template by replacing conditional tags and fragments.
    """
    if processed_templates is None:
        processed_templates = set()
    
    # Conditional tags [[@COND ? then : else]]
    pattern = r'\[\[@([A-Z0-9_]+)\s*\?\s*([^:]+)\s*:\s*([^\]]+)\]\]'
    for match in re.finditer(pattern, template_content):
        full_tag = match.group(0)
        condition, then_branch, else_branch = match.groups()
        value = then_branch.strip() if condition == "UBER" and is_uber else else_branch.strip()

        if then_branch.strip().startswith('@STDCAPTURE') and std_includes is not None and not is_uber:
            include_match = re.search(r'<([^>]+)>', else_branch)
            if include_match:
                std_includes.add(f"<{include_match.group(1)}>")
            replacement = f"#include {else_branch}"
        elif value.startswith('@STDCAPTURE'):
            replacement = ""
        elif value.startswith('@'):
            fragment_name = value[1:]

            if fragment_name == "STANDARD_INCLUDES" and is_uber:
                replacement = '\n'.join([f"#include {inc}" for inc in fragments.get("STANDARD_INCLUDES", [])])
            elif fragment_name == "ASSERT_MACRO":
                replacement = '\n'.join(fragments.get("ASSERT_MACRO", []))
            elif fragment_name in fragments:
                replacement = '\n'.join(fragments[fragment_name])
            elif fragment_name in headers and base_path is not None and is_uber:
                header_file = headers[fragment_name]
                if fragment_name in ["PROCESSOR_FEATURES_INFO"]:
                    namespace = "platform"
                elif fragment_name in ["SIMD_COMPARISONS"]:
                    namespace = "simd"
                else:
                    namespace = "cppon"
                template_path = os.path.join(base_path, "templates", "headers", namespace, header_file)
                if template_path in processed_templates:
                    replacement = f"/* Circular reference detected: {fragment_name} */"
                else:
                    try:
                        processed_templates.add(template_path)
                        template_content_inner = read_file(template_path)
                        replacement = process_template(template_content_inner, fragments, headers, 
                                                       is_uber, std_includes, base_path, processed_templates)
                    except Exception as e:
                        replacement = f"/* Error processing {fragment_name}: {str(e)} */"
            else:
                replacement = f"/* Fragment or header not found: {fragment_name} */"
        else:
            header_extensions = ['.h"', '.hpp"', '.hxx"', '.h++"']
            if not is_uber and ((value.startswith('"') and any(value.endswith(ext) for ext in header_extensions)) or 
                                (value.startswith('<') and value.endswith('>'))):
                replacement = f'#include {value}'
            else:
                replacement = value.strip('\'"')
        
        template_content = _replace_tag(template_content, full_tag, replacement)
    
    # Simple inclusions [[FRAGMENT]]
    pattern = r'\[\[([A-Z0-9_]+)\]\]'
    for match in re.finditer(pattern, template_content):
        full_tag = match.group(0)
        fragment_name = match.group(1)
        if fragment_name in fragments:
            replacement = '\n'.join(fragments[fragment_name])
        else:
            replacement = ""
        template_content = _replace_tag(template_content, full_tag, replacement)
    
    # Final cleanup
    return _cleanup_content(template_content)

def generate_modular_files(config, base_path):
    """Generate modular header files from templates."""
    fragments = config['fragments']
    headers = config['headers']
    std_includes = set()
    
    for key, template_name in headers.items():
        if key == "CPPON":
            continue
        if key in ["PROCESSOR_FEATURES_INFO"]:
            namespace = "platform"
        elif key in ["SIMD_COMPARISONS"]:
            namespace = "simd"
        else:
            namespace = "cppon"
        output_name = template_name.replace('.tmpl', '.h')
        template_path = os.path.join(base_path, "templates", "headers", namespace, template_name)
        output_path = os.path.join(base_path, "include", namespace, output_name)
        template_content = read_file(template_path)
        content = process_template(template_content, fragments, headers, 
                                   is_uber=False, std_includes=std_includes, 
                                   base_path=base_path)
        write_file(output_path, content)
        print(f"Generated {output_path}")
    
    template_path = os.path.join(base_path, "templates", "headers", "c++on.tmpl")
    output_path = os.path.join(base_path, "include", "cppon", "c++on.h")
    template_content = read_file(template_path)
    content = process_template(template_content, fragments, headers, 
                               is_uber=False, std_includes=std_includes, 
                               base_path=base_path)
    write_file(output_path, content)
    print(f"Generated {output_path}")
    
    return std_includes

def generate_uber_file(config, base_path, std_includes):
    fragments = config['fragments']
    headers = config['headers']
    fragments["STANDARD_INCLUDES"] = sorted(list(std_includes))
    template_path = os.path.join(base_path, "templates", "headers", "c++on.tmpl")
    template_content = read_file(template_path)
    content = process_template(template_content, fragments, headers, 
                               is_uber=True, std_includes=None, 
                               base_path=base_path)
    output_path = os.path.join(base_path, "single_include", "cppon", "c++on.h")
    write_file(output_path, content)
    print(f"Generated uber header: {output_path}")

def main():
    base_path = ".."
    config_path = os.path.join(base_path, "templates", "template.json")
    with open(config_path, 'r', encoding='utf-8') as f:
        config = json.load(f)
    std_includes = generate_modular_files(config, base_path)
    generate_uber_file(config, base_path, std_includes)
    print("Header generation complete!")

if __name__ == "__main__":
    main()