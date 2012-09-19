objects = $(patsubst %.java,%.class,$(wildcard *.java))
java_opts=$(JAVA_OPTS)

all: $(objects)
	cd fuzzball && $(MAKE)

$(objects): %class: %java
	javac $(java_opts) $<

clean:
	rm -f $(objects)
	cd fuzzball && $(MAKE) clean

