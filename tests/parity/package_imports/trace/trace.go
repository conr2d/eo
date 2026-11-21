// Package trace records the order in which the packages of the program initialize.
package trace

var Events []string

func Add(e string) {
	Events = append(Events, e)
}

func Mark(e string) string {
	Add(e)
	return e
}

func init() {
	Add("trace.init")
}
