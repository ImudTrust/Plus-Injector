# PlusInjector

## Download

Download the latest compiled version from [GitHub Releases](https://github.com/ImudTrust/Plus-Injector/releases/latest).

> Download the `.exe` that matches the target game's architecture: `x64` for 64-bit games or `x86` for 32-bit games.

## Usage

```console
pi.exe inject|eject [options]
```

### Options

| Option | Description |
|---|---|
| `-p`, `--process <name\|pid>` | Target process name or PID. |
| `-a`, `--assembly <path\|address>` | Assembly DLL path when injecting, or loaded assembly address when ejecting. |
| `-n`, `--namespace <name>` | Loader namespace. |
| `-c`, `--class <name>` | Loader class name. |
| `-m`, `--method <name>` | Method to execute after loading. |
| `-d`, `--delay <time>` | Delay before execution, such as `250ms`, `10s`, or `1m`. |
| `-h`, `--hide` | Hide the console window after startup. |
| `-log`, `--log <path>` | Save console output to a log file. |
| `-out`, `--result <path>` | Save the resulting assembly address to a file. |
| `--help` | Display the built-in help message. |

## Inject

```console
pi.exe inject --process ExampleGame.exe --assembly C:\Mods\Example.dll --namespace Example --class Loader --method Load
```

With logging and result output:

```console
pi.exe inject --process ExampleGame.exe --assembly C:\Mods\Example.dll --namespace Example --class Loader --method Load --log C:\Logs\inject.log --result C:\Logs\address.txt
```

## Eject

```console
pi.exe eject --process ExampleGame.exe --assembly 0x000001D23A98B000 --namespace Example --class Loader --method Unload
```

## From Another Program

```console
pi.exe inject --process ExampleGame.exe --assembly C:\Mods\Example.dll --namespace Example --class Loader --method Load
```

Check the process exit code:

| Exit code | Meaning |
|---|---|
| `0` | Operation completed successfully. |
| `1` | Operation failed. |
| `2` | Invalid command-line arguments. |

Use `--result` to save the assembly address:

```text
0x000001D23A98B000
```

## Hide Console

```console
pi.exe inject --process ExampleGame.exe --assembly C:\Mods\Example.dll --namespace Example --class Loader --method Load --hide
```

## Help

```console
pi.exe --help
```