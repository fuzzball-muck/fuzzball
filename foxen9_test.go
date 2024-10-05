package main

// go test -v foxen9.go foxen9_test.go dbref.go dbobject.go lvllog.go

import (
	"testing"
	"fmt"
)

func TestImport(t *testing.T) {
	var f F9DFObjectsFactory
	log := &LvlLog{LVLLOG_DEBUG}
	//log := &LvlLog{LVLLOG_FATAL}
	//f = f.Import(log, "dbs/minimal/data/minimal.db")
	// also slurp mufs in muf dir
	f = f.Import(log, "dbs/starterdb/data/starterdb.db")
	// List config keys
	debug(log, "Configs:")
	for k, _ := range f.configs {
		fmt.Println(k)
	}
	// List DbRef IDs
	debug(log, "DbRefs:")
	for i, _ := range f.objects {
		fmt.Println(fmt.Sprintf("#%d", i))
	}
}

