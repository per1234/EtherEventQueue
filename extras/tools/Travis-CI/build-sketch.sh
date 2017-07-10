#!/bin/bash

# This is a helper script for the Travis CI build of the library. Necessary to deal with the incompatibility of the Entropy example with some of the boards.

function etherevent_build_sketch()
{
  local -r sketchName="$1"
  local -r boardID="$2"
  local -r allowFailure="$3"
  local -r IDEversionStart="$4"
  local -r IDEversionEnd="$5"

  local -r entropyRegex="Entropy.ino"
  if [[ "$boardID" != "arduino:avr:mega:cpu=atmega2560" && "$boardID" != "arduino:sam:arduino_due_x_dbg"  && "$sketchName" =~ $entropyRegex ]]; then
    echo "The Entropy example sketch is not compatible with this board, skipping"
    return 0
  fi

  build_sketch "$sketchName" "$boardID" "$allowFailure" "$IDEversionStart" "$IDEversionEnd"
}
