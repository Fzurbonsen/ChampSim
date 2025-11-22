#!/bin/bash

export http_proxy="http://proxy.ethz.ch:3128"
export https_proxy="https://proxy.ethz.ch:3128"

podman rmi champsim:latest
podman build -t champsim:latest .