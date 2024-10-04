#!/bin/bash
set -euo pipefail

fuzzball \
	--host=0.0.0.0 \
	--port=4201 \
	--pidfile=/var/run/fuzzball.pid

