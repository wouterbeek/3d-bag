#!/bin/sh
sed -e 's/&/&amp;/g' $1 > $2
