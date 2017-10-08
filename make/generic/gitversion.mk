
gitversion:
	echo -n -e "#define gitversion \""  > gitversion.h
	git log -n 1 --pretty=format:"%h %ai" >> gitversion.h
	echo -n -e "\"" >> gitversion.h
