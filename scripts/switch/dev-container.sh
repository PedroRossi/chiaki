#!/bin/bash

cd "`dirname $(readlink -f ${0})`/../.."

touch "`pwd`/scripts/switch/dev-container.bash_history"

# port 28771 for nxlink stdio
podman run --rm \
	-v "`pwd`:/build/chiaki" \
	-w "/build/chiaki" \
	-v "`pwd`/scripts/switch/dev-container.bash_history:/root/.bash_history" \
	-p 0.0.0.0:28771:28771 \
	--name chiaki-switch-dev \
	-it \
	quay.io/thestr4ng3r/chiaki-build-switch:latest \
	/bin/bash
