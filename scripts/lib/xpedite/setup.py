"""
Xpedite setup

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from setuptools import setup

setup(name='xpedite',
      version='1.0',
      description='Xpedite is a probe based profiler used to optimize performance of ultra-low-latency applications',
      long_description=(
        """
        A probe based profiler used to, measure and optimise, performance of ultra-low-latency / real time systems.

        The main features include

          1. Quantify how efficiently "a software stack" or "a section of code", is running
             in a target platform (CPU/OS).
          2. Do Cycle accounting and bottleneck analysis using hardware performance counters and
             top-down micro architecture analysis methodology
          3. Filter, query and visualise performance statistics with real time interactive shell.
          4. Prevent regressions, by benchmarking latency statistics for multiple runs/builds side-by-side.
        """
      ),
      classifiers=[
        'Programming Language :: Python :: 2.7',
        'Topic :: Ultra low latency :: Performance optimization',
      ],
      keywords='perf optimization ull lowlatency',
      url='https://github.com/Morgan-Stanley/Xpedite',
      author='Manikandan Dhamodharan',
      author_email='Mani-D@users.noreply.github.com',
      license='Apache 2.0',
      packages=['xpedite'],
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
      include_package_data=True,
      zip_safe=False)
