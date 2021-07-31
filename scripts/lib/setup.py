"""
Xpedite setup

Author: Manikandan Dhamodharan, Morgan Stanley
"""

import os
from glob import glob
from setuptools import setup
from setuptools import find_packages
from pybind11.setup_helpers import Pybind11Extension

moduleDirPath = os.path.dirname(os.path.abspath(__file__))
bindingModulePath = os.path.abspath(os.path.join(moduleDirPath, '../../include'))
print('bindings path - {}'.format(bindingModulePath))

ext_modules = [
  Pybind11Extension(
    'xpediteBindings',
    ['ext/lib/xpedite/pybind/Bindings.cpp'],
    include_dirs = [bindingModulePath]
  ),
]

setup(name='xpedite',
      version='1.0',
      description='A non-sampling profiler, purpose built to measure and optimise, performance of ultra-low-latency/real time systems.',
      long_description=(
        """
        A non-sampling profiler, purpose built to measure and optimise, performance of ultra-low-latency / real time systems.
        """
      ),
      long_description_content_type='text/markdown',
      classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'Programming Language :: Python :: 2.7',
      ],
      keywords='profiler performance performance-counters low-latency ultra-low-latency real-time perf-events cpu-profiler intel benchmarking',
      url='https://www.xpedite.dev',
      project_urls={
        'Source': 'https://github.com/Morgan-Stanley/Xpedite',
        'Tracker': 'https://github.com/Morgan-Stanley/Xpedite/issues',
      },
      author='Manikandan Dhamodharan',
      author_email='Mani-D@users.noreply.github.com',
      license='Apache 2.0',
      setup_requires=['setuptools_scm'],
      packages=find_packages(include='xpedite.*'),
      ext_modules = ext_modules,
      include_package_data=True,
      install_requires=[
        'enum34',
        'functools32',
        'futures',
        'netifaces',
        'numpy',
        'pygments',
        'rpyc',
        'cement',
        'termcolor',
        'py-cpuinfo',
        'jupyter',
        'six',
      ],
      zip_safe=False)
