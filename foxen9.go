package main

import (
	"fmt"
	"os"
	"bufio"
	"strconv"
	"strings"
)

const DB_VERSION_STRING = "***Foxen9 TinyMUCK DUMP Format***"

// Foxen9DumpFormatObjectsFactory
type F9DFObjectsFactory struct {
	topId DbRef  // the 'top' of the database; it is the highest dbref + 1
	dbParmsInfo string
	configs map[string]string
	objects map[int64]DbObject
}

func readLine(s *bufio.Scanner) string {
	s.Scan()
	return s.Text()
}

func readNDbRefs(log *LvlLog, scanner *bufio.Scanner, n int64) []DbRef {
	var list []DbRef
	for n > 0 {
		n--
		d, err := DbRef{}.Set(readLine(scanner))
		fatal_if(err, log, "Failed dbref read")
		list = append(list, d)
	}
	return list
}

func readProps(log *LvlLog, scanner *bufio.Scanner) []string {
	var propsList []string
	props := readLine(scanner)
	debug(log, fmt.Sprintf("Props header: %s", props))
	if props == "*Props*" {
		props = readLine(scanner)
		for props != "*End*" {
			propsList = append(propsList, props)
			props = readLine(scanner)
		}
		debug(log, fmt.Sprintf("Props footer: %s", props))
		scanner.Scan()
	}
	return propsList
}

func (fof F9DFObjectsFactory) readObject(log *LvlLog, ref DbRef, scanner *bufio.Scanner) DbObject {
	var err error
	var oc DbObjectCommonFields
	oc._ref = ref
	r, err := oc._ref.Get()
	fatal_if(err,log, "Invalid 'ref' dbref get")
	debug(log, fmt.Sprintf("Ref: %s", r))

	oc._name = readLine(scanner)
	debug(log, fmt.Sprintf("Name: %s", oc._name))

	oc._location, err = DbRef{}.Set(readLine(scanner))
	fatal_if(err, log, "Invalid 'location' dbref read")
	loc, err := oc._location.Get()
	fatal_if(err, log, "Invalid 'location' dbref get")
	debug(log, fmt.Sprintf("Location: %s", loc))

	// does it own or is part of the contents list?
	oc._head, err = DbRef{}.Set(readLine(scanner)) // head of contents/exits list
	fatal_if(err, log, "Invalid 'head' dbref read")
	head, err := oc._head.Get()
	fatal_if(err, log, "Invalid 'head' dbref get")
	debug(log, fmt.Sprintf("Head: %s", head))

	oc._next, err = DbRef{}.Set(readLine(scanner)) // next item of contents/exits list
	fatal_if(err, log, "Invalid 'next' dbref read")
	next, err := oc._next.Get()
	fatal_if(err, log, "Invalid 'next' dbref get")
	debug(log, fmt.Sprintf("Next: %s", next))

	flags, err := strconv.ParseInt(readLine(scanner), 10, 32)
	oc._flags = int32(flags)
	fatal_if(err, log, "Invalid 'flags' stringint read")
	debug(log, fmt.Sprintf("Flags: %d", oc._flags))

	creationTime, err := strconv.ParseInt(readLine(scanner), 10, 32)
	oc._creationTime = int32(creationTime)
	fatal_if(err, log, "Invalid 'creationTime' stringint read")
	debug(log, fmt.Sprintf("Creation Time: %d", oc._creationTime))

	lastUsedTime, err := strconv.ParseInt(readLine(scanner), 10, 32)
	oc._lastUsedTime = int32(lastUsedTime)
	fatal_if(err, log, "Invalid 'lastUsedTime' stringint read")
	debug(log, fmt.Sprintf("Last Used Time: %d", oc._lastUsedTime))

	useCount, err := strconv.ParseInt(readLine(scanner), 10, 32)
	oc._useCount = int32(useCount)
	fatal_if(err, log, "Invalid 'useCount' stringint read")
	debug(log, fmt.Sprintf("Use Count: %d", oc._useCount))

	modificationTime, err := strconv.ParseInt(readLine(scanner), 10, 32)
	oc._modificationTime = int32(modificationTime)
	fatal_if(err, log, "Invalid 'modificationTime' stringint read")
	debug(log, fmt.Sprintf("Modification Time: %d", oc._modificationTime))

	oc._propsList = readProps(log, scanner)
	props := scanner.Text()

	for _, v := range oc._propsList {
		// TODO: create prop class
		debug(log, fmt.Sprintf(">> %s <<", v))
//		x := strings.Split(v, ":")
//		debug(log, fmt.Sprintf(">> %s <<", x[0]))
//		debug(log, fmt.Sprintf(">> %s <<", x[1]))
//		debug(log, fmt.Sprintf(">> %s <<", x[2]))
	}

	var obj DbObject = nil
	switch (oc._flags & TYPE_MASK) {
		case TYPE_ROOM: {
			debug(log, "Type: Room")

			dropTo, err := DbRef{}.Set(props)
			fatal_if(err, log, "Failed dropTo")

			head, err := DbRef{}.Set(readLine(scanner))
			fatal_if(err, log, "Failed head")

			owner, err := DbRef{}.Set(readLine(scanner))
			fatal_if(err, log, "Failed owner")

			obj = ObjectRoom{oc, dropTo, head, owner}
			break
		}
		case TYPE_THING: {
			debug(log, "Type: Thing")

			home, err := DbRef{}.Set(props)
			fatal_if(err, log, "Failed home")

			head, err := DbRef{}.Set(readLine(scanner))
			fatal_if(err, log, "Failed head")

			owner, err := DbRef{}.Set(readLine(scanner))
			fatal_if(err, log, "Failed owner")

			obj = ObjectThing{oc, home, head, owner}
			break
		}
		case TYPE_EXIT: {
			debug(log, "Type: Exit")

			numExits, err := strconv.ParseInt(props, 10, 32)
			fatal_if(err, log, "Failed numExits")
			exits := readNDbRefs(log, scanner, numExits)

			owner, err := DbRef{}.Set(readLine(scanner))
			fatal_if(err, log, "Failed owner")

			obj = ObjectExit{oc, numExits, exits, owner}
			break
		}
		case TYPE_PLAYER: {
			debug(log, "Type: Player")

			home, err := DbRef{}.Set(props)
			fatal_if(err, log, "Failed home")

			head, err := DbRef{}.Set(readLine(scanner))
			fatal_if(err, log, "Failed head")

			pwhashb64 := readLine(scanner)

			obj = ObjectPlayer{oc, home, head, pwhashb64}
			break
		}
		case TYPE_PROGRAM: {
			debug(log, "Type: Program")

			owner, err := DbRef{}.Set(props)
			fatal_if(err, log, "Failed owner")

			// TODO: slurp MUF program from disk

			obj = ObjectProgram{oc, owner}
			break
		}
	}
	return obj
}

func (fof F9DFObjectsFactory) Import(log *LvlLog, fn string) F9DFObjectsFactory {
	file, err := os.Open(fn)
	if err != nil {
		log.Log(LVLLOG_FATAL, err.Error())
		panic(err.Error())
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	line := readLine(scanner)
	// maybe this should have been done before calling here?
	// the struct has a 'F9' in its name after all
	if line != DB_VERSION_STRING {
		panic("Version string missing")
	}

	// next id to allocate
	line = readLine(scanner)
	fof.topId.Set(line)
	debug(log, fmt.Sprintf("Top ID: %s", line))

	if err != nil {
		log.Log(LVLLOG_FATAL, err.Error())
		panic(err.Error())
	}

	// 'an ignored dbflags value' aka DB_PARMSINFO
	// it's obsolete, used to be used but no longer
	fof.dbParmsInfo = readLine(scanner)

	// number of config lines to follow
	line = readLine(scanner)
	numConfigs, err := strconv.ParseUint(line, 10, 64)
	if err != nil {
		log.Log(LVLLOG_FATAL, err.Error())
		panic(err)
	}

	fof.objects = make(map[int64]DbObject)

	// these usually but do not have to begin with a %
	// since it's just one config, it appears to be a bug
	fof.configs = make(map[string]string)
	for numConfigs > 0 {
		line = readLine(scanner)
		//fmt.Println(line)
		s := strings.Split(line, "=")
		fof.configs[s[0]] = s[1]
		debug(log, fmt.Sprintf(">> %s = %s <<", s[0], s[1]))
		numConfigs--
	}
	debug(log, fmt.Sprintf("Configs loaded: %d", len(fof.configs)))

	for {
		ref := readLine(scanner)
		if ref == "***END OF DUMP***" {
			break
		}
		r, err := DbRef{}.Set(ref)
		fatal_if(err, log, "Invalid 'ref' dbref read")

		fof.objects[r._ref] = fof.readObject(log, r, scanner)
	}
	return fof
}

