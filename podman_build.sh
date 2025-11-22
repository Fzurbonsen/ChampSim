#!/bin/bash

export http_proxy="http://proxy.ethz.ch:3128"
export https_proxy="https://proxy.ethz.ch:3128"

rm -rf champsim_latest.tar
podman rmi champsim:latest
podman build -t champsim:latest .
podman save -o champsim_latest.tar champsim:latest