#!/bin/bash
set -e

source envsetup.sh
rm -rf ${INSTALL_PATH}
rm -rf ${PROJECT_ROOT}/regression/regression_out
source build.sh RELEASE

mlir_version="$(grep MLIR_VERSION ${BUILD_PATH}/CMakeCache.txt | cut -d "=" -f2)"
release_archive="./tpu-mlir_${mlir_version}"

rm -rf ${release_archive}*
cp -rf ${INSTALL_PATH} ${release_archive}

cp -rf ${PROJECT_ROOT}/regression ${release_archive}

# commands to bmrt_test
regression_sh=${release_archive}/regression/run.sh
echo "# for test" >> ${regression_sh}
echo "mkdir -p bmodels" >> ${regression_sh}
echo "pushd bmodels" >> ${regression_sh}
echo "../prepare_bmrttest.py ../regression_out" >> ${regression_sh}
echo "cp -f ../run_bmrttest.py ./run.py" >> ${regression_sh}
echo "popd" >> ${regression_sh}

# build a envsetup.sh
__envsetupfile=${release_archive}/envsetup.sh
rm -f __envsetupfile

echo "Create ${__envsetupfile}" 1>&2
more > "${__envsetupfile}" <<'//MY_CODE_STREAM'
#!/bin/bash
# set environment variable
TPUC_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export PATH=${TPUC_ROOT}/bin:$PATH
export PATH=${TPUC_ROOT}/python/tools:$PATH
export PATH=${TPUC_ROOT}/python/utils:$PATH
export LD_LIBRARY_PATH=$TPUC_ROOT/lib:$LD_LIBRARY_PATH
export PYTHONPATH=${TPUC_ROOT}/python:$PYTHONPATH
export NNMODELS_PATH=${TPUC_ROOT}/../nnmodels
export REGRESSION_PATH=${TPUC_ROOT}/regression
//MY_CODE_STREAM

# generate readme.md
echo "Create readme.md" 1>&2
more > "${release_archive}/readme.md" <<'//MY_CODE_STREAM'
# For test

0. Setup

If you have this file, congratulation, you have finished the first step of
exploring the "TPU-MLIR" release SDK.

Before getting a start, you need to prepare some configuration.

- a. Get the sohgo-sdk docker image `docker pull mattlu/sophgo:18.04`.

- b. Clone the "nnmodels" repository to the same directory in which you unpack
  TPU-MLIR. `git clone ssh://YourName@gerrit-ai.sophgo.vip:29418/nnmodels`.


- c. (Optional, If you want to test more cases.) get into the nnmodels folder and
  pull LFS files from the server.
  `git lfs install && git lfs pull --include "*.onnx`

- d. Create a docker container and map the directory of "TPU-MLIR" to it.
  `docker run -v $PWD:/workspace/ -ti mattlu/sophgo:18.04 /bin/bash`

- e. If everything goes well, you can go to the next stage.

1. Set envrironment variables.

You need to set some system variables to ensure all the functions of TPU-MLIR
can work well. The commands below will make all the executable files provided by
TPU-MLIR visible to the system.

``` bash
source ./envsetup.sh
```

2. Run demo

This step will run lots of test cases to demonstrate all the features of
TPU-MLIR, which include:

- a. Translate ONNX/TFLite models to framework-independent TOP MLIR.

- b. (Optional) Do calibration, and tranform float computes model to integer computations
  which are friendly to hardware.

- c. Transform TOP MLIR to TPU MLIR, which is close to the hardware.

- d. Generate bm1684x machine code (bmodel).

``` bash
cd regression
 ./run.sh
```

After run regression test, all the bmodels will be in regression_out.

3. Test the performance of Bmodels on BM1684X.

TPU-MLIR does not provide "bmrt_test" excutable file. It would be best to have
an environment where "bmrt_test" is available.

- If your BM1684X devide runs in PCIE mode, use the commands below.

``` bash
cd to/the/folder/of/bmodels
./run.py
```

A "*.csv" file(report) will be generated in this folder.

- If your BM1684X device runs in SOC mode, use the commands below.

``` bash
cp -rf bmodels /to/your/BM1684X/soc/board # eg. ~/
cd /to/bmodels # eg. ~/
./run.py
```

A "*.csv" file(report) will be generated in this folder.


//MY_CODE_STREAM

tar -cvzf "tpu-mlir_${mlir_version}.tar.gz" ${release_archive}
rm -rf ${release_archive}
