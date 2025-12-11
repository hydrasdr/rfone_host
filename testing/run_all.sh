#!/bin/bash
set -eu

if docker compose version >/dev/null 2>&1; then
    echo "docker compose (v2) is installed."
    COMPOSE="docker compose"
elif docker-compose version >/dev/null 2>&1; then
    echo "docker-compose (v1) is installed."
    COMPOSE="docker-compose"
else
    echo "No docker compos found, exiting."
    exit 1
fi

# get all services
SERVICES=$(${COMPOSE} config --services)
echo -e "Building images:\n${SERVICES}"

# build all
for SERVICE in ${SERVICES}; do
    ${COMPOSE} build "${SERVICE}"
done

# run all
for SERVICE in ${SERVICES}; do
    echo -e "\n\n\nStarting image: ${SERVICE}"
    ${COMPOSE} run --rm "${SERVICE}"
done
