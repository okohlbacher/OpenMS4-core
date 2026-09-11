# Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
# SPDX-License-Identifier: BSD-3-Clause
# $Maintainer: OpenMS Team $

import json
import os
from pathlib import Path
import subprocess

source = Path(__file__).resolve().parent
build = source / 'build'
prefix = Path(os.environ['CONDA_PREFIX']) / 'Library'
subprocess.run(['cmake','-S',str(source),'-B',str(build),'-G','Visual Studio 17 2022','-A','x64',f'-DCMAKE_PREFIX_PATH={prefix.as_posix()}','-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL'],check=True)
subprocess.run(['cmake','--build',str(build),'--config','Release','--parallel','4'],check=True)
results = []
for mode in ('default','no-prebuffer','shutdown','system'):
    env = dict(os.environ, PROBE_QUIET="1")
    if mode == 'system': env['ARROW_DEFAULT_MEMORY_POOL'] = 'system'
    record = dict(mode=mode,passed=0)
    for index in range(1000):
        p = subprocess.run([str(build/'Release/arrow_probe.exe'),mode],env=env,cwd=build,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        if p.returncode:
            record.update(returncode=p.returncode,output=p.stdout,attempt=index+1)
            print(json.dumps(record),flush=True)
            break
        record['passed'] += 1
    else: print(json.dumps(record),flush=True)
    results.append(record)
(source/'results.json').write_text(json.dumps(results,indent=2))
