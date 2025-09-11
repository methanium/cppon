@echo off
echo Generating C++ON uber header...
cd %~dp0
python generate_uber_header.py
echo Uber header generated successfully!
.pyproj