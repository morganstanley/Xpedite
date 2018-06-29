"""
Xpedite setup

Author: Manikandan Dhamodharan, Morgan Stanley
"""

from setuptools import setup

setup(name='xpedite',
      version='1.0',
      description='Xpedite is a probe based profiler build to optimize performance of ultra-low-latency applications',
      long_description=(
        'Xpedite is a probe based profiler build to optimize performance of '
        'ultra-low-latency applications'
      ),
      classifiers=[
        'Programming Language :: Python :: 2.7',
        'Topic :: Ultra low latency :: Performance optimization',
      ],
      keywords='perf optimization ull lowlatency',
      url='https://github.com/Morgan-Stanley/Xpedite',
      author='Manikandan Dhamodharan',
      author_email='Mani-D@users.noreply.github.com',
      license='MIT',
      packages=['xpedite'],
      install_requires=[
        'enum34',
        'functools32',
        'futures',
        'html',
        'netifaces',
        'numpy',
        'pygments',
        'rpyc',
        'cement',
        'termcolor',
        'py-cpuinfo',
        'ipython_genutils',
        'jsonschema',
        'nbformat',
        'notebook',
        'six',
        'tornado',
        'traitlets'
      ],
      include_package_data=True,
      zip_safe=False)
