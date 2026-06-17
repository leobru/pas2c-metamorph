#!/bin/sh -x
fn=`echo "$1" | tr '*/' '#_'`
cat << EOF > tmp$$
*name dtran
*disc:1/local
*file:pascom,67
*libra:22
*table:liblist($1)
*call library
*libra:23
*call setftn:one,long
*call dtran($1)
EOF
if [ -f "$fn.dtr" ]; then
	cat "$fn.dtr" >> tmp$$
fi
cat << EOF >> tmp$$
*edit
*r:1
*ll
*ee
*end file
EOF
dubna tmp$$ | sed '/,NAME,/,/,END,/!d' > $fn.dtran
if [ -s "$fn.dtran" ]; then
	rm tmp$$
else
	mv tmp$$ $fn.err
fi
