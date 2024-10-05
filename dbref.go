package main

import (
	"strconv"
)

const DBREF_GOD = 1        // player one
const DBREF_NOTHING = -1   // nothing/null
const DBREF_AMBIGUOUS = -2 // multiple matches
const DBREF_HOME = -3      // virtual room, mover's home
const DBREF_NIL = -4       // do-nothing link for actions

// design decision: don't store the #, just add it on output
type DbRef struct {
	_ref int64
}

func (d DbRef) Set(s string) (DbRef, error) {
	// remove initial # if present
	if s[0] == '#' {
		s = s[1:]
	}
	var err error
	d._ref, err = strconv.ParseInt(s, 10, 64)
	return d, err
}

func (d DbRef) Get() (string, error) {
	var err error = nil
	var s string = ""
	s = strconv.FormatInt(d._ref, 10)
	return "#" + s, err
}

