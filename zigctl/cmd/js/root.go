package js

import (
	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/spf13/cobra"
)

func Register(root *cobra.Command) error {
	jsCmd := &cobra.Command{
		Use:   "js",
		Short: "Run JavaScript scripts with the zigctl runtime",
		Long:  "JavaScript runtime backed by go-go-goja with a native zigctl module.",
	}

	runCmd, err := NewRunCommand()
	if err != nil {
		return err
	}
	cobraRun, err := buildCobra(runCmd)
	if err != nil {
		return err
	}

	replCmd, err := NewReplCommand()
	if err != nil {
		return err
	}
	cobraRepl, err := buildCobra(replCmd)
	if err != nil {
		return err
	}

	jsCmd.AddCommand(cobraRun, cobraRepl)
	root.AddCommand(jsCmd)
	return nil
}

func buildCobra(cmd cmds.Command) (*cobra.Command, error) {
	return cli.BuildCobraCommand(cmd,
		cli.WithParserConfig(cli.CobraParserConfig{
			ShortHelpLayers: []string{schema.DefaultSlug},
			MiddlewaresFunc: cli.CobraCommandDefaultMiddlewares,
		}),
	)
}
