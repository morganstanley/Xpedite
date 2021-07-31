#!/bin/sh -l

PYTHON_VERSION=cp38-cp38


/opt/python/${PYTHON_VERSION}/bin/pip install pybind11

echo "builiding package for python version - ${PYTHON_VERSION}"
cd /github/workspace/scripts/lib
/opt/python/${PYTHON_VERSION}/bin/python setup.py bdist_wheel

for pkg in $(ls -1 dist):
do
  file=$(pwd)/dist/${pkg}
  if [ -f ${file} ]; then
    auditwheel repair ${file}
  fi
done
