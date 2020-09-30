#!/bin/bash
set -x

IMG="quay.io/iovisor/bpftrace:v0.10.0"

docker run -ti --rm \
  -v /usr/src:/usr/src:ro \
  -v /lib/modules/:/lib/modules:ro \
  -v /sys/kernel/debug/:/sys/kernel/debug:rw \
  --net=host --pid=host --privileged \
  ${IMG} \
  "$@"
