#! /bin/bash

for f in *.xsl
do
	b=`basename $f .xsl`
	c=$b.cpp
	echo "// Generated from $f by convertxslt.sh. Do not edit manually" > $c
	echo "QString xslt_$b(\"\\" >> $c
	sed -e s/\"/\\\\\"/g -e s/$/\\\\/g $f >> $c
	echo "\");" >> $c
done