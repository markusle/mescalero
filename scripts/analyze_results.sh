#!/bin/bash

# 
# this script is specifically for Arch Linux Users to determine which
# packages own changed/new/deleted files to help track down if a
# pacman upgrade cause detected changes
#
# (C) 2012 Markus Dittrich
#

PACMAN_LOG_PATH="/var/log/pacman.log"


if [ $# != 1 ]; then
  echo "Usage: "$0" <resultsfile>"
  exit 1
fi

RESULTSFILE="$1"

echo "Unknown or missing files:"
echo "-------------------------"

# extract mismatched files
mismatched=$(cat $RESULTSFILE | gawk ' /filename/ { print $3 } ' | xargs pacman -Qo | gawk ' {print $5"->"$6} ' | sort -u)

# extract unknown files
unknown=$(cat $RESULTSFILE | gawk ' /Warning/ { print $2 } ' | xargs pacman -Qo | gawk ' {print $5"->"$6} ' | sort -u)

# combine and filter unique ones
all=( ${mismatched[@]} ${unknown[@]} )
allUniq=( $(for i in "${all[@]}"; do echo ${i}; done | sort -u) )

echo
echo "Packages responsible for other file changes"
echo "-------------------------------------------"
for i in "${allUniq[@]}"; do
  echo ${i}
done


echo
echo "Last relevant package upgrade:"
echo "------------------------------"
for i in "${allUniq[@]}"; do
  name="$(echo ${i} | cut -d'>' -f1)"
  name="${name%?}"
  results=$(grep "$name" "$PACMAN_LOG_PATH")
  if [[ $? != 0 ]]; then
    echo "WARNING: ${name} has not been upgraded recently."
  else
    echo "$results" | tail -n 1
  fi
done
