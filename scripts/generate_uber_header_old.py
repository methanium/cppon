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

def process_template(template_content, fragments, headers, is_uber, std_includes=None, base_path=None, processed_templates=None):
    """
    Process template by replacing conditional tags and fragments.
    
    Args:
        template_content: Le contenu du template à traiter
        fragments: Dictionnaire des fragments
        headers: Dictionnaire des headers (pour résolution récursive)
        is_uber: Si True, on est en mode uber
        std_includes: Set pour collecter les includes standards (mode non-uber)
        base_path: Chemin de base pour charger les templates
        processed_templates: Set des templates déjà traités (pour éviter les boucles infinies)
    """
    # Initialiser le set des templates traités pour éviter les boucles infinies
    if processed_templates is None:
        processed_templates = set()
    
    # Traiter les directives conditionnelles
    pattern = r'\[\[@([A-Z0-9_]+)\s*\?\s*([^:]+)\s*:\s*([^\]]+)\]\]'
    for match in re.finditer(pattern, template_content):
        condition, then_branch, else_branch = match.groups()
        value = then_branch.strip() if condition == "UBER" and is_uber else else_branch.strip()
        
        # Capture des includes standards
        if then_branch.strip().startswith('@STDCAPTURE') and std_includes is not None and not is_uber:
            include_match = re.search(r'<([^>]+)>', else_branch)
            if include_match:
                # On capture sans le "#include", on l'ajoutera au moment d'utiliser
                std_includes.add(f"<{include_match.group(1)}>")
            # Mais pour le remplacement immédiat, on doit l'ajouter
            replacement = f"#include {else_branch}"
        # Si la valeur commence par @ c'est un fragment ou un header
        elif value.startswith('@STDCAPTURE'):
            # Mode uber, ne rien inclure directement
            replacement = ""
        elif value.startswith('@'):
            fragment_name = value[1:]  # Garde la casse (MAJUSCULES)

            # Cas spécial pour STANDARD_INCLUDES
            if fragment_name == "STANDARD_INCLUDES" and is_uber:
                replacement = '\n'.join([f"#include {inc}" for inc in fragments.get("STANDARD_INCLUDES", [])])
            # Cas spécial pour ASSERT_MACRO
            elif fragment_name == "ASSERT_MACRO":
                replacement = '\n'.join(fragments.get("ASSERT_MACRO", []))
            # Chercher dans les fragments 
            elif fragment_name in fragments:
                replacement = '\n'.join(fragments[fragment_name])
            # Sinon, chercher dans les headers pour inclusion récursive
            elif fragment_name in headers and base_path is not None and is_uber:
                header_file = headers[fragment_name]
                
                # Déterminer le namespace
                if fragment_name in ["PROCESSOR_FEATURES_INFO"]:
                    namespace = "platform"
                elif fragment_name in ["SIMD_COMPARISONS"]:
                    namespace = "simd"
                else:
                    namespace = "cppon"
                
                # Construire le chemin du template
                template_path = os.path.join(base_path, "templates", "headers", namespace, header_file)
                
                # Éviter les boucles infinies
                if template_path in processed_templates:
                    replacement = f"/* Circular reference detected: {fragment_name} */"
                else:
                    # Charger et traiter récursivement le template
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
            # Ajouter #include pour les fichiers .h
            header_extensions = ['.h"', '.hpp"', '.hxx"', '.h++"']
            if not is_uber and ((value.startswith('"') and any(value.endswith(ext) for ext in header_extensions)) or 
                               (value.startswith('<') and value.endswith('>'))):
                replacement = f'#include {value}'
            else:
                replacement = value.strip('\'"')
            
        template_content = template_content.replace(match.group(0), replacement)
    
    # Traiter les inclusions simples de fragments
    pattern = r'\[\[([A-Z0-9_]+)\]\]'
    for match in re.finditer(pattern, template_content):
        fragment_name = match.group(1)  # Garde la casse (MAJUSCULES)
        if fragment_name in fragments:
            replacement = '\n'.join(fragments[fragment_name])
            template_content = template_content.replace(match.group(0), replacement)
    
    return template_content

def generate_modular_files(config, base_path):
    """Generate modular header files from templates."""
    fragments = config['fragments']
    headers = config['headers']
    std_includes = set()  # Pour collecter les includes standards
    
    # Parcourir et traiter tous les headers
    for key, template_name in headers.items():
        # Ignorer le fichier CPPON principal, il sera traité séparément
        if key == "CPPON":
            continue
            
        # Déterminer le bon chemin
        if key in ["PROCESSOR_FEATURES_INFO"]:
            namespace = "platform"
        elif key in ["SIMD_COMPARISONS"]:
            namespace = "simd"
        else:
            namespace = "cppon"
            
        # Construire les chemins
        output_name = template_name.replace('.tmpl', '.h')
        
        template_path = os.path.join(base_path, "templates", "headers", namespace, template_name)
        output_path = os.path.join(base_path, "include", namespace, output_name)
        
        # Lire et traiter le template
        template_content = read_file(template_path)
        content = process_template(template_content, fragments, headers, 
                                 is_uber=False, std_includes=std_includes, 
                                 base_path=base_path)
        
        # Écrire le fichier résultant
        write_file(output_path, content)
        print(f"Generated {output_path}")
    
    # Traiter le fichier principal c++on.h
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
    
    # Ne pas ajouter "#include" ici - on le fera au moment du remplacement
    fragments["STANDARD_INCLUDES"] = sorted(list(std_includes))
    
    # Read the main template
    template_path = os.path.join(base_path, "templates", "headers", "c++on.tmpl")
    template_content = read_file(template_path)
    
    # Process the template for uber file
    content = process_template(template_content, fragments, headers, 
                             is_uber=True, std_includes=None, 
                             base_path=base_path)
    
    # Write the uber file
    output_path = os.path.join(base_path, "single_include", "cppon", "c++on.h")
    write_file(output_path, content)
    print(f"Generated uber header: {output_path}")

def main():
    # Define paths
    base_path = ".."  # Adjust if needed
    
    # Load configuration
    config_path = os.path.join(base_path, "templates", "template.json")
    with open(config_path, 'r', encoding='utf-8') as f:
        config = json.load(f)
    
    # Generate modular files and collect standard includes
    std_includes = generate_modular_files(config, base_path)
    
    # Generate uber file with collected standard includes
    generate_uber_file(config, base_path, std_includes)
    
    print("Header generation complete!")

if __name__ == "__main__":
    main()