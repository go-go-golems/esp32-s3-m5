package main

import (
	"os"

	"github.com/go-go-golems/zigctl/cmd"
)

func main() {
	if err := cmd.Execute(); err != nil {
		os.Exit(1)
	}
}
