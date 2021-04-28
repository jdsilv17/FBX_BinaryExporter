The .exe looks for all files with the extension '.fbx' in the immediate directory and exports a '.mesh', '.mats', '.anim' binary files, in that same directory
filled with mesh data, materials data and animation data respectively

The FBXExport_TEST is the console app I used to test the exporter
The FBXExporter is the actual dll that does a mediocre job of reading all that fbx goodness