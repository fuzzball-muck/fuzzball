package main

type notInited struct {
}

func (notInited) Error() string {
	return "Not initialised"
}

