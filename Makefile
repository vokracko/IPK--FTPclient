flags=--std=c++98

all:
	g++ $(flags) *.cpp -o ftpclient

clean:
	@rm -f *.o *.tar *.gz 2> /dev/null

pack: clean
	tar -cf xvokra00.tar *.cpp *.h Makefile
	gzip xvokra00.tar

