import os
from conan.tools.files import copy

# Conan2 helper tool for simplifying deploy copies.
def deploy(graph, output_folder, **kwargs):
    for name, dep in graph.root.conanfile.dependencies.items():
        if dep.is_build_context:
            continue
        for inc_dir in dep.cpp_info.includedirs:
            copy(graph.root.conanfile, "*", src=inc_dir, dst=os.path.join(output_folder, "include"))
        for lib_dir in dep.cpp_info.libdirs:
            copy(graph.root.conanfile, "*.so*", src=lib_dir, dst=os.path.join(output_folder, "lib"))
            copy(graph.root.conanfile, "*.dylib*", src=lib_dir, dst=os.path.join(output_folder, "lib"))
        for bin_dir in dep.cpp_info.bindirs:
            copy(graph.root.conanfile, "*.dll", src=bin_dir, dst=os.path.join(output_folder, "bin"))

