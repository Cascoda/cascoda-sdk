#!/bin/bash

# This script creates a flash_all.csv and ram_all.csv file to
# track the resource usage of different binaries. It also generates
# individual csv files for each target (flash_<target>.csv and
# ram_<target.csv). It is a binding script intended to be used with
# jenkins and serves no other useful purpose. It takes the list of 
# arguments provided to the script, and adds the sizes to CSV files
# as required by the jenkins 'plot' plugin.

# Function to die with error
die() { echo "Error: " "$*" 1>&2 ; exit 1; }

shellcheck "$0" || die "Failed self-shellcheck"

NAMES=""
FLASH=""
RAM=""

# Iterate through arguments, updating shell variables
for target in "$@"
do
	NAMES="${NAMES}${target},"
	FILEPATH=$(find . -type f -name "${target}" -print -quit)
	# shellcheck disable=SC2207
	SIZES=($(arm-none-eabi-size "${FILEPATH}" | awk 'FNR == 1 {next} {print $1 + $2, $2 + $3}'))
	FLASH="${FLASH}${SIZES[0]},"
	RAM="${RAM}${SIZES[1]},"
	echo "${target}" > "flash_${target}.csv"
	echo "${SIZES[0]}" >> "flash_${target}.csv"
	echo "${target}" > "ram_${target}.csv"
	echo "${SIZES[1]}" >> "ram_${target}.csv"
done

echo "${NAMES}" > flash_all.csv
echo "${FLASH}" >> flash_all.csv

echo "${NAMES}" > ram_all.csv
echo "${RAM}" >> ram_all.csv
