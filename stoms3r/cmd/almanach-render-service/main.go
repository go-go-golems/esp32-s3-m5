package main

import (
	"github.com/spf13/cobra"
)

// Version is set at build time via -ldflags.
var Version = "dev"

func main() {
	rootCmd, err := newRootCommand(Version)
	cobra.CheckErr(err)
	cobra.CheckErr(rootCmd.Execute())
}
