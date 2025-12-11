# Software platform testing

This features a dockerized build for testing of the library and soapy module for different distributions.

## Requirements

Install:<br>
docker<br>
docker compose (sometimes called docker-compose)

## Usage

To build and test all, run:<br>
```bash
bash ./run_all.sh
```

To build and test a single dist:
```bash
docker compose build ubuntu_noble
docker compose run --rm -it ubuntu_noble
```

## Description

The `Dockerfile.*` contains build instructions for each packaging system.<br>
These have special args that can be used to control the build.

All details to build distribution versions is in `docker-compose.yml` and new ones can easily be added.<br>

When running it will launch [test.sh](test.sh) inside the container.
