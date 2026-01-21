import { useEffect, useRef } from 'preact/hooks'
import { keymap } from '@codemirror/view'
import { EditorState } from '@codemirror/state'
import { EditorView } from '@codemirror/view'
import { javascript } from '@codemirror/lang-javascript'
import { defaultKeymap, indentWithTab } from '@codemirror/commands'
import { oneDark } from '@codemirror/theme-one-dark'

type Props = {
  initialValue: string
  onChange: (value: string) => void
  onRun: () => void
}

export function CodeEditor({ initialValue, onChange, onRun }: Props) {
  const hostRef = useRef<HTMLDivElement | null>(null)
  const viewRef = useRef<EditorView | null>(null)
  const suppressChangeRef = useRef(false)

  useEffect(() => {
    const host = hostRef.current
    if (!host) return

    const state = EditorState.create({
      doc: initialValue,
      extensions: [
        EditorView.lineWrapping,
        oneDark,
        javascript(),
        keymap.of([
          { key: 'Mod-Enter', run: () => (onRun(), true) },
          indentWithTab,
          ...defaultKeymap,
        ]),
        EditorView.updateListener.of((update) => {
          if (!update.docChanged) return
          if (suppressChangeRef.current) return
          onChange(update.state.doc.toString())
        }),
        EditorView.theme({
          '&': { height: '300px', borderRadius: '6px' },
          '.cm-scroller': { fontFamily: 'ui-monospace, SFMono-Regular, Menlo, monospace' },
        }),
      ],
    })

    const view = new EditorView({ state, parent: host })
    viewRef.current = view
    return () => {
      viewRef.current = null
      view.destroy()
    }
  }, [initialValue, onChange, onRun])

  useEffect(() => {
    const view = viewRef.current
    if (!view) return

    // Keep the editor doc in sync if something external overwrites it later.
    // (We currently only set the initial value, but this makes future features safer.)
    const current = view.state.doc.toString()
    if (current === initialValue) return

    suppressChangeRef.current = true
    view.dispatch({
      changes: { from: 0, to: view.state.doc.length, insert: initialValue },
    })
    suppressChangeRef.current = false
  }, [initialValue])

  return <div ref={hostRef} class="codeEditor" />
}

