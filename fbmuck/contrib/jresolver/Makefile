objects = $(patsubst %.java,%.class,$(wildcard *.java))
java_opts=$(JAVA_OPTS)
class_path=`pwd`

all: $(objects)
	export CLASSPATH=$(class_path) ; cd org && $(MAKE)

$(objects): %class: %java
	javac $(java_opts) $<

clean:
	rm -f $(objects)
	cd org && $(MAKE) clean

cleaner: clean
	rm -f resolver Resolver.MF resolver.jar

Resolver.MF:
	echo "Main-Class: org.fuzzball.Resolver" > Resolver.MF

resolver:
	echo "#!/bin/sh" > resolver
	echo 'CLASSPATH=`dirname $$0`/resolver.jar' >> resolver
	echo "export CLASSPATH" >> resolver
	echo "java -ms1M -mx1M -jar resolver.jar" >> resolver
	chmod +rx resolver

jar: all Resolver.MF resolver
	jar cmf Resolver.MF resolver.jar org/fuzzball/*.class

