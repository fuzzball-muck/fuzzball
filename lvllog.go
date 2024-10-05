package main

import (
	"fmt"
)

type LvlLog struct {
	_level uint8
}

func (l LvlLog) Log(level uint8, s string) {
	if level <= l._level {
		fmt.Println(s)
	}
}

const LVLLOG_FATAL = 0
const LVLLOG_ERROR = 1
const LVLLOG_WARN = 2
const LVLLOG_INFO = 3
const LVLLOG_VERBOSE = 4
const LVLLOG_DEBUG = 5

func fatal_if(err error, log *LvlLog, s string) {
	if err != nil {
		log.Log(LVLLOG_FATAL, s)
		panic(s)
	}
}

func debug(log *LvlLog, s string) {
	log.Log(LVLLOG_DEBUG, s)
}

