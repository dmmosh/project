for i in {3..7}
do
	echo -e -n "$((2**i))KB cache size:  "
	./SIM $((2**(i+10))) 4 0 1 $1 | grep Miss


done
