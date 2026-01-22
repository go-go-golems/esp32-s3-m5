package commands

import "github.com/go-go-golems/glazed/pkg/cmds/fields"

type ClientSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`
}

func clientFlags() []*fields.Definition {
	return []*fields.Definition{
		fields.New("server", fields.TypeString, fields.WithDefault("http://localhost:8765"), fields.WithHelp("Base URL of the running mled-server instance")),
		fields.New("timeout-ms", fields.TypeInteger, fields.WithDefault(5000), fields.WithHelp("HTTP client timeout (milliseconds)")),
	}
}
