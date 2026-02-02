package jsruntime

import (
	"github.com/dop251/goja"
	"github.com/dop251/goja_nodejs/require"
	"github.com/go-go-golems/go-go-goja/engine"

	_ "github.com/go-go-golems/zigctl/pkg/jsruntime/zigctlmod"
)

// New returns a goja runtime with require() enabled and the zigctl module registered.
func New() (*goja.Runtime, *require.RequireModule) {
	return engine.New()
}
