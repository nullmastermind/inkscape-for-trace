<div align="center">
    <h1><code>Inkscape</code></h1>
    <p><strong>Used only for automation scripts</strong></p>
</div>

## <code>Features</code>

- [x] Add options for tracing

## <code>Quick guide</code>

```bash
# New trace action with fully optional parameters
selection-trace:{scans},{is_smooth[false|true]},{is_stack[false|true]},{is_remove_background[false|true],{speckles},{smooth_corners},{optimize}}
# Example:
# Trace 256 colors then export to output.svg
$ inkscape.exe --actions="select-all;selection-trace:256,false,true,true,4,1.0,0.20;export-filename:output.svg;export-do;" "input.png" --batch-process

# If "false|true" does not work, please replace it with "0|1", because my project has been bankrupt for a long time and is no longer maintained, but rest assured that it still works very well.
```
