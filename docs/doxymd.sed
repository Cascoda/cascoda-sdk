#!/bin/sed -Ef
# A sed script to remove html tags that doxygen can't process

# Remove 'details', and convert 'summary' to 'h4'
s/<\/?details(\s+open\s*)?>//g
s/<(\/?)summary>/<\1h4>/g
