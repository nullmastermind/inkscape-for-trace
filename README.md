<div align="center">
    <h1><code>Inkscape</code></h1>
    <p><strong>Used only for automation scripts</strong></p>
</div>

## <code>Features</code>

- [x] Add options for tracing

## <code>Quick guide</code>

```bash
# New trace action with fully optional parameters
selection-trace:{scans},{is_smooth[0|1]},{is_stack[0|1]},{is_remove_background[0|1],{speckles},{smooth_corners},{optimize}}
# Example:
# Trace 256 colors then export to output.svg
$ inkscape.exe --actions="select-all;selection-trace:256,0,1,1,4,1.0,0.20;export-filename:output.svg;export-do;" "input.png" --batch-process
```