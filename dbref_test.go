package main

// go test -v dbref.go dbref_test.go

import (
	"testing"
)

func TestDbrefStringToInt(t *testing.T) {
	s := "#24353"
	d, err := DbRef{}.Set(s)
	if err != nil {
		t.Fatalf("Set failed")
	}
	u := d._ref
	if err != nil {
		t.Fatalf(err.Error())
	}
	if u != 24353 {
		t.Fatalf("Number wrong")
	}
}

func TestDbrefIntToString(t *testing.T) {
	var i int64 = 14356
	d := DbRef{i}
	s, err := d.Get()
	if err != nil {
		t.Fatalf(err.Error())
	}
	if s != "#14356" {
		t.Fatalf("String wrong")
	}
}

