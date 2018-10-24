#!/bin/sh
sed -e 's/<gml:Solid>/<gml:Solid srsDimension="3">/g' $1 > $2
