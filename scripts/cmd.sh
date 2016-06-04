


for cmd in $@ ; 
do
	case $cmd in
		a)
		echo a;
			;;
		*)
			
			exit 3;
			;;
	esac
done
