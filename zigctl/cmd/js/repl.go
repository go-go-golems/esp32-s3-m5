package js

import (
	"bufio"
	"context"
	"fmt"
	"os"
	"strings"

	"github.com/dop251/goja"
	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/layers"
	"github.com/go-go-golems/zigctl/pkg/jsruntime"
)

type ReplCommand struct {
	*cmds.CommandDescription
}

var _ cmds.BareCommand = (*ReplCommand)(nil)

func NewReplCommand() (*ReplCommand, error) {
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	cmdDesc := cmds.NewCommandDescription(
		"repl",
		cmds.WithShort("Start an interactive JS REPL"),
		cmds.WithLong(`
Start a simple JS REPL with the zigctl runtime.

Special commands:
  :help  show help
  :quit  exit
`),
		cmds.WithLayersList(commandSettingsLayer),
	)

	return &ReplCommand{CommandDescription: cmdDesc}, nil
}

func (c *ReplCommand) Run(ctx context.Context, _ *layers.ParsedLayers) error {
	vm, _ := jsruntime.New()
	reader := bufio.NewReader(os.Stdin)
	fmt.Println("zigctl-js> type JS code (:help for help)")

	for {
		fmt.Print("js> ")
		line, err := reader.ReadString('\n')
		if err != nil {
			if err.Error() == "EOF" {
				fmt.Println()
				return nil
			}
			return err
		}

		line = strings.TrimSpace(line)
		if line == "" {
			continue
		}

		switch line {
		case ":quit", ":exit":
			return nil
		case ":help":
			fmt.Println("Commands:\n  :help  show help\n  :quit  exit\nOtherwise any line is evaluated as JavaScript.")
			continue
		}

		val, err := vm.RunString(line)
		if err != nil {
			fmt.Printf("Error: %v\n", err)
			continue
		}
		if val != nil && !goja.IsUndefined(val) {
			fmt.Println(val)
		}
	}
}
