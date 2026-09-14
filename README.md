# PlusInjector

PlusInjector is a Windows command-line utility for loading and unloading a managed assembly in a **Mono process you own or are explicitly authorized to test**. It invokes a named, zero-argument managed method after loading the assembly.

## Build

Open `plusinjector.slnx` in Visual Studio and build either `Release | x64` or `Release | Win32`. Match the executable architecture to the target process whenever possible.

The resulting executable is `x64\\Release\\pi.exe` for an x64 build.

## Command line

```text
pi.exe inject|eject [options]
```

Short and long flags are interchangeable; use one spelling per option.

| Option | Purpose |
| --- | --- |
| `-p`, `--process <name\|pid>` | Target process executable name or PID. |
| `-a`, `--assembly <path\|address>` | Assembly DLL to load, or the printed address to unload. |
| `-n`, `--namespace <name>` | Optional namespace of the loader class. |
| `-c`, `--class <name>` | Loader class name. |
| `-m`, `--method <name>` | Zero-argument method to run. |
| `-d`, `--delay <time>` | Wait before running: `250ms`, `10s`, or `1m`. |
| `-h`, `--hide` | Hide the console after startup. Useful only when the caller has already captured or redirected output. |
| `-log`, `--log <path>` | Duplicate console output into a new log file. |
| `-out`, `--result <path>` | On injection, save only the resulting assembly address to a file. |
| `--help` | Show built-in help. |

### Inject

```powershell
pi.exe inject --process ExampleGame.exe --assembly C:\\Mods\\Example.dll --namespace Example --class Loader --method Load --log C:\\Logs\\example-inject.log --result C:\\Logs\\example-address.txt
```

On success, the tool prints the remote assembly address. Save that address if it will later be needed for ejection.

### Eject

```powershell
pi.exe eject --process ExampleGame.exe --assembly 0x000001D23A98B000 --namespace Example --class Loader --method Unload --log C:\\Logs\\example-eject.log
```

## Calling from an authorized launcher or script

Use the stable long options and check the process exit code instead of parsing human-readable error text. For an inject operation, `--result` writes the address alone (for example `0x000001D23A98B000`) after a successful load, so a caller can retain it for a later eject operation.

| Exit code | Meaning |
| --- | --- |
| `0` | Operation completed successfully. |
| `1` | The requested operation failed. |
| `2` | Command-line usage or validation error. |

For no visible console output, have the calling program redirect standard output and standard error, and use `--log` when a local diagnostic record is needed. `--hide` hides the console window but does not suppress output or logging.

## Requirements and limitations

- Windows and a process hosting the Mono runtime are required.
- The loader method must take no arguments.
- The target must be a process you control or have explicit permission to test.
- Store the address printed by `inject`; it identifies the loaded assembly for `eject`.

## Repository contents

The `.gitignore` excludes Visual Studio state and compiled/debug artifacts, so a GitHub upload contains project configuration and source rather than build output.
