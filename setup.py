# setup.py file
import sys
import os
from os import path
import numpy
import warnings
import subprocess
import shutil

from setuptools import Extension, setup
from Cython.Distutils import build_ext
from Cython.Build import cythonize

currentFolder = path.dirname(path.abspath(__file__))
NAME = "py_grassmind"

# Name of the package (and directory of pyx and pxd files)
packageName = NAME
packageDir = path.join(currentFolder, packageName)
buildDir = path.join(currentFolder, "build", "Test")
sourceDir = "./"
include_dirs = [packageDir, sourceDir, numpy.get_include()]

libDirs = [currentFolder, buildDir]

if sys.platform == "win32":
    raise RuntimeError("Python binding not yet adapted for windows!")
else:
    libraries = ["grassmind3lib"]
    extra_compile_args = ["-O0", "-g", "-DCYTHON_TRACE=1"]
    define_macros = []


extensions = [
    Extension(
        packageName + "." + NAME,
        sources=[path.join(packageName, NAME + ".pyx")],
        libraries=libraries,
        library_dirs=libDirs,
        language="c++",
        include_dirs=include_dirs,
        define_macros=define_macros,
        extra_compile_args=extra_compile_args,
        extra_link_args=["-g"] if sys.platform != "win32" else ["/DEBUG"],
    )
]

setup(
    name=packageName + "." + NAME,
    ext_modules=cythonize(
        extensions,
        gdb_debug=True,
        compiler_directives={
            "linetrace": True,
            "binding": True,
        },
    ),
    python_requires=">=3.8",
    packages=[packageName],
    package_data={
        packageName: ["*.pxd", "*.pyx", "*.cpp", "*.c", "*.h"],
    },
    zip_safe=False,
)

# setup(
#    name=packageName + "." + NAME,
#    cmdclass={"build_ext": build_ext},
#    setup_requires=["numpy", "cython"],
#    ext_modules=[
#        Extension(
#            packageName + "." + NAME,
#            sources=[path.join(packageName, NAME + ".pyx")],
#            libraries=libraries,
#            library_dirs=libDirs,
#            language="c++",
#            include_dirs=include_dirs,
#            define_macros=define_macros,
#            extra_compile_args=extra_compile_args,
#        )
#    ],
#    python_requires=">=3.8",
#    packages=[packageName],
#    package_data={
#        packageName: ["*.pxd", "*.pyx", "*.cpp", "*.c", "*.h"],
#    },
#    zip_safe=False,
# )
